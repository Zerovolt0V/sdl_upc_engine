#pragma once

#include <string>
#include <cmath>

#include <SDL3/SDL.h>

#include "../../engine/Component.h"
#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/Camera.h"

// Capa de fondo tileada con parallax. Se dibuja repetida (tiles de 320x320) y se
// desplaza verticalmente a 'factor' de la velocidad del mundo:
//   factor pequeño = capa lejana (se mueve poco)   factor grande = capa cercana.
// Todas las capas usan factor < 1 (más lento que el gameplay) para dar profundidad.
class ParallaxLayer : public Component {
public:
    SDL_Texture* tex = nullptr;
    float factor = 0.3f;

    void render() override {
        if (!tex) return;
        Scene* scene = gameObject->scene;
        SDL_Renderer* r = scene->getRenderer();

        int w = 0, h = 0;
        SDL_GetCurrentRenderOutputSize(r, &w, &h);

        Camera* cam = scene->getActiveCamera();
        float camY = cam ? cam->gameObject->transform->y : 0.0f;

        // El asset está pensado para llenar el ANCHO de la pantalla (no se tilea en X).
        // Lo estiramos a todo el ancho con escala uniforme y solo LOOPEAMOS en vertical.
        float scale = (float)w / 320.0f;
        float tile  = 320.0f * scale; // alto del tile en pantalla (cuadrado -> = w)

        float offY = std::fmod(-camY * factor, tile);
        if (offY < 0.0f) offY += tile;

        // Una sola columna (ancho completo), repetida verticalmente para cubrir la pantalla.
        for (float y = offY - tile; y < (float)h; y += tile) {
            SDL_FRect dst{ 0.0f, y, (float)w, tile };
            SDL_RenderTexture(r, tex, nullptr, &dst);
        }
    }
};

// Crea una capa de fondo. Créalas de la MÁS LEJANA a la MÁS CERCANA (y antes que
// el jugador/enemigos) para que el orden de dibujo quede correcto.
inline GameObject* addParallaxLayer(Scene& scene, const std::string& path, float factor) {
    GameObject* go = scene.createGameObject("bg");
    auto p = go->addComponent<ParallaxLayer>();
    p->tex = scene.getAssets().loadTexture(path);
    p->factor = factor;
    return go;
}
