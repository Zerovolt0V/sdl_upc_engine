#pragma once
#include "../engine/Component.h"

// Componentes del engine que usamos solo por puntero: declaracion adelantada,
// asi el header no arrastra SDL ni los headers completos.
class SpriteRenderer;
class GameObject;

// Controla la nave del jugador (vertical shmup):
//  - movimiento en 8 direcciones con clamp a la pantalla
//  - inclinacion (banking) al moverse de lado
//  - barrel roll con invulnerabilidad y direccion bloqueada (cooldown)
//  - disparo y bomba con cooldown (por ahora son STUBS: solo loguean)
//  - estado de vida (corazones + vidas), bombas y escudo
// Es un Component normal: toda la logica vive en update(); SDL solo entra en el .cpp.

class PlayerController : public Component {
public:
    // ---- Parametros ajustables desde main ----
    float speed          = 280.0f; // velocidad en px/seg
    float maxTilt        = 18.0f;  // grados de inclinacion al moverse de lado
    float tiltSpeed      = 12.0f;  // que tan rapido inclina / endereza

    float shootCooldown  = 0.5f;   // seg entre disparos (doc)
    float barrelCooldown = 5.0f;   // recarga del barrel roll (doc)
    float barrelDuration = 0.5f;   // cuanto dura el roll
    float shieldDuration = 10.0f;  // seg de escudo (doc)
    float hitInvulnTime  = 1.0f;   // invulnerabilidad tras recibir un golpe

    // Tamano de la celda de la nave dentro del spritesheet (debe coincidir con
    // el recorte que se le da al SpriteRenderer en main).
    float frameW = 64.0f;
    float frameH = 64.0f;

    // ---- Ciclo de vida ----
    void awake() override;
    void update(float dt) override;

    // ---- API para colisiones / powerups (se usara mas adelante) ----
    void takeDamage();      // pierde un corazon (si no es invulnerable)
    void addHeart();        // powerup VIDA
    void addBomb();         // powerup BOMBAS (tope maxBombs)
    void activateShield();  // powerup ESCUDO

    // ---- Getters para el HUD ----
    int  getHearts()      const { return hearts; }
    int  getMaxHearts()   const { return maxHearts; }
    int  getLives()       const { return lives; }
    int  getBombs()       const { return bombs; }
    int  getMaxBombs()    const { return maxBombs; }
    bool isShielded()     const { return shieldTimer > 0.0f; }
    bool isInvulnerable() const;
    float getShieldTimeLeft() const { return shieldTimer > 0.0f ? shieldTimer : 0.0f; }
    float getShieldDuration() const { return shieldDuration; }
    float getCollisionWidth() const { return frameW * baseScaleX; }
    float getCollisionHeight() const { return frameH * baseScaleY; }

    // La llama del motor es otro GameObject; main lo registra para sincronizarlo.
    void setExhaust(GameObject* e) { exhaust = e; }

private:
    // ---- Estado de juego ----
    int maxHearts = 3;
    int hearts    = 3;
    int lives     = 1;
    int bombs     = 0;
    int maxBombs  = 6;

    // ---- Timers (en segundos; >0 = activo / en recarga) ----
    float shootTimer  = 0.0f;
    float barrelTimer = 0.0f;
    float shieldTimer = 0.0f;
    float hitTimer    = 0.0f;

    // ---- Barrel roll ----
    enum class State { Normal, Rolling };
    State state     = State::Normal;
    float rollTimer = 0.0f; // tiempo restante del roll en curso
    float rollDirX  = 0.0f; // direccion horizontal bloqueada durante el roll

    // ---- Inclinacion ----
    float currentTilt = 0.0f;

    // ---- Escala base (se captura en awake; el roll deforma scaleX temporalmente) ----
    float baseScaleX = 1.0f;
    float baseScaleY = 1.0f;

    // ---- Deteccion de "tecla recien presionada" (no mantener) ----
    bool prevRoll = false;
    bool prevBomb = false;

    // ---- Referencias (no somos dueno) ----
    SpriteRenderer* sprite  = nullptr;
    GameObject*     exhaust = nullptr;

    // ---- Helpers ----
    void startRoll(float inX);
    void updateRoll(float dt);
    void applyMovement(float dt, float moveX, float moveY);
    void updateBanking(float dt, float inX);
    void clampToScreen();
    void updateTimers(float dt);
    void syncExhaust();
    void fire();    // STUB: a futuro creara la bala
    void useBomb(); // STUB: a futuro creara la explosion en area
};
