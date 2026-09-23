#pragma once

#include "lvgl.h"

#include <cstddef>

namespace trophy {

const lv_image_dsc_t *board7_icon_descriptor(const char *name, int requested_size);
std::size_t board7_icon_memory_bytes();

}  // namespace trophy
