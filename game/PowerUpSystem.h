#pragma once

#include "../engine/Component.h"

struct SDL_Texture;
class PlayerController;
class SpriteAnimator;
class SpriteRenderer;

enum class PowerUpType {
    Heart,
    Bomb,
    Shield
};

class PowerUpPickup : public Component {
public:
    PowerUpPickup(PowerUpType type, PlayerController* player,
                  float worldX, float firstSpawnDelay, float respawnDelay);

    void awake() override;
    void update(float dt) override;

private:
    PowerUpType type;
    PlayerController* player = nullptr;
    SpriteRenderer* sprite = nullptr;
    SpriteAnimator* animator = nullptr;

    float baseX = 0.0f;
    float spawnTimer = 0.0f;
    float respawnDelay = 8.0f;
    float fallSpeed = 95.0f;
    float wave = 0.0f;
    bool active = false;

    void activate();
    void deactivate();
    void applyToPlayer();
    bool overlapsPlayer() const;
    int bonusColumn() const;
};

class ShieldVisual : public Component {
public:
    explicit ShieldVisual(PlayerController* player) : player(player) {}

    void awake() override;
    void update(float dt) override;

private:
    PlayerController* player = nullptr;
    SpriteRenderer* sprite = nullptr;
    float pulse = 0.0f;
};

class PowerUpHUD : public Component {
public:
    explicit PowerUpHUD(PlayerController* player) : player(player) {}

    void awake() override;
    void render() override;

private:
    PlayerController* player = nullptr;
    SDL_Texture* bonusesTexture = nullptr;

    void drawIcon(float x, float y, int column, float alpha = 255.0f) const;
    void drawSlot(float x, float y) const;
    void drawBar(float x, float y, float w, float h, float ratio,
                 unsigned char r, unsigned char g, unsigned char b) const;
};

class PowerUpDirector : public Component {
public:
    explicit PowerUpDirector(PlayerController* player) : player(player) {}

    void awake() override;

private:
    PlayerController* player = nullptr;

    void spawnPickup(PowerUpType type, float worldX,
                     float firstSpawnDelay, float respawnDelay);
};
