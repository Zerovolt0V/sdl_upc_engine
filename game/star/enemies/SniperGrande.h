#pragma once
#include "../Enemy.h"

// AMARILLO / Grande — Francotirador pesado: dispara un ABANICO apuntado (5 orbes).
// (Ejemplo del bloque fireSpread.)
class SniperGrande : public Enemy {
public:
    void pattern(float dt) override {
        advance(dt);
        faceTarget();
        if (every(fireInterval, dt))
            fireSpread(5, 40.f, angleToTarget(0), 300.f, BulletVisual::AmarilloOrbe);
    }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=92; d.srcY=106; d.srcW=45; d.srcH=36; d.lives=3; d.scale=1.8f; d.speed=85.f;
        return makeEnemy<SniperGrande>(s, x, y, t, d);
    }
};
