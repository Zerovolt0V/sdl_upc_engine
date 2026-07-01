#pragma once
#include <cmath>
#include "../Enemy.h"

// ROJO / Mediano — Horda PESADA. No dispara: su cuerpo es la amenaza (3 vidas).
// Todas las variantes son puro movimiento, más lento y deliberado que el pequeño.
class HordeMediano : public Enemy {
public:
    enum Move { Charger, Bulldozer, Tumbler, Stalker, Bull };

    float playerX() const { return target ? target->transform->x : homeX; }

    void pattern(float dt) override {
        switch ((Move)variant) {

        case Charger: // se ALINEA a tu columna y EMBISTE recto hacia abajo
            switch (phase) {
            case 0:
                advance(dt);
                driftTo(playerX(), gameObject->transform->y, 2.5f, dt); // ajusta X hacia ti
                faceTarget();                                           // te encara (telegrafía)
                if (depthOnScreen() > -160.f) nextPhase();
                break;
            default:
                rotateToward(0.f, 360.f, dt);              // endereza el morro hacia abajo
                gameObject->transform->y += speed * 2.6f * dt; // embestida rápida y recta
                break;
            }
            break;

        case Bulldozer: // baja centrándose y BARRE amplio y lento (muro móvil), luego sale
            switch (phase) {
            case 0:
                advance(dt);
                driftTo(0.f, gameObject->transform->y, 1.5f, dt);   // se centra mientras baja
                if (depthOnScreen() > -100.f) { age = 0.f; nextPhase(); }
                break;
            case 1:
                holdOnScreen(dt);
                sweepX(0.f, 230.f, 0.22f, dt);   // barrido centrado en pantalla (no se sale)
                if (tPhase > 4.5f) nextPhase();
                break;
            default: advance(dt); break; // continúa hacia abajo y sale
            }
            break;

        case Tumbler: // desciende serpenteando mientras RUEDA como una roca
            sineWeave(homeX, 70.f, 0.45f, dt);
            spin(160.f, dt);
            break;

        case Stalker: // repta apuntándote, PREDICE tu movimiento y se LANZA adelantado
            switch (phase) {
            case 0:
                advance(dt); faceTarget();
                prevPX = playerX(); // listo para estimar tu velocidad al empezar a acechar
                if (depthOnScreen() > -140.f) nextPhase();
                break;
            case 1: {
                holdOnScreen(dt);
                driftTo(playerX(), gameObject->transform->y, 1.4f, dt); // reptar hacia tu columna
                faceTarget();                                           // te encara mientras acecha
                float px = playerX();                                   // estima tu velocidad en X (suavizada)
                if (dt > 0.f) { float inst = (px - prevPX) / dt; playerVx += (inst - playerVx) * 0.15f; }
                prevPX = px;
                if (tPhase > 2.8f) {                                    // fija el rumbo ADELANTADO hacia ti
                    float lead = playerVx * 0.4f;                       // 0.4 s de anticipación
                    if (lead >  220.f) lead =  220.f;
                    if (lead < -220.f) lead = -220.f;
                    Transform* t = gameObject->transform;
                    float ty = target ? target->transform->y : t->y;
                    lungeAng = std::atan2(ty - t->y, (px + lead) - t->x) * 57.2957795f;
                    t->rotation = lungeAng - 90.f;                      // encara el punto predicho
                    nextPhase();
                }
                break;
            }
            default: retreat(lungeAng, speed * 3.6f, dt); break; // embestida rápida (rumbo fijo, esquivable)
            }
            break;

        default: // Bull: EMBESTIDA con leve autoguiado (lo puedes esquivar de lado)
            if (tPhase < 3.5f) { chase(speed * 1.5f, 45.f, dt); faceTarget(); } // más veloz y sigue mejor tu X
            else               { rotateToward(0.f, 240.f, dt); advance(dt); }   // se rinde y cae
            break;
        }
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = Charger) {
        EnemyDef d;
        d.srcX=150; d.srcY=25; d.srcW=32; d.srcH=20; d.lives=3; d.scale=2.0f; d.speed=110.f;
        d.variant = (int)move;
        return makeEnemy<HordeMediano>(s, x, y, t, d);
    }

private:
    float lungeAng = 90.f;  // rumbo fijado (adelantado) para la embestida del Stalker
    float prevPX   = 0.f;   // X del jugador el frame anterior (para estimar su velocidad)
    float playerVx = 0.f;   // velocidad horizontal del jugador, suavizada
};
