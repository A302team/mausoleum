#pragma once
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <unordered_set>
#include <iostream>

using json = nlohmann::json;
using WebSocketType = uWS::WebSocket<false, true, int>;

class VoiceServer {
private:
    std::unordered_map<std::string, std::unordered_set<WebSocketType*>> roomSockets;
    std::unordered_map<WebSocketType*, std::string> socketToRoom;
    // 모든 유저들을 모아놓은
    std::unordered_set<WebSocketType*> allSockets;
public:
    void run(int port) {
        uWS::App().ws<int>("/*", {
            .maxPayloadLength = 16 * 1024 * 1024,
            .idleTimeout = 120,
            .open = [this](auto* ws) {
                std::cout << "[Voice] 클라이언트 접속!" << std::endl;
                allSockets.insert(ws);
            },
            .message = [this](auto* ws, std::string_view msg, uWS::OpCode opCode) {
                this->onMessage(ws, msg, opCode);
            },
            .close = [this](auto* ws, int, std::string_view) {
                this->onDisconnect(ws);
            }
        }).listen(port, [port](auto* token) {
            if (token) std::cout << "[Voice] 서버 시작! 포트: " << port << std::endl;
        }).run();
    }

private:
    void onMessage(WebSocketType* ws, std::string_view msg, uWS::OpCode opCode) {
        try {
            json received = json::parse(msg);
            std::string type = received["type"];
            
            if (type == "join_voice") {
                std::string roomCode = received["roomCode"];
                if (socketToRoom.count(ws)) {
                    std::string oldRoom = socketToRoom[ws];
                    roomSockets[oldRoom].erase(ws);
                }
                socketToRoom[ws] = roomCode;
                roomSockets[roomCode].insert(ws);
                std::cout << "[Voice] 클라이언트가 [" << roomCode << "] 채널 합류" << std::endl;
            }
            else if (type == "voice_data") {
                // if (socketToRoom.find(ws) == socketToRoom.end()) return;

                std::string roomCode = socketToRoom[ws];

                std::cout << "[Voice] 음성 수신 중... (방: " << roomCode << " / 크기: " << msg.length() << " bytes)" << std::endl;
                // // 릴레이 (본인 제외)

                BroadcastToAll(msg, opCode);
                // for (auto* client : roomSockets[roomCode]) {
                //     if (client != ws) {
                //         std::cout << "[Voice] 음성 릴레이 중... (방: " << roomCode << " / 크기: " << msg.length() << " bytes)" << std::endl;
                //         client->send(msg, opCode);
                //     }
                // }
            }
        } catch (const std::exception& e) {
            std::cerr << "[Voice] JSON 파싱 오류: " << e.what() << std::endl;
        }
    }

    void onDisconnect(WebSocketType* ws) {
        if (socketToRoom.count(ws)) {
            std::string roomCode = socketToRoom[ws];
            roomSockets[roomCode].erase(ws);
            
            if (roomSockets[roomCode].empty()) {
                roomSockets.erase(roomCode);
            }
            
            socketToRoom.erase(ws);
            std::cout << "[Voice] 클라이언트 연결 해제 (방: " << roomCode << ")" << std::endl;
        }
        allSockets.erase(ws);
    }

    void BroadcastToAll(const std::string_view& message, uWS::OpCode opCode) {
        for (auto* client : allSockets) {
            std::cout << "[Voice] 전체 릴레이 중... (크기: " << message.length() << " bytes)" << std::endl;
            client->send(message, opCode);
        }
    }
};