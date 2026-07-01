#include "StarShooter.h"

#include <cstdlib>
#include <ctime>
#include <memory>

#include <SDL3/SDL.h>

#include "../engine/Scene.h"
#include "../engine/GameObject.h"
#include "../engine/Component.h"
#include "../engine/Camera.h"
#include "../engine/Spawner.h"

#include "star/Background.h"
#include "star/ScrollCamera.h"
#include "star/Player.h"
#include "star/Enemies.h"

namespace {

// === Catálogo de enemigos para el modo DEV =================================
// Solo los tipos ya IMPLEMENTADOS (con set de variantes). Los stubs Grande/
// Gigante/HordeMediano se omiten hasta que se diseñen.
// Adaptadores: castean el índice de variante al enum Move de cada tipo.
Enemy* devHordeP (Scene& s, float x, float y, GameObject* t, int v) { return HordePequeno::spawn(s, x, y, t, (HordePequeno::Move)v); }
Enemy* devGunnerP(Scene& s, float x, float y, GameObject* t, int v) { return GunnerPequeno::spawn(s, x, y, t, (GunnerPequeno::Move)v); }
Enemy* devSniperP(Scene& s, float x, float y, GameObject* t, int v) { return SniperPequeno::spawn(s, x, y, t, (SniperPequeno::Move)v); }
Enemy* devHordeM (Scene& s, float x, float y, GameObject* t, int v) { return HordeMediano::spawn(s, x, y, t, (HordeMediano::Move)v); }
Enemy* devGunnerM(Scene& s, float x, float y, GameObject* t, int v) { return GunnerMediano::spawn(s, x, y, t, (GunnerMediano::Move)v); }
Enemy* devSniperM(Scene& s, float x, float y, GameObject* t, int v) { return SniperMediano::spawn(s, x, y, t, (SniperMediano::Move)v); }

const char* V_HORDE_P[]  = { "Zigzag", "PathS", "PathC", "PathCL", "Swoop", "SwoopL", "Dive", "LoopDown" };
const char* V_GUNNER_P[] = { "Strafer", "StrafeRun", "Spreader", "RingDrop", "Spiral" };
const char* V_SNIPER_P[] = { "DiveSnipe", "DiveSnipeL", "WeaveSnipe", "Orbiter", "SpinTurret", "Ambush" };
const char* V_HORDE_M[]  = { "Charger", "Bulldozer", "Tumbler", "Stalker", "Bull" };
const char* V_GUNNER_M[] = { "AlternateStrafe", "CrossFire", "TwinSpiral", "StrafeAcrossR", "StrafeAcrossL" };
const char* V_SNIPER_M[] = { "Hunter", "Suppressor", "Marksman", "StrafeSnipeR", "StrafeSnipeL" };

struct DevType {
    const char* name;
    const char* const* variants;
    int variantCount;
    Enemy* (*spawn)(Scene&, float, float, GameObject*, int);
};

const DevType DEV_TYPES[] = {
    { "HordePequeno",  V_HORDE_P,  8, devHordeP  },
    { "GunnerPequeno", V_GUNNER_P, 5, devGunnerP },
    { "SniperPequeno", V_SNIPER_P, 6, devSniperP },
    { "HordeMediano",  V_HORDE_M,  5, devHordeM  },
    { "GunnerMediano", V_GUNNER_M, 5, devGunnerM },
    { "SniperMediano", V_SNIPER_M, 5, devSniperM },
};
const int DEV_TYPE_COUNT = 6;

// Selector en RUNTIME: K/L = tipo de enemigo, I/O = variante. Imprime en consola.
class DevPicker : public Component {
public:
    std::shared_ptr<int> type, variant;
    void update(float) override {
        const bool* k = SDL_GetKeyboardState(nullptr);
        bool K = k[SDL_SCANCODE_K], L = k[SDL_SCANCODE_L];
        bool I = k[SDL_SCANCODE_I], O = k[SDL_SCANCODE_O];
        if (K && !kDown) { *type = (*type + DEV_TYPE_COUNT - 1) % DEV_TYPE_COUNT; *variant = 0; announce(); }
        if (L && !lDown) { *type = (*type + 1) % DEV_TYPE_COUNT;                  *variant = 0; announce(); }
        if (I && !iDown) { int n = DEV_TYPES[*type].variantCount; *variant = (*variant + n - 1) % n; announce(); }
        if (O && !oDown) { int n = DEV_TYPES[*type].variantCount; *variant = (*variant + 1) % n;     announce(); }
        kDown = K; lDown = L; iDown = I; oDown = O;
    }
    void announce() {
        const DevType& t = DEV_TYPES[*type];
        SDL_Log("[Dev] Enemigo: %s  |  Variante %d: %s", t.name, *variant, t.variants[*variant]);
    }
private:
    bool kDown = false, lDown = false, iDown = false, oDown = false;
};

} // namespace

void buildStarShooter(Scene& scene) {
    // Cámara con scroll PRIMERO (actualiza antes que el player, sin lag).
    GameObject* cam = scene.createGameObject("MainCamera");
    cam->addComponent<Camera>();
    cam->addComponent<ScrollCamera>();

    // --- Fondo parallax (detrás de todo) ---
    addParallaxLayer(scene, "assets/space/Background_Space-0001.png",      0.10f);
    addParallaxLayer(scene, "assets/space/Background_SmallStars-0001.png", 0.25f);
    addParallaxLayer(scene, "assets/space/Background_Nebula-0001.png",     0.40f);
    addParallaxLayer(scene, "assets/space/Background_Stars-0001.png",      0.60f);

    // Jugador abajo-centro.
    Player* player = Player::spawn(scene, 0.0f, 390.0f);
    GameObject* target = player->gameObject;

    // === MODO DESARROLLO: selector de enemigo en runtime =======================
    // K / L  = tipo de enemigo (solo los implementados).
    // I / O  = variante del tipo actual.
    // Todo en caliente, SIN recompilar; la consola dice qué está activo. La X de
    // spawn es ALEATORIA (nacen arriba y entran con el scroll, como en juego real).
    auto typeIdx    = std::make_shared<int>(3); // HordeMediano
    auto variantIdx = std::make_shared<int>(0);
    {
        const DevType& t = DEV_TYPES[*typeIdx];
        SDL_Log("[Dev] Enemigo: %s | Variante %d: %s   ( K/L = tipo, I/O = variante )",
                t.name, *variantIdx, t.variants[*variantIdx]);
    }

    DevPicker* picker = scene.createGameObject("DevPicker")->addComponent<DevPicker>();
    picker->type = typeIdx;
    picker->variant = variantIdx;

    std::srand((unsigned)std::time(nullptr));
    auto spawnDev = [target, typeIdx, variantIdx](Scene& sc) {
        float camY = 0.0f;
        if (Camera* c = sc.getActiveCamera()) camY = c->gameObject->transform->y;
        float x = (float)(std::rand() % 561 - 280);              // X aleatoria en [-280, 280]
        const DevType& t = DEV_TYPES[*typeIdx];
        t.spawn(sc, x, camY - 560.0f, target, *variantIdx);      // nace ARRIBA y entra con el scroll
    };
    spawnDev(scene); // uno ya visible al arrancar

    GameObject* dev = scene.createGameObject("DevSpawner");
    auto sp = dev->addComponent<Spawner>();
    sp->interval = 2.2f;
    sp->spawn = spawnDev;
}
