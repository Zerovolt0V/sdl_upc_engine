# Guía StarShooter — cómo hacer enemigos, variantes y el boss

Guía para el equipo. Todo lo del shmup vive en `game/star/`. El **engine del profe** (`engine/`)
no se toca. Cada **tipo de enemigo** es un archivo en `game/star/enemies/`.

Índice:
1. [Arquitectura en 1 minuto](#1-arquitectura-en-1-minuto)
2. [Anatomía de un enemigo](#2-anatomía-de-un-enemigo)
3. [Cómo agregar un nuevo Move (paso a paso)](#3-cómo-agregar-un-nuevo-move-paso-a-paso)
4. [Referencia del toolkit de comportamientos](#4-referencia-del-toolkit)
5. [Convenciones que DEBES conocer](#5-convenciones-que-debes-conocer)
6. [Balas y curvas disponibles](#6-balas-y-curvas)
7. [Modo DEV: probar sin recompilar](#7-modo-dev)
8. [Implementar los GIGANTES (stubs → variantes)](#8-implementar-los-gigantes)
9. [Revisar / corregir otros enemigos](#9-revisar--corregir-otros-enemigos)
10. [Implementar el BOSS final](#10-implementar-el-boss-final)
11. [Compilar y correr](#11-compilar-y-correr)
12. [Convenciones de commits](#12-convenciones-de-commits)
13. [Apéndice: powerups y HUD (para Juanpi)](#13-apéndice-powerups-y-hud)

---

## 1. Arquitectura en 1 minuto

- El mundo se mide en **píxeles**, con **Y hacia abajo**. La cámara hace scroll subiendo;
  el mundo "baja" en pantalla. El player "cabalga" la cámara.
- Todos los enemigos heredan de **`Enemy`** (`Enemy.h` / `Enemy.cpp`), que trae:
  - un **toolkit** de funciones de movimiento, rotación y disparo (sección 4),
  - una **máquina de estados** simple (`phase`, `tPhase`, `nextPhase()`),
  - el **ensamblado** desde una ficha `EnemyDef` (sprite, vida, escala, cañones…).
- Cada tipo solo implementa **`void pattern(float dt)`** (su comportamiento) y un
  **`static spawn(...)`** que arma su `EnemyDef`.
- Un tipo puede tener **varias VARIANTES** (`enum Move`) — cada variante es un **patrón
  completo** (movimiento + disparo + rotación + fases). La variante se elige al spawnear.

**Colores = comportamiento. Tamaño = vidas.**

| Color | Rol | Dispara |
|---|---|---|
| 🔴 Rojo | **Horde** | No dispara (el cuerpo es la amenaza) |
| 🩷 Rosa | **Gunner** | Dispara hacia abajo / cortinas |
| 🟡 Amarillo | **Sniper** | Te **apunta** |

| Tamaño | Vidas |
|---|---|
| Pequeño | 1 |
| Mediano | 3 |
| Grande | 5 |
| Gigante | 8 |

---

## 2. Anatomía de un enemigo

Mira `enemies/SniperMediano.h` como ejemplo completo. Un tipo tiene 3 partes:

```cpp
#pragma once
#include "../Enemy.h"

class SniperMediano : public Enemy {
public:
    // (A) Las variantes: cada una es un patrón completo.
    enum Move { Hunter, Suppressor, Marksman, StrafeSnipeR, StrafeSnipeL };

    // (B) El comportamiento: se llama cada frame. dt = segundos desde el frame anterior.
    void pattern(float dt) override {
        switch ((Move)variant) {
        case Hunter: /* ... */ break;
        // ...
        default:     /* StrafeSnipeL */ break;
        }
    }

    // (C) La ficha + creación. 'move' elige la variante.
    static Enemy* spawn(Scene& s, float x, float y, GameObject* t, Move move = Hunter) {
        EnemyDef d;
        d.srcX=150; d.srcY=114; d.srcW=32; d.srcH=20; // recorte en el spritesheet
        d.lives=3; d.scale=2.2f; d.speed=100.f;
        d.fireInterval = 1.0f;   // cadencia base (seg)
        d.bulletSpeed  = 210.f;  // velocidad de bala base (px/s)
        d.muzzles = { {-4.f, 8.f}, {4.f, 8.f} }; // cañones EN PÍXELES del sprite
        d.variant = (int)move;
        return makeEnemy<SniperMediano>(s, x, y, t, d);
    }
};
```

### Campos de `EnemyDef` (ver `Enemy.h`)
| Campo | Para qué |
|---|---|
| `sheet` | ruta del spritesheet (por defecto `SpaceShips_Enemy-0001.png`) |
| `srcX/Y/W/H` | recorte del sprite dentro del sheet |
| `lives` | vidas |
| `scale` | escala visual (y del hitbox, cañones, etc.) |
| `speed` | velocidad base (la usan `advance`, `sineWeave`…) |
| `fireInterval` | cadencia base en segundos |
| `bulletSpeed` | velocidad base de bala en px/s |
| `muzzles` | cañones en **píxeles del sprite** desde el centro (`+Y` = al frente/hacia el jugador). Se escalan solos por `scale`. Vacío = 1 cañón al morro. |
| `variant` | qué variante (índice del `enum Move`) |
| `ignoreGate` | `true` = activo desde que nace y se gestiona solo (ver sección 5) |
| `animated`, `frameW/H`, `columns`, `frames`, `fps` | animación (para el boss, sección 10) |

> Todo lo que NO seteas usa el valor por defecto de `EnemyDef`.

---

## 3. Cómo agregar un nuevo Move (paso a paso)

Ejemplo: agregar una variante `Dropper` al `GunnerMediano`.

1. **Agrega el nombre al `enum Move`** del tipo:
   ```cpp
   enum Move { AlternateStrafe, CrossFire, TwinSpiral, StrafeAcrossR, StrafeAcrossL, Dropper };
   ```
2. **Agrega su `case`** en `pattern()`. Un patrón suele tener **fases**: entrar → atacar → salir.
   ```cpp
   case Dropper:
       switch (phase) {
       case 0: // ENTRAR: baja hasta cierta altura
           advance(dt);
           if (depthOnScreen() > -160.f) nextPhase();
           break;
       case 1: // ATACAR: se planta y suelta bombas rectas ~3s
           holdOnScreen(dt);
           if (every(0.5f, dt))
               for (int m = 0; m < muzzleCount(); ++m)
                   fireOne(90.f, bulletSpeed, BulletVisual::RojoBolt, 1.0f, m);
           if (tPhase > 3.0f) nextPhase();
           break;
       default: // SALIR
           retreat(90.f, 300.f, dt);
           break;
       }
       break;
   ```
3. **Regístralo en el modo DEV** para probarlo (sección 7): agrega `"Dropper"` al arreglo
   `V_GUNNER_M[]` en `StarShooter.cpp` y sube su contador en la tabla `DEV_TYPES`.
4. **Compila** (sección 11) y pruébalo con las teclas `I`/`O`.

**Regla de oro:** una variante = un **patrón completo y distinto**. No repitas lo que ya
hace otra variante o el mismo enemigo en otro tamaño. Combina 2–3 bloques del toolkit de
forma creativa (p. ej. `holdOnScreen` + `sweepX` + `fireAimed`).

---

## 4. Referencia del toolkit

Todas estas funciones están en `Enemy` (declaradas en `Enemy.h`, implementadas en `Enemy.cpp`).
Se llaman desde `pattern()`.

### Movimiento
| Función | Qué hace |
|---|---|
| `advance(dt)` | Baja recto a `speed`. El clásico "solo baja". |
| `holdOnScreen(dt)` | Se **ancla a la pantalla** (sigue la cámara). Úsalo para "plantarse" en un punto y no irte con el scroll. |
| `moveTo(tx,ty,spd,dt) → bool` | Va en línea recta a un punto **mundial**; devuelve `true` al llegar. |
| `driftTo(tx,ty,smooth,dt)` | Se acerca **suave** (exponencial) a un punto mundial. `smooth` ≈ 1–3. |
| `sineWeave(centerX,amp,freq,dt)` | Serpentea en X alrededor de `centerX` **mientras baja**. |
| `sweepX(centerX,amp,freq,dt)` | Solo mueve en X (no baja). Combínalo con `holdOnScreen`. |
| `chase(spd,maxTurnDeg,dt)` | Persigue al jugador con **giro limitado** (homing esquivable). |
| `orbit(scrCx,scrCy,radius,degPerSec,dt)` | Orbita un punto **anclado a la pantalla**. |
| `retreat(dirDeg,spd,dt)` | Se mueve en una **dirección fija** (ángulo en grados). |
| `followPath(pts,speed,dt) → bool` | Sigue una **curva** (Catmull-Rom) de `Paths`. `true` al terminar. Requiere `ignoreGate=true`. |

### Rotación
> El sprite apunta hacia abajo con `rotation = 0`. Un ángulo de movimiento de **90° = hacia abajo**.

| Función | Qué hace |
|---|---|
| `faceTarget()` | Orienta el morro **hacia el jugador**. |
| `faceVelocity(vx,vy)` | Orienta según un vector de dirección. |
| `spin(degPerSec,dt)` | Gira continuo (rueda). |
| `rotateToward(targetDeg,degPerSec,dt)` | Gira **suave** hacia un ángulo. `rotateToward(0,...)` = enderezar mirando abajo. |

### Cañones y disparo
> `scale` (tamaño de bala) va **antes** que `muzzle` en la firma. ⚠️ Error común: pasar el
> índice de cañón donde va `scale` → balas de tamaño 0 (invisibles).

| Función | Qué hace |
|---|---|
| `muzzleCount()` | Nº de cañones (mínimo 1). |
| `angleToTarget(muzzle=0)` | Ángulo hacia el jugador desde un cañón. |
| `fireOne(angleDeg, speed, v, scale=1, muzzle=0)` | Una bala a un ángulo fijo. |
| `fireAimed(speed, v, scale=1, muzzle=0)` | Una bala **apuntada** al jugador. |
| `fireSpread(count, arcDeg, centerDeg, speed, v, scale=1, muzzle=0)` | Abanico de `count` balas. |
| `fireRing(count, speed, phaseDeg, v, scale=1, muzzle=0)` | Anillo 360° (para bosses/gigantes). |
| `fireStack(count, minSpeed, maxSpeed, angleDeg, v, scale=1, muzzle=0)` | Varias balas mismo ángulo, distinta velocidad (muro que se estira). |
| `fireSpiral(step, speed, v, scale=1, muzzle=0)` | Una bala girando el ángulo `step` cada vez (espiral). |
| `fireForward(speed, v, scale=1, muzzle=0)` | Dispara en la dirección **que mira** la nave (útil con `spin`/`faceTarget`). |

> **Todas las funciones de fuego salen de UN cañón** (el parámetro `muzzle`). Para disparar
> desde todos: `for (int m=0; m<muzzleCount(); ++m) fireXxx(..., m);`

### Máquina de estados
| Elemento | Qué es |
|---|---|
| `int phase` | Fase actual (empieza en 0). Tú decides qué hace cada fase. |
| `float tPhase` | Segundos transcurridos **en la fase actual**. |
| `nextPhase()` | Avanza a la siguiente fase (resetea `tPhase` y el timer de `every`). |
| `every(interval, dt) → bool` | `true` cada `interval` seg. Úsalo para cadencia de disparo. |
| `every(interval, dt, timer) → bool` | Igual pero con un **timer propio** (2º cañón, 2ª cadencia). Declara `float miTimer=0;` como miembro. |
| `depthOnScreen()` | Profundidad en pantalla: **-480 (arriba) … 0 (centro) … +480 (abajo)**. Úsalo para "baja hasta cierta altura". |
| `variant` | Índice de la variante actual (tu `enum Move`). |
| `target` | El jugador (`GameObject*`). `target->transform->x/y` = su posición. |
| `homeX` | La X donde nació el enemigo. |

---

## 5. Convenciones que DEBES conocer

- **Ángulos:** `90°` = hacia abajo (hacia el jugador). `0°` = derecha, `180°` = izquierda,
  `-90°`/`270°` = arriba. Para `rotation`: `0` = mirando abajo.
- **Cañones (`muzzles`)** se dan en **PÍXELES DEL SPRITE** desde el centro. `+Y` = frente
  (hacia el jugador). Se escalan solos por `scale`. Mídelos sobre el sprite recortado.
- **El scroll / la cámara:** `advance` baja en el mundo; en pantalla se ve más rápido por el
  scroll. `holdOnScreen` cancela el scroll (te quedas fijo en pantalla).
- **Gate de visibilidad (importante):** por defecto un enemigo está **dormido hasta que
  entra a la vista**, y **muere cuando sale** tras haber sido visto. Por eso:
  - Si tu patrón se **planta** en pantalla, dale SIEMPRE una **salida** (que al final
    `advance` hacia abajo, o `retreat`, o un tope de tiempo). Si no, se queda vivo por
    siempre plantado.
  - Si tu patrón **sale y vuelve a entrar** de pantalla (curvas de `Paths`, scripts que se
    autogestionan), pon **`d.ignoreGate = true`** o el gate lo matará al salir.
- **No dejes balas/estados "colgados":** el enemigo y sus hitboxes hijos se limpian solos al
  morir (`die()`), no tienes que hacer nada especial.

---

## 6. Balas y curvas

### Visuales de bala (`BulletVisual.h`)
El color codifica la **amenaza**, no al enemigo. `Bolt` = rectangular (se ve bien recto);
`Orbe` = circular (se ve bien en diagonal / apuntado).

| Visual | Úsalo para |
|---|---|
| `AzulBolt` | balas del **player** |
| `RojoBolt` | disparo enemigo **recto** (hacia abajo) |
| `RojoOrbe` | disparo enemigo en **diagonal / espiral / abanico** |
| `AmarilloOrbe` | disparo enemigo **apuntado** (sniper) |

### Curvas (`Paths.h`)
Waypoints **relativos a la pantalla** (centro = 0,0). Extremos fuera de pantalla para entrar
y salir por los bordes. Requieren `ignoreGate=true`. Disponibles:
`SCurve`, `CRight`, `CLeft`, `SwoopRight`, `SwoopLeft`, `DiveMid`, `Loop`.
Uso: `if (followPath(Paths::SwoopRight, 240.f, dt)) die();`

---

## 7. Modo DEV

Al correr, el juego arranca en **modo desarrollo**: aparece un enemigo cada ~2s con X
aleatoria (nace arriba y entra con el scroll). Cambias en caliente, **sin recompilar**:

| Tecla | Acción |
|---|---|
| **K / L** | Tipo de enemigo anterior / siguiente |
| **I / O** | Variante anterior / siguiente del tipo actual |

La consola imprime `Enemigo: X | Variante N: Y` en cada cambio.

### Registrar un tipo en el selector (en `StarShooter.cpp`)
Los tipos viven en la tabla `DEV_TYPES`. Para agregar uno (ej. `HordeGigante`):

1. **Adaptador** (castea el índice al `enum Move`):
   ```cpp
   Enemy* devHordeGi(Scene& s, float x, float y, GameObject* t, int v) {
       return HordeGigante::spawn(s, x, y, t, (HordeGigante::Move)v);
   }
   ```
2. **Nombres de variantes:**
   ```cpp
   const char* V_HORDE_GI[] = { "Var1", "Var2", "Var3" };
   ```
3. **Fila en la tabla** `DEV_TYPES` (nombre, arreglo de nombres, nº de variantes, adaptador):
   ```cpp
   { "HordeGigante", V_HORDE_GI, 3, devHordeGi },
   ```
4. **Sube `DEV_TYPE_COUNT`** en 1.

Para **arrancar** en tu tipo mientras lo desarrollas: cambia el índice inicial
`auto typeIdx = std::make_shared<int>(N);` (N = fila de tu tipo en la tabla).

---

## 8. Implementar los GIGANTES

Los tres gigantes (`HordeGigante.h`, `GunnerGigante.h`, `SniperGigante.h`) hoy son **stubs**
de un solo patrón (solo bajan / disparan recto). Con **8 vidas** aguantan patrones largos y
por fases. Conviértelos en tipos con variantes, igual que los medianos:

1. Agrega un `enum Move { ... }` con 3–5 variantes.
2. Cambia `pattern()` a un `switch ((Move)variant)`.
3. Añade `Move move = <primera>` al `spawn` y `d.variant = (int)move;`.
4. Regístralo en el modo DEV (sección 7) y pruébalo.

**Ideas por color (que sean distintas de los medianos):**
- 🔴 **HordeGigante** (no dispara): muro enorme que baja y **empuja** (obliga a esquivar al
  ras del borde); o entra por una `Paths::Loop` gigante y sale; o desciende girando (`spin`)
  como un asteroide pesado.
- 🩷 **GunnerGigante**: ya tiene 3 cañones — barre **cortinas** (`fireSpread` amplio) que
  dejan huecos; o alterna anillos parciales (`fireRing` + `fireStack`); patrón por fases que
  cambia de ataque cada ~3s.
- 🟡 **SniperGigante**: cañonazos **apuntados en ráfaga** (`fireStack` apuntado), o abanicos
  que te predicen (mira cómo el `Stalker` del `HordeMediano` estima tu velocidad en X).

Recuerda: cada patrón debe **telegrafiar** antes de castigar (fase de aviso → ataque), y
dejar huecos para esquivar. Con 8 vidas el jugador verá el patrón varias veces: que valga la
pena mirarlo.

---

## 9. Revisar / corregir otros enemigos

- Corre en modo DEV y recorre **todos** los tipos con `K/L` e `I/O`. Anota lo que se sienta
  mal (muy OP, se sale de pantalla, balas raras, no telegrafía).
- Bugs típicos a vigilar:
  - **Se "corta" a mitad de patrón** → probablemente sale de pantalla y el **gate lo mata**.
    Centra el movimiento (`sweepX(0, ...)`) o pon `ignoreGate` si entra/sale a propósito.
  - **Balas invisibles** → se pasó el `muzzle` donde va `scale` (ver ⚠️ en sección 4).
  - **Salto de posición** al cambiar de fase → resetea/usa un centro coherente (ver cómo el
    `Bulldozer` del `HordeMediano` resetea `age=0` y centra en 0).
  - **Se queda plantado por siempre** → falta la fase de salida (sección 5).
- Ajusta números (velocidades, cadencias, tiempos de fase) — están todos a la vista en cada
  `case`. Cambia, recompila, prueba. Rápido.

---

## 10. Implementar el BOSS final

Sprite: `assets/space/SpaceShip_Boss-0001.png` (revisa si es **animado** — varios frames).
El boss es un `Enemy` más, pero:

- **Sprite animado:** en el `spawn`, en vez de `srcX/Y/W/H`, usa:
  ```cpp
  d.sheet   = "assets/space/SpaceShip_Boss-0001.png";
  d.animated = true;
  d.frameW = 128; d.frameH = 128; d.columns = 4;  // según el sheet real
  d.frames = {0,1,2,3};  d.fps = 8.f;             // celdas de la animación
  ```
- **Vida alta** (ej. `d.lives = 60`) y **muchos cañones** (`d.muzzles = { ... }` con varias
  posiciones en píxeles del sprite).
- **`ignoreGate = true`** (el boss se gestiona solo: entra, se planta arriba y pelea).
- **Multi-fase por ataques:** usa `phase` para encadenar ataques distintos, y **cambia de
  fase por vida** para que se ponga más agresivo cuando está bajo:
  ```cpp
  void pattern(float dt) override {
      // entra y se planta arriba
      if (phase == 0) { if (moveTo(0, lastCamY()-300, 120, dt)) nextPhase(); return; }

      // fase enrage al bajar de la mitad de vida
      if (lives <= 30 && phase < 5) { phase = 5; tPhase = 0; }

      switch (phase) {
      case 1: /* barrido de abanicos */ if (tPhase>4) nextPhase(); break;
      case 2: /* anillos fireRing     */ if (tPhase>4) nextPhase(); break;
      case 3: /* láser/apuntado       */ if (tPhase>4) nextPhase(); break;
      case 4: phase = 1; tPhase = 0; break; // vuelve a empezar el ciclo
      case 5: /* ENRAGE: más denso, dos ataques a la vez */ break;
      }
  }
  ```
  (`lives` es público en `Enemy`, puedes leerlo para decidir la fase.)
- Combina las funciones de fuego: `fireRing` (anillos), `fireSpread` (abanicos), `fireAimed`
  (apuntado), `fireSpiral` (espirales), `fireStack` (muros). Alterna cañones con `muzzle`.
- Regístralo también en el modo DEV para iterarlo cómodo.

> VFX y sonido (explosión con `Explosion-0001.png`, screen shake, música) son un chunk
> aparte; primero que el patrón funcione.

---

## 11. Compilar y correr

Visual Studio (recomendado): abre `sdl_upc_engine.sln`, configuración **Debug / x64**, F5.

Por línea de comandos (PowerShell):
```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" `
  sdl_upc_engine.vcxproj /p:Configuration=Debug /p:Platform=x64 /m
```

⚠️ **Si sale `LNK1104` / `LNK1168` "no se puede abrir el .exe"** → el **juego está abierto**.
Ciérralo y recompila (el error de link es solo porque el .exe está en uso, tu código
compiló bien).

Cada **enemigo nuevo** debe estar incluido en `Enemies.h` (ya están los 12). Si creas un
archivo `.h` nuevo, agrégalo ahí **y** al proyecto (`sdl_upc_engine.vcxproj`) — o solo
inclúyelo desde `Enemies.h` (al ser header no necesita entrar al `.vcxproj`, pero mantén la
lista ordenada).

---

## 12. Convenciones de commits

- Rama de trabajo: `feat/StarShooter` (la del profe se sincroniza aparte; ver a Sol).
- **Commits pequeños y por cosa**: "un enemigo / un fix / un feature" por commit.
- Mensajes claros en español, prefijo `StarShooter:` — ej.
  `StarShooter: HordeGigante con variantes (Wall/Roller/Loop)`.
- **No** agregues trailers de co-autor de herramientas de IA.

---

## 13. Apéndice: powerups y HUD

(Aún no implementado — assets disponibles: `Bonuses-0001.png` para los powerups,
`UI_sprites-0001.png` para el HUD, `Barrier-0001.png` para escudo.)

Ideas de enganche (a coordinar con Sol para no chocar con `Player`):
- **Powerup** = un `GameObject` con `SpriteRenderer` + un `Hitbox` `Faction::Neutral/Pickup`
  que **cae** (como una bala lenta) y al colisionar con el player aplica un efecto
  (subir cadencia, arma doble, escudo/`Barrier`, bomba extra).
- **HUD** = objetos con `SpriteRenderer` **anclados a la pantalla** (posición relativa a la
  cámara, como el fondo) que dibujan vidas, bombas y powerup activo con recortes de
  `UI_sprites-0001.png`.
- Revisa `Player.h` para ver qué stats existen (cadencia, cooldown, etc.) — un powerup
  básicamente modifica esos campos por un tiempo.

Cualquier duda de cómo está armado el player o las escenas, pregúntale a Sol.
