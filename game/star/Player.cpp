#include "Player.h"

#include <SDL3/SDL.h>
#include <algorithm>
#include <cmath>

#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/SpriteRenderer.h"
#include "../../engine/Camera.h"

#include "Hitbox.h"
#include "Projectiles.h"
#include "Exhaust.h"

Player* Player::spawn(Scene& scene, float x, float y) {
    // Tamaño de la nave. Los hitboxes, tubos, clamp y cañón se midieron a escala 1.5,
    // así que todo se escala por k = SCALE/1.5 y queda consistente al cambiar el tamaño.
    const float SCALE = 1.25f;
    const float k = SCALE / 1.5f;

    // Fuegos de motor PRIMERO: al crearse antes que la nave, se dibujan DETRÁS de
    // ella (el engine pinta en orden de creación). Se enganchan más abajo.
    GameObject* fireL = createExhaust(scene, FlameColor::Azul, k, false);
    GameObject* fireR = createExhaust(scene, FlameColor::Azul, k, false);

    GameObject* go = scene.createGameObject("Player");
    go->transform->x = x;
    go->transform->y = y;
    go->transform->scaleX = go->transform->scaleY = SCALE;

    // El sheet SpaceShips_Player es una grilla 4x4 de celdas de 64x64.
    // Tomamos el caza azul (fila 0, columna 2). La nave ya apunta hacia arriba.
    auto sr = go->addComponent<SpriteRenderer>("assets/space/SpaceShips_Player-0001.png");
    sr->setSourceRect(128.0f, 0.0f, 64.0f, 64.0f);

    auto player = go->addComponent<Player>();
    player->halfSize *= k; // clamp proporcional al tamaño
    player->muzzleY  *= k; // el cañón (morro) también escala

    // Al ser golpeado por el CUERPO de un enemigo o por una bala enemiga, el
    // jugador pierde una vida. (Los powerups se conectarán en el paso 9.)
    auto onBody = [player](Hitbox* /*self*/, Hitbox* other) {
        if (other->faction == Faction::Enemy &&
            (other->kind == HitboxKind::Body || other->kind == HitboxKind::EnemyBullet))
            player->takeDamage(1);
    };

    // Hitboxes del cuerpo en cruz: uno vertical (fuselaje) y uno horizontal (alas).
    // Cubren mejor la silueta que un solo cuadro. Son del bando Player, tipo Body.
    //
    // Medido sobre el sprite: la nave NO está en el centro de su celda 64x64, sino
    // ~15px más abajo (en mundo, con escala 1.5). Por eso centramos la cruz en
    // offsetY=+15. La silueta real mide ~58x61 px en mundo.
    addHitbox(scene, go, 0.0f, 15.0f*k, 22.0f*k, 56.0f*k, Faction::Player, HitboxKind::Body, onBody); // fuselaje
    addHitbox(scene, go, 0.0f, 11.0f*k, 56.0f*k, 22.0f*k, Faction::Player, HitboxKind::Body, onBody); // alas

    // Enganchar los fuegos a los dos tubos del sprite (medidos a escala 1.5, escalados por k).
    attachExhaust(fireL, go, -8.25f*k, 50.0f*k);
    attachExhaust(fireR, go,  7.0f*k, 50.0f*k);

    return player;
}

void Player::update(float dt) {
    const bool* keys = SDL_GetKeyboardState(nullptr);
    Transform* t = gameObject->transform;

    // i-frames: baja el contador de invulnerabilidad mientras esté activo.
    if (invulnTimer > 0.0f) invulnTimer -= dt;

    // Viajar con la cámara: seguimos su desplazamiento para quedar FIJOS en pantalla
    // cuando no se presiona nada. Sin esto, el mundo scrollea y la nave "se iría"
    // hacia abajo. Usar el delta de la cámara funciona aunque el scroll cambie.
    if (Camera* cam = gameObject->scene->getActiveCamera()) {
        float camY = cam->gameObject->transform->y;
        if (camTracked) t->y += camY - lastCamY;
        lastCamY = camY;
        camTracked = true;
    }

    // Dirección de movimiento a partir de las flechas (Y crece hacia abajo).
    float mx = 0.0f, my = 0.0f;
    if (keys[SDL_SCANCODE_LEFT])  mx -= 1.0f;
    if (keys[SDL_SCANCODE_RIGHT]) mx += 1.0f;
    if (keys[SDL_SCANCODE_UP])    my -= 1.0f;
    if (keys[SDL_SCANCODE_DOWN])  my += 1.0f;

    // Limitamos la MAGNITUD del vector a 1 como máximo (forma general, lista para
    // mando analógico): si pasa de 1 (diagonal a tope) lo recortamos; si es menor
    // (empuje suave de un stick) lo dejamos para conservar la velocidad lenta. Con
    // teclado equivale a normalizar la diagonal, y evita dividir entre 0 en reposo.
    float len = std::sqrt(mx * mx + my * my);
    if (len > 1.0f) {
        mx /= len;
        my /= len;
    }

    t->x += mx * speed * dt;
    t->y += my * speed * dt;

    // Inclinación (banking) al ir a los lados: + grados = horario (derecha),
    // - grados = antihorario (izquierda). Suavizamos hacia el objetivo para que
    // entre y salga de la inclinación de forma fluida.
    float targetRot = mx * tiltMaxDeg;
    float k = std::min(1.0f, tiltSpeed * dt);
    t->rotation += (targetRot - t->rotation) * k;

    // Disparo con Z y cooldown: mantener Z presionado dispara cada shootCooldown
    // segundos (el timer baja con dt y se reinicia al disparar).
    shootTimer -= dt;
    if (keys[SDL_SCANCODE_Z] && shootTimer <= 0.0f) {
        shoot();
        shootTimer = shootCooldown;
    }

    clampToView();
}

void Player::shoot() {
    Transform* t = gameObject->transform;
    // Sale desde el "cañón" (centro + muzzle) hacia arriba a bulletSpeed.
    createBullet(*gameObject->scene, Faction::Player, BulletVisual::AzulBolt,
                 t->x + muzzleX, t->y + muzzleY,
                 0.0f, -bulletSpeed, bulletScale);
}

void Player::takeDamage(int dmg) {
    if (invulnTimer > 0.0f) return; // invulnerable: ignora el golpe
    lives -= dmg;
    if (lives < 0) lives = 0;
    invulnTimer = invulnTime;
    SDL_Log("Player golpeado! vidas restantes = %d", lives);
}

void Player::clampToView() {
    Scene* scene = gameObject->scene;
    Camera* cam = scene->getActiveCamera();

    // Tamaño real de la ventana (se adapta solo si cambia de resolución).
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(scene->getRenderer(), &w, &h);

    float z    = cam ? cam->getZoom() : 1.0f;
    float camX = cam ? cam->gameObject->transform->x : 0.0f;
    float camY = cam ? cam->gameObject->transform->y : 0.0f;

    // Mitad del área visible en unidades de mundo (divide entre el zoom).
    float halfW = (w * 0.5f) / z;
    float halfH = (h * 0.5f) / z;

    // Bordes válidos para el CENTRO de la nave (resta su medio tamaño para que
    // no asome fuera de la pantalla).
    float minX = camX - halfW + halfSize, maxX = camX + halfW - halfSize;
    float minY = camY - halfH + halfSize, maxY = camY + halfH - halfSize;

    Transform* t = gameObject->transform;
    t->x = std::clamp(t->x, minX, maxX);
    t->y = std::clamp(t->y, minY, maxY);
}
