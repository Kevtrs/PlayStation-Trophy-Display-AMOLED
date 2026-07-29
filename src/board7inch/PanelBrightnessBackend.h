#pragma once

#include <esp_display_panel.hpp>

#include "services/IBrightnessBackend.h"

// Retroeclairage pilote par l'IO expander CH422G (marche/arret uniquement,
// pas de gradation PWM sur ce board -- voir esp_panel_board_custom_conf.h,
// ESP_PANEL_BACKLIGHT_TYPE_SWITCH_EXPANDER). Backlight::setBrightness()
// gere elle-meme le seuil marche/arret pour ce type de retroeclairage (voir
// ESP32_Display_Panel, drivers/backlight/esp_panel_backlight.hpp) -- a
// reverifier une fois le materiel en main si un pilotage PWM reel s'avere
// possible/souhaite.
class PanelBrightnessBackend : public IBrightnessBackend {
 public:
  explicit PanelBrightnessBackend(esp_panel::drivers::Backlight* backlight) : backlight_(backlight) {}

  void apply(int percent) override {
    if (!backlight_) return;
    int clamped = percent < 0 ? 0 : (percent > 100 ? 100 : percent);
    backlight_->setBrightness(clamped);
  }

 private:
  esp_panel::drivers::Backlight* backlight_;
};
