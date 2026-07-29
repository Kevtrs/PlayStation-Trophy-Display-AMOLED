#pragma once

#include "lvgl.h"

namespace trophy {

// TEST -- anneau genere par IA a pleine intensite (pas d'attenuation, contrairement
// a trophy_ring.hpp) : seul element decoratif des ecrans Trophees/Statistiques
// v3, voir screens_wide.hpp. Retour utilisateur du 2026-07-29.
const lv_image_dsc_t *trophy_ring_full_descriptor();

} // namespace trophy
