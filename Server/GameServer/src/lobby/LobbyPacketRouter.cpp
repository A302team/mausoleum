#include "LobbyPacketRouter.h"

LobbyPacketRouter::LobbyPacketRouter(RoomManager& rm, LobbyClientManager& cm)
    : roomManager(rm), clientManager(cm)
{
    registerHandlers();
}

void LobbyPacketRouter::dispatch(WebSocketType* ws, std::string_view msg)
{
    try
    {
        json received = json::parse(msg);
        std::string type = received["type"];
        json data = received["data"];

        auto it = handlers.find(type);
        if (it != handlers.end())
        {
            it->second(ws, data);
        }
        else
        {
            LOG_WARN("Lobby", "알 수 없는 메시지 타입: " << type);
        }
    }
    catch (const std::exception& e)
    {
        LOG_ERROR("Lobby", "JSON 파싱 오류: " << e.what());
    }
}

void LobbyPacketRouter::registerHandlers()
{
    handlers["create_room"] = [this](auto* ws, const json& data) { handleCreateRoom(ws, data); };
    handlers["join_room"] = [this](auto* ws, const json& data) { handleJoinRoom(ws, data); };
    handlers["ready"] = [this](auto* ws, const json& data) { handleReady(ws, data); };
    handlers["start_game"] = [this](auto* ws, const json& data) { handleStartGame(ws, data); };
    handlers["get_room_list"] = [this](auto* ws, const json& data) { handleGetRoomList(ws, data); };
    handlers["check_nickname"] = [this](auto* ws, const json& data) { handleCheckNickname(ws, data); };
    handlers["leave_room"] = [this](auto* ws, const json& data) { handleLeaveRoom(ws, data); };
    handlers["chat_message"] = [this](auto* ws, const json& data) { handleChatMessage(ws, data); };
}

auto LobbyPacketRouter::createSendFunc(WebSocketType* ws) {
    return [ws](const std::string& msg) {
        ws->send(msg, uWS::OpCode::TEXT);
    };
}

void LobbyPacketRouter::sendError(WebSocketType* ws, const std::string& message)
{
    ws->send(json({{"type", "error"},
                   {"data", {{"message", message}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}

bool LobbyPacketRouter::checkNameAndSendErrorIfTaken(WebSocketType* ws, const std::string& playerName)
{
    if (roomManager.isNameTakenGlobal(playerName))
    {
        sendError(ws, "이미 사용 중인 닉네임입니다.");
        return true;
    }
    return false;
}

bool LobbyPacketRouter::extractContext(const json& data, std::string& outRoomCode, std::string& outPlayerName, Room*& outRoom)
{
    outRoomCode = data.value("roomCode", "");
    outPlayerName = data.value("playerName", "");
    outRoom = roomManager.getRoom(outRoomCode);
    return outRoom != nullptr;
}

void LobbyPacketRouter::handleCreateRoom(WebSocketType* ws, const json& data)
{
    std::string playerName = data["playerName"];

    if (checkNameAndSendErrorIfTaken(ws, playerName)) return;

    Room& room = roomManager.createRoom();
    room.setHostId(playerName);
    room.addPlayer(Player(playerName, createSendFunc(ws)));
    clientManager.addClient(ws, {room.getRoomCode(), playerName});

    LOG_INFO("Lobby", "[" << room.getRoomCode() << "] " << playerName << " 님 방 생성 (방장)");

    ws->send(json({{"type", "room_created"},
                   {"data", {{"roomCode", room.getRoomCode()}, {"playerName", playerName}, {"isHost", true}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}

void LobbyPacketRouter::handleJoinRoom(WebSocketType* ws, const json& data)
{
    std::string roomCode = data["roomCode"];
    std::string playerName = data["playerName"];

    if (!roomManager.roomExists(roomCode))
    {
        sendError(ws, "존재하지 않는 코드입니다.");
        return;
    }

    if (checkNameAndSendErrorIfTaken(ws, playerName)) return;

    Room* room = roomManager.getRoom(roomCode);
    if (room)
    {
        room->addPlayer(Player(playerName, createSendFunc(ws)));
        clientManager.addClient(ws, {roomCode, playerName});

        LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 입장");

        ws->send(json({{"type", "room_joined"},
                       {"data", {{"roomCode", roomCode}, {"playerName", playerName}, {"isHost", room->getHostId() == playerName}}}})
                     .dump(),
                 uWS::OpCode::TEXT);

        room->broadcast({{"type", "player_entered"},
                         {"data", {{"playerName", playerName}}}});
    }
}

void LobbyPacketRouter::handleReady(WebSocketType* ws, const json& data)
{
    std::string roomCode, playerName;
    Room* room = nullptr;

    if (extractContext(data, roomCode, playerName, room))
    {
        Player* player = room->getPlayer(playerName);
        if (player)
        {
            player->setReady(true);
            LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 레디!");

            room->broadcast({{"type", "player_ready"},
                             {"data", {{"playerName", playerName}}}});
        }
    }
}

void LobbyPacketRouter::handleStartGame(WebSocketType* ws, const json& data)
{
    std::string roomCode, playerName;
    Room* room = nullptr;

    if (extractContext(data, roomCode, playerName, room))
    {
        if (room->getHostId() != playerName)
        {
            sendError(ws, "방장만 시작 가능합니다.");
            return;
        }

        if (!room->allReady())
        {
            sendError(ws, "모든 플레이어가 레디해야 합니다.");
            return;
        }

        room->setPhase(GamePhase::InGame);
        LOG_INFO("Lobby", "===== [" << roomCode << "] 게임 시작! =====");

        room->broadcast({{"type", "game_started"},
                         {"data", {{"roomCode", roomCode}}}});
    }
}

void LobbyPacketRouter::handleGetRoomList(WebSocketType* ws, const json& data)
{
    json roomList = json::array();

    for (const auto& [roomCode, room] : roomManager.getAllRooms())
    {
        if (room.getPhase() == GamePhase::Waiting)
        {
            roomList.push_back({{"roomCode", roomCode},
                                {"playerCount", room.getPlayerCount()}});
        }
    }

    ws->send(json({{"type", "room_list"},
                   {"data", {{"rooms", roomList}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}

void LobbyPacketRouter::handleCheckNickname(WebSocketType* ws, const json& data)
{
    std::string playerName = data["playerName"];

    if (checkNameAndSendErrorIfTaken(ws, playerName)) return;

    ws->send(json({{"type", "nickname_available"},
                   {"data", {{"playerName", playerName}}}})
                 .dump(),
             uWS::OpCode::TEXT);
}

void LobbyPacketRouter::handleLeaveRoom(WebSocketType* ws, const json& data)
{
    std::string roomCode, playerName;
    Room* room = nullptr;

    if (extractContext(data, roomCode, playerName, room))
    {
        room->removePlayer(playerName);
        clientManager.removeClient(ws);

        LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << " 님 방에서 퇴장");

        room->broadcast({{"type", "player_left"},
                         {"data", {{"playerName", playerName}}}});

        roomManager.removeRoomIfEmpty(roomCode);
    }
}

void LobbyPacketRouter::handleChatMessage(WebSocketType* ws, const json& data)
{
    std::string roomCode, playerName;
    Room* room = nullptr;
    std::string message = data.value("message", "");

    if (extractContext(data, roomCode, playerName, room))
    {
        LOG_INFO("Lobby", "[" << roomCode << "] " << playerName << ": " << message);

        room->broadcast({
            {"type", "chat_message"},
            {"data", {{"playerName", playerName}, {"message", message}}}
        });
    }
}
