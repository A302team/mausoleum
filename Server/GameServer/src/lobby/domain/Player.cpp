#include "Player.h"

Player::Player(std::string name, SendFunc sendFunc) 
    : name(std::move(name)), sendFunc(std::move(sendFunc)) {} // 이동 생성자 사용하여 불필요한 복사 제거

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
