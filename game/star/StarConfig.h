#pragma once

// Configuración compartida de StarShooter.

// Velocidad a la que sube la cámara = avance del nivel (px/s). La usan el
// ScrollCamera y el bloque holdOnScreen (para que un enemigo se vea "quieto"
// en pantalla debe bajar a esta misma velocidad).
constexpr float SCROLL_SPEED = 60.0f;
