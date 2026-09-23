#include "assets/profile_avatar.hpp"

#include "assets/profile_avatar_demo.hpp"

#include <cctype>

namespace assets {
namespace {

ProfileAvatarResolver g_resolver = demo_profile_avatar_descriptor;

bool is_allowed_avatar_char(char c) {
    const unsigned char uc = static_cast<unsigned char>(c);
    return std::isalnum(uc) || c == '_' || c == '-' || c == '.';
}

}  // namespace

bool is_safe_pocketpsn_avatar_name(const std::string &avatar_file_name) {
    if(avatar_file_name.empty() || avatar_file_name.size() > 64) return false;
    if(avatar_file_name.find("..") != std::string::npos) return false;
    if(avatar_file_name.find('/') != std::string::npos || avatar_file_name.find('\\') != std::string::npos) return false;
    for(char c : avatar_file_name) {
        if(!is_allowed_avatar_char(c)) return false;
    }
    return avatar_file_name.size() > 4 && avatar_file_name.rfind(".png") == avatar_file_name.size() - 4;
}

std::string pocketpsn_avatar_url(const std::string &avatar_file_name) {
    if(!is_safe_pocketpsn_avatar_name(avatar_file_name)) return "";
    return std::string(kPocketPsnAvatarBaseUrl) + avatar_file_name;
}

void set_profile_avatar_resolver(ProfileAvatarResolver resolver) {
    g_resolver = resolver;
}

const lv_image_dsc_t *profile_avatar_descriptor(const std::string &avatar_file_name) {
    if(!g_resolver || !is_safe_pocketpsn_avatar_name(avatar_file_name)) return nullptr;
    return g_resolver(avatar_file_name.c_str());
}

}  // namespace assets
