#pragma once

#include <vector>
#include "Enemy.h" // Vec2

// Curvas predefinidas (waypoints RELATIVOS a la pantalla: centro=(0,0),
// x∈[-360,360], y∈[-480,480]). Los EXTREMOS van FUERA de la pantalla (y≈±560,
// x≈±460) para que los enemigos entren y salgan por los bordes, no mid-screen.
// (Requiere ignoreGate=true en el enemigo, para que el gate no lo mate al salir.)
namespace Paths {

// S de arriba (fuera) hacia abajo (fuera).
inline const std::vector<Vec2> SCurve = {
    {-200,-560},{240,-260},{-240,40},{240,320},{-100,560}
};

// C que se abomba a la DERECHA.
inline const std::vector<Vec2> CRight = {
    {-60,-560},{200,-280},{300,0},{200,280},{-60,560}
};

// C que se abomba a la IZQUIERDA.
inline const std::vector<Vec2> CLeft = {
    {60,-560},{-200,-280},{-300,0},{-200,280},{60,560}
};

// Entra por la IZQUIERDA (fuera) y cruza en diagonal saliendo por la DERECHA (fuera).
inline const std::vector<Vec2> SwoopRight = {
    {-460,-300},{-120,-140},{160,20},{460,240}
};

// Entra por la DERECHA y cruza hacia la IZQUIERDA.
inline const std::vector<Vec2> SwoopLeft = {
    {460,-300},{120,-140},{-160,20},{-460,240}
};

// Baja casi recto por el centro con una leve ondulación.
inline const std::vector<Vec2> DiveMid = {
    {0,-560},{50,-120},{-50,140},{0,560}
};

// Baja, hace un rizo (loop) y sale por abajo.
inline const std::vector<Vec2> Loop = {
    {0,-560},{200,-260},{240,0},{40,100},{-140,-40},{40,-240},{200,-140},{120,220},{0,560}
};

} // namespace Paths
