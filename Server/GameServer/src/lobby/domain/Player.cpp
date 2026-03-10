#include "Player.h"

Player::Player(std::string name, SendFunc sendFunc) 
    : name(std::move(name)), sendFunc(std::move(sendFunc)) {}

void Player::send(const json& message) const {
    if (sendFunc) {
        sendFunc(message.dump());
    }
}

void Player::send(const std::string& message) const {
    if (sendFunc) {
        sendFunc(message);
    }
}
