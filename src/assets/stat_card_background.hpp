#pragma once

#include "lvgl.h"

namespace trophy {

// Fond complet de la carte des ecrans Statistiques (bordure degradee,
// texture, bandeau, bracket -- tout compose en une seule image) -- voir
// src/assets/stat_card_background.cpp pour la provenance.
const lv_image_dsc_t *stat_card_background_descriptor();

} // namespace trophy
