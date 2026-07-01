#pragma once

#include "Hitbox.h"
#include "Cleanup.h"
#include "BulletVisual.h"
#include "../../engine/SpriteRenderer.h"
#include "../../engine/RigidBody2D.h"
#include "../../engine/Lifetime.h"

// Fábrica de balas de StarShooter. Reusa los componentes del engine (RigidBody2D
// para el movimiento, Lifetime para la limpieza) pero usa NUESTRO sprite del pack
// space y NUESTRO sistema de Hitbox (Faction/Kind) en vez de colisión por nombre.
//
// El sheet Bullets-0001.png son celdas de 16x16. Filas de color en y=16,48,80,112,...
// (azul, verde, rojo, naranja/amarillo, ...). Columnas: orbes (1-3), bolts (5-7).
inline GameObject* createBullet(Scene& scene, Faction faction, BulletVisual visual,
                                float x, float y, float vx, float vy,
                                float scale = 1.2f) {
    GameObject* go = scene.createGameObject("Bullet");
    go->transform->x = x;
    go->transform->y = y;
    go->transform->scaleX = go->transform->scaleY = scale; // 16px * scale

    // Recorte del sprite y forma de la caja según el aspecto elegido.
    float srcX, srcY, cw, ch;
    switch (visual) {
        case BulletVisual::RojoBolt:     srcX = 80.f; srcY = 80.f;  cw = 6.f;  ch = 12.f; break; // bolt rojo
        case BulletVisual::AmarilloOrbe: srcX = 32.f; srcY = 112.f; cw = 10.f; ch = 10.f; break; // orbe amarillo (redondo)
        case BulletVisual::RojoOrbe:     srcX = 32.f; srcY = 80.f;  cw = 10.f; ch = 10.f; break; // orbe rojo (redondo)
        case BulletVisual::AzulBolt:
        default:                         srcX = 80.f; srcY = 16.f;  cw = 6.f;  ch = 12.f; break; // bolt azul
    }
    auto sr = go->addComponent<SpriteRenderer>("assets/space/Bullets-0001.png");
    sr->setSourceRect(srcX, srcY, 16.0f, 16.0f);

    HitboxKind kind = (faction == Faction::Player) ? HitboxKind::PlayerBullet
                                                   : HitboxKind::EnemyBullet;

    // Movimiento: velocidad constante, sin gravedad (componente del engine).
    auto rb = go->addComponent<RigidBody2D>();
    rb->gravityScale = 0.0f;
    rb->velocityX = vx;
    rb->velocityY = vy;

    // Caja de colisión proporcional a la forma (bolt = alto y angosto; orbe = cuadrado).
    auto col = go->addComponent<BoxCollider>();
    col->width  = cw * scale;
    col->height = ch * scale;
    col->isTrigger = true;

    // Hitbox propio (owner = la bala misma): no sigue a un padre, se mueve sola.
    auto hb = go->addComponent<Hitbox>();
    hb->owner   = go;
    hb->faction = faction;
    hb->kind    = kind;
    // La bala desaparece al impactar el CUERPO del bando contrario. (El daño al
    // enemigo lo aplicará el Hitbox del propio enemigo, en el paso 5.)
    hb->onHit = [](Hitbox* self, Hitbox* other) {
        bool toEnemy  = self->faction == Faction::Player &&
                        other->faction == Faction::Enemy  && other->kind == HitboxKind::Body;
        bool toPlayer = self->faction == Faction::Enemy  &&
                        other->faction == Faction::Player && other->kind == HitboxKind::Body;
        if (toEnemy || toPlayer)
            self->gameObject->scene->destroy(self->gameObject);
    };

    // Se destruye al salir de la pantalla (recorre toda la vista sin importar su
    // velocidad). El Lifetime largo es solo un respaldo por si nunca entra a vista.
    go->addComponent<DestroyWhenOffscreen>();
    go->addComponent<Lifetime>()->seconds = 10.0f;
    return go;
}
