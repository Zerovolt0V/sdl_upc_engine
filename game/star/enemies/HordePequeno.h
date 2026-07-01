#pragma once
#include "../Enemy.h"
#include "../Paths.h"

// ROJO / Pequeño — Horda (no dispara). Varias VARIANTES de movimiento (enum Move),
// elegidas en el spawn. Casi todas son una curva de la librería Paths; Zigzag es el
// patrón por fases (serpentea y cae rotando).
class HordePequeno : public Enemy {
public:
    enum Move { Zigzag, PathS, PathC, PathCL, Swoop, SwoopL, Dive, LoopDown };

    void pattern(float dt) override {
        // Las variantes de curva solo eligen QUÉ path seguir; el followPath es uno solo.
        const std::vector<Vec2>* path = nullptr;
        switch ((Move)variant) {
        case PathS:    path = &Paths::SCurve;     break;
        case PathC:    path = &Paths::CRight;     break;
        case PathCL:   path = &Paths::CLeft;      break;
        case Swoop:    path = &Paths::SwoopRight; break;
        case SwoopL:   path = &Paths::SwoopLeft;  break;
        case Dive:     path = &Paths::DiveMid;    break;
        case LoopDown: path = &Paths::Loop;       break;
        case Zigzag:
        default:
            if (phase == 0) { sineWeave(homeX, 45.f, 0.8f, dt); if (tPhase > 1.6f) nextPhase(); }
            else            { advance(dt); spin(540.f, dt); } // cae rotando
            return;
        }
        if (followPath(*path, 240.f, dt)) die();
    }

    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = Zigzag) {
        EnemyDef d;
        d.srcX=198; d.srcY=27; d.srcW=21; d.srcH=17; d.lives=1; d.scale=1.6f; d.speed=130.f;
        d.variant = (int)move;
        d.ignoreGate = (move != Zigzag); // las curvas se gestionan solas (entran/salen de pantalla)
        return makeEnemy<HordePequeno>(s, x, y, t, d);
    }
};
