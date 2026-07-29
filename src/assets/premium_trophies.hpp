#pragma once

#include "lvgl.h"
#include "ui/ui_model.hpp"

#include <cstddef>
#include <cstdint>

namespace trophy {

const lv_image_dsc_t *premium_trophy_descriptor(TrophyKind kind, int requested_size = 128);
const int *premium_trophy_available_sizes(size_t *count);
uint32_t premium_trophy_memory_bytes(int size);

} // namespace trophy
