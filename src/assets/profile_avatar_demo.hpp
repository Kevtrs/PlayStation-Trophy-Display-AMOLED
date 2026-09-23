#pragma once

#include "lvgl.h"

#include <cstddef>

namespace assets {

const lv_image_dsc_t *demo_profile_avatar_descriptor(const char *avatar_file_name);
std::size_t demo_profile_avatar_memory_bytes();

}  // namespace assets
