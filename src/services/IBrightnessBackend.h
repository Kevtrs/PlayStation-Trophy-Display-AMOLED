#pragma once

// Abstraction materielle pour le retro-eclairage (voir docs/ARCHITECTURE.md).
// PowerManager (portable) ne doit jamais dependre directement du pilote
// d'ecran (Arduino_GFX/CO5300) ni de LVGL : seule l'implementation
// firmware (src/board/) connait le materiel reel. L'implementation
// simulateur ne fait qu'enregistrer la valeur (aucun effet visuel ajoute --
// voir la note de compromis dans docs/IMPLEMENTATION_PLAN.md : le rendu
// visuel de l'assombrissement appartient a une passe UI ulterieure).
class IBrightnessBackend {
 public:
  virtual ~IBrightnessBackend() = default;

  // percent : 0-100. L'implementation convertit vers l'unite materielle
  // reelle (ex: 0-255 pour Arduino_CO5300::setBrightness).
  virtual void apply(int percent) = 0;
};
