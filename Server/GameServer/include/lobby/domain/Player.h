#pragma once
#include <string>
#include <functional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Player {
public:
    using SendFunc = std::function<void(const std::string&)>;

private:
    std::string name;
    bool ready = false;
    bool alive = true;
    SendFunc sendFunc;

public:
    Player() = default;
    Player(std::string name, SendFunc sendFunc);

    const std::string& getName() const { return name; }
    
    bool isReady() const { return ready; }
    void setReady(bool state) { ready = state; }
    
    bool isAlive() const { return alive; }
    void setAlive(bool state) { alive = state; }

    void send(const json& message) const;
    void send(const std::string& message) const;
};