#pragma once

#include <functional>

#include "../../engine/Component.h"
#include "../../engine/Scene.h"
#include "../../engine/GameObject.h"
#include "../../engine/Transform.h"
#include "../../engine/BoxCollider.h"

// === Sistema de hitboxes de StarShooter =====================================
// El engine solo admite UN BoxCollider por GameObject, así que cada hitbox vive
// en un GameObject HIJO que sigue a su dueño (la nave, un enemigo, una bala...).
// Una entidad puede tener varios hitboxes (varios hijos) para cubrir mejor su
// sprite o, más adelante, las partes del jefe.
//
// El bando (Faction) y el tipo (HitboxKind) permiten decidir el efecto al chocar
// SIN usar capas (el engine no las tiene): en onCollision leemos el Hitbox del
// otro y actuamos según su faction/kind.

enum class Faction { Player, Enemy, Neutral };

enum class HitboxKind {
    Body,          // cuerpo de una nave (recibe daño / hace daño por contacto)
    PlayerBullet,  // bala del jugador
    EnemyBullet,   // bala enemiga
    BombExplosion, // área de la bomba
    PowerupLife,   // recoge vida
    PowerupBomb,   // recoge bomba
    PowerupShield  // recoge escudo
};

class Hitbox : public Component {
public:
    Faction    faction = Faction::Neutral;
    HitboxKind kind    = HitboxKind::Body;
    int        damage  = 1;

    GameObject* owner   = nullptr;   // entidad dueña (el padre lógico)
    float       offsetX = 0.0f;      // posición relativa al dueño
    float       offsetY = 0.0f;

    // Se llama cuando este hitbox toca a otro. 'self' es este, 'other' el ajeno.
    // El dueño define aquí qué hacer (perder vida, restar salud, recoger, etc.).
    std::function<void(Hitbox* self, Hitbox* other)> onHit;

    void update(float) override {
        // Hitbox propio (owner = el mismo objeto, p.ej. una bala): no sigue a nadie,
        // el objeto se mueve solo (con su RigidBody2D). No hay nada que sincronizar.
        if (owner == gameObject) return;

        // Si el dueño (otro objeto) murió, este hitbox se va con él (evita
        // punteros colgantes a un dueño que ya no existe).
        if (!owner || !owner->alive) {
            gameObject->scene->destroy(gameObject);
            return;
        }
        // Sigue al dueño manteniendo su offset.
        gameObject->transform->x = owner->transform->x + offsetX;
        gameObject->transform->y = owner->transform->y + offsetY;
    }

    void onCollision(GameObject* other) override {
        if (Hitbox* oh = other->getComponent<Hitbox>())
            if (onHit) onHit(this, oh);
    }
};

// Crea un GameObject hijo con un BoxCollider (trigger) + un Hitbox y lo engancha
// al 'owner'. Devuelve el Hitbox para poder ajustarlo si hace falta.
inline Hitbox* addHitbox(Scene& scene, GameObject* owner,
                         float offX, float offY, float w, float h,
                         Faction faction, HitboxKind kind,
                         std::function<void(Hitbox*, Hitbox*)> onHit = {},
                         int damage = 1) {
    GameObject* go = scene.createGameObject(owner->name + ":hitbox");
    go->transform->x = owner->transform->x + offX;
    go->transform->y = owner->transform->y + offY;

    auto col = go->addComponent<BoxCollider>();
    col->width = w; col->height = h;
    col->isTrigger = true; // los hitboxes notifican, no empujan

    auto hb = go->addComponent<Hitbox>();
    hb->owner = owner;
    hb->offsetX = offX; hb->offsetY = offY;
    hb->faction = faction; hb->kind = kind; hb->damage = damage;
    hb->onHit = std::move(onHit);
    return hb;
}
