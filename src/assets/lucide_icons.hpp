#pragma once

#include "lvgl.h"

#include <cstddef>

namespace trophy {
lv_obj_t *make_lucide_icon(lv_obj_t *parent, const char *name, int size, lv_color_t color);
const char *const *lucide_icon_names(size_t *count);
} // namespace trophy
