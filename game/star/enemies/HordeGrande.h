#pragma once
#include "../Enemy.h"

// ROJO / Grande — Horda: solo baja.
class HordeGrande : public Enemy {
public:
    void pattern(float dt) override { advance(dt); }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=92; d.srcY=17; d.srcW=45; d.srcH=36; d.lives=3; d.scale=1.8f; d.speed=85.f;
        return makeEnemy<HordeGrande>(s, x, y, t, d);
    }
};
