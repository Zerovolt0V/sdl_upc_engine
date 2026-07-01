#pragma once

#include <vector>

#include "../../engine/Component.h"
#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"

#include "BulletVisual.h"

struct Vec2 { float x = 0.0f, y = 0.0f; };

// Ficha de un tipo de enemigo: define su SPRITE (sheet + recorte o animación),
// stats y CAÑONES. Cada tipo construye una EnemyDef en su spawn().
struct EnemyDef {
    const char* sheet = "assets/space/SpaceShips_Enemy-0001.png"; // cada tipo puede cambiarlo
    float srcX = 0, srcY = 0, srcW = 32, srcH = 32;               // recorte estático
    int   lives = 1;
    float scale = 1.6f;
    float speed = 100.0f;
    float fireInterval = 1.5f;      // cadencia de disparo base (seg)
    float bulletSpeed  = 300.0f;    // velocidad de bala base (px/s)
    std::vector<Vec2> muzzles;      // cañones en PÍXELES del sprite desde el centro (+Y = hacia
                                    // el jugador). Se escalan por 'scale'. Vacío = un cañón al morro.

    // Animación opcional (p. ej. el jefe). Si animated=true se ignora srcX/Y/W/H.
    bool  animated = false;
    int   frameW = 0, frameH = 0, columns = 1;
    std::vector<int> frames;        // celdas de la animación (índices)
    float fps = 10.0f;

    int   variant = 0;              // variante de patrón (un tipo con varios movimientos)
    bool  ignoreGate = false;       // true = activo desde que nace y se gestiona solo
                                    //        (para curvas/scripts que entran y salen de pantalla)
};

// === Base de todos los enemigos + TOOLKIT de comportamientos =================
// Toda la maquinaria compartida vive aquí. Cada TIPO está en game/star/enemies/*.h.
class Enemy : public Component {
public:
    int   lives        = 1;
    bool  active       = true;
    float speed        = 100.0f;
    float fireInterval = 1.5f;    // cadencia de disparo (los patrones la usan)
    float bulletSpeed  = 300.0f;  // velocidad de bala (los patrones la usan)
    GameObject* target = nullptr;

    std::vector<GameObject*> parts; // hitboxes/visuales hijos (se destruyen con el enemigo)

    void update(float dt) override;
    virtual void pattern(float dt) = 0;
    void takeDamage(int dmg);

    // Ensambla el enemigo desde su ficha. Lo llama makeEnemy.
    void setup(Scene& scene, float x, float y, GameObject* target, const EnemyDef& def);

protected:
    float age=0, homeX=0, heading=90.0f, orbitAngle=0, spiralAngle=0;
    float orbCx=0, orbCy=0; // centro de órbita capturado (para empezar sin salto)
    float fireTimer=0, tPhase=0, lastCamY=0, camDelta=0, frontY=0;
    bool  camTracked=false;
    bool  seen=false;      // ya entró a la vista al menos una vez
    bool  ignoreGate=false;// auto-gestionado: no usa el gate de visibilidad
    int   phase=0;
    int   variant=0;     // variante de patrón elegida al spawnear
    int   pathSeg=0;     // estado de followPath: segmento actual
    float pathU=0.0f;    // estado de followPath: avance dentro del segmento
    std::vector<Vec2> muzzles;      // cañones de esta instancia

    void die();

    // Profundidad del enemigo en pantalla: -480 (borde superior) .. +480 (inferior),
    // 0 = centro. Útil para "avanzar hasta cierta altura" sin pelear con el scroll.
    float depthOnScreen() const { return gameObject->transform->y - lastCamY; }

    // === MOVIMIENTO ===
    void advance(float dt);
    void holdOnScreen(float dt);
    bool moveTo(float tx, float ty, float spd, float dt);
    void driftTo(float tx, float ty, float smooth, float dt);
    void sineWeave(float centerX, float amp, float freq, float dt);
    void sweepX(float centerX, float amp, float freq, float dt);
    void chase(float spd, float maxTurnDeg, float dt);
    void orbit(float screenCx, float screenCy, float radius, float degPerSec, float dt);
    void retreat(float dirDeg, float spd, float dt);
    // Sigue una CURVA suave (Catmull-Rom) por waypoints RELATIVOS a la pantalla.
    // true al terminar. Con spawns escalonados sobre la misma curva, los enemigos
    // "se siguen entre sí". También orienta la nave según la curva.
    bool followPath(const std::vector<Vec2>& pts, float speed, float dt);

    // === ROTACIÓN ===
    void faceVelocity(float vx, float vy);
    void faceTarget();
    void spin(float degPerSec, float dt);
    void rotateToward(float targetDeg, float degPerSec, float dt); // gira SUAVE hacia un ángulo

    // === CAÑONES + DISPARO ===
    int   muzzleCount() const;                       // nº de cañones (mínimo 1)
    void  muzzleAt(int i, float& wx, float& wy);     // posición mundial del cañón i (rotada)
    float angleToTarget(int muzzle = 0);             // ángulo hacia el jugador desde un cañón

    void fireOne(float angleDeg, float speed, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireAimed(float speed, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireSpread(int count, float arcDeg, float centerDeg, float speed, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireRing(int count, float speed, float phaseDeg, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireStack(int count, float minSpeed, float maxSpeed, float angleDeg, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireSpiral(float step, float speed, BulletVisual v, float scale = 1.0f, int muzzle = 0);
    void fireForward(float speed, BulletVisual v, float scale = 1.0f, int muzzle = 0); // en la dirección que MIRA la nave

    // === Máquina de estados ===
    void nextPhase();
    bool every(float interval, float dt);
    bool every(float interval, float dt, float& timer);

private:
    bool onScreen(float margin);  // ¿está dentro del área visible (con holgura)?
    void spawnBullet(float wx, float wy, float angleDeg, float speed, BulletVisual v, float scale);
};

// Crea un enemigo del tipo T con su ficha. Lo usa el spawn de cada tipo.
template <typename T>
inline Enemy* makeEnemy(Scene& scene, float x, float y, GameObject* target, const EnemyDef& def) {
    GameObject* go = scene.createGameObject("Enemy");
    T* e = go->addComponent<T>();
    e->setup(scene, x, y, target, def);
    return e;
}
