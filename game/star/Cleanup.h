#pragma once

#include <SDL3/SDL.h>

#include "../../engine/Component.h"
#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/Camera.h"

// Destruye el objeto cuando sale del área visible de la cámara. Usa una bandera
// "seen": solo destruye DESPUÉS de haber estado dentro al menos una vez, para no
// matar objetos que nacen fuera de pantalla (p. ej. balas de un enemigo que aún
// no entra a la vista). Reutilizable para balas ahora y enemigos en el paso 7.
class DestroyWhenOffscreen : public Component {
public:
    float margin = 60.0f; // holgura extra fuera del borde antes de destruir

    void update(float) override {
        Scene* scene = gameObject->scene;
        Camera* cam = scene->getActiveCamera();

        int w = 0, h = 0;
        SDL_GetCurrentRenderOutputSize(scene->getRenderer(), &w, &h);
        float z    = cam ? cam->getZoom() : 1.0f;
        float camX = cam ? cam->gameObject->transform->x : 0.0f;
        float camY = cam ? cam->gameObject->transform->y : 0.0f;

        float halfW = (w * 0.5f) / z + margin;
        float halfH = (h * 0.5f) / z + margin;

        Transform* t = gameObject->transform;
        bool inside = (t->x > camX - halfW && t->x < camX + halfW &&
                       t->y > camY - halfH && t->y < camY + halfH);

        if (inside) seen = true;
        else if (seen) scene->destroy(gameObject);
    }

private:
    bool seen = false;
};
