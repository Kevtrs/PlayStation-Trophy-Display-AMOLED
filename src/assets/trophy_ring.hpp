#pragma once

#include "lvgl.h"

namespace trophy {

// Anneau lumineux pour les ecrans Trophees (voir
// src/assets/trophy_ring.cpp pour la provenance) -- remplace les deux
// appels make_dual_ring() (code) par un seul asset genere par IA plus riche.
const lv_image_dsc_t *trophy_ring_descriptor();

} // namespace trophy
