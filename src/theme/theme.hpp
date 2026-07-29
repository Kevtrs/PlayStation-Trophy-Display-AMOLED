#pragma once

#include "lvgl.h"

namespace trophy {

struct Palette {
    lv_color_t background_primary;
    lv_color_t background_secondary;
    lv_color_t surface_primary;
    lv_color_t surface_secondary;
    lv_color_t accent_blue;
    lv_color_t accent_violet;
    lv_color_t accent_cyan;
    lv_color_t text_primary;
    lv_color_t text_secondary;
    lv_color_t text_muted;
    lv_color_t success;
    lv_color_t warning;
    lv_color_t error;
    lv_color_t platinum;
    lv_color_t gold;
    lv_color_t silver;
    lv_color_t bronze;
};

extern Palette colors;

extern lv_style_t style_screen;
extern lv_style_t style_title;
extern lv_style_t style_subtitle;
extern lv_style_t style_label;
extern lv_style_t style_micro;
extern lv_style_t style_hero;
extern lv_style_t style_value;
extern lv_style_t style_button;
extern lv_style_t style_button_pressed;
extern lv_style_t style_surface;
extern lv_style_t style_chip;
extern lv_style_t style_debug;

void init_theme();
void add_text_style(lv_obj_t *obj, lv_style_t *style);
lv_color_t dim(lv_color_t color, uint8_t amount);

} // namespace trophy
