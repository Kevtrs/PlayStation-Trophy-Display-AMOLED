#pragma once

#include "lvgl.h"

namespace trophy {

// TEST -- scene complete generee par IA (anneau + decorations laterales, tirets,
// hexagones) redimensionnee pour tenir dans le budget flash (600x360,
// ratio 1.667 identique a l'ecran 800x480 -- voir
// scratchpad/convert_trophy_scene_wide.py) : retour utilisateur du
// 2026-07-29 ("pour les trophee jaurais tout pris ca").
const lv_image_dsc_t *trophy_scene_wide_descriptor();

} // namespace trophy
