#pragma once
#include "../Enemy.h"

// AMARILLO / Mediano — Francotirador elite con 2 cañones. 3 vidas.
class SniperMediano : public Enemy {
public:
    enum Move { Hunter, Suppressor, Marksman, StrafeSnipeR, StrafeSnipeL };

    void fireTwinAimed() {
        for (int m = 0; m < muzzleCount(); ++m)
            fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe, 1.0f, m);
    }

    // Cruza la pantalla APUNTÁNDOTE todo el tiempo (dir = +1 der, -1 izq).
    void strafeSnipe(float dir, float dt) {
        switch (phase) {
        case 0: { // baja y deriva al lado de inicio
            advance(dt); faceTarget();
            float startX = -dir * 260.f, k = 2.f * dt; if (k > 1.f) k = 1.f;
            gameObject->transform->x += (startX - gameObject->transform->x) * k;
            if (depthOnScreen() > -200.f) nextPhase();
            break;
        }
        default: // cruza al otro lado apuntándote y disparando (sale -> el gate limpia)
            holdOnScreen(dt);
            gameObject->transform->x += dir * 130.f * dt;
            faceTarget();
            if (every(fireInterval * 0.8f, dt)) fireTwinAimed();
            break;
        }
    }

    void pattern(float dt) override {
        switch ((Move)variant) {

        case Hunter: // entra y te PERSIGUE lento disparando apuntado
            switch (phase) {
            case 0: advance(dt); faceTarget(); if (depthOnScreen() > -200.f) nextPhase(); break;
            case 1:
                chase(70.f, 60.f, dt);
                faceTarget();
                if (every(fireInterval, dt)) fireTwinAimed();
                if (tPhase > 4.5f) nextPhase();
                break;
            default: rotateToward(0.f, 360.f, dt); retreat(90.f, 320.f, dt); break;
            }
            break;

        case Suppressor: // se planta y dispara un ABANICO apuntado (fuerza a moverte de lado)
            switch (phase) {
            case 0: advance(dt); faceTarget(); if (depthOnScreen() > -180.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt); faceTarget();
                if (every(fireInterval * 1.1f, dt)) { // cada cañón apunta y abre ±10° -> tenaza
                    fireOne(angleToTarget(0) - 10.f, bulletSpeed, BulletVisual::AmarilloOrbe, 1.0f, 0);
                    fireOne(angleToTarget(1) + 10.f, bulletSpeed, BulletVisual::AmarilloOrbe, 1.0f, 1);
                }
                if (tPhase > 3.5f) nextPhase();
                break;
            default: rotateToward(0.f, 360.f, dt); retreat(90.f, 300.f, dt); break;
            }
            break;

        case Marksman: // se planta y suelta RÁFAGAS precisas (3 tiros rápidos, luego pausa)
            switch (phase) {
            case 0: advance(dt); faceTarget(); if (depthOnScreen() > -180.f) nextPhase(); break;
            case 1:
                holdOnScreen(dt); faceTarget();
                if (burstLeft > 0) {
                    if (every(0.12f, dt, burstTimer)) { fireTwinAimed(); burstLeft--; }
                } else if (every(1.6f, dt)) {
                    burstLeft = 3; burstTimer = 0.f; // recarga la ráfaga
                }
                if (tPhase > 4.0f) nextPhase();
                break;
            default: rotateToward(0.f, 360.f, dt); retreat(90.f, 300.f, dt); break;
            }
            break;

        case StrafeSnipeR: strafeSnipe(+1.f, dt); break;
        default:           strafeSnipe(-1.f, dt); break; // StrafeSnipeL
        }
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = Hunter) {
        EnemyDef d;
        d.srcX=150; d.srcY=114; d.srcW=32; d.srcH=20; d.lives=3; d.scale=2.2f; d.speed=100.f;
        d.fireInterval = 1.0f;
        d.bulletSpeed  = 210.f;
        d.muzzles = { {-4.f, 8.f}, {4.f, 8.f} };
        d.variant = (int)move;
        return makeEnemy<SniperMediano>(s, x, y, t, d);
    }

private:
    float burstTimer = 0.f;
    int   burstLeft  = 0;
};
