#pragma once

#include <cstdint>

#include "services/IBrightnessBackend.h"

// Gestion de la luminosite et de la veille (voir docs/IMPLEMENTATION_PLAN.md).
// Logique 100% portable ; l'application reelle au materiel passe toujours
// par IBrightnessBackend (jamais d'acces direct a LVGL/Arduino_GFX ici).
class PowerManager {
 public:
  enum class PowerState { kAwake, kDimmed, kAsleep };

  explicit PowerManager(IBrightnessBackend& backend);

  // brightnessPercent/sleepTimeoutSeconds : valeurs issues d'AppSettings.
  // sleepTimeoutSeconds == 0 => veille desactivee (mode toujours allume).
  void configure(int brightnessPercent, int sleepTimeoutSeconds);

  // A appeler a chaque interaction utilisateur (tactile) : reveille
  // immediatement l'ecran sans redemarrage si necessaire.
  void notifyActivity(uint32_t nowMillis);

  // A appeler periodiquement (non bloquant).
  void poll(uint32_t nowMillis);

  PowerState state() const { return state_; }
  bool isAsleep() const { return state_ == PowerState::kAsleep; }
  int currentBrightnessPercent() const { return currentBrightnessPercent_; }

 private:
  void applyBrightness(int percent);

  IBrightnessBackend& backend_;
  int configuredBrightnessPercent_ = 70;
  int sleepTimeoutSeconds_ = 180;
  int currentBrightnessPercent_ = 70;
  uint32_t lastActivityMillis_ = 0;
  PowerState state_ = PowerState::kAwake;
};
