#pragma once
#include <string>
#include <uwebsockets/App.h>

struct Player {
    std::string name;
    bool isReady = false;
    bool isAlive = true;
    bool isEscaped = false;
    bool hasKillItem = false;
    uWS::WebSocket<false, true, int>* ws = nullptr;
};