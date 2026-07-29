#pragma once

#include "lvgl.h"

namespace trophy {

// Icones de l'ecran Statistiques (jeux termines / completion / rang mondial /
// temps de jeu), pour le board 7" (voir screens_wide.cpp) -- Google Material
// Symbols (Rounded, Filled), licence Apache 2.0, choisies par l'utilisateur
// le 2026-07-28 parmi 5 pistes comparees. Remplacent make_icon_box() (traits
// fins Lucide) pour ces 4 icones specifiquement : plus de poids visuel,
// mieux assorti aux trophees premium.
enum class StatIconKind { Gamepad, Percent, Medal, Clock };

const lv_image_dsc_t *stat_icon_descriptor(StatIconKind kind);

} // namespace trophy
