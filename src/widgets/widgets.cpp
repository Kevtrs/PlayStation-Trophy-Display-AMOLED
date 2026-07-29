#include "widgets/widgets.hpp"

#include "assets/fonts/td_fonts.hpp"
#include "assets/lucide_icons.hpp"
#include "assets/premium_trophies.hpp"
#include "theme/theme.hpp"
#include "ui/layout.hpp"

#include <cstdio>
#include <cstring>

namespace trophy {

lv_obj_t *screen_root() {
    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_set_size(root, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_add_style(root, &style_screen, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    return root;
}

lv_obj_t *make_label(lv_obj_t *parent, const char *text, lv_style_t *style, int x, int y, int w) {
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, style, 0);
    if(w != LV_SIZE_CONTENT) {
        lv_obj_set_width(label, w);
        lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    }
    lv_obj_set_pos(label, x, y);
    return label;
}

lv_obj_t *make_chip(lv_obj_t *parent, const char *text, int x, int y, int w) {
    lv_obj_t *chip = lv_obj_create(parent);
    lv_obj_add_style(chip, &style_chip, 0);
    lv_obj_clear_flag(chip, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_height(chip, 30);
    lv_obj_set_width(chip, w == LV_SIZE_CONTENT ? 106 : w);
    lv_obj_set_pos(chip, x, y);
    lv_obj_t *label = lv_label_create(chip);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &style_micro, 0);
    lv_obj_center(label);
    return chip;
}

lv_obj_t *make_icon_box(lv_obj_t *parent, const char *name, int box_size, int glyph_size, lv_color_t color) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(box, box_size, box_size);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_t *icon = make_lucide_icon(box, name, glyph_size, color);
    lv_obj_center(icon);
    return box;
}

lv_obj_t *make_primary_button(lv_obj_t *parent, const char *text, lv_event_cb_t cb, void *user_data) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_add_style(btn, &style_button, 0);
    lv_obj_add_style(btn, &style_button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, 180, 48);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user_data);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &style_label, 0);
    lv_obj_set_style_text_font(label, &td_font_16, 0);
    lv_obj_center(label);
    return btn;
}

lv_obj_t *make_status_badge(lv_obj_t *parent, const char *text, lv_color_t accent, int x, int y, const char *icon_name) {
    lv_obj_t *badge = lv_obj_create(parent);
    lv_obj_clear_flag(badge, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(badge, 124, 30);
    lv_obj_set_pos(badge, x, y);
    lv_obj_set_style_radius(badge, 16, 0);
    lv_obj_set_style_bg_color(badge, lv_color_mix(accent, colors.surface_secondary, 45), 0);
    lv_obj_set_style_bg_opa(badge, LV_OPA_80, 0);
    lv_obj_set_style_border_width(badge, 1, 0);
    lv_obj_set_style_border_color(badge, accent, 0);
    lv_obj_set_style_border_opa(badge, LV_OPA_50, 0);
    lv_obj_set_style_pad_all(badge, 0, 0);
    lv_obj_t *dot = make_icon_box(badge, icon_name ? icon_name : "circle-dot", icons::BADGE_BOX, icons::BADGE_GLYPH, accent);
    lv_obj_set_pos(dot, 7, 5);
    lv_obj_t *label = lv_label_create(badge);
    lv_label_set_text(label, text);
    lv_obj_add_style(label, &style_micro, 0);
    lv_obj_set_style_text_color(label, colors.text_primary, 0);
    lv_obj_set_width(label, 86);
    lv_obj_set_height(label, 16);
    lv_label_set_long_mode(label, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(label, 31, 7);
    return badge;
}

lv_obj_t *make_trophy_icon(lv_obj_t *parent, TrophyKind kind, int size) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_clear_flag(box, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(box, size, size);
    lv_obj_set_style_bg_opa(box, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(box, 0, 0);
    lv_obj_set_style_pad_all(box, 0, 0);
    lv_obj_set_style_radius(box, 0, 0);

    lv_obj_t *img = lv_image_create(box);
    lv_image_set_src(img, premium_trophy_descriptor(kind, size));
    lv_obj_set_size(img, size, size);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_center(img);
    return box;
}

lv_obj_t *make_stat_tile(lv_obj_t *parent, const char *label, const char *value, const char *symbol, int x, int y, int w, int h) {
    lv_obj_t *tile = lv_obj_create(parent);
    lv_obj_add_style(tile, &style_surface, 0);
    lv_obj_clear_flag(tile, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(tile, w, h);
    lv_obj_set_pos(tile, x, y);
    lv_obj_set_style_pad_all(tile, 0, 0);

    lv_obj_t *sym = make_icon_box(tile, symbol, icons::STAT_BOX, icons::STAT_GLYPH, colors.accent_cyan);
    lv_obj_align(sym, LV_ALIGN_TOP_MID, 0, 10);

    lv_obj_t *v = lv_label_create(tile);
    lv_label_set_text(v, value);
    lv_obj_add_style(v, &style_value, 0);
    lv_obj_set_style_text_font(v, std::strlen(value) >= 8 ?&td_font_20 : &td_font_22, 0);
    lv_obj_set_width(v, w - 24);
    lv_label_set_long_mode(v, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(v, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(v, 12, 39);

    lv_obj_t *l = lv_label_create(tile);
    lv_label_set_text(l, label);
    lv_obj_add_style(l, &style_micro, 0);
    lv_obj_set_width(l, w - 24);
    lv_label_set_long_mode(l, LV_LABEL_LONG_CLIP);
    lv_obj_set_style_text_align(l, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(l, 12, h - 26);
    return tile;
}

lv_obj_t *make_trophy_row(lv_obj_t *parent, TrophyKind kind, const char *label, int value, int x, int y, int w) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_add_style(row, &style_surface, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(row, w, 58);
    lv_obj_set_pos(row, x, y);
    lv_obj_set_style_radius(row, 28, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_t *icon = make_trophy_icon(row, kind, 48);
    lv_obj_set_pos(icon, 5, 5);
    lv_obj_t *name = lv_label_create(row);
    lv_label_set_text(name, label);
    lv_obj_add_style(name, &style_label, 0);
    lv_obj_set_style_text_font(name, &td_font_16, 0);
    lv_obj_set_pos(name, 66, 10);
    lv_obj_t *sub = lv_label_create(row);
    lv_label_set_text(sub, "trophées");
    lv_obj_add_style(sub, &style_micro, 0);
    lv_obj_set_pos(sub, 66, 32);
    lv_obj_t *val = lv_label_create(row);
    std::string n = format_number(value);
    lv_label_set_text(val, n.c_str());
    lv_obj_add_style(val, &style_value, 0);
    lv_obj_set_style_text_font(val, &td_font_22, 0);
    lv_obj_set_width(val, 88);
    lv_obj_align(val, LV_ALIGN_RIGHT_MID, -14, 0);
    return row;
}

lv_obj_t *make_circular_progress(lv_obj_t *parent, int value, int size, lv_color_t color, int bg_width, int arc_width,
                                  int gap_start_angle, int gap_end_angle) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, value);
    lv_arc_set_bg_angles(arc, gap_start_angle, gap_end_angle);
    lv_arc_set_rotation(arc, 270);
    lv_obj_set_style_arc_color(arc, lv_color_hex(0x0F1930), LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, bg_width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, arc_width, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_INDICATOR);
    // lv_obj_remove_style_all() ne suffit pas a masquer la poignee
    // (LV_PART_KNOB) que lv_arc dessine par defaut a l'extremite de la
    // valeur -- visible sur ecran reel comme un petit triangle/flgraphe
    // pres du debut de l'anneau (jamais remarque avant car masque par le
    // chevauchement anneau/badge, corrige separement). Cet anneau n'est
    // qu'un indicateur visuel, jamais un curseur interactif : la poignee
    // est donc explicitement rendue transparente et sans epaisseur.
    lv_obj_set_style_bg_opa(arc, LV_OPA_TRANSP, LV_PART_KNOB);
    lv_obj_set_style_border_width(arc, 0, LV_PART_KNOB);
    lv_obj_set_style_pad_all(arc, 0, LV_PART_KNOB);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

lv_obj_t *make_glow_ring(lv_obj_t *parent, int size, lv_color_t color, int width, lv_opa_t opa) {
    lv_obj_t *ring = lv_arc_create(parent);
    lv_obj_remove_style_all(ring);
    lv_obj_set_size(ring, size, size);
    lv_arc_set_range(ring, 0, 100);
    lv_arc_set_value(ring, 100);
    lv_arc_set_bg_angles(ring, 0, 360);
    lv_obj_set_style_arc_color(ring, color, LV_PART_MAIN);
    lv_obj_set_style_arc_width(ring, width, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(ring, opa, LV_PART_MAIN);
    lv_obj_clear_flag(ring, LV_OBJ_FLAG_CLICKABLE);
    return ring;
}

lv_obj_t *make_page_indicator(lv_obj_t *parent, int active, int count) {
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(wrap, 120, 14);
    lv_obj_set_style_bg_opa(wrap, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(wrap, 0, 0);
    lv_obj_set_style_pad_all(wrap, 0, 0);
    for(int i = 0; i < count; ++i) {
        lv_obj_t *dot = lv_obj_create(wrap);
        lv_obj_clear_flag(dot, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(dot, i == active ? 20 : 7, 7);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(dot, i == active ? colors.accent_cyan : colors.text_muted, 0);
        lv_obj_set_style_bg_opa(dot, i == active ? LV_OPA_COVER : LV_OPA_50, 0);
        lv_obj_set_style_border_width(dot, 0, 0);
        lv_obj_set_pos(dot, 10 + i * 18, 3);
    }
    lv_obj_align(wrap, LV_ALIGN_TOP_MID, 0, PAGE_INDICATOR_Y);
    return wrap;
}

void set_obj_hidden(lv_obj_t *obj, bool hidden) {
    if(hidden) lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
    else lv_obj_remove_flag(obj, LV_OBJ_FLAG_HIDDEN);
}

void pulse_obj(lv_obj_t *obj, int delay) {
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 255, 238);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_duration(&a, 520);
    lv_anim_set_playback_duration(&a, 520);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, [](void *target, int32_t v) {
        lv_obj_set_style_transform_scale_x(static_cast<lv_obj_t *>(target), v, 0);
        lv_obj_set_style_transform_scale_y(static_cast<lv_obj_t *>(target), v, 0);
    });
    lv_anim_start(&a);
}

void fade_in(lv_obj_t *obj, int delay, int duration) {
    lv_obj_set_style_opa(obj, LV_OPA_TRANSP, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_duration(&a, duration);
    lv_anim_set_exec_cb(&a, [](void *target, int32_t v) {
        lv_obj_set_style_opa(static_cast<lv_obj_t *>(target), static_cast<lv_opa_t>(v), 0);
    });
    lv_anim_start(&a);
}

void float_in(lv_obj_t *obj, int delay, int dy) {
    // lv_obj_update_layout() force la resolution des coordonnees avant de
    // les lire ci-dessous : immediatement apres creation/positionnement
    // (lv_obj_set_pos()/lv_obj_align()), LVGL 9.4 n'a pas encore execute de
    // passe de mise en page, et lv_obj_get_y() peut alors renvoyer une
    // valeur perimee (constate : 0) au lieu de la position reelle qui vient
    // d'etre fixee. float_in() calculait alors son animation (v+dy -> v)
    // a partir de cette valeur perimee, faisant retomber l'objet anime a
    // la mauvaise position finale -- bug reel trouve et corrige le
    // 2026-07-23 sur les ecrans Statistiques et A propos (voir
    // docs/KNOWN_ISSUES.md pour les captures avant/apres et l'analyse
    // complete). lv_obj_update_layout(obj) resout toujours l'ecran entier
    // qui le contient (voir son implementation dans LVGL), donc cet appel
    // suffit quel que soit l'objet passe.
    lv_obj_update_layout(obj);
    const lv_coord_t y = lv_obj_get_y(obj);
    lv_obj_set_y(obj, y + dy);
    fade_in(obj, delay, 260);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, y + dy, y);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_duration(&a, 320);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, [](void *target, int32_t v) {
        lv_obj_set_y(static_cast<lv_obj_t *>(target), v);
    });
    lv_anim_start(&a);
}

void animate_arc_value(lv_obj_t *arc, int target, int delay) {
    lv_arc_set_value(arc, 0);
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, arc);
    lv_anim_set_values(&a, 0, target);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_duration(&a, 680);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_set_exec_cb(&a, [](void *target_obj, int32_t v) {
        lv_arc_set_value(static_cast<lv_obj_t *>(target_obj), v);
    });
    lv_anim_start(&a);
}

} // namespace trophy
