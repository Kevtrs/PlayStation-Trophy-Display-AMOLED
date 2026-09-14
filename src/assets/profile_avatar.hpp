#pragma once

#include "lvgl.h"

#include <string>

namespace assets {

constexpr const char *kPocketPsnAvatarBaseUrl = "https://img.pocketpsn.com/psn_avatars/";

using ProfileAvatarResolver = const lv_image_dsc_t *(*)(const char *avatar_file_name);

bool is_safe_pocketpsn_avatar_name(const std::string &avatar_file_name);
std::string pocketpsn_avatar_url(const std::string &avatar_file_name);

void set_profile_avatar_resolver(ProfileAvatarResolver resolver);
const lv_image_dsc_t *profile_avatar_descriptor(const std::string &avatar_file_name);

}  // namespace assets
