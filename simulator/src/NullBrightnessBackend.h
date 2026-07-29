#pragma once

#include "services/IBrightnessBackend.h"

// Simulateur : n'applique aucun effet visuel (design hors perimetre, voir
// docs/IMPLEMENTATION_PLAN.md). Se contente d'enregistrer la derniere
// valeur demandee pour que le panneau de debug puisse l'afficher en texte.
class NullBrightnessBackend : public IBrightnessBackend {
 public:
  void apply(int percent) override { lastAppliedPercent_ = percent; }

  int lastAppliedPercent() const { return lastAppliedPercent_; }

 private:
  int lastAppliedPercent_ = 70;
};
