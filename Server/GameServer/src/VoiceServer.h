#pragma once
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <functional>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>

#pragma comment(lib, "ws2_32.lib")

// 고정 크기 헤더 (Zero-copy UDP 전송용)
#pragma pack(push, 1)
struct VoicePacketHeader {
    uint8_t packetType;       // 0: Join, 1: Voice Data, 2: Leave
    char roomCode[16];        // 룸 코드 (가변 가능하지만 고정 크기 16바이트로 제한)
    char speakerName[32];     // 발화자 이름 고정 (최대 32바이트)
    uint32_t payloadSize;     // 뒤에 따라오는 음성 데이터 
};
#pragma pack(pop)

struct ClientEndpoint {
    sockaddr_in addr;
    std::chrono::steady_clock::time_point lastSeen;
};

// IP:Port를 uint64_t 키로 변환하는 헬퍼
inline uint64_t makeClientKey(const sockaddr_in& addr) {
    return (static_cast<uint64_t>(addr.sin_addr.s_addr) << 16) | ntohs(addr.sin_port);
}

class VoiceServer {
private:
    SOCKET udpSocket;
    // Map of roomCode -> (speakerName -> ClientEndpoint)
    std::unordered_map<std::string, std::unordered_map<std::string, ClientEndpoint>> rooms;
    // Server에 접속한 모든 클라이언트 (O(1) 조회/삭제를 위해 unordered_map으로 변경)
    std::unordered_map<uint64_t, ClientEndpoint> connectedClients;
    // Timeout in seconds for inactive clients
    const int CLIENT_TIMEOUT_SEC = 300; // 5분 타임아웃
    // Cleanup 주기 제한 (매 패킷마다 호출 방지)
    const int CLEANUP_INTERVAL_SEC = 10;
    std::chrono::steady_clock::time_point lastCleanupTime;

public:
    VoiceServer() : udpSocket(INVALID_SOCKET), lastCleanupTime(std::chrono::steady_clock::now()) {}

    void run(int port) {
        // Initialize Winsock
        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0) {
            std::cerr << "[Voice] WSAStartup failed: " << iResult << std::endl;
            return;
        }

        // Create a UDP socket
        udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
        if (udpSocket == INVALID_SOCKET) {
            std::cerr << "[Voice] Socket creation failed: " << WSAGetLastError() << std::endl;
            WSACleanup();
            return;
        }

        // Windows에서 WSAECONNRESET(10054) 오류 방지
        // 종료된 클라이언트에게 sendto 할 때 발생하는 ICMP 에러가
        // 다음 recvfrom에 전파되는 것을 억제합니다.
        BOOL bNewBehavior = FALSE;
        DWORD dwBytesReturned = 0;
        WSAIoctl(udpSocket, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
                 NULL, 0, &dwBytesReturned, NULL, NULL);

        // Bind the socket
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        serverAddr.sin_port = htons(port);

        if (bind(udpSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "[Voice] Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(udpSocket);
            WSACleanup();
            return;
        }

        std::cout << "[Voice] UDP 서버 시작! 포트: " << port << std::endl;

        runReceiveLoop();

        closesocket(udpSocket);
        WSACleanup();
    }

private:
    void runReceiveLoop() {
        char buffer[65536];
        sockaddr_in clientAddr;
        int clientAddrLen = sizeof(clientAddr);

        while (true) {
            int bytesReceived = recvfrom(udpSocket, buffer, sizeof(buffer), 0,
                                         (SOCKADDR*)&clientAddr, &clientAddrLen);
                                         
            if (bytesReceived == SOCKET_ERROR) {
                int err = WSAGetLastError();
                if (err == WSAECONNRESET) {
                    // 10054: 이미 종료된 클라이언트에 대한 ICMP 에러 → 무시
                    continue;
                }
                std::cerr << "[Voice] recvfrom failed: " << err << std::endl;
                continue;
            }

            // 아주 작은 패킷 무시 (최소 헤더 크기)
            if (bytesReceived < sizeof(VoicePacketHeader)) {
                std::cerr << "[Voice] 패킷 크기가 헤더보다 작음: " << bytesReceived << std::endl;
                continue;
            }

            auto now = std::chrono::steady_clock::now();

            // 10초마다 한 번만 Cleanup 수행 (매 패킷마다 O(N) 순회 방지)
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastCleanupTime).count();
            if (elapsed > CLEANUP_INTERVAL_SEC) {
                cleanupStaleClients();
                lastCleanupTime = now;
            }

            // 패킷 앞부분을 헤더 구조체로 캐스팅
            VoicePacketHeader* header = reinterpret_cast<VoicePacketHeader*>(buffer);

            // 문자열을 C++ string으로 변환
            std::string roomCode(header->roomCode);
            std::string speakerName(header->speakerName);

            uint64_t clientKey = makeClientKey(clientAddr);

            // ===== connectedClients 갱신 — O(1) upsert =====
            auto upsertClient = [&]() {
                connectedClients[clientKey] = {clientAddr, now};
            };

            // ===== packetType == 0: Join =====
            if (header->packetType == 0) {
                rooms[roomCode][speakerName] = {clientAddr, now};
                upsertClient();
                std::cout << "[Voice] " << speakerName << " 클라이언트가 [" << roomCode << "] 채널에 참여 (UDP - Raw Struct)" 
                          << " | 현재 접속자 수: " << connectedClients.size() << std::endl;
                continue; // Join 패킷은 브로드캐스트하지 않음
            }

            // ===== packetType == 2: Leave =====
            if (header->packetType == 2) {
                std::cout << "[Voice] " << speakerName << " 클라이언트가 [" << roomCode << "] 채널에서 나감 (Leave)" << std::endl;
                
                // rooms에서 제거
                if (rooms.count(roomCode)) {
                    rooms[roomCode].erase(speakerName);
                    if (rooms[roomCode].empty()) {
                        rooms.erase(roomCode);
                    }
                }
                
                // connectedClients에서 O(1) 제거
                connectedClients.erase(clientKey);
                std::cout << "[Voice] Leave 처리 완료 | 남은 접속자 수: " << connectedClients.size() << std::endl;
                continue; // Leave 패킷은 브로드캐스트하지 않음
            }

            // ===== packetType == 1: Voice Data (브로드캐스트) =====
            if (header->packetType == 1) {
                rooms[roomCode][speakerName] = {clientAddr, now};
                upsertClient();

                // 수신 로그 (페이로드 2바이트 이하는 Silence이므로 생략)
                if (header->payloadSize > 2) {
                    std::cout << "[Voice] 음성 수신 (방: " << roomCode << " / 화자: " << speakerName 
                              << " / 전체: " << bytesReceived << "B / 페이로드: " << header->payloadSize 
                              << "B) | 브로드캐스트 대상: " << (connectedClients.size() - 1) << "명" << std::endl;
                }

                // 브로드캐스트: connectedClients에 있는 나를 제외한 모든 유저에게
                int broadcastCount = 0;
                for (const auto& [key, endpoint] : connectedClients) {
                    if (key != clientKey) {
                        int sentBytes = sendto(udpSocket, buffer, bytesReceived, 0, (SOCKADDR*)&endpoint.addr, sizeof(endpoint.addr));
                        if (sentBytes == SOCKET_ERROR) {
                            std::cerr << "[Voice] sendto 실패! 대상: " << inet_ntoa(endpoint.addr.sin_addr) 
                                      << ":" << ntohs(endpoint.addr.sin_port) 
                                      << " 에러: " << WSAGetLastError() << std::endl;
                        } else {
                            broadcastCount++;
                        }
                    }
                }
                // 실제 브로드캐스트 로그 (Silence 제외)
                if (header->payloadSize > 2) {
                    std::cout << "[Voice] 브로드캐스트 완료: " << broadcastCount << "명에게 " << bytesReceived << "B 전송" << std::endl;
                }
            }
        }
    }

    void cleanupStaleClients() {
        auto now = std::chrono::steady_clock::now();

        // 1. rooms 맵에서 타임아웃된 클라이언트 제거
        for (auto itRoom = rooms.begin(); itRoom != rooms.end(); ) {
            auto& clientsMap = itRoom->second;
            for (auto itClient = clientsMap.begin(); itClient != clientsMap.end(); ) {
                auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - itClient->second.lastSeen).count();
                if (duration > CLIENT_TIMEOUT_SEC) {
                    std::cout << "[Voice] 클라이언트 연결 타임아웃 종료 (방: " << itRoom->first << " / 화자: " << itClient->first << ")" << std::endl;
                    itClient = clientsMap.erase(itClient);
                } else {
                    ++itClient;
                }
            }
            if (clientsMap.empty()) {
                itRoom = rooms.erase(itRoom);
            } else {
                ++itRoom;
            }
        }

        // 2. connectedClients에서 타임아웃된 클라이언트 제거 — O(1) 삭제
        for (auto it = connectedClients.begin(); it != connectedClients.end(); ) {
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - it->second.lastSeen).count();
            if (duration > CLIENT_TIMEOUT_SEC) {
                std::cout << "[Voice] connectedClients에서 타임아웃 클라이언트 제거: "
                          << inet_ntoa(it->second.addr.sin_addr) << ":" << ntohs(it->second.addr.sin_port) << std::endl;
                it = connectedClients.erase(it);
            } else {
                ++it;
            }
        }
    }
};