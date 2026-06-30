#include "Player.h"

#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/SpriteRenderer.h"

Player* Player::spawn(Scene& scene, float x, float y) {
    GameObject* go = scene.createGameObject("Player");
    go->transform->x = x;
    go->transform->y = y;
    go->transform->scaleX = go->transform->scaleY = 1.5f; // 64px -> 96px en pantalla

    // El sheet SpaceShips_Player es una grilla 4x4 de celdas de 64x64.
    // Tomamos el caza azul (fila 0, columna 2). La nave ya apunta hacia arriba.
    auto sr = go->addComponent<SpriteRenderer>("assets/space/SpaceShips_Player-0001.png");
    sr->setSourceRect(128.0f, 0.0f, 64.0f, 64.0f);

    return go->addComponent<Player>();
}
