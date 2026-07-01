#pragma once

#include "../../engine/Component.h"

class Scene;

// Nave del jugador. Paso 2: movimiento en 8 direcciones con inclinación al
// girar y recorte (clamp) a los bordes de la pantalla. Disparo, bombas y daño
// llegan en los pasos siguientes.
class Player : public Component {
public:
    // --- Ajustes (tuning) ---
    float speed         = 320.0f; // píxeles por segundo
    float tiltMaxDeg    = 8.0f;  // inclinación máxima al ir a los lados (grados)
    float tiltSpeed     = 12.0f;  // qué tan rápido alcanza/regresa la inclinación
    float halfSize      = 48.0f;  // medio tamaño visual de la nave (para el clamp)
    float shootCooldown = 0.35f;   // segundos entre disparos (GDD)
    float bulletSpeed   = 1700.0f; // velocidad de la bala (px/s, hacia arriba)
    float bulletScale   = 1.2f;   // tamaño de la bala
    float muzzleX       = 0.0f;   // cañón: offset en X desde el centro de la nave
    float muzzleY       = -22.0f; // cañón: offset en Y (negativo = hacia la punta)
    float invulnTime    = 1.2f;   // segundos de invulnerabilidad tras recibir daño

    // --- Estado (GDD): vidas (máx 3) y bombas (máx 6) ---
    int lives = 3;
    int bombs = 3;

    void update(float dt) override;

    // Recibe daño: pierde 'dmg' vidas (si no está en i-frames) y activa la
    // invulnerabilidad temporal. Lo llaman los hitboxes del cuerpo al ser golpeados.
    void takeDamage(int dmg);

    // Crea el GameObject del jugador (sprite + este componente) en (x, y).
    static Player* spawn(Scene& scene, float x, float y);

private:
    float shootTimer  = 0.0f; // cuenta atrás del cooldown de disparo
    float invulnTimer = 0.0f; // cuenta atrás de la invulnerabilidad (i-frames)
    float lastCamY    = 0.0f; // para seguir el desplazamiento de la cámara
    bool  camTracked  = false;

    void shoot();             // crea una bala del jugador
    // Mantiene la nave dentro del área visible de la cámara.
    void clampToView();
};
