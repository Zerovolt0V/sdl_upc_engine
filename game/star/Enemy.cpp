#include "Enemy.h"

#include <cmath>

#include <SDL3/SDL.h>

#include "../../engine/Transform.h"
#include "../../engine/SpriteRenderer.h"
#include "../../engine/SpriteAnimator.h"
#include "../../engine/Camera.h"

#include "Hitbox.h"
#include "Projectiles.h"
#include "Exhaust.h"
#include "StarConfig.h"

static constexpr float DEG2RAD = 0.01745329252f;
static constexpr float RAD2DEG = 57.2957795131f;
static constexpr float TAU     = 6.28318530718f;

// === Ensamblado desde la ficha ==============================================
void Enemy::setup(Scene& scene, float x, float y, GameObject* tgt, const EnemyDef& def) {
    GameObject* go = gameObject;
    go->transform->x = x;
    go->transform->y = y;
    go->transform->scaleX = go->transform->scaleY = def.scale;

    auto sr = go->addComponent<SpriteRenderer>(def.sheet);
    if (def.animated) {
        auto an = go->addComponent<SpriteAnimator>(def.frameW, def.frameH, def.columns);
        an->addAnimation("idle", def.frames, def.fps, true);
        an->play("idle");
    } else {
        sr->setSourceRect(def.srcX, def.srcY, def.srcW, def.srcH);
    }
    sr->flipY = true; // las naves del sheet apuntan arriba; hacia el jugador

    lives        = def.lives;
    speed        = def.speed;
    fireInterval = def.fireInterval;
    bulletSpeed  = def.bulletSpeed;
    target       = tgt;
    homeX      = x;
    heading    = 90.0f;
    variant    = def.variant;
    ignoreGate = def.ignoreGate;
    // Los cañones se dan en píxeles del sprite -> se escalan al tamaño de la nave.
    muzzles.clear();
    for (const Vec2& m : def.muzzles) muzzles.push_back({ m.x * def.scale, m.y * def.scale });

    // Tamaño del cuerpo (según animación o recorte estático) para hitbox y morro.
    float w = def.animated ? (float)def.frameW : def.srcW;
    float h = def.animated ? (float)def.frameH : def.srcH;
    frontY = h * def.scale * 0.5f;

    Enemy* self = this;
    Hitbox* hb = addHitbox(scene, go, 0.0f, 0.0f, w * def.scale * 0.8f, h * def.scale * 0.8f,
                           Faction::Enemy, HitboxKind::Body,
        [self](Hitbox*, Hitbox* other) {
            if (other->kind == HitboxKind::PlayerBullet ||
                other->kind == HitboxKind::BombExplosion)
                self->takeDamage(other->damage);
        });
    parts.push_back(hb->gameObject);

    GameObject* fire = addExhaust(scene, go, 0.0f, -(h * def.scale * 0.5f + 2.0f),
                                  FlameColor::RojoA, def.scale * 0.5f, false);
    parts.push_back(fire);
}

bool Enemy::onScreen(float margin) {
    Scene* scene = gameObject->scene;
    int w = 0, h = 0;
    SDL_GetCurrentRenderOutputSize(scene->getRenderer(), &w, &h);
    Camera* cam = scene->getActiveCamera();
    float z    = cam ? cam->getZoom() : 1.0f;
    float camX = cam ? cam->gameObject->transform->x : 0.0f;
    float camY = cam ? cam->gameObject->transform->y : 0.0f;
    float halfW = (w * 0.5f) / z + margin;
    float halfH = (h * 0.5f) / z + margin;
    Transform* t = gameObject->transform;
    return t->x > camX - halfW && t->x < camX + halfW &&
           t->y > camY - halfH && t->y < camY + halfH;
}

void Enemy::update(float dt) {
    if (!active) return;

    if (ignoreGate) {
        // Auto-gestionado (curvas/scripts): activo siempre; solo se limpia como
        // respaldo si se aleja MUCHO (la curva ya se destruye sola al terminar).
        if (!onScreen(400.0f)) { die(); return; }
    } else {
        // Gate de visibilidad: dormido hasta entrar a vista (no corre patrón ni
        // dispara fuera de pantalla); y cuando ya se mostró y sale, se destruye.
        bool vis = onScreen(80.0f);
        if (vis) seen = true;
        if (!seen) return;
        if (!vis) { die(); return; }
    }

    age    += dt;
    tPhase += dt;

    if (Camera* cam = gameObject->scene->getActiveCamera()) {
        float camY = cam->gameObject->transform->y;
        camDelta = camTracked ? (camY - lastCamY) : 0.0f;
        lastCamY = camY;
        camTracked = true;
    }

    pattern(dt);
}

void Enemy::takeDamage(int dmg) {
    lives -= dmg;
    if (lives <= 0) die(); // (chunk VFX: aquí irá spawnExplosion)
}

void Enemy::die() {
    Scene* scene = gameObject->scene;
    for (GameObject* p : parts) scene->destroy(p);
    scene->destroy(gameObject);
}

// === MOVIMIENTO =============================================================
void Enemy::advance(float dt) { gameObject->transform->y += speed * dt; }

void Enemy::holdOnScreen(float /*dt*/) { gameObject->transform->y += camDelta; }

bool Enemy::moveTo(float tx, float ty, float spd, float dt) {
    Transform* t = gameObject->transform;
    float dx = tx - t->x, dy = ty - t->y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float step = spd * dt;
    if (dist <= step || dist < 0.001f) { t->x = tx; t->y = ty; return true; }
    t->x += dx / dist * step; t->y += dy / dist * step;
    return false;
}

void Enemy::driftTo(float tx, float ty, float smooth, float dt) {
    Transform* t = gameObject->transform;
    float k = smooth * dt; if (k > 1.0f) k = 1.0f;
    t->x += (tx - t->x) * k; t->y += (ty - t->y) * k;
}

void Enemy::sineWeave(float centerX, float amp, float freq, float dt) {
    gameObject->transform->y += speed * dt;
    gameObject->transform->x = centerX + amp * std::sin(TAU * freq * age);
}

void Enemy::sweepX(float centerX, float amp, float freq, float /*dt*/) {
    gameObject->transform->x = centerX + amp * std::sin(TAU * freq * age);
}

void Enemy::chase(float spd, float maxTurnDeg, float dt) {
    if (!target) { advance(dt); return; }
    Transform* t = gameObject->transform;
    float desired = std::atan2(target->transform->y - t->y,
                               target->transform->x - t->x) * RAD2DEG;
    float diff = desired - heading;
    while (diff > 180.0f) diff -= 360.0f;
    while (diff < -180.0f) diff += 360.0f;
    float maxStep = maxTurnDeg * dt;
    if (diff >  maxStep) diff =  maxStep;
    if (diff < -maxStep) diff = -maxStep;
    heading += diff;
    float r = heading * DEG2RAD;
    t->x += std::cos(r) * spd * dt; t->y += std::sin(r) * spd * dt;
}

void Enemy::orbit(float screenCx, float screenCy, float radius, float degPerSec, float dt) {
    float camX = 0.0f, camY = 0.0f;
    if (Camera* cam = gameObject->scene->getActiveCamera()) {
        camX = cam->gameObject->transform->x;
        camY = cam->gameObject->transform->y;
    }
    orbitAngle += degPerSec * dt;
    float r = orbitAngle * DEG2RAD;
    gameObject->transform->x = camX + screenCx + radius * std::cos(r);
    gameObject->transform->y = camY + screenCy + radius * std::sin(r);
}

void Enemy::retreat(float dirDeg, float spd, float dt) {
    float r = dirDeg * DEG2RAD;
    gameObject->transform->x += std::cos(r) * spd * dt;
    gameObject->transform->y += std::sin(r) * spd * dt;
}

bool Enemy::followPath(const std::vector<Vec2>& pts, float speed, float dt) {
    int n = (int)pts.size();
    if (n < 2) return true;
    auto P = [&](int i) -> Vec2 { if (i < 0) i = 0; if (i > n - 1) i = n - 1; return pts[i]; };

    // Avanza el parámetro según la longitud del segmento actual (~velocidad constante).
    Vec2 a = P(pathSeg), b = P(pathSeg + 1);
    float chord = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
    if (chord < 1.0f) chord = 1.0f;
    pathU += (speed / chord) * dt;
    while (pathU >= 1.0f) {
        pathU -= 1.0f; pathSeg++;
        if (pathSeg >= n - 1) return true; // llegó al final de la curva
    }

    // Posición y tangente de la curva de Catmull-Rom en (pathSeg, pathU).
    Vec2 P0 = P(pathSeg - 1), P1 = P(pathSeg), P2 = P(pathSeg + 1), P3 = P(pathSeg + 2);
    float u = pathU, u2 = u * u, u3 = u2 * u;
    float px = 0.5f * ((2*P1.x) + (-P0.x+P2.x)*u + (2*P0.x-5*P1.x+4*P2.x-P3.x)*u2 + (-P0.x+3*P1.x-3*P2.x+P3.x)*u3);
    float py = 0.5f * ((2*P1.y) + (-P0.y+P2.y)*u + (2*P0.y-5*P1.y+4*P2.y-P3.y)*u2 + (-P0.y+3*P1.y-3*P2.y+P3.y)*u3);
    float tx = 0.5f * ((-P0.x+P2.x) + 2*(2*P0.x-5*P1.x+4*P2.x-P3.x)*u + 3*(-P0.x+3*P1.x-3*P2.x+P3.x)*u2);
    float ty = 0.5f * ((-P0.y+P2.y) + 2*(2*P0.y-5*P1.y+4*P2.y-P3.y)*u + 3*(-P0.y+3*P1.y-3*P2.y+P3.y)*u2);

    // Los waypoints son relativos a la pantalla (centro de la cámara).
    float camX = 0.0f, camY = 0.0f;
    if (Camera* c = gameObject->scene->getActiveCamera()) {
        camX = c->gameObject->transform->x;
        camY = c->gameObject->transform->y;
    }
    gameObject->transform->x = camX + px;
    gameObject->transform->y = camY + py;
    faceVelocity(tx, ty); // orienta la nave según la dirección de la curva
    return false;
}

// === ROTACIÓN (sprite volteado: apunta abajo en rotation=0, por eso -90) =====
void Enemy::faceVelocity(float vx, float vy) {
    if (vx == 0.0f && vy == 0.0f) return;
    gameObject->transform->rotation = std::atan2(vy, vx) * RAD2DEG - 90.0f;
}
void Enemy::faceTarget() {
    if (!target) return;
    Transform* t = gameObject->transform;
    t->rotation = std::atan2(target->transform->y - t->y,
                             target->transform->x - t->x) * RAD2DEG - 90.0f;
}
void Enemy::spin(float degPerSec, float dt) {
    gameObject->transform->rotation += degPerSec * dt;
}

void Enemy::rotateToward(float targetDeg, float degPerSec, float dt) {
    Transform* t = gameObject->transform;
    float diff = targetDeg - t->rotation;
    while (diff > 180.0f) diff -= 360.0f;   // camino más corto
    while (diff < -180.0f) diff += 360.0f;
    float step = degPerSec * dt;
    if (diff >  step) diff =  step;
    if (diff < -step) diff = -step;
    t->rotation += diff;
}

// === CAÑONES + DISPARO ======================================================
int Enemy::muzzleCount() const {
    return muzzles.empty() ? 1 : (int)muzzles.size();
}

void Enemy::muzzleAt(int i, float& wx, float& wy) {
    // Offset del cañón rotado con el enemigo (sale del punto correcto aunque gire).
    float offX, offY;
    if (i >= 0 && i < (int)muzzles.size()) { offX = muzzles[i].x; offY = muzzles[i].y; }
    else                                   { offX = 0.0f;         offY = frontY;       }
    float r = gameObject->transform->rotation * DEG2RAD;
    float c = std::cos(r), s = std::sin(r);
    wx = gameObject->transform->x + (offX * c - offY * s);
    wy = gameObject->transform->y + (offX * s + offY * c);
}

float Enemy::angleToTarget(int muzzle) {
    if (!target) return 90.0f;
    float wx, wy; muzzleAt(muzzle, wx, wy);
    return std::atan2(target->transform->y - wy, target->transform->x - wx) * RAD2DEG;
}

void Enemy::spawnBullet(float wx, float wy, float angleDeg, float speed, BulletVisual v, float scale) {
    float r = angleDeg * DEG2RAD;
    createBullet(*gameObject->scene, Faction::Enemy, v, wx, wy,
                 std::cos(r) * speed, std::sin(r) * speed, scale);
}

void Enemy::fireOne(float angleDeg, float speed, BulletVisual v, float scale, int muzzle) {
    float wx, wy; muzzleAt(muzzle, wx, wy);
    spawnBullet(wx, wy, angleDeg, speed, v, scale);
}

void Enemy::fireAimed(float speed, BulletVisual v, float scale, int muzzle) {
    if (!target) return;
    float wx, wy; muzzleAt(muzzle, wx, wy);
    float ang = std::atan2(target->transform->y - wy, target->transform->x - wx) * RAD2DEG;
    spawnBullet(wx, wy, ang, speed, v, scale);
}

void Enemy::fireSpread(int count, float arcDeg, float centerDeg, float speed,
                       BulletVisual v, float scale, int muzzle) {
    float wx, wy; muzzleAt(muzzle, wx, wy);
    if (count <= 1) { spawnBullet(wx, wy, centerDeg, speed, v, scale); return; }
    float startAng = centerDeg - arcDeg * 0.5f;
    float stepAng  = arcDeg / (float)(count - 1);
    for (int i = 0; i < count; ++i)
        spawnBullet(wx, wy, startAng + stepAng * i, speed, v, scale);
}

void Enemy::fireRing(int count, float speed, float phaseDeg, BulletVisual v, float scale, int muzzle) {
    if (count < 1) return;
    float wx, wy; muzzleAt(muzzle, wx, wy);
    float stepAng = 360.0f / (float)count;
    for (int i = 0; i < count; ++i)
        spawnBullet(wx, wy, phaseDeg + stepAng * i, speed, v, scale);
}

void Enemy::fireStack(int count, float minSpeed, float maxSpeed, float angleDeg,
                      BulletVisual v, float scale, int muzzle) {
    if (count < 1) return;
    float wx, wy; muzzleAt(muzzle, wx, wy);
    for (int i = 0; i < count; ++i) {
        float f = (count == 1) ? 0.0f : (float)i / (float)(count - 1);
        spawnBullet(wx, wy, angleDeg, minSpeed + (maxSpeed - minSpeed) * f, v, scale);
    }
}

void Enemy::fireSpiral(float step, float speed, BulletVisual v, float scale, int muzzle) {
    float wx, wy; muzzleAt(muzzle, wx, wy);
    spawnBullet(wx, wy, spiralAngle, speed, v, scale);
    spiralAngle += step;
}

void Enemy::fireForward(float speed, BulletVisual v, float scale, int muzzle) {
    float wx, wy; muzzleAt(muzzle, wx, wy);
    // "Adelante" del sprite: en rotation=0 la nave mira hacia abajo (= 90° en
    // nuestro sistema), así que la dirección de disparo es rotation + 90.
    float ang = gameObject->transform->rotation + 90.0f;
    spawnBullet(wx, wy, ang, speed, v, scale);
}

// === Máquina de estados =====================================================
void Enemy::nextPhase() { phase++; tPhase = 0.0f; fireTimer = 0.0f; }

bool Enemy::every(float interval, float dt) { return every(interval, dt, fireTimer); }

bool Enemy::every(float interval, float dt, float& timer) {
    timer -= dt;
    if (timer <= 0.0f) { timer = interval; return true; }
    return false;
}
