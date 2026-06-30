#pragma once

#include "../../engine/Component.h"

class Scene;

// Nave del jugador. Paso 2: movimiento en 8 direcciones con inclinación al
// girar y recorte (clamp) a los bordes de la pantalla. Disparo, bombas y daño
// llegan en los pasos siguientes.
class Player : public Component {
public:
    // --- Ajustes (tuning) ---
    float speed      = 320.0f; // píxeles por segundo
    float tiltMaxDeg = 15.0f;  // inclinación máxima al ir a los lados (grados)
    float tiltSpeed  = 12.0f;  // qué tan rápido alcanza/regresa la inclinación
    float halfSize   = 48.0f;  // medio tamaño visual de la nave (para el clamp)

    // --- Estado (GDD): vidas (máx 3) y bombas (máx 6) ---
    int lives = 3;
    int bombs = 3;

    void update(float dt) override;

    // Crea el GameObject del jugador (sprite + este componente) en (x, y).
    static Player* spawn(Scene& scene, float x, float y);

private:
    // Mantiene la nave dentro del área visible de la cámara.
    void clampToView();
};
