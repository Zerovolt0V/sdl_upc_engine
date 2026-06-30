#pragma once

#include "../../engine/Component.h"

class Scene;

// Nave del jugador. Por ahora solo existe y se dibuja; el movimiento,
// disparo, bombas y daño se van agregando en los pasos siguientes del plan.
class Player : public Component {
public:
    // Estado del jugador (GDD): vidas (máx 3) y bombas (máx 6).
    int lives = 3;
    int bombs = 3;

    // Crea el GameObject del jugador (sprite + este componente) en (x, y)
    // de mundo y devuelve el componente Player ya enganchado.
    static Player* spawn(Scene& scene, float x, float y);
};
