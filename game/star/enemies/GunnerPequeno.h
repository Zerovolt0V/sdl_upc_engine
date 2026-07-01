#pragma once
#include "../Enemy.h"
#include "../Paths.h"

// ROSA / Pequeño — Artillero (dispara hacia abajo / cortinas). Varias personalidades.
class GunnerPequeno : public Enemy {
public:
    enum Move { Strafer, StrafeRun, Spreader, RingDrop, Spiral };

    void pattern(float dt) override {
        switch ((Move)variant) {

        case Strafer: // baja disparando recto hacia abajo
            advance(dt);
            if (every(fireInterval, dt)) fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt);
            break;

        case StrafeRun: // pasada por una curva ametrallando hacia abajo
            if (followPath(Paths::SwoopRight, 220.f, dt)) { die(); return; }
            if (every(fireInterval, dt)) fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt);
            break;

        case Spreader: // entra, se planta y suelta un abanico hacia abajo, y se va
            switch (phase) {
            case 0: advance(dt); if (depthOnScreen() > -150.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt);
                if (every(fireInterval, dt)) fireSpread(5, 60.f, 90.f, bulletSpeed, BulletVisual::RojoOrbe);
                if (tPhase > 3.0f) nextPhase();
                break;
            default: retreat(90.f, 300.f, dt); break;
            }
            break;

        case RingDrop: // baja soltando anillos de balas (orbe rojo, redondo)
            advance(dt);
            if (every(1.6f, dt)) fireRing(10, bulletSpeed * 0.9f, 0.f, BulletVisual::RojoOrbe);
            break;

        default: // Spiral: se planta y rocía una espiral
            switch (phase) {
            case 0: advance(dt); if (depthOnScreen() > -150.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt);
                if (every(0.48f, dt)) fireSpiral(24.f, bulletSpeed * 0.85f, BulletVisual::RojoOrbe);
                if (tPhase > 3.0f) nextPhase();
                break;
            default: retreat(90.f, 300.f, dt); break;
            }
            break;
        }
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = Strafer) {
        EnemyDef d;
        d.srcX=198; d.srcY=203; d.srcW=21; d.srcH=17; d.lives=1; d.scale=1.6f; d.speed=110.f;
        d.fireInterval = 1.0f;   // cadencia base (como el sniper)
        d.bulletSpeed  = 210.f;  // velocidad de bala base (más lenta, como el sniper)
        d.variant = (int)move;
        d.ignoreGate = (move == StrafeRun); // el de curva se auto-gestiona
        return makeEnemy<GunnerPequeno>(s, x, y, t, d);
    }
};
