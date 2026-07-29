#include "theme/theme.hpp"

#include "assets/fonts/td_fonts.hpp"

namespace trophy {

Palette colors{
    lv_color_hex(0x000000),
    lv_color_hex(0x060913),
    lv_color_hex(0x101626),
    lv_color_hex(0x172036),
    lv_color_hex(0x2F7CFF),
    lv_color_hex(0x6D45FF),
    lv_color_hex(0x27D8FF),
    lv_color_hex(0xF4F8FF),
    lv_color_hex(0xB9C5D9),
    lv_color_hex(0x6F7B92),
    lv_color_hex(0x42F5A7),
    lv_color_hex(0xFFD166),
    lv_color_hex(0xFF5C7A),
    lv_color_hex(0xDFFBFF),
    lv_color_hex(0xFFD66B),
    lv_color_hex(0xC8D4E8),
    lv_color_hex(0xC77A3A),
};

lv_style_t style_screen;
lv_style_t style_title;
lv_style_t style_subtitle;
lv_style_t style_label;
lv_style_t style_micro;
lv_style_t style_hero;
lv_style_t style_value;
lv_style_t style_button;
lv_style_t style_button_pressed;
lv_style_t style_surface;
lv_style_t style_chip;
lv_style_t style_debug;

static void init_text(lv_style_t *style, const lv_font_t *font, lv_color_t color, lv_text_align_t align = LV_TEXT_ALIGN_CENTER) {
    lv_style_init(style);
    lv_style_set_text_font(style, font);
    lv_style_set_text_color(style, color);
    lv_style_set_text_align(style, align);
    lv_style_set_text_letter_space(style, 0);
    lv_style_set_text_line_space(style, 0);
}

void init_theme() {
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, colors.background_primary);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_radius(&style_screen, 0);

    init_text(&style_title, &td_font_24, colors.text_primary);
    init_text(&style_subtitle, &td_font_14, colors.text_secondary);
    init_text(&style_label, &td_font_16, colors.text_primary);
    init_text(&style_micro, &td_font_12, colors.text_muted);
    init_text(&style_hero, &td_font_48, colors.text_primary);
    init_text(&style_value, &td_font_28, colors.text_primary);

    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, colors.accent_blue);
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_radius(&style_button, 28);
    lv_style_set_border_width(&style_button, 1);
    lv_style_set_border_color(&style_button, colors.accent_cyan);
    lv_style_set_border_opa(&style_button, LV_OPA_50);
    lv_style_set_pad_left(&style_button, 22);
    lv_style_set_pad_right(&style_button, 22);
    lv_style_set_pad_top(&style_button, 10);
    lv_style_set_pad_bottom(&style_button, 10);
    lv_style_set_text_color(&style_button, colors.text_primary);
    lv_style_set_text_font(&style_button, &td_font_16);

    lv_style_init(&style_button_pressed);
    lv_style_set_transform_scale_x(&style_button_pressed, 245);
    lv_style_set_transform_scale_y(&style_button_pressed, 245);
    lv_style_set_bg_color(&style_button_pressed, colors.accent_violet);

    lv_style_init(&style_surface);
    lv_style_set_bg_color(&style_surface, colors.surface_primary);
    lv_style_set_bg_opa(&style_surface, LV_OPA_80);
    lv_style_set_radius(&style_surface, 18);
    lv_style_set_border_width(&style_surface, 1);
    lv_style_set_border_color(&style_surface, lv_color_hex(0x26354F));
    lv_style_set_border_opa(&style_surface, LV_OPA_60);
    lv_style_set_pad_all(&style_surface, 10);

    lv_style_init(&style_chip);
    lv_style_set_bg_color(&style_chip, colors.surface_secondary);
    lv_style_set_bg_opa(&style_chip, LV_OPA_70);
    lv_style_set_radius(&style_chip, 16);
    lv_style_set_border_width(&style_chip, 1);
    lv_style_set_border_color(&style_chip, colors.accent_blue);
    lv_style_set_border_opa(&style_chip, LV_OPA_40);
    lv_style_set_pad_left(&style_chip, 10);
    lv_style_set_pad_right(&style_chip, 10);
    lv_style_set_pad_top(&style_chip, 5);
    lv_style_set_pad_bottom(&style_chip, 5);

    lv_style_init(&style_debug);
    lv_style_set_bg_color(&style_debug, lv_color_hex(0x0B1020));
    lv_style_set_bg_opa(&style_debug, LV_OPA_90);
    lv_style_set_radius(&style_debug, 12);
    lv_style_set_border_width(&style_debug, 1);
    lv_style_set_border_color(&style_debug, colors.accent_cyan);
    lv_style_set_border_opa(&style_debug, LV_OPA_40);
    lv_style_set_pad_all(&style_debug, 8);
    lv_style_set_text_color(&style_debug, colors.text_secondary);
    lv_style_set_text_font(&style_debug, &td_font_10);
}

void add_text_style(lv_obj_t *obj, lv_style_t *style) {
    lv_obj_add_style(obj, style, 0);
}

lv_color_t dim(lv_color_t color, uint8_t amount) {
    return lv_color_mix(lv_color_hex(0x000000), color, amount);
}

} // namespace trophy
