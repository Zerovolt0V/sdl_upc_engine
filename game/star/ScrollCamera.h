#pragma once

#include "../../engine/Component.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"

#include "StarConfig.h"

// Sube la cámara a velocidad constante. Como screenY = worldY - camY + alto/2,
// al BAJAR camY todo el mundo se ve descender en pantalla → sensación de avanzar.
// El jugador viaja con la cámara gracias a su clampToView (ya implementado).
class ScrollCamera : public Component {
public:
    float speed = SCROLL_SPEED;

    void update(float dt) override {
        gameObject->transform->y -= speed * dt;
    }
};
