#pragma once

#include <cstdint>

// Reconnaissance de geste tactile (swipe/tap/appui long), portable entre le
// firmware (tactile reel CST9217) et le simulateur (souris SDL) -- reprend
// exactement les seuils du simulateur de reference du design retenu (voir
// src/main.cpp du projet design : swipe si deplacement horizontal > 54 px
// et vertical < 72 px, tap si deplacement < 18 px et duree < 520 ms, appui
// long a 760 ms) pour piloter trophy::ui_swipe_left()/ui_swipe_right()/
// ui_activate()/ui_long_press() (voir RoundUiBridge) exactement comme le
// ferait le simulateur original.
class GestureRecognizer {
 public:
  enum class Gesture { kNone, kSwipeLeft, kSwipeRight, kTap, kLongPress };

  // A appeler a chaque tick avec l'etat courant du pointeur (coordonnees
  // absolues en pixels, pressed = contact actif). Renvoie le geste detecte
  // a l'instant precis ou il se produit (jamais rejoue au tick suivant).
  Gesture update(int x, int y, bool pressed, uint32_t nowMs);

 private:
  bool wasPressed_ = false;
  int downX_ = 0;
  int downY_ = 0;
  uint32_t downMs_ = 0;
  bool longPressFired_ = false;
};
