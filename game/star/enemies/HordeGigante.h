#pragma once
#include "../Enemy.h"

// ROJO / Gigante — Horda: solo baja (mucha vida, muy lento).
class HordeGigante : public Enemy {
public:
    void pattern(float dt) override { advance(dt); }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=33; d.srcY=11; d.srcW=47; d.srcH=53; d.lives=8; d.scale=1.9f; d.speed=60.f;
        return makeEnemy<HordeGigante>(s, x, y, t, d);
    }
};
