#pragma once

#include <Arduino_GFX_Library.h>

#include "services/IBrightnessBackend.h"

// Implementation firmware reelle : convertit un pourcentage (0-100) vers
// l'octet 0-255 attendu par Arduino_CO5300::setBrightness(). Firmware
// uniquement (depend d'Arduino_GFX_Library) -- jamais compile pour le
// simulateur.
class Co5300BrightnessBackend : public IBrightnessBackend {
 public:
  explicit Co5300BrightnessBackend(Arduino_CO5300* gfx) : gfx_(gfx) {}

  void apply(int percent) override {
    if (!gfx_) return;
    int clamped = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    gfx_->setBrightness(static_cast<uint8_t>((clamped * 255) / 100));
  }

 private:
  Arduino_CO5300* gfx_;
};
