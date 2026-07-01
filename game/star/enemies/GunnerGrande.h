#pragma once
#include "../Enemy.h"

// ROSA / Grande — Artillero: baja y dispara recto hacia abajo.
class GunnerGrande : public Enemy {
public:
    void pattern(float dt) override {
        advance(dt);
        if (every(fireInterval, dt)) fireOne(90.f, 380.f, BulletVisual::RojoBolt);
    }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=92; d.srcY=193; d.srcW=45; d.srcH=36; d.lives=3; d.scale=1.8f; d.speed=85.f;
        return makeEnemy<GunnerGrande>(s, x, y, t, d);
    }
};
