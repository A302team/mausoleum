#pragma once
#include <string>
#include <uwebsockets/App.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Player {
private:
    std::string name;
    bool ready = false;
    bool alive = true;
    uWS::WebSocket<false, true, int>* ws = nullptr;

public:
    Player() = default;
    Player(std::string name, uWS::WebSocket<false, true, int>* ws) 
        : name(std::move(name)), ws(ws) {}

    const std::string& getName() const { return name; }
    
    bool isReady() const { return ready; }
    void setReady(bool state) { ready = state; }
    
    bool isAlive() const { return alive; }
    void setAlive(bool state) { alive = state; }

    uWS::WebSocket<false, true, int>* getSocket() const { return ws; }

    // 플레이어에게 직접 메시지 전송
    void send(const json& message) const {
        if (ws) {
            ws->send(message.dump(), uWS::OpCode::TEXT);
        }
    }
};