#include "PowerUpSystem.h"

#include "PlayerController.h"
#include "../engine/GameObject.h"
#include "../engine/Scene.h"
#include "../engine/SpriteAnimator.h"
#include "../engine/SpriteRenderer.h"
#include "../engine/Transform.h"

#include <SDL3/SDL.h>
#include <cmath>

namespace {
constexpr const char* kBonusSheet = "assets/Shoot`em Up/Bonuses-0001.png";
constexpr float kBonusFrame = 32.0f;
constexpr float kBarrierFrame = 88.0f;

float clamp01(float value) {
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}
}

PowerUpPickup::PowerUpPickup(PowerUpType type, PlayerController* player,
                             float worldX, float firstSpawnDelay,
                             float respawnDelay)
    : type(type),
      player(player),
      baseX(worldX),
      spawnTimer(firstSpawnDelay),
      respawnDelay(respawnDelay) {}

void PowerUpPickup::awake() {
    sprite = gameObject->getComponent<SpriteRenderer>();
    animator = gameObject->getComponent<SpriteAnimator>();

    if (sprite) {
        sprite->setSourceRect(bonusColumn() * kBonusFrame, 0.0f,
                              kBonusFrame, kBonusFrame);
    }

    if (animator) {
        int col = bonusColumn();
        animator->addAnimation("idle",
                               { col, col + 5, col + 10, col + 15, col + 20 },
                               9.0f);
        animator->play("idle");
    }

    gameObject->transform->scaleX = 1.5f;
    gameObject->transform->scaleY = 1.5f;
    gameObject->transform->x = baseX;
    gameObject->transform->y = -1000.0f;
}

void PowerUpPickup::update(float dt) {
    if (!player) return;

    if (!active) {
        spawnTimer -= dt;
        if (spawnTimer <= 0.0f) activate();
        return;
    }

    Transform* t = gameObject->transform;
    wave += dt * 3.0f;
    t->x = baseX + std::sin(wave) * 22.0f;
    t->y += fallSpeed * dt;

    if (overlapsPlayer()) {
        applyToPlayer();
        deactivate();
        return;
    }

    if (t->y > 430.0f) {
        deactivate();
    }
}

void PowerUpPickup::activate() {
    active = true;
    wave = 0.0f;
    gameObject->transform->x = baseX;
    gameObject->transform->y = -420.0f;
    gameObject->transform->scaleX = 1.5f;
    gameObject->transform->scaleY = 1.5f;
}

void PowerUpPickup::deactivate() {
    active = false;
    spawnTimer = respawnDelay;
    gameObject->transform->x = baseX;
    gameObject->transform->y = -1000.0f;
}

void PowerUpPickup::applyToPlayer() {
    switch (type) {
    case PowerUpType::Heart:
        player->addHeart();
        break;
    case PowerUpType::Bomb:
        player->addBomb();
        break;
    case PowerUpType::Shield:
        player->activateShield();
        break;
    }
}

bool PowerUpPickup::overlapsPlayer() const {
    if (!player || !player->gameObject) return false;

    const Transform* pickup = gameObject->transform;
    const Transform* ship = player->gameObject->transform;

    float pw = kBonusFrame * pickup->scaleX;
    float ph = kBonusFrame * pickup->scaleY;
    float sw = player->getCollisionWidth();
    float sh = player->getCollisionHeight();

    return pickup->x < ship->x + sw &&
           pickup->x + pw > ship->x &&
           pickup->y < ship->y + sh &&
           pickup->y + ph > ship->y;
}

int PowerUpPickup::bonusColumn() const {
    switch (type) {
    case PowerUpType::Heart:  return 0;
    case PowerUpType::Shield: return 1;
    case PowerUpType::Bomb:   return 3;
    }
    return 0;
}

void ShieldVisual::awake() {
    sprite = gameObject->getComponent<SpriteRenderer>();
    if (sprite) {
        sprite->setSourceRect(kBarrierFrame, 0.0f, kBarrierFrame, kBarrierFrame);
    }
    gameObject->transform->scaleX = 0.0f;
    gameObject->transform->scaleY = 0.0f;
    gameObject->transform->y = -1000.0f;
}

void ShieldVisual::update(float dt) {
    if (!player || !player->gameObject || !player->isShielded()) {
        gameObject->transform->scaleX = 0.0f;
        gameObject->transform->scaleY = 0.0f;
        gameObject->transform->y = -1000.0f;
        return;
    }

    pulse += dt * 5.0f;
    float scale = 1.12f + std::sin(pulse) * 0.04f;
    Transform* shield = gameObject->transform;
    Transform* ship = player->gameObject->transform;

    float centerX = ship->x + player->getCollisionWidth() * 0.5f;
    float centerY = ship->y + player->getCollisionHeight() * 0.5f;
    float drawn = kBarrierFrame * scale;

    shield->scaleX = scale;
    shield->scaleY = scale;
    shield->x = centerX - drawn * 0.5f;
    shield->y = centerY - drawn * 0.5f;
}

void PowerUpHUD::awake() {
    bonusesTexture = gameObject->scene->getAssets().loadTexture(kBonusSheet);
}

void PowerUpHUD::render() {
    if (!player) return;

    SDL_Renderer* renderer = gameObject->scene->getRenderer();
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    constexpr float left = 24.0f;
    constexpr float top = 22.0f;
    constexpr float iconStep = 34.0f;

    for (int i = 0; i < player->getMaxHearts(); ++i) {
        drawSlot(left + i * iconStep, top);
        if (i < player->getHearts()) drawIcon(left + i * iconStep, top, 0);
    }

    float bombsY = top + 42.0f;
    for (int i = 0; i < player->getMaxBombs(); ++i) {
        float x = left + i * 26.0f;
        drawSlot(x, bombsY);
        if (i < player->getBombs()) drawIcon(x, bombsY, 3, 235.0f);
    }

    float shieldY = bombsY + 42.0f;
    drawIcon(left, shieldY, 1, player->isShielded() ? 255.0f : 80.0f);
    float shieldRatio = player->getShieldDuration() > 0.0f
        ? player->getShieldTimeLeft() / player->getShieldDuration()
        : 0.0f;
    drawBar(left + 42.0f, shieldY + 8.0f, 160.0f, 10.0f,
            shieldRatio, 70, 180, 255);
}

void PowerUpHUD::drawIcon(float x, float y, int column, float alpha) const {
    if (!bonusesTexture) return;

    SDL_SetTextureAlphaMod(bonusesTexture, static_cast<Uint8>(alpha));
    SDL_FRect src{ column * kBonusFrame, 0.0f, kBonusFrame, kBonusFrame };
    SDL_FRect dst{ x, y, 28.0f, 28.0f };
    SDL_RenderTexture(gameObject->scene->getRenderer(), bonusesTexture, &src, &dst);
    SDL_SetTextureAlphaMod(bonusesTexture, 255);
}

void PowerUpHUD::drawSlot(float x, float y) const {
    SDL_Renderer* renderer = gameObject->scene->getRenderer();
    SDL_FRect slot{ x - 2.0f, y - 2.0f, 32.0f, 32.0f };
    SDL_SetRenderDrawColor(renderer, 12, 18, 35, 150);
    SDL_RenderFillRect(renderer, &slot);
}

void PowerUpHUD::drawBar(float x, float y, float w, float h, float ratio,
                         unsigned char r, unsigned char g,
                         unsigned char b) const {
    SDL_Renderer* renderer = gameObject->scene->getRenderer();
    SDL_FRect bg{ x, y, w, h };
    SDL_SetRenderDrawColor(renderer, 12, 18, 35, 180);
    SDL_RenderFillRect(renderer, &bg);

    SDL_FRect fill{ x, y, w * clamp01(ratio), h };
    SDL_SetRenderDrawColor(renderer, r, g, b, 230);
    SDL_RenderFillRect(renderer, &fill);
}

void PowerUpDirector::awake() {
    spawnPickup(PowerUpType::Heart, -320.0f, 1.5f, 13.0f);
    spawnPickup(PowerUpType::Bomb, 40.0f, 4.5f, 9.0f);
    spawnPickup(PowerUpType::Shield, 280.0f, 8.0f, 17.0f);
}

void PowerUpDirector::spawnPickup(PowerUpType type, float worldX,
                                  float firstSpawnDelay, float respawnDelay) {
    if (!player || !gameObject || !gameObject->scene) return;

    GameObject* pickup = gameObject->scene->createGameObject("PowerUp");
    pickup->addComponent<SpriteRenderer>(kBonusSheet);
    pickup->addComponent<SpriteAnimator>(32, 32, 5);
    pickup->addComponent<PowerUpPickup>(type, player, worldX,
                                        firstSpawnDelay, respawnDelay);
}
