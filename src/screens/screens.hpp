#pragma once

#include "lvgl.h"

namespace trophy {

class App;

lv_obj_t *build_boot_screen(App &app);
lv_obj_t *build_welcome_screen(App &app);
lv_obj_t *build_dashboard_screen(App &app);
lv_obj_t *build_trophies_screen(App &app);
lv_obj_t *build_statistics_screen(App &app);
lv_obj_t *build_sync_screen(App &app);
lv_obj_t *build_celebration_screen(App &app);
lv_obj_t *build_settings_screen(App &app);
lv_obj_t *build_about_screen(App &app);
lv_obj_t *build_icon_gallery_screen(App &app);
lv_obj_t *build_offline_screen(App &app);
lv_obj_t *build_error_screen(App &app);

} // namespace trophy
