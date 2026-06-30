#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <string>

#include "engine/Scene.h"
#include "engine/GameObject.h"
#include "engine/SpriteRenderer.h"
#include "engine/SpriteAnimator.h"
#include "engine/Transform.h"
#include "engine/Camera.h"

#include "game/PlayerController.h"
#include "game/PowerUpSystem.h"

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Error al inicializar SDL: %s", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("StarShooter", 1280, 720, 0);
    if (!window) {
        SDL_Log("Error al crear la ventana: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("Error al crear el renderer: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    Scene scene(renderer);
    const char* shootAssets = "assets/Shoot`em Up/";

    // ---- Camara estatica (en un shmup no sigue a nadie; queda en (0,0)) ----
    // Con la camara en el origen, el punto (0,0) del mundo cae en el CENTRO de la
    // pantalla. Asi el playfield queda centrado y mas adelante podemos usar zoom.
    GameObject* camara = scene.createGameObject("MainCamera");
    camara->addComponent<Camera>();

    // ---- Fondo placeholder (se reemplazara por el scroll vertical por capas) ----
    // Se crea PRIMERO para que quede detras de todo (el render no ordena por capas).
    GameObject* bg = scene.createGameObject("Background");
    bg->transform->x = -640.0f; // esquina sup-izq de la vista (camara en (0,0))
    bg->transform->y = -360.0f;
    bg->transform->scaleX = 1280.0f / 320.0f; // estira la imagen de 320x320 a la ventana
    bg->transform->scaleY = 720.0f / 320.0f;
    bg->addComponent<SpriteRenderer>(std::string(shootAssets) + "Background_Full-0001.png");

    // ---- Llama del motor (animada). Se crea ANTES que el player para quedar detras. ----
    GameObject* exhaust = scene.createGameObject("Exhaust");
    exhaust->transform->scaleX = 1.5f;
    exhaust->transform->scaleY = 1.5f;
    exhaust->addComponent<SpriteRenderer>(std::string(shootAssets) + "Exhaust-0001.png");
    SpriteAnimator* exAnim = exhaust->addComponent<SpriteAnimator>(32, 32, 4); // 128x128 => 4x4
    exAnim->addAnimation("thrust", {0, 1, 2, 3}, 12.0f);
    exAnim->play("thrust");

    // ---- Player ----
    GameObject* player = scene.createGameObject("Player");
    player->transform->scaleX = 1.5f;
    player->transform->scaleY = 1.5f;

    // OJO: el SpriteRenderer va ANTES del PlayerController (este lo busca en awake()).
    SpriteRenderer* playerSprite = player->addComponent<SpriteRenderer>(std::string(shootAssets) + "SpaceShips_Player-0001.png");
    // Elegimos UNA nave del sheet 4x4 de 64x64. Fila 0, columna 2 = nave azul.
    // Para cambiarla: setSourceRect(col*64, fila*64, 64, 64).
    playerSprite->setSourceRect(2 * 64.0f, 0 * 64.0f, 64.0f, 64.0f);

    // Posicion inicial: centrado horizontalmente y hacia la parte baja.
    player->transform->x = -(64.0f * player->transform->scaleX) * 0.5f;
    player->transform->y = 200.0f;

    PlayerController* pc = player->addComponent<PlayerController>();
    pc->setExhaust(exhaust);

    // ---- Escudo visual, powerups y HUD ----
    GameObject* shieldFx = scene.createGameObject("ShieldFx");
    shieldFx->addComponent<SpriteRenderer>(std::string(shootAssets) + "Barrier-0001.png");
    shieldFx->addComponent<ShieldVisual>(pc);

    GameObject* powerUps = scene.createGameObject("PowerUpDirector");
    powerUps->addComponent<PowerUpDirector>(pc);

    GameObject* hud = scene.createGameObject("PowerUpHUD");
    hud->addComponent<PowerUpHUD>(pc);

    // ---- Bucle principal ----
    bool running = true;
    Uint64 lastTime = SDL_GetTicks();

    while (running) {
        Uint64 now = SDL_GetTicks();
        float dt = (now - lastTime) / 1000.0f;
        lastTime = now;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) running = false;
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.scancode == SDL_SCANCODE_ESCAPE) running = false;
        }

        scene.update(dt);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        scene.render();
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
