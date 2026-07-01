#pragma once
#include "../Enemy.h"

// ROSA / Gigante — Artillero con TRES cañones: dispara recto abajo desde los 3.
// (Ejemplo de cañones múltiples definidos en la ficha.)
class GunnerGigante : public Enemy {
public:
    void pattern(float dt) override {
        advance(dt);
        if (every(fireInterval, dt))
            for (int m = 0; m < muzzleCount(); ++m)
                fireOne(90.f, 360.f, BulletVisual::RojoBolt, 1.0f, m);
    }
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t) {
        EnemyDef d;
        d.srcX=33; d.srcY=187; d.srcW=47; d.srcH=53; d.lives=7; d.scale=1.9f; d.speed=60.f;
        d.muzzles = { {-16.f, 21.f}, {0.f, 26.f}, {16.f, 21.f} }; // izq, centro, der (px de sprite)
        return makeEnemy<GunnerGigante>(s, x, y, t, d);
    }
};
