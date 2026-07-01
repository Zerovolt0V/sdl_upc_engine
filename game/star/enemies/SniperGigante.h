#pragma once
#include "../Enemy.h"

// AMARILLO / Gigante — Francotirador: baja, te apunta y dispara orbes.
class SniperGigante : public Enemy {
public:
    void pattern(float dt) override {
        advance(dt);
        faceTarget();
        if (every(fireInterval, dt)) fireAimed(340.f, BulletVisual::AmarilloOrbe);
    }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=33; d.srcY=100; d.srcW=47; d.srcH=53; d.lives=7; d.scale=1.9f; d.speed=60.f;
        return makeEnemy<SniperGigante>(s, x, y, t, d);
    }
};
