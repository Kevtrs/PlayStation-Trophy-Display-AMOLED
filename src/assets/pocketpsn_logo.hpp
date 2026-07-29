#pragma once

#include <cstddef>
#include <cstdint>
#include "lvgl.h"

namespace assets {

const lv_image_dsc_t* pocketpsn_logo_descriptor();
std::size_t pocketpsn_logo_memory_bytes();
constexpr int kPocketPsnLogoWidth = 44;
constexpr int kPocketPsnLogoHeight = 23;

}  // namespace assets
