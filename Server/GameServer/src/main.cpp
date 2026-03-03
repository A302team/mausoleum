#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>
#include <iostream>
#include "RoomManager.h"

using json = nlohmann::json;

RoomManager roomManager;

void handleMessage(uWS::WebSocket<false, true, int> *ws,
                   std::string_view msg,
                   RoomManager &rm)
{
    try
    {
        json received = json::parse(msg);
        std::string type = received["type"];
        json data = received["data"];

        if (type == "create_room")
        {
            std::string playerName = data["playerName"];

            if (rm.isNameTakenGlobal(playerName))
            {
                ws->send(json({{"type", "error"},
                               {"data", {{"message", "이미 사용 중인 닉네임입니다."}}}})
                             .dump(),
                         uWS::OpCode::TEXT);
                return;
            }

            Room &room = rm.createRoom();
            std::string roomCode = room.roomId;

            Player player;
            player.name = playerName;
            player.ws = ws;
            room.addPlayer(player);
            room.hostId = playerName;

            std::cout << "[" << roomCode << "]" << playerName << " 님 방 생성 (방장)" << std::endl;

            ws->send(json({{"type", "room_created"},
                           {"data", {{"roomCode", roomCode}, {"playerName", playerName}, {"isHost", true}}}})
                         .dump(),
                     uWS::OpCode::TEXT);
        }

        // 방 입장
        else if (type == "join_room")
        {
            std::string roomCode = data["roomCode"];
            std::string playerName = data["playerName"];

            if (!rm.roomExists(roomCode))
            {
                ws->send(json({{"type", "error"},
                               {"data", {{"message", "존재하지 않는 코드입니다."}}}})
                             .dump(),
                         uWS::OpCode::TEXT);
                return;
            }

            Room &room = rm.rooms[roomCode];

            // 닉네임 중복 체크
            if (rm.isNameTakenGlobal(playerName))
            {
                ws->send(json({{"type", "error"},
                               {"data", {{"message", "이미 사용 중인 닉네임입니다."}}}})
                             .dump(),
                         uWS::OpCode::TEXT);
                return;
            }

            Player player;
            player.name = playerName;
            player.ws = ws;
            room.addPlayer(player);

            std::cout << "[" << roomCode << "] " << playerName << " 님 입장" << std::endl;

            room.sendTo(playerName, {{"type", "room_joined"},
                                     {"data", {{"roomCode", roomCode}, {"playerName", playerName}, {"isHost", room.hostId == playerName}}}});

            room.broadcast({{"type", "player_entered"},
                            {"data", {{"playerName", playerName}}}});
        }

        else if (type == "ready")
        {
            std::string roomCode = data["roomCode"];
            std::string playerName = data["playerName"];

            if (!rm.roomExists(roomCode))
                return;

            Room &room = rm.rooms[roomCode];
            if (room.players.count(playerName))
            {
                room.players[playerName].isReady = true;
                std::cout << "[" << roomCode << "] " << playerName << " 님 레디!" << std::endl;

                room.broadcast({{"type", "player_ready"},
                                {"data", {{"playerName", playerName}}}});
            }
        }

        else if (type == "start_game")
        {
            std::string roomCode = data["roomCode"];
            std::string playerName = data["playerName"];

            Room &room = rm.rooms[roomCode];

            if (room.hostId != playerName)
            {
                room.sendTo(playerName, {{"type", "error"},
                                         {"data", {{"message", "방장만 시작 가능합니다."}}}});
                return;
            }

            if (!room.allReady())
            {
                room.sendTo(playerName, {{"type", "error"},
                                         {"data", {{"message", "모든 플레이어가 레디해야 합니다."}}}});
                return;
            }

            room.phase = GamePhase::InGame;
            std::cout << "===== [" << roomCode << "] 게임 시작! =====" << std::endl;

            room.broadcast({{"type", "game_started"},
                            {"data", {{"roomCode", roomCode}}}});
        }

        else if (type == "get_room_list")
        {
            json roomList = json::array();
            for (auto &[roomCode, room] : rm.rooms)
            {
                if (room.phase == GamePhase::Waiting)
                {
                    std::cout << "[" << roomCode << "] 현재 인원: " << room.players.size() << std::endl;
                    roomList.push_back({{"roomCode", roomCode},
                                        {"playerCount", room.players.size()}});
                }
            }

            ws->send(json({{"type", "room_list"},
                           {"data", {{"rooms", roomList}}}})
                         .dump(),
                     uWS::OpCode::TEXT);

            std::cout << "방 목록 요청" << std::endl;
        }

        else if (type == "check_nickname")
        {
            std::string playerName = data["playerName"];

            if (rm.isNameTakenGlobal(playerName))
            {
                ws->send(json({{"type", "error"},
                               {"data", {{"message", "이미 사용 중인 닉네임입니다."}}}})
                             .dump(),
                         uWS::OpCode::TEXT);
                return;
            }

            ws->send(json({{"type", "nickname_available"},
                           {"data", {{"playerName", playerName}}}})
                         .dump(),
                     uWS::OpCode::TEXT);

            std::cout << "[닉네임 체크] " << playerName << " 사용 가능" << std::endl;
        }

        else if (type == "leave_room")
        {
            std::string roomCode = data["roomCode"];
            std::string playerName = data["playerName"];

            if (!rm.roomExists(roomCode))
                return;

            Room &room = rm.rooms[roomCode];
            room.removePlayer(playerName);

            std::cout << "[" << roomCode << "] " << playerName << " 님 퇴장" << std::endl;

            room.broadcast({{"type", "player_left"},
                            {"data", {{"playerName", playerName}}}});

            rm.removeRoomIfEmpty(roomCode);
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "오류: " << e.what() << std::endl;
    }
}

int main()
{
    uWS::App().ws<int>("/*", {.open = [](auto *ws)
                              { std::cout << "클라이언트 접속!" << std::endl; },
                              .message = [](auto *ws, std::string_view msg, uWS::OpCode opCode)
                              { handleMessage(ws, msg, roomManager); },
                              .close = [](auto *ws, int, std::string_view)
                              {
            // 연결 끊기면 방에서 제거
            auto [roomId, playerId] = roomManager.findPlayerByWs(ws);
            if (!roomId.empty()) {
                Room& room = roomManager.rooms[roomId];
                std::string playerName = room.players[playerId].name;
                room.removePlayer(playerId);
                std::cout << "[" << roomId << "] " << playerName << " 님 퇴장" << std::endl;

                room.broadcast({
                    {"type", "player_left"},
                    {"data", {{"playerId", playerId}}}
                });

                roomManager.removeRoomIfEmpty(roomId);
            } }})
        .listen(9001, [](auto *token)
                {
        if (token) std::cout << "서버 시작! 포트: 9001" << std::endl; })
        .run();
}