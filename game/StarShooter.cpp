#include "StarShooter.h"

#include "../engine/Scene.h"
#include "../engine/GameObject.h"
#include "../engine/Camera.h"

#include "star/Player.h"

void buildStarShooter(Scene& scene) {
    // Paso 1: solo el jugador y una cámara fija.
    // La cámara (0,0) deja el origen del mundo en el centro de la pantalla (480 en
    // una ventana de 960 de alto). Colocamos la nave bien abajo (y=390 -> ~y=870 en
    // pantalla) para dejar casi toda la columna libre para moverse, estilo DemonStar.
    Player::spawn(scene, 0.0f, 390.0f);

    // Cámara fija por ahora. El scroll vertical (ScrollCamera) llega en el paso 6.
    GameObject* cam = scene.createGameObject("MainCamera");
    cam->addComponent<Camera>();
}
