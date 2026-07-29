#pragma once

#include "lvgl.h"
#include "ui/ui_model.hpp"

#include <string>

namespace trophy {

lv_obj_t *screen_root();
lv_obj_t *make_label(lv_obj_t *parent, const char *text, lv_style_t *style, int x, int y, int w = LV_SIZE_CONTENT);
lv_obj_t *make_chip(lv_obj_t *parent, const char *text, int x, int y, int w = LV_SIZE_CONTENT);
lv_obj_t *make_icon_box(lv_obj_t *parent, const char *name, int box_size, int glyph_size, lv_color_t color);
lv_obj_t *make_primary_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user_data);
lv_obj_t *make_status_badge(lv_obj_t *parent, const char *text, lv_color_t accent, int x, int y, const char *icon_name = "circle-dot");
lv_obj_t *make_trophy_icon(lv_obj_t *parent, TrophyKind kind, int size);
lv_obj_t *make_stat_tile(lv_obj_t *parent, const char *label, const char *value, const char *symbol, int x, int y, int w, int h);
lv_obj_t *make_trophy_row(lv_obj_t *parent, TrophyKind kind, const char *label, int value, int x, int y, int w);
// gap_start_angle/gap_end_angle : memes unites que lv_arc_set_bg_angles()
// (avant rotation, qui reste fixee a 270 en interne) -- par defaut (128,52)
// pour ne rien changer au rendu existant. Le Dashboard passe des valeurs
// differentes pour aligner l'ouverture de l'anneau avec le badge
// "% vers +1" qui vient s'y nicher (voir build_dashboard_screen()).
lv_obj_t *make_circular_progress(lv_obj_t *parent, int value, int size, lv_color_t color, int bg_width, int arc_width,
                                  int gap_start_angle = 128, int gap_end_angle = 52);
lv_obj_t *make_glow_ring(lv_obj_t *parent, int size, lv_color_t color, int width, lv_opa_t opa);
lv_obj_t *make_page_indicator(lv_obj_t *parent, int active, int count);
void set_obj_hidden(lv_obj_t *obj, bool hidden);
void pulse_obj(lv_obj_t *obj, int delay = 0);
void fade_in(lv_obj_t *obj, int delay = 0, int duration = 260);
void float_in(lv_obj_t *obj, int delay = 0, int dy = 12);
void animate_arc_value(lv_obj_t *arc, int target, int delay = 0);

} // namespace trophy
