#pragma once
#include "../Enemy.h"
#include "../Paths.h"

// AMARILLO / Pequeño — Francotirador (te apunta). Varias personalidades (enum Move).
// Cadencia/velocidad de bala salen de fireInterval/bulletSpeed (ajustables en spawn).
class SniperPequeno : public Enemy {
public:
    enum Move { DiveSnipe, DiveSnipeL, WeaveSnipe, Orbiter, SpinTurret, Ambush };

    void pattern(float dt) override {
        switch ((Move)variant) {

        case DiveSnipe: // cruza izquierda -> derecha disparando
            if (followPath(Paths::SwoopRight, 220.f, dt)) { die(); return; }
            if (every(fireInterval, dt)) fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe);
            break;

        case DiveSnipeL: // cruza derecha -> izquierda disparando
            if (followPath(Paths::SwoopLeft, 220.f, dt)) { die(); return; }
            if (every(fireInterval, dt)) fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe);
            break;

        case WeaveSnipe:
            sineWeave(homeX, 60.f, 0.5f, dt);
            faceTarget();
            if (every(fireInterval, dt)) fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe);
            break;

        case Orbiter:
            switch (phase) {
            case 0: { // ENTRA bajando y APUNTÁNDOTE; deriva a una columna segura para la órbita
                advance(dt);
                faceTarget();
                float safeX = homeX; if (safeX > 200.f) safeX = 200.f; if (safeX < -200.f) safeX = -200.f;
                float k = 4.f * dt; if (k > 1.f) k = 1.f;
                gameObject->transform->x += (safeX - gameObject->transform->x) * k;
                if (depthOnScreen() > -120.f) {
                    orbCx = gameObject->transform->x;  // ya en rango -> la órbita empieza aquí, sin salto
                    orbCy = depthOnScreen() - 120.f;   // centro 120px ARRIBA de la entrada
                    orbitAngle = 90.f;                 // entrada = punto INFERIOR del círculo
                    nextPhase();
                }
                break;
            }
            case 1: // orbita EMPEZANDO donde terminó de bajar (sin salto)
                orbit(orbCx, orbCy, 120.f, 100.f, dt);
                faceTarget();
                if (every(fireInterval, dt)) fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe);
                if (tPhase > 3.0f) nextPhase();
                break;
            default: rotateToward(0.f, 360.f, dt); retreat(90.f, 260.f, dt); break; // gira SUAVE a mirar abajo y se va
            }
            break;

        case SpinTurret:
            switch (phase) {
            case 0: advance(dt); if (depthOnScreen() > -120.f) nextPhase(); break; // ENTRA a vista
            case 1:
                holdOnScreen(dt);
                spin(150.f, dt);
                if (every(fireInterval * 0.7f, dt)) fireForward(bulletSpeed, BulletVisual::AmarilloOrbe);
                if (tPhase > 2.5f) nextPhase();
                break;
            default: rotateToward(0.f, 360.f, dt); retreat(90.f, 260.f, dt); break; // deja de girar SUAVE hacia abajo
            }
            break;

        default: // Ambush
            switch (phase) {
            case 0: advance(dt); faceTarget(); if (depthOnScreen() > -170.f) nextPhase(); break; // baja apuntándote
            case 1:
                holdOnScreen(dt); faceTarget();
                if (every(fireInterval, dt)) fireAimed(bulletSpeed, BulletVisual::AmarilloOrbe);
                if (tPhase > 2.5f) nextPhase();
                break;
            default: faceTarget(); retreat(90.f, 300.f, dt); break; // sigue apuntándote al huir
            }
            break;
        }
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = WeaveSnipe) {
        EnemyDef d;
        d.srcX=198; d.srcY=116; d.srcW=21; d.srcH=17; d.lives=1; d.scale=1.6f; d.speed=120.f;
        d.fireInterval = 1.0f;   // cadencia base
        d.bulletSpeed  = 210.f;  // velocidad de bala base
        d.variant = (int)move;
        d.ignoreGate = (move == DiveSnipe || move == DiveSnipeL); // los de curva se auto-gestionan
        return makeEnemy<SniperPequeno>(s, x, y, t, d);
    }
};
