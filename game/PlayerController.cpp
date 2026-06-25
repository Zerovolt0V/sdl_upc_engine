#include "PlayerController.h"

#include "../engine/GameObject.h"
#include "../engine/Transform.h"
#include "../engine/Scene.h"
#include "../engine/SpriteRenderer.h"
#include "../engine/Camera.h"

#include <SDL3/SDL.h>
#include <cmath>

// ---------------------------------------------------------------------------
// Ciclo de vida
// ---------------------------------------------------------------------------
void PlayerController::awake() {
    // El SpriteRenderer ya debe existir (main lo agrega ANTES que este componente).
    sprite = gameObject->getComponent<SpriteRenderer>();

    // Guardamos la escala original: el barrel roll deforma scaleX y luego la restaura.
    baseScaleX = gameObject->transform->scaleX;
    baseScaleY = gameObject->transform->scaleY;
}

void PlayerController::update(float dt) {
    // Init perezoso por si el SpriteRenderer se agrego despues (el engine casi no
    // llama a start(), asi que no dependemos de el).
    if (!sprite) sprite = gameObject->getComponent<SpriteRenderer>();

    const bool* keys = SDL_GetKeyboardState(nullptr);

    // --- Lectura de input direccional ---
    float inX = 0.0f, inY = 0.0f;
    if (keys[SDL_SCANCODE_LEFT])  inX -= 1.0f;
    if (keys[SDL_SCANCODE_RIGHT]) inX += 1.0f;
    if (keys[SDL_SCANCODE_UP])    inY -= 1.0f;
    if (keys[SDL_SCANCODE_DOWN])  inY += 1.0f;

    // --- Barrel roll (tecla C): tiene prioridad sobre el control normal ---
    bool rollPressed = keys[SDL_SCANCODE_C] && !prevRoll;
    if (state == State::Normal && rollPressed && barrelTimer <= 0.0f) {
        startRoll(inX);
    }

    if (state == State::Rolling) {
        // Durante el roll no se controla: direccion bloqueada y giro animado.
        updateRoll(dt);
    } else {
        // Movimiento normal en 8 direcciones (diagonal normalizada).
        float mx = inX, my = inY;
        if (mx != 0.0f && my != 0.0f) {
            mx *= 0.70710678f;
            my *= 0.70710678f;
        }
        applyMovement(dt, mx, my);
        updateBanking(dt, inX);

        // Disparo (Z): el cooldown regula la cadencia aunque se mantenga la tecla.
        if (keys[SDL_SCANCODE_Z] && shootTimer <= 0.0f) {
            fire();
        }
        // Bomba (X): un toque, no se repite al mantener.
        if (keys[SDL_SCANCODE_X] && !prevBomb && bombs > 0) {
            useBomb();
        }
    }

    clampToScreen();
    updateTimers(dt);
    syncExhaust();

    // Guardamos el estado de teclas para detectar el flanco el proximo frame.
    prevRoll = keys[SDL_SCANCODE_C];
    prevBomb = keys[SDL_SCANCODE_X];
}

// ---------------------------------------------------------------------------
// Movimiento e inclinacion
// ---------------------------------------------------------------------------
void PlayerController::applyMovement(float dt, float moveX, float moveY) {
    Transform* t = gameObject->transform;
    t->x += moveX * speed * dt;
    t->y += moveY * speed * dt;
}

void PlayerController::updateBanking(float dt, float inX) {
    // La nave inclina la nariz hacia donde se mueve y se endereza sola al soltar.
    float target = inX * maxTilt;
    float k = tiltSpeed * dt;
    if (k > 1.0f) k = 1.0f;
    currentTilt += (target - currentTilt) * k;
    gameObject->transform->rotation = currentTilt;
}

void PlayerController::clampToScreen() {
    SDL_Renderer* renderer = gameObject->scene->getRenderer();
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(renderer, &w, &h);

    // Con camara, el mundo se centra en pantalla; sin camara, son coords directas.
    Camera* cam = gameObject->scene->getActiveCamera();
    float zoom = cam ? cam->getZoom() : 1.0f;
    float camX = (cam && cam->gameObject) ? cam->gameObject->transform->x : 0.0f;
    float camY = (cam && cam->gameObject) ? cam->gameObject->transform->y : 0.0f;

    float halfW = (w * 0.5f) / zoom;
    float halfH = (h * 0.5f) / zoom;

    // El sprite se dibuja desde su esquina superior-izquierda: restamos su tamano
    // (usando la escala base para que el roll no afecte los limites).
    float drawnW = frameW * baseScaleX;
    float drawnH = frameH * baseScaleY;

    Transform* t = gameObject->transform;
    float minX = camX - halfW;
    float maxX = camX + halfW - drawnW;
    float minY = camY - halfH;
    float maxY = camY + halfH - drawnH;

    if (t->x < minX) t->x = minX;
    if (t->x > maxX) t->x = maxX;
    if (t->y < minY) t->y = minY;
    if (t->y > maxY) t->y = maxY;
}

// ---------------------------------------------------------------------------
// Barrel roll
// ---------------------------------------------------------------------------
void PlayerController::startRoll(float inX) {
    state       = State::Rolling;
    rollTimer   = barrelDuration;
    barrelTimer = barrelCooldown; // la recarga arranca al iniciar el roll
    // Direccion bloqueada: -1, 0 o +1 segun el input al momento de activarlo.
    rollDirX    = (inX > 0.0f) ? 1.0f : (inX < 0.0f ? -1.0f : 0.0f);
    currentTilt = 0.0f;
    gameObject->transform->rotation = 0.0f;
}

void PlayerController::updateRoll(float dt) {
    rollTimer -= dt;

    // Progreso 0..1 del giro.
    float p = 1.0f - (rollTimer / barrelDuration);
    if (p < 0.0f) p = 0.0f;
    if (p > 1.0f) p = 1.0f;

    // Se sigue moviendo en la direccion bloqueada (no se puede corregir).
    applyMovement(dt, rollDirX, 0.0f);

    // Animacion del giro sin arte extra: aplastamos scaleX (1->0->1) y volteamos
    // a la mitad. Da la ilusion de barrel roll usando campos que el engine ya tiene.
    Transform* t = gameObject->transform;
    t->scaleX = baseScaleX * std::fabs(std::cos(p * 3.14159265f));
    t->rotation = 0.0f;
    if (sprite) sprite->flipX = (p > 0.5f);

    if (rollTimer <= 0.0f) {
        // Fin del roll: restauramos el sprite a su estado normal.
        state = State::Normal;
        rollTimer = 0.0f;
        t->scaleX = baseScaleX;
        if (sprite) sprite->flipX = false;
    }
}

// ---------------------------------------------------------------------------
// Timers y feedback
// ---------------------------------------------------------------------------
void PlayerController::updateTimers(float dt) {
    if (shootTimer  > 0.0f) shootTimer  -= dt;
    if (barrelTimer > 0.0f) barrelTimer -= dt;
    if (shieldTimer > 0.0f) shieldTimer -= dt;
    if (hitTimer    > 0.0f) hitTimer    -= dt;
}

bool PlayerController::isInvulnerable() const {
    // Invulnerable mientras gira, con escudo activo, o en los i-frames tras un golpe.
    return state == State::Rolling || shieldTimer > 0.0f || hitTimer > 0.0f;
}

void PlayerController::syncExhaust() {
    if (!exhaust) return;
    Transform* t = gameObject->transform;
    Transform* e = exhaust->transform;

    // Centramos la llama bajo la nave (usando la escala base, estable durante el roll).
    float shipCenterX = t->x + frameW * baseScaleX * 0.5f;
    float shipBottomY = t->y + frameH * baseScaleY * 0.85f;
    e->x = shipCenterX - 32.0f * e->scaleX * 0.5f; // la llama es de 32 px de ancho
    e->y = shipBottomY;
}

// ---------------------------------------------------------------------------
// API de juego (la usaran las colisiones / powerups mas adelante)
// ---------------------------------------------------------------------------
void PlayerController::takeDamage() {
    // I-frames del roll o del ultimo golpe: ignorar.
    if (state == State::Rolling || hitTimer > 0.0f) return;

    // El escudo absorbe un impacto y se rompe (alternativa del doc: vida extra).
    if (shieldTimer > 0.0f) {
        shieldTimer = 0.0f;
        hitTimer = hitInvulnTime;
        SDL_Log("Escudo roto");
        return;
    }

    hearts--;
    hitTimer = hitInvulnTime;
    SDL_Log("Golpe! corazones=%d", hearts);

    if (hearts <= 0) {
        lives--;
        if (lives > 0) {
            hearts = maxHearts; // respawn con corazones llenos
            SDL_Log("Vida perdida. vidas restantes=%d", lives);
        } else {
            SDL_Log("GAME OVER"); // STUB: lo manejara la escena de juego
        }
    }
}

void PlayerController::addHeart() {
    if (hearts < maxHearts) hearts++;
    SDL_Log("Powerup VIDA. corazones=%d", hearts);
}

void PlayerController::addBomb() {
    if (bombs < maxBombs) bombs++;
    SDL_Log("Powerup BOMBA. bombas=%d", bombs);
}

void PlayerController::activateShield() {
    shieldTimer = shieldDuration;
    SDL_Log("Powerup ESCUDO (%.0fs)", shieldDuration);
}

// ---------------------------------------------------------------------------
// STUBS: cuando el engine tenga crear/destruir objetos, aqui se generan balas.
// ---------------------------------------------------------------------------
void PlayerController::fire() {
    shootTimer = shootCooldown;
    SDL_Log("PEW - disparo (stub)"); // TODO: crear bala hacia arriba en la pos de la nave
}

void PlayerController::useBomb() {
    bombs--;
    SDL_Log("BOMBA lanzada (stub). bombas=%d", bombs); // TODO: dano en area
}
