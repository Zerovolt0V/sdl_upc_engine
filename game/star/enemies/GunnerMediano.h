#pragma once
#include "../Enemy.h"
#include "../Paths.h"

// ROSA / Mediano — Artillero pesado con 2 CAÑONES gemelos. 3 vidas.
class GunnerMediano : public Enemy {
public:
    enum Move { AlternateStrafe, CrossFire, TwinSpiral, StrafeAcrossR, StrafeAcrossL };

    void fireTwinDown() {
        for (int m = 0; m < muzzleCount(); ++m)
            fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt, 1.0f, m);
    }

    // Entra en diagonal al lado de inicio y CRUZA a la pantalla al otro lado
    // (dir = +1 derecha, -1 izquierda), bancando y ametrallando. Sale por el borde.
    void strafeAcross(float dir, float dt) {
        switch (phase) {
        case 0: { // baja y deriva al lado de inicio (opuesto al destino)
            advance(dt);
            float startX = -dir * 260.f, k = 2.f * dt; if (k > 1.f) k = 1.f;
            gameObject->transform->x += (startX - gameObject->transform->x) * k;
            if (depthOnScreen() > -220.f) nextPhase();
            break;
        }
        default: // cruza al otro lado bancando y disparando (sale -> el gate lo limpia)
            holdOnScreen(dt);
            gameObject->transform->x += dir * 160.f * dt;
            rotateToward(dir * 20.f, 180.f, dt); // banca hacia el movimiento
            if (every(fireInterval * 0.6f, dt)) fireTwinDown();
            break;
        }
    }

    void pattern(float dt) override {
        switch ((Move)variant) {

        case AlternateStrafe: // baja; los 2 cañones disparan ALTERNADOS (medio tiempo)
            advance(dt);
            if (every(fireInterval, dt))            fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt, 1.0f, 0);
            if (every(fireInterval, dt, sideTimer)) fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt, 1.0f, 1);
            break;

        case CrossFire: // se planta; los cañones cruzan sus disparos en X
            switch (phase) {
            case 0: advance(dt); if (depthOnScreen() > -180.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt);
                if (every(fireInterval * 0.8f, dt)) {
                    fireOne(112.f, bulletSpeed, BulletVisual::RojoOrbe, 1.0f, 0); // izq -> abajo-derecha
                    fireOne(68.f,  bulletSpeed, BulletVisual::RojoOrbe, 1.0f, 1); // der -> abajo-izquierda
                }
                if (tPhase > 3.5f) nextPhase();
                break;
            default: retreat(90.f, 300.f, dt); break;
            }
            break;

        case TwinSpiral: // se planta; 2 espirales que giran en sentidos OPUESTOS
            switch (phase) {
            case 0: advance(dt); if (depthOnScreen() > -180.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt);
                if (every(0.3f, dt)) { // menos cadencia
                    fireOne(spinA, bulletSpeed * 0.9f, BulletVisual::RojoOrbe, 1.0f, 0); spinA += 42.f; // brazos más ralos
                    fireOne(spinB, bulletSpeed * 0.9f, BulletVisual::RojoOrbe, 1.0f, 1); spinB -= 42.f;
                }
                if (tPhase > 3.0f) nextPhase();
                break;
            default: retreat(90.f, 300.f, dt); break;
            }
            break;

        case StrafeAcrossR: strafeAcross(+1.f, dt); break;
        default:            strafeAcross(-1.f, dt); break; // StrafeAcrossL
        }
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = AlternateStrafe) {
        EnemyDef d;
        d.srcX=150; d.srcY=201; d.srcW=32; d.srcH=20; d.lives=3; d.scale=2.2f; d.speed=90.f;
        d.fireInterval = 1.0f;
        d.bulletSpeed  = 210.f;
        d.muzzles = { {-4.f, 8.f}, {4.f, 8.f} }; // dos bocas gemelas al frente
        d.variant = (int)move;
        return makeEnemy<GunnerMediano>(s, x, y, t, d);
    }

private:
    float sideTimer = 0.5f;   // 2º cañón (offset medio tiempo -> alterna)
    float spinA = 90.f, spinB = 90.f; // ángulos de las 2 espirales
};
