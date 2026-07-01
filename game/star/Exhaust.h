#pragma once

#include <vector>
#include <cmath>

#include "../../engine/Component.h"
#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/SpriteRenderer.h"
#include "../../engine/SpriteAnimator.h"

// Sigue a un dueño (como el hitbox) pero SIN colisión: para visuales pegados a la
// nave, como el fuego de los motores. No se autodestruye: quien lo crea lo mete en
// su lista de "partes" y lo destruye en el mismo frame que el dueño (igual que los
// hitboxes), para no dejar punteros colgantes.
class FollowOwner : public Component {
public:
    GameObject* owner = nullptr;
    float offsetX = 0.0f, offsetY = 0.0f;

    void update(float) override {
        if (!owner) return;
        // Rotamos el offset con la nave para que el fuego siga a los motores cuando
        // se inclina, y copiamos la rotación para que la llama también gire.
        float rad = owner->transform->rotation * 0.0174532925f; // grados -> radianes
        float c = std::cos(rad), s = std::sin(rad);
        float rx = offsetX * c - offsetY * s;
        float ry = offsetX * s + offsetY * c;
        gameObject->transform->x = owner->transform->x + rx;
        gameObject->transform->y = owner->transform->y + ry;
        gameObject->transform->rotation = owner->transform->rotation;
    }
};

enum class FlameColor { RojoA, RojoB, Azul };

// Crea el GameObject del fuego (sprite animado + FollowOwner SIN dueño todavía).
// IMPORTANTE: el engine dibuja en orden de creación, así que crea el fuego ANTES
// que la nave para que quede DETRÁS de ella; luego engánchalo con attachExhaust.
//   flipDown = true  -> la llama apunta hacia ABAJO; false -> hacia ARRIBA.
inline GameObject* createExhaust(Scene& scene, FlameColor color, float scale, bool flipDown) {
    GameObject* go = scene.createGameObject("exhaust");
    go->transform->scaleX = go->transform->scaleY = scale;

    auto sr = go->addComponent<SpriteRenderer>("assets/space/Exhaust-0001.png");
    sr->flipY = flipDown;

    // Cada llama es 16x32 con 4 frames a lo ancho (columnas 2..5). Los tipos están
    // apilados: roja A en fila 0 (y=0), roja B en fila 1 (y=32), azul en fila 3 (y=96).
    int row = (color == FlameColor::RojoA) ? 0 : (color == FlameColor::RojoB) ? 1 : 3;
    int base = row * 8; // SpriteAnimator usa 8 columnas (sheet de 128/16)
    std::vector<int> frames = { base + 2, base + 3, base + 4, base + 5 };

    sr->setSourceRect(2 * 16.0f, row * 32.0f, 16.0f, 32.0f); // frame inicial (evita parpadeo)
    auto an = go->addComponent<SpriteAnimator>(16, 32, 8);
    an->addAnimation("flame", frames, 14.0f, true);
    an->play("flame");

    go->addComponent<FollowOwner>(); // el dueño se asigna en attachExhaust
    return go;
}

// Engancha un fuego ya creado a un dueño (posición relativa = offset).
inline void attachExhaust(GameObject* exhaust, GameObject* owner, float offX, float offY) {
    auto f = exhaust->getComponent<FollowOwner>();
    f->owner = owner; f->offsetX = offX; f->offsetY = offY;
    exhaust->transform->x = owner->transform->x + offX;
    exhaust->transform->y = owner->transform->y + offY;
}

// Conveniencia: crea y engancha de una. (Queda DELANTE del dueño si éste ya existe.)
inline GameObject* addExhaust(Scene& scene, GameObject* owner,
                              float offX, float offY, FlameColor color,
                              float scale, bool flipDown) {
    GameObject* go = createExhaust(scene, color, scale, flipDown);
    attachExhaust(go, owner, offX, offY);
    return go;
}
