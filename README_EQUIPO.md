# 🚀 Guía del equipo — StarShooter (fork de Zerovolt0V)

Este repo es un **fork** de `MoisesUPC/sdl_upc_engine` (el engine del profe).
Nuestro `main` se mantiene sincronizado automáticamente con el del profe, así que
solo trabajamos en ramas y de ahí salen los Pull Requests.

## 1. Requisitos (una sola vez)
- **Visual Studio 2022** con *Desarrollo para escritorio con C++*.
- **SDL3** y **SDL3_image** instalados en `C:\SDL3` y `C:\SDL3_image`
  (mismas rutas que usa el `.vcxproj`).
- Pídele a Zerovolt0V que te agregue como **Collaborator**
  (GitHub → Settings → Collaborators), así puedes subir tus ramas.

## 2. Clonar el proyecto
```bash
git clone https://github.com/Zerovolt0V/sdl_upc_engine.git
cd sdl_upc_engine
```

## 3. Crear tu rama de trabajo
Nunca trabajes en `main` (es el espejo del engine del profe).
Crea tu propia rama partiendo de lo último:
```bash
git checkout main
git pull origin main
git checkout -b feat/mi-tarea        # ej: feat/enemigos, feat/hud
```

## 4. Compilar y correr
- Abre `sdl_upc_engine.sln` en Visual Studio.
- En la barra superior elige **Debug / x64**
  (es la única configuración con SDL lista).
- F5 para compilar y ejecutar.

## 5. Subir tu trabajo
```bash
git add -A
git commit -m "Descripción de lo que hiciste"
git push -u origin feat/mi-tarea
```
Luego abre un **Pull Request** en GitHub para revisarlo en equipo.

## 6. Traer los cambios del profe (cuando actualice el engine)
El fork se sincroniza solo con el repo del profe.
Para incorporar esos cambios a tu rama:
```bash
git checkout main
git pull origin main              # baja lo último del profe (ya sincronizado)
git checkout feat/mi-tarea
git merge main                    # lo trae a tu rama
# si hay conflictos: resuélvelos en VS, luego  git add -A  y  git commit
git push origin feat/mi-tarea
```

## ⚠️ Reglas del equipo
- **No se programa en `main`** — solo en ramas. Todo el juego vive en ramas/PRs.
- Compila siempre en **Debug | x64** antes de hacer push.
- Si el `.vcxproj` da conflicto al fusionar, la solución casi siempre es
  **conservar ambas listas de archivos** (las del profe + las nuestras de `game/`).
