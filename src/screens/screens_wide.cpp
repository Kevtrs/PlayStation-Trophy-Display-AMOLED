#include "screens/screens_wide.hpp"

#include "assets/fonts/td_fonts.hpp"
#include "assets/board7_icons.hpp"
#include "assets/pocketpsn_logo.hpp"
#include "assets/ps_plus_badge.hpp"
#include "assets/profile_avatar.hpp"
#include "assets/stat_card_background.hpp"
#include "assets/stat_glow_dot.hpp"
#include "assets/stat_icons.hpp"
#include "assets/trophy_ring.hpp"
#include "assets/trophy_ring_full.hpp"
#include "assets/trophy_scene_wide.hpp"
#include "theme/theme.hpp"
#include "ui/layout_wide.hpp"
#include "ui/strings.hpp"
#include "widgets/widgets.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace trophy {
namespace {

lv_color_t trophy_color_wide(TrophyKind kind) {
    switch(kind) {
        case TrophyKind::Platinum: return colors.platinum;
        case TrophyKind::Gold: return colors.gold;
        case TrophyKind::Silver: return colors.silver;
        case TrophyKind::Bronze: return colors.bronze;
        case TrophyKind::Multiple: return colors.accent_cyan;
    }
    return colors.accent_cyan;
}

const char *trophy_focus_label_wide(TrophyKind kind) {
    switch(kind) {
        case TrophyKind::Platinum: return "Chef-d'oeuvre";
        case TrophyKind::Gold: return "Performance";
        case TrophyKind::Silver: return "Progression";
        case TrophyKind::Bronze: return "Collection";
        case TrophyKind::Multiple: return "Trophees";
    }
    return "Trophees";
}

lv_obj_t *wide_screen_root() {
    lv_obj_t *root = lv_obj_create(nullptr);
    lv_obj_set_size(root, WIDE_WIDTH, WIDE_HEIGHT);
    lv_obj_add_style(root, &style_screen, 0);
    lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
    return root;
}

// Trois grands anneaux flous en arriere-plan (meme technique que
// base_screen() sur l'ecran rond, deja eprouvee en production, voir
// screens.cpp) : sur ce grand ecran 800x480, le contenu central (halo +
// trophee/icone + valeur) ne couvre qu'une bande etroite -- sans
// decoration de fond, le reste paraissait plat et vide (retour utilisateur
// du 2026-07-28).
//
// Un degrade lineaire LVGL a ete essaye ici en premier (noir -> teinte ->
// noir sur toute la largeur) : ecarte, banding visible en bandes
// horizontales sur ce panneau 16 bits (RGB565, LV_DITHER_GRADIENT non
// configure dans lv_conf.h) -- capture le 2026-07-28. Anneaux fins a
// faible opacite : meme resultat de profondeur, aucun degrade donc aucun
// risque de banding. "accent" porte l'identite de l'ecran (couleur du
// trophee/violet stats/cyan dashboard) ; le cyan de marque sur l'anneau du
// milieu relie les 3 ecrans entre eux visuellement.
// Demi-anneau (haut uniquement, angles LVGL 180->360 = 9h -> 12h -> 3h) :
// make_glow_ring() dessine un cercle COMPLET, qui redescendait jusque dans
// la zone du texte (valeur/legende) en bas d'ecran et la traversait
// visuellement -- retour utilisateur du 2026-07-28 ("ca doit pas se
// chevaucher avec les calques"). Cantonne l'ambiance a la moitie haute,
// autour du trophee/icone, jamais derriere le texte.
lv_obj_t *make_glow_arc_top(lv_obj_t *parent, int size, lv_color_t color, int width, lv_opa_t opa) {
    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_set_size(arc, size, size);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 100);
    lv_arc_set_bg_angles(arc, 180, 360);
    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    return arc;
}

lv_opa_t wide_opa_pct(int pct) {
    return static_cast<lv_opa_t>(std::clamp(pct, 0, 100) * LV_OPA_COVER / 100);
}

lv_obj_t *wide_bg_rect(lv_obj_t *parent, int x, int y, int w, int h, lv_color_t fill, lv_opa_t fill_opa,
                       lv_color_t border, lv_opa_t border_opa, int border_width = 0, int radius = 0) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_remove_style_all(obj);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_color(obj, fill, 0);
    lv_obj_set_style_bg_opa(obj, fill_opa, 0);
    lv_obj_set_style_border_color(obj, border, 0);
    lv_obj_set_style_border_opa(obj, border_opa, 0);
    lv_obj_set_style_border_width(obj, border_width, 0);
    return obj;
}

void wide_bg_arc(lv_obj_t *root, int size, int x, int y, lv_color_t color, lv_opa_t opa, int width) {
    lv_obj_t *arc = lv_arc_create(root);
    lv_obj_remove_style_all(arc);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, x, y);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, 100);
    lv_arc_set_bg_angles(arc, 0, 360);
    lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
    lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(arc, true, LV_PART_MAIN);
    lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
}

void wide_bg_polyline(lv_obj_t *root, const lv_point_precise_t *points, uint32_t point_count, lv_color_t color, int opacity,
                      int width) {
    lv_point_precise_t *stored = new lv_point_precise_t[point_count];
    for(uint32_t i = 0; i < point_count; ++i) stored[i] = points[i];
    lv_obj_t *line = lv_line_create(root);
    lv_obj_remove_style_all(line);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_line_set_points(line, stored, point_count);
    lv_obj_set_style_line_color(line, color, 0);
    lv_obj_set_style_line_opa(line, wide_opa_pct(opacity), 0);
    lv_obj_set_style_line_width(line, width, 0);
    lv_obj_set_style_line_rounded(line, true, 0);
    lv_obj_add_event_cb(
        line,
        [](lv_event_t *e) {
            delete[] static_cast<lv_point_precise_t *>(lv_event_get_user_data(e));
        },
        LV_EVENT_DELETE, stored);
}

template <size_t N>
void wide_bg_polyline(lv_obj_t *root, const lv_point_precise_t (&points)[N], lv_color_t color, int opacity, int width) {
    wide_bg_polyline(root, points, static_cast<uint32_t>(N), color, opacity, width);
}

void wide_bg_button_mark(lv_obj_t *root, char kind, int x, int y, int size, lv_color_t color, int opacity, int width = 2) {
    switch(kind) {
        case 's':
            wide_bg_rect(root, x, y, size, size, lv_color_hex(0x000000), LV_OPA_TRANSP, color, wide_opa_pct(opacity),
                         width, std::max(4, size / 10));
            break;
        case 'o':
            wide_bg_arc(root, size, x, y, color, wide_opa_pct(opacity), width);
            break;
        case 'x': {
            const int inset = std::max(1, size / 8);
            const lv_point_precise_t slash_a[] = {
                {static_cast<lv_value_precise_t>(x + inset), static_cast<lv_value_precise_t>(y + inset)},
                {static_cast<lv_value_precise_t>(x + size - inset), static_cast<lv_value_precise_t>(y + size - inset)},
            };
            const lv_point_precise_t slash_b[] = {
                {static_cast<lv_value_precise_t>(x + size - inset), static_cast<lv_value_precise_t>(y + inset)},
                {static_cast<lv_value_precise_t>(x + inset), static_cast<lv_value_precise_t>(y + size - inset)},
            };
            wide_bg_polyline(root, slash_a, color, opacity, width);
            wide_bg_polyline(root, slash_b, color, opacity, width);
            break;
        }
        case 't': {
            const int half = size / 2;
            const lv_point_precise_t tri[] = {
                {static_cast<lv_value_precise_t>(x + half), static_cast<lv_value_precise_t>(y)},
                {static_cast<lv_value_precise_t>(x + size), static_cast<lv_value_precise_t>(y + size)},
                {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y + size)},
                {static_cast<lv_value_precise_t>(x + half), static_cast<lv_value_precise_t>(y)},
            };
            wide_bg_polyline(root, tri, color, opacity, width);
            break;
        }
    }
}

void wide_bg_neon_button(lv_obj_t *root, char kind, int x, int y, int size, lv_color_t color, int opacity, int width = 2) {
    wide_bg_button_mark(root, kind, x - 2, y - 2, size + 4, color, std::max(3, opacity / 2), width + 2);
    wide_bg_button_mark(root, kind, x, y, size, color, opacity, width);
}

void ambient_rings(lv_obj_t *root, lv_color_t accent) {
    const lv_color_t ps_blue = lv_color_hex(0x0A55C8);
    wide_bg_rect(root, 0, 0, WIDE_WIDTH, WIDE_HEIGHT, lv_color_hex(0x000612), LV_OPA_COVER, lv_color_hex(0x000612),
                 LV_OPA_TRANSP);

    const char kinds[] = {'s', 'o', 'x', 't'};
    for(int row = 0; row < 5; ++row) {
        for(int col = 0; col < 8; ++col) {
            const int x = -18 + col * 108 + (row % 2) * 42;
            const int y = 24 + row * 86;
            const int variant = (row * 3 + col * 5) % 4;
            const int size = 18 + ((row + col) % 3) * 8;
            wide_bg_neon_button(root, kinds[variant], x, y, size, ps_blue, 14 - ((row + col) % 3) * 2, 2);
        }
    }
    for(int row = 0; row < 4; ++row) {
        for(int col = 0; col < 7; ++col) {
            const int x = 34 + col * 114 + (row % 2) * 22;
            const int y = 66 + row * 96;
            const int variant = (row + col + 2) % 4;
            wide_bg_neon_button(root, kinds[variant], x, y, 16 + ((row + col) % 2) * 8, ps_blue, 10, 2);
        }
    }

    lv_obj_t *far = make_glow_arc_top(root, 760, ps_blue, 1, LV_OPA_10);
    lv_obj_set_pos(far, wide_center_x(760), WIDE_HERO_HALO_Y - 380);
    lv_obj_t *outer = make_glow_arc_top(root, 620, ps_blue, 1, LV_OPA_10);
    lv_obj_set_pos(outer, wide_center_x(620), WIDE_HERO_HALO_Y - 310);
    lv_obj_t *inner = make_glow_arc_top(root, 480, accent, 1, LV_OPA_10);
    lv_obj_set_pos(inner, wide_center_x(480), WIDE_HERO_HALO_Y - 240);
}

// Anneau double-ton segmente (couleur_droite a 3h, couleur_gauche a 9h,
// melange 50/50 en haut/bas) : compose de plusieurs petits arcs a couleur
// PLATE (jamais de degrade calcule au runtime -- meme prudence que pour le
// fond, voir ambient_wash abandonne plus haut dans l'historique) avec un
// petit espace entre chaque segment pour l'aspect "gradue"/instrument
// technique. Inspire d'une maquette generee par IA (retour utilisateur du
// 2026-07-28) : remplace les deux anneaux fins unis (halo/halo2) des
// ecrans Trophees/Statistiques/Dashboard.
lv_obj_t *make_dual_ring(lv_obj_t *parent, int size, lv_color_t color_right, lv_color_t color_left, int width,
                          lv_opa_t opa, int segments = 28) {
    lv_obj_t *group = lv_obj_create(parent);
    lv_obj_remove_style_all(group);
    lv_obj_set_size(group, size, size);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(group, LV_OBJ_FLAG_CLICKABLE);

    const int gap_deg = 3;
    const int span = 360 / segments;
    for (int i = 0; i < segments; ++i) {
        const int start = i * span;
        const int end = start + span - gap_deg;
        const int mid = start + span / 2;
        const double rad = mid * 3.14159265358979323846 / 180.0;
        // 255 = couleur_droite pure (3h, cos=1), 0 = couleur_gauche pure
        // (9h, cos=-1), 50/50 en haut/bas (cos=0).
        const uint8_t factor = static_cast<uint8_t>((1.0 + std::cos(rad)) / 2.0 * 255.0);
        const lv_color_t seg_color = lv_color_mix(color_right, color_left, factor);

        lv_obj_t *arc = lv_arc_create(group);
        lv_obj_remove_style_all(arc);
        lv_obj_set_size(arc, size, size);
        lv_arc_set_range(arc, 0, 100);
        lv_arc_set_value(arc, 100);
        lv_arc_set_bg_angles(arc, start, end);
        lv_obj_set_style_arc_color(arc, seg_color, LV_PART_MAIN);
        lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
        lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
        lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_center(arc);
    }
    return group;
}

// Petite decoration "slider" (trait + encoche) de part et d'autre du halo,
// meme inspiration que le double anneau ci-dessus -- purement decoratif,
// aucune fonction (ce board n'a pas de tactile).
void add_slider_deco(lv_obj_t *root, int x, int y, lv_color_t color) {
    lv_obj_t *line = lv_obj_create(root);
    lv_obj_remove_style_all(line);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(line, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(line, 2, 26);
    lv_obj_set_style_bg_color(line, color, 0);
    lv_obj_set_style_bg_opa(line, LV_OPA_50, 0);
    lv_obj_set_pos(line, x, y);

    lv_obj_t *notch = lv_obj_create(root);
    lv_obj_remove_style_all(notch);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(notch, 8, 8);
    lv_obj_set_style_radius(notch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(notch, color, 0);
    lv_obj_set_style_bg_opa(notch, LV_OPA_60, 0);
    lv_obj_set_pos(notch, x - 3, y + 12);
}

// Points de pagination "glow" (asset genere par IA recolorable, voir
// assets/stat_glow_dot.cpp) pour la carte des ecrans Statistiques -- local
// a ce fichier plutot que de toucher au widget partage make_page_indicator
// (utilise aussi par les ecrans Trophees, restes en points plats). Choisi
// par l'utilisateur le 2026-07-28 ("et les autres assets aller tous").
lv_obj_t *make_glow_dots(lv_obj_t *parent, int active, int count) {
    constexpr int kSmall = 14;
    constexpr int kLarge = 22;
    constexpr int kGap = 10;
    const int total_w = count * kSmall + (count - 1) * kGap + (kLarge - kSmall);
    lv_obj_t *wrap = lv_obj_create(parent);
    lv_obj_remove_style_all(wrap);
    lv_obj_clear_flag(wrap, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(wrap, total_w, kLarge);
    int x = 0;
    for (int i = 0; i < count; ++i) {
        const bool is_active = (i == active);
        const int size = is_active ? kLarge : kSmall;
        lv_obj_t *dot = lv_image_create(wrap);
        lv_image_set_src(dot, stat_glow_dot_descriptor());
        lv_obj_set_size(dot, size, size);
        lv_obj_set_pos(dot, x, (kLarge - size) / 2);
        lv_obj_set_style_image_recolor(dot, is_active ? colors.accent_cyan : colors.accent_violet, 0);
        lv_obj_set_style_image_recolor_opa(dot, is_active ? LV_OPA_COVER : LV_OPA_50, 0);
        x += size + kGap;
    }
    return wrap;
}

lv_obj_t *card(lv_obj_t *parent, int x, int y, int w, int h, int radius = 18, lv_opa_t opa = LV_OPA_80) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_add_style(obj, &style_surface, 0);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(obj, w, h);
    lv_obj_set_pos(obj, x, y);
    lv_obj_set_style_radius(obj, radius, 0);
    lv_obj_set_style_bg_opa(obj, opa, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
    return obj;
}

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color, int x, int y, int w,
                 lv_text_align_t align = LV_TEXT_ALIGN_CENTER) {
    lv_obj_t *obj = lv_label_create(parent);
    lv_label_set_text(obj, text);
    lv_obj_add_style(obj, &style_label, 0);
    lv_obj_set_style_text_font(obj, font, 0);
    lv_obj_set_style_text_color(obj, color, 0);
    lv_obj_set_style_text_align(obj, align, 0);
    lv_obj_set_width(obj, w);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_DOT);
    lv_obj_set_pos(obj, x, y);
    return obj;
}

lv_obj_t *wide_progress_bar(lv_obj_t *parent, int x, int y, int w, int h, int value, lv_color_t accent) {
    lv_obj_t *bar = lv_bar_create(parent);
    lv_obj_remove_style_all(bar);
    lv_obj_set_size(bar, w, h);
    lv_obj_set_pos(bar, x, y);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, std::clamp(value, 0, 100), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, colors.surface_secondary, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, h / 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, accent, LV_PART_INDICATOR);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_INDICATOR);
    lv_obj_set_style_radius(bar, h / 2, LV_PART_INDICATOR);
    lv_obj_clear_flag(bar, LV_OBJ_FLAG_CLICKABLE);
    return bar;
}

lv_obj_t *wide_icon_circle(lv_obj_t *parent, const char *icon, int cx, int cy, int box, int glyph, lv_color_t color) {
    lv_obj_t *holder = lv_obj_create(parent);
    lv_obj_remove_style_all(holder);
    lv_obj_clear_flag(holder, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(holder, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(holder, box, box);
    lv_obj_set_pos(holder, cx - box / 2, cy - box / 2);
    lv_obj_set_style_radius(holder, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(holder, colors.surface_primary, 0);
    lv_obj_set_style_bg_opa(holder, LV_OPA_50, 0);
    lv_obj_set_style_border_width(holder, 2, 0);
    lv_obj_set_style_border_color(holder, color, 0);
    lv_obj_set_style_border_opa(holder, LV_OPA_80, 0);

    if(const lv_image_dsc_t *dsc = board7_icon_descriptor(icon, glyph)) {
        const int asset_size = glyph <= 24 ? 24 : (glyph <= 32 ? 32 : 48);
        lv_obj_t *inner = lv_image_create(holder);
        lv_image_set_src(inner, dsc);
        lv_obj_set_size(inner, asset_size, asset_size);
        lv_image_set_inner_align(inner, LV_IMAGE_ALIGN_CONTAIN);
        lv_obj_set_style_image_recolor(inner, color, 0);
        lv_obj_set_style_image_recolor_opa(inner, LV_OPA_COVER, 0);
        lv_obj_center(inner);
    } else {
        lv_obj_t *inner = make_icon_box(holder, icon, glyph + 10, glyph, color);
        lv_obj_center(inner);
    }
    return holder;
}

void add_profile_avatar_content(lv_obj_t *avatar, const ProfileData &p, int image_size, int icon_box_size,
                                int icon_size, lv_color_t fallback_color) {
    lv_obj_set_style_clip_corner(avatar, true, 0);
    const lv_image_dsc_t *avatar_dsc = assets::profile_avatar_descriptor(p.avatar_file_name);
    if(avatar_dsc) {
        lv_obj_t *img = lv_image_create(avatar);
        lv_image_set_src(img, avatar_dsc);
        lv_image_set_inner_align(img, LV_IMAGE_ALIGN_COVER);
        lv_obj_set_size(img, image_size, image_size);
        lv_obj_center(img);
        return;
    }

    lv_obj_t *avatar_icon = make_icon_box(avatar, "circle-user-round", icon_box_size, icon_size, fallback_color);
    lv_obj_center(avatar_icon);
}

void dashboard_status_badge(lv_obj_t *root, const char *text) {
    lv_obj_t *badge = wide_bg_rect(root, 650, 67, 104, 28, lv_color_hex(0x03120F), wide_opa_pct(92),
                                   colors.success, LV_OPA_80, 1, 14);
    lv_obj_t *glyph = make_icon_box(badge, "circle-dot", 18, 10, colors.success);
    lv_obj_set_pos(glyph, 9, 5);
    label(badge, text, &td_font_12, colors.text_primary, 30, 5, 66, LV_TEXT_ALIGN_LEFT);
}

void dashboard_country_chip(lv_obj_t *root, int x, int y, const std::string &country) {
    lv_obj_t *chip = wide_bg_rect(root, x, y, 56, 28, lv_color_hex(0x061A2F), wide_opa_pct(90),
                                  colors.accent_blue, LV_OPA_90, 1, 14);
    constexpr int flag_w = 18;
    constexpr int flag_h = 12;
    constexpr int label_w = 18;
    constexpr int gap = 5;
    constexpr int group_w = flag_w + gap + label_w;
    const int flag_x = (56 - group_w) / 2;
    const int flag_y = (28 - flag_h) / 2;
    wide_bg_rect(chip, flag_x, flag_y, flag_w, flag_h, colors.text_primary, LV_OPA_COVER, colors.text_primary,
                 LV_OPA_TRANSP, 0, 2);
    wide_bg_rect(chip, flag_x, flag_y, 6, flag_h, lv_color_hex(0x2446A8), LV_OPA_COVER, lv_color_hex(0x2446A8),
                 LV_OPA_TRANSP);
    wide_bg_rect(chip, flag_x + 12, flag_y, 6, flag_h, lv_color_hex(0xE63B45), LV_OPA_COVER, lv_color_hex(0xE63B45),
                 LV_OPA_TRANSP);
    label(chip, country.empty() ? "--" : country.c_str(), &td_font_12, colors.text_primary, flag_x + flag_w + gap, 7,
          label_w, LV_TEXT_ALIGN_CENTER);
}

void dashboard_ps_plus_chip(lv_obj_t *root, int x, int y) {
    wide_bg_rect(root, x - 2, y - 2, 44, 44, colors.gold, wide_opa_pct(8), colors.gold, LV_OPA_TRANSP, 0, 22);
    lv_obj_t *img = lv_image_create(root);
    lv_image_set_src(img, assets::ps_plus_badge_descriptor());
    lv_obj_set_size(img, 40, 40);
    lv_obj_set_pos(img, x, y);
    lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
}

void dashboard_glow_pill(lv_obj_t *root, int x, int y, int w, int h, lv_color_t accent) {
    wide_bg_rect(root, x, y, w, h, accent, wide_opa_pct(7), accent, LV_OPA_TRANSP, 0, h / 2);
}

void dashboard_xp_ring(lv_obj_t *parent, int cx, int cy, int size, int percent) {
    lv_obj_t *bg = lv_arc_create(parent);
    lv_obj_remove_style_all(bg);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(bg, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(bg, size, size);
    lv_obj_set_pos(bg, cx - size / 2, cy - size / 2);
    lv_arc_set_range(bg, 0, 100);
    lv_arc_set_value(bg, 100);
    lv_arc_set_bg_angles(bg, 130, 50);
    lv_obj_set_style_arc_color(bg, lv_color_hex(0x113052), LV_PART_MAIN);
    lv_obj_set_style_arc_width(bg, 16, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(bg, false, LV_PART_MAIN);
    lv_obj_set_style_arc_width(bg, 0, LV_PART_INDICATOR);

    lv_obj_t *arc = lv_arc_create(parent);
    lv_obj_remove_style_all(arc);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(arc, size, size);
    lv_obj_set_pos(arc, cx - size / 2, cy - size / 2);
    lv_arc_set_range(arc, 0, 100);
    lv_arc_set_value(arc, std::clamp(percent, 0, 100));
    lv_arc_set_bg_angles(arc, 130, 50);
    lv_obj_set_style_arc_width(arc, 0, LV_PART_MAIN);
    lv_obj_set_style_arc_color(arc, colors.accent_cyan, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

    lv_obj_t *tick = lv_arc_create(parent);
    lv_obj_remove_style_all(tick);
    lv_obj_clear_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tick, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(tick, size, size);
    lv_obj_set_pos(tick, cx - size / 2, cy - size / 2);
    lv_arc_set_range(tick, 0, 100);
    lv_arc_set_value(tick, 100);
    lv_arc_set_bg_angles(tick, 137, 142);
    lv_obj_set_style_arc_width(tick, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_color(tick, colors.text_primary, LV_PART_MAIN);
    lv_obj_set_style_arc_width(tick, 0, LV_PART_INDICATOR);
}

void dashboard_metric_card(lv_obj_t *root, int x, int y, int w, const char *value, const char *caption,
                           lv_color_t accent) {
    lv_obj_t *metric = card(root, x, y, w, 80, 12, LV_OPA_80);
    lv_obj_set_style_border_color(metric, accent, 0);
    lv_obj_set_style_border_opa(metric, wide_opa_pct(48), 0);
    label(metric, value, std::strlen(value) > 7 ? &td_font_20 : &td_font_22, colors.text_primary, 8, 21, w - 16,
          LV_TEXT_ALIGN_CENTER);
    label(metric, caption, &td_font_12, colors.text_secondary, 8, 50, w - 16, LV_TEXT_ALIGN_CENTER);
    wide_progress_bar(metric, 22, 70, w - 44, 4, 100, accent);
}

int trophy_percent_of_total_wide(int value, int total) {
    if(total <= 0 || value <= 0) return 0;
    return std::max(1, static_cast<int>(std::round((static_cast<double>(value) * 100.0) / total)));
}

std::string trophy_percent_text_wide(int value, int total) {
    char buf[24];
    const double pct = total > 0 ? (static_cast<double>(value) * 100.0) / total : 0.0;
    std::snprintf(buf, sizeof(buf), "%.1f %%", pct);
    return std::string(buf);
}

std::string trophy_share_sentence_wide(int value, int total) {
    return trophy_percent_text_wide(value, total) + " de " + format_number(total);
}

std::string dashboard_username_text(const std::string &username) {
    if(username.size() <= 10) return username;
    return username.substr(0, 7) + "...";
}

void wide_title_block(lv_obj_t *root, const char *title, const char *subtitle, lv_color_t accent = colors.accent_cyan) {
    label(root, title, &td_font_28, colors.text_primary, 48, 64, 260, LV_TEXT_ALIGN_LEFT);
    if(subtitle && subtitle[0] != '\0') {
        label(root, subtitle, &td_font_16, colors.text_secondary, 50, 100, 300, LV_TEXT_ALIGN_LEFT);
    }
    wide_bg_rect(root, 50, 125, 198, 2, accent, LV_OPA_COVER, accent, LV_OPA_TRANSP, 0, 1);
}

void wide_card_accent(lv_obj_t *root, int x, int y, int w, int h, lv_color_t accent, int inset = 34) {
    wide_bg_rect(root, x + inset, y + 14, w - inset * 2, 1, accent, wide_opa_pct(72), accent, LV_OPA_TRANSP);
    wide_bg_rect(root, x + inset, y + h - 16, w - inset * 2, 1, accent, wide_opa_pct(18), accent, LV_OPA_TRANSP);
}

void wide_card_top_accent(lv_obj_t *root, int x, int y, int w, lv_color_t accent, int inset = 34) {
    wide_bg_rect(root, x + inset, y + 14, w - inset * 2, 1, accent, wide_opa_pct(72), accent, LV_OPA_TRANSP);
}

int sync_progress_wide(SyncState state, int progress) {
    if(progress > 0) return std::clamp(progress, 0, 100);
    switch(state) {
        case SyncState::Connecting: return 24;
        case SyncState::Fetching: return 64;
        case SyncState::Processing: return 82;
        case SyncState::Done: return 100;
        case SyncState::Error: return 100;
        case SyncState::Idle: return 0;
    }
    return 0;
}

const char *sync_title_wide(SyncState state, AppLanguage lang) {
    if(state == SyncState::Done) return tr(lang, Str::kSyncedTitle);
    if(state == SyncState::Error) return tr(lang, Str::kErrorBadge);
    return tr(lang, Str::kSyncingTitle);
}

const char *sync_subtitle_wide(SyncState state, AppLanguage lang) {
    switch(state) {
        case SyncState::Connecting: return tr(lang, Str::kSyncConnectingTitle);
        case SyncState::Fetching: return tr(lang, Str::kSyncFetching);
        case SyncState::Processing: return tr(lang, Str::kSyncProcessingTitle);
        case SyncState::Done: return tr(lang, Str::kDataUpToDate);
        case SyncState::Error: return tr(lang, Str::kErrSyncFailedTitle);
        case SyncState::Idle: return tr(lang, Str::kSyncIdle);
    }
    return tr(lang, Str::kSyncIdle);
}

void wide_state_button(lv_obj_t *root, int x, int y, lv_color_t accent, bool error) {
    wide_bg_rect(root, x, y, 168, 40, error ? lv_color_hex(0x321106) : lv_color_hex(0x042937), wide_opa_pct(94),
                 accent, LV_OPA_COVER, 1, 21);
    label(root, "Réessayer", &td_font_14, colors.text_primary, x + 40, y + 11, 88, LV_TEXT_ALIGN_CENTER);
}

struct SettingsRowTouchContext {
    lv_obj_t *row;
    const char *name;
    WideSettingAction action;
    WideSettingHandler handler;
    void *owner;
};

void settings_row_touch_event(lv_event_t *e) {
    auto *ctx = static_cast<SettingsRowTouchContext *>(lv_event_get_user_data(e));
    if(ctx == nullptr || ctx->row == nullptr) return;

    const lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_PRESSED) {
        lv_obj_set_style_bg_opa(ctx->row, wide_opa_pct(96), 0);
        lv_obj_set_style_border_opa(ctx->row, LV_OPA_COVER, 0);
        return;
    }
    if(code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
        lv_obj_set_style_bg_opa(ctx->row, LV_OPA_80, 0);
        lv_obj_set_style_border_opa(ctx->row, wide_opa_pct(78), 0);
        return;
    }
    if(code == LV_EVENT_CLICKED) {
        // Les lignes informatives (profil) portent kNone : elles gardent le
        // retour visuel a l'appui, mais ne declenchent rien.
        if(ctx->action != WideSettingAction::kNone && ctx->handler != nullptr) {
            ctx->handler(ctx->action, ctx->owner);
        }
        return;
    }
    if(code == LV_EVENT_DELETE) {
        delete ctx;
    }
}

void settings_row_wide(lv_obj_t *root, int y, const char *icon, const char *name, const char *value, lv_color_t accent,
                       WideSettingAction action, WideSettingHandler handler, void *owner, int bar_pct = -1,
                       bool toggle = false) {
    lv_obj_t *row = card(root, 77, y, 646, 44, 22, LV_OPA_80);
    lv_obj_set_style_border_color(row, accent, 0);
    lv_obj_set_style_border_opa(row, wide_opa_pct(78), 0);
    lv_obj_set_style_border_width(row, 1, 0);
    wide_icon_circle(root, icon, 108, y + 22, 30, 18, accent);
    label(root, name, &td_font_16, colors.text_primary, 144, y + 13, 260, LV_TEXT_ALIGN_LEFT);

    if(bar_pct >= 0) {
        wide_progress_bar(root, 506, y + 20, 86, 5, bar_pct, accent);
        label(root, value, &td_font_12, colors.text_secondary, 600, y + 14, 32, LV_TEXT_ALIGN_RIGHT);
    } else if(toggle) {
        wide_bg_rect(root, 640, y + 12, 48, 20, lv_color_hex(0x03120F), wide_opa_pct(95), accent, LV_OPA_80, 1, 10);
        wide_bg_rect(root, 672, y + 17, 10, 10, accent, LV_OPA_COVER, accent, LV_OPA_TRANSP, 0, LV_RADIUS_CIRCLE);
        label(root, value, &td_font_12, colors.text_secondary, 600, y + 14, 32, LV_TEXT_ALIGN_RIGHT);
    } else {
        label(root, value, &td_font_12, colors.text_secondary, 604, y + 14, 86, LV_TEXT_ALIGN_RIGHT);
    }

    lv_obj_t *hit = lv_obj_create(root);
    lv_obj_remove_style_all(hit);
    lv_obj_clear_flag(hit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(hit, 646, 48);
    lv_obj_set_pos(hit, 77, y - 2);
    lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);
    auto *ctx = new SettingsRowTouchContext{row, name, action, handler, owner};
    lv_obj_add_event_cb(hit, settings_row_touch_event, LV_EVENT_ALL, ctx);
}

void trophy_overview_row(lv_obj_t *root, TrophyKind kind, int value, int total, int y, AppLanguage lang) {
    const lv_color_t accent = trophy_color_wide(kind);
    const int share = trophy_percent_of_total_wide(value, total);
    const std::string value_text = format_number(value);
    const std::string pct_text = trophy_percent_text_wide(value, total);

    lv_obj_t *row = card(root, 72, y, 656, 58, 14, LV_OPA_80);
    lv_obj_set_style_border_color(row, accent, 0);
    lv_obj_set_style_border_opa(row, kind == TrophyKind::Platinum ? LV_OPA_80 : wide_opa_pct(62), 0);
    wide_bg_rect(row, 1, 1, 654, 1, accent, wide_opa_pct(18), accent, LV_OPA_TRANSP);

    lv_obj_t *icon = make_trophy_icon(row, kind, 44);
    lv_obj_set_pos(icon, 20, 7);
    label(row, trophy_kind_label(kind, lang), &td_font_20, colors.text_primary, 86, 13, 120, LV_TEXT_ALIGN_LEFT);
    label(row, pct_text.c_str(), &td_font_10, colors.text_secondary, 86, 36, 64, LV_TEXT_ALIGN_LEFT);
    wide_progress_bar(row, 262, 42, 286, 4, share, accent);
    label(row, value_text.c_str(), &td_font_20, kind == TrophyKind::Bronze ? accent : colors.text_primary, 570, 15,
          62, LV_TEXT_ALIGN_RIGHT);
}

} // namespace

lv_obj_t *build_dashboard_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    const int next_level = p.level + 1;
    const std::string level_text = format_number(p.level);
    const std::string remaining_text = p.level_remaining_points > 0 ? format_number(p.level_remaining_points) : "-";
    const std::string progress_text = std::to_string(std::clamp(p.progress, 0, 100)) + "%";
    const std::string next_level_text = std::to_string(next_level);
    const char *world_rank = p.world_rank.empty() ? "-" : p.world_rank.c_str();
    const char *national_rank = p.national_rank.empty() ? "-" : p.national_rank.c_str();
    const char *trophy_points = p.trophy_points.empty() ? "-" : p.trophy_points.c_str();

    const int profile_x = 36;
    const int profile_y = 54;
    const int profile_w = 264;
    const int profile_h = 334;
    const int xp_x = 332;
    const int xp_y = 54;
    const int xp_w = 432;
    const int xp_h = 264;

    lv_obj_t *profile_card = card(root, profile_x, profile_y, profile_w, profile_h, 16, LV_OPA_80);
    lv_obj_set_style_border_color(profile_card, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(profile_card, wide_opa_pct(55), 0);
    wide_bg_rect(root, 58, 70, 220, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan, LV_OPA_TRANSP);

    lv_obj_t *avatar = card(root, 60, 89, 86, 86, LV_RADIUS_CIRCLE, LV_OPA_70);
    lv_obj_set_style_border_color(avatar, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(avatar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(avatar, 2, 0);
    add_profile_avatar_content(avatar, p, 86, 70, 48, colors.accent_cyan);

    const std::string username_text = dashboard_username_text(p.username);
    lv_obj_t *name = label(root, username_text.c_str(), username_text.size() > 9 ? &td_font_20 : &td_font_24,
                           colors.text_primary, 164, 106, 118, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_height(name, 28);
    lv_label_set_long_mode(name, LV_LABEL_LONG_DOT);
    label(root, "Profil actif", &td_font_12, colors.text_secondary, 166, 138, 100, LV_TEXT_ALIGN_LEFT);
    dashboard_country_chip(root, 164, 154, p.country);
    if(p.has_ps_plus) dashboard_ps_plus_chip(root, 236, 150);

    lv_obj_t *level_card = card(root, 57, 207, 222, 160, 14, LV_OPA_80);
    lv_obj_set_style_bg_color(level_card, lv_color_hex(0x07111A), 0);
    lv_obj_set_style_bg_opa(level_card, wide_opa_pct(94), 0);
    lv_obj_set_style_border_color(level_card, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(level_card, wide_opa_pct(32), 0);
    dashboard_glow_pill(root, 72, 254, 172, 76, colors.accent_cyan);
    wide_bg_rect(root, 72, 220, 96, 26, lv_color_hex(0x001A20), LV_OPA_COVER, colors.accent_cyan, LV_OPA_COVER, 1,
                 13);
    label(root, "NIVEAU PSN", &td_font_12, colors.accent_cyan, 72, 226, 96, LV_TEXT_ALIGN_CENTER);
    label(root, level_text.c_str(), &td_font_96, colors.text_primary, 72, 253, 172, LV_TEXT_ALIGN_CENTER);
    label(root, remaining_text.c_str(), &td_font_12, colors.text_secondary, 72, 341, 68, LV_TEXT_ALIGN_LEFT);
    label(root, "pts restants", &td_font_12, colors.text_secondary, 142, 341, 82, LV_TEXT_ALIGN_LEFT);
    label(root, progress_text.c_str(), &td_font_12, colors.accent_cyan, 230, 341, 36, LV_TEXT_ALIGN_RIGHT);
    wide_progress_bar(root, 72, 358, 188, 8, p.progress, colors.accent_cyan);
    label(root, next_level_text.c_str(), &td_font_12, colors.text_secondary, 224, 228, 34, LV_TEXT_ALIGN_RIGHT);
    label(root, "suivant", &td_font_10, colors.text_muted, 218, 244, 44, LV_TEXT_ALIGN_RIGHT);

    lv_obj_t *xp_card = card(root, xp_x, xp_y, xp_w, xp_h, 18, LV_OPA_70);
    lv_obj_set_style_border_color(xp_card, colors.accent_blue, 0);
    lv_obj_set_style_border_opa(xp_card, LV_OPA_40, 0);
    dashboard_status_badge(root, p.offline ? tr(lang, Str::kOffline) : p.updated.c_str());
    dashboard_xp_ring(root, 548, 186, 216, p.progress);

    char center_pct[16];
    std::snprintf(center_pct, sizeof(center_pct), "%d%%", std::clamp(p.progress, 0, 100));
    char center_next[48];
    std::snprintf(center_next, sizeof(center_next), "vers niveau %d", next_level);
    label(root, center_pct, &td_font_24, colors.text_primary, 506, 148, 84, LV_TEXT_ALIGN_CENTER);
    label(root, center_next, &td_font_16, colors.text_secondary, 484, 181, 132, LV_TEXT_ALIGN_CENTER);
    label(root, "progression PSN", &td_font_12, colors.accent_cyan, 491, 214, 116, LV_TEXT_ALIGN_CENTER);

    dashboard_metric_card(root, 332, 346, 132, world_rank, "Monde", colors.accent_violet);
    dashboard_metric_card(root, 476, 346, 132, national_rank, "France", lv_color_hex(0x52C6FF));
    dashboard_metric_card(root, 620, 346, 144, trophy_points, "Points", colors.gold);

    return root;
}

lv_obj_t *build_trophies_overview_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);
    label(root, tr(lang, Str::kTrophiesTitle), &td_font_28, colors.text_primary, 48, 64, 260, LV_TEXT_ALIGN_LEFT);
    label(root, lang == AppLanguage::kFrench ? "Répartition globale" : "Global split", &td_font_16,
          colors.text_secondary, 50, 100, 260, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(root, 50, 125, 198, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan, LV_OPA_TRANSP, 0, 1);
    dashboard_status_badge(root, p.updated.c_str());

    lv_obj_t *total = wide_bg_rect(root, 580, 105, 148, 32, lv_color_hex(0x07111A), wide_opa_pct(78),
                                   colors.accent_cyan, wide_opa_pct(28), 1, 16);
    label(total, "Total", &td_font_10, colors.text_secondary, 20, 8, 42, LV_TEXT_ALIGN_LEFT);
    const std::string total_text = format_number(p.total);
    label(total, total_text.c_str(), &td_font_14, colors.text_primary, 60, 4, 66, LV_TEXT_ALIGN_RIGHT);

    trophy_overview_row(root, TrophyKind::Platinum, p.platinum, p.total, 148, lang);
    trophy_overview_row(root, TrophyKind::Gold, p.gold, p.total, 222, lang);
    trophy_overview_row(root, TrophyKind::Silver, p.silver, p.total, 296, lang);
    trophy_overview_row(root, TrophyKind::Bronze, p.bronze, p.total, 370, lang);

    return root;
}

lv_obj_t *build_trophy_screen_wide(TrophyKind kind, int value, int total, int sub_index, int sub_count,
                                   AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = trophy_color_wide(kind);
    const int share = trophy_percent_of_total_wide(value, total);
    const std::string value_text = format_number(value);
    const std::string share_text = trophy_share_sentence_wide(value, total);
    ambient_rings(root, accent);

    label(root, trophy_kind_label(kind, lang), &td_font_28, colors.text_primary, 48, 56, 220, LV_TEXT_ALIGN_LEFT);
    label(root, trophy_focus_label_wide(kind), &td_font_16, colors.text_secondary, 50, 90, 220, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(root, 50, 116, 54, 2, accent, LV_OPA_COVER, accent, LV_OPA_TRANSP);

    constexpr int kCardX = 112;
    constexpr int kCardY = 116;
    constexpr int kCardW = 576;
    constexpr int kCardH = 270;
    lv_obj_t *main_card = card(root, kCardX, kCardY, kCardW, kCardH, 18, LV_OPA_80);
    lv_obj_set_style_border_color(main_card, accent, 0);
    lv_obj_set_style_border_opa(main_card, LV_OPA_60, 0);

    wide_bg_rect(root, kCardX + 44, kCardY + 2, kCardW - 88, 1, accent, wide_opa_pct(18), accent, LV_OPA_TRANSP);

    constexpr int kTrophyCenterX = 266;
    constexpr int kTrophyCenterY = 252;
    lv_obj_t *halo = make_dual_ring(root, 210, accent, colors.accent_violet, 3, LV_OPA_70);
    lv_obj_set_pos(halo, kTrophyCenterX - 105, kTrophyCenterY - 105);
    lv_obj_t *halo2 = make_dual_ring(root, 182, accent, colors.accent_violet, 1, LV_OPA_40, 20);
    lv_obj_set_pos(halo2, kTrophyCenterX - 91, kTrophyCenterY - 91);

    lv_obj_t *glow = card(root, 174, 160, 184, 184, LV_RADIUS_CIRCLE, kind == TrophyKind::Platinum ? LV_OPA_20 : LV_OPA_10);
    lv_obj_set_style_bg_color(glow, accent, 0);
    lv_obj_set_style_border_width(glow, 0, 0);

    lv_obj_t *trophy = make_trophy_icon(root, kind, 128);
    lv_obj_set_pos(trophy, 200, 188);

    label(root, trophy_kind_label(kind, lang), &td_font_18, colors.text_secondary, 416, 164, 150, LV_TEXT_ALIGN_LEFT);
    label(root, value_text.c_str(), &td_font_48, accent, 416, 204, 190, LV_TEXT_ALIGN_LEFT);
    label(root, "trophées", &td_font_16, colors.text_secondary, 416, 270, 170, LV_TEXT_ALIGN_LEFT);
    wide_progress_bar(root, 416, 314, 208, 8, share, accent);
    label(root, share_text.c_str(), &td_font_16, colors.text_secondary, 416, 338, 190, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    return root;
}

lv_obj_t *build_credits_screen_wide(AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    label(root, tr(lang, Str::kAboutLabel), &td_font_28, colors.text_primary, 48, 64, 260, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(root, 50, 125, 198, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan, LV_OPA_TRANSP, 0, 1);

    constexpr int kCardX = 286;
    constexpr int kCardY = 118;
    constexpr int kCardW = 462;
    constexpr int kCardH = 280;
    lv_obj_t *panel = card(root, kCardX, kCardY, kCardW, kCardH, 16, LV_OPA_80);
    lv_obj_set_style_border_color(panel, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(panel, wide_opa_pct(48), 0);
    wide_bg_rect(panel, 46, 18, kCardW - 92, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan,
                 LV_OPA_TRANSP, 0, 1);

    label(panel, "Trophy Display", &td_font_28, colors.text_primary, 0, 54, kCardW, LV_TEXT_ALIGN_CENTER);
    label(panel, "Version 1.0", &td_font_12, colors.text_secondary, 0, 90, kCardW, LV_TEXT_ALIGN_CENTER);
    label(panel, lang == AppLanguage::kFrench ? "Données PlayStation fournies par" : "PlayStation data provided by",
          &td_font_12, colors.text_muted, 0, 126, kCardW, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *pocket = wide_bg_rect(panel, 116, 150, 230, 64, lv_color_hex(0x07111A), wide_opa_pct(92),
                                    colors.accent_cyan, wide_opa_pct(55), 1, 8);
    lv_obj_t *logo = lv_image_create(pocket);
    lv_image_set_src(logo, assets::pocketpsn_logo_descriptor());
    lv_image_set_scale(logo, 384);
    lv_obj_set_size(logo, 66, 35);
    lv_obj_set_pos(logo, 20, 15);
    label(pocket, "Pocket PSN", &td_font_18, colors.text_primary, 96, 14, 112, LV_TEXT_ALIGN_LEFT);
    label(pocket, "pocketpsn.com", &td_font_12, colors.accent_cyan, 96, 36, 112, LV_TEXT_ALIGN_LEFT);

    label(panel, lang == AppLanguage::kFrench ? "Conception et développement" : "Design and development",
          &td_font_10, colors.text_muted, 0, 232, kCardW, LV_TEXT_ALIGN_CENTER);
    label(panel, "Kevin Torres", &td_font_12, colors.text_primary, 0, 248, kCardW, LV_TEXT_ALIGN_CENTER);
    wide_bg_rect(panel, 46, 272, kCardW - 92, 1, colors.text_muted, LV_OPA_30, colors.text_muted, LV_OPA_TRANSP);

    return root;
}

void stat_overview_tile(lv_obj_t *root, int x, int y, const char *icon, lv_color_t accent, const char *value,
                         const char *caption) {
    constexpr int kW = 216;
    constexpr int kH = 108;
    lv_obj_t *tile = card(root, x, y, kW, kH, 14, LV_OPA_80);
    lv_obj_set_style_border_color(tile, accent, 0);
    lv_obj_set_style_border_opa(tile, LV_OPA_70, 0);
    lv_obj_set_style_border_width(tile, 1, 0);
    wide_bg_rect(tile, 24, 16, kW - 48, 1, accent, LV_OPA_30, accent, LV_OPA_TRANSP);
    wide_bg_rect(tile, 18, 24, 68, 68, accent, wide_opa_pct(7), accent, LV_OPA_TRANSP, 0, 34);
    wide_icon_circle(tile, icon, 54, 58, 52, 34, accent);

    constexpr int kTextX = 100;
    constexpr int kTextW = kW - kTextX - 12;
    const lv_font_t *value_font = std::strlen(value) >= 8 ? &td_font_20 : &td_font_24;
    label(tile, value, value_font, colors.text_primary, kTextX, 30, kTextW, LV_TEXT_ALIGN_LEFT);
    label(tile, caption, &td_font_14, colors.text_secondary, kTextX, 68, kTextW, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(tile, kTextX, 94, 46, 3, accent, LV_OPA_80, accent, LV_OPA_TRANSP, 0, 2);
}

lv_obj_t *build_statistics_overview_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_violet);

    label(root, "Statistiques", &td_font_28, colors.text_primary, 48, 64, 260, LV_TEXT_ALIGN_LEFT);
    label(root, lang == AppLanguage::kFrench ? "Vue rapide du profil" : "Profile quick view", &td_font_16,
          colors.text_secondary, 50, 100, 260, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(root, 50, 125, 198, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan, LV_OPA_TRANSP, 0, 1);
    make_status_badge(root, p.updated.c_str(), colors.success, 650, 67);

    char completion[16];
    std::snprintf(completion, sizeof(completion), "%d %%", p.completion);
    const std::string total = format_number(p.total);
    const std::string completed = format_number(p.games_completed);
    const std::string platinum = format_number(p.platinum);
    const char *rank = p.world_rank.empty() ? "-" : p.world_rank.c_str();
    const char *play_time = p.play_time.empty() ? "-" : p.play_time.c_str();

    stat_overview_tile(root, 48, 146, "trophy-pro", colors.accent_cyan, total.c_str(),
                       lang == AppLanguage::kFrench ? "trophées" : "trophies");
    stat_overview_tile(root, 292, 146, "gamepad-pro", colors.success, completed.c_str(),
                       lang == AppLanguage::kFrench ? "jeux terminés" : "games done");
    stat_overview_tile(root, 536, 146, "percent-pro", colors.accent_violet, completion,
                       lang == AppLanguage::kFrench ? "completion" : "completion");
    stat_overview_tile(root, 48, 288, "seal-check-pro", colors.platinum, platinum.c_str(),
                       lang == AppLanguage::kFrench ? "platine" : "platinum");
    stat_overview_tile(root, 292, 288, "chart-no-axes-combined", colors.gold, rank,
                       lang == AppLanguage::kFrench ? "rang mondial" : "world rank");
    stat_overview_tile(root, 536, 288, "timer-pro", colors.bronze, play_time,
                       lang == AppLanguage::kFrench ? "temps joué" : "play time");

    return root;
}

lv_obj_t *build_sync_screen_wide(SyncState state, int progress, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    const int pct = sync_progress_wide(state, progress);
    make_status_badge(root, state == SyncState::Done ? tr(lang, Str::kReady) : "en cours",
                      state == SyncState::Error ? colors.warning : colors.success, 650, 67,
                      state == SyncState::Done ? "badge-check" : "refresh-cw");
    dashboard_xp_ring(root, 400, 198, 220, pct);
    wide_icon_circle(root, state == SyncState::Done ? "badge-check" : "refresh-cw", 400, 198, 74, 42,
                     state == SyncState::Error ? colors.warning : colors.accent_cyan);
    label(root, sync_title_wide(state, lang), &td_font_28, colors.text_primary, 272, 320, 256,
          LV_TEXT_ALIGN_CENTER);
    label(root, sync_subtitle_wide(state, lang), &td_font_16, colors.text_secondary, 292, 356, 216,
          LV_TEXT_ALIGN_CENTER);
    wide_progress_bar(root, 326, 392, 148, 5, pct, state == SyncState::Error ? colors.warning : colors.accent_cyan);

    char pct_text[16];
    std::snprintf(pct_text, sizeof(pct_text), "%d %%", pct);
    label(root, pct_text, &td_font_12, state == SyncState::Error ? colors.warning : colors.accent_cyan, 378, 405, 44,
          LV_TEXT_ALIGN_CENTER);

    const char *steps[] = {"Connexion", "Profil", "Données"};
    const int active_step = pct >= 80 ? 2 : (pct >= 45 ? 1 : 0);
    for(int i = 0; i < 3; ++i) {
        const int x = 285 + i * 82;
        const lv_color_t c = i == active_step ? colors.accent_cyan : colors.text_muted;
        wide_bg_rect(root, x, 438, 5, 5, c, LV_OPA_COVER, c, LV_OPA_TRANSP, 0, LV_RADIUS_CIRCLE);
        label(root, steps[i], &td_font_10, c, x + 10, 434, 64, LV_TEXT_ALIGN_LEFT);
    }

    return root;
}

lv_obj_t *build_offline_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = lv_color_hex(0x52C6FF);
    ambient_rings(root, accent);
    wide_title_block(root, tr(lang, Str::kOffline), lang == AppLanguage::kFrench ? "Dernières données disponibles"
                                                                                  : "Last data available",
                     accent);

    constexpr int kCardX = 102;
    constexpr int kCardY = 158;
    constexpr int kCardW = 596;
    constexpr int kCardH = 184;
    lv_obj_t *panel = card(root, kCardX, kCardY, kCardW, kCardH, 14, LV_OPA_80);
    lv_obj_set_style_border_color(panel, accent, 0);
    lv_obj_set_style_border_opa(panel, wide_opa_pct(55), 0);
    wide_card_top_accent(root, kCardX, kCardY, kCardW, accent);
    wide_icon_circle(root, "wifi-off", 196, 246, 84, 42, accent);

    label(root, lang == AppLanguage::kFrench ? "Connexion absente" : "No connection", &td_font_24,
          colors.text_primary, 276, 196, 306, LV_TEXT_ALIGN_LEFT);
    const char *message = p.offline_message.empty()
                              ? (lang == AppLanguage::kFrench ? "Le profil reste lisible avec la dernière sauvegarde."
                                                               : "The profile remains readable from the last save.")
                              : p.offline_message.c_str();
    label(root, message, &td_font_16, colors.text_secondary, 278, 236, 396, LV_TEXT_ALIGN_LEFT);
    label(root, lang == AppLanguage::kFrench ? "Nouvelle tentative possible depuis cet écran."
                                             : "A new attempt is available from this screen.",
          &td_font_12, colors.text_muted, 278, 264, 390, LV_TEXT_ALIGN_LEFT);
    wide_state_button(root, 278, 288, accent, false);

    return root;
}

lv_obj_t *build_error_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = colors.warning;
    ambient_rings(root, accent);
    wide_title_block(root, tr(lang, Str::kErrorBadge),
                     lang == AppLanguage::kFrench ? "Synchronisation interrompue" : "Sync interrupted", accent);

    constexpr int kCardX = 102;
    constexpr int kCardY = 158;
    constexpr int kCardW = 596;
    constexpr int kCardH = 184;
    lv_obj_t *panel = card(root, kCardX, kCardY, kCardW, kCardH, 14, LV_OPA_80);
    lv_obj_set_style_border_color(panel, accent, 0);
    lv_obj_set_style_border_opa(panel, wide_opa_pct(45), 0);
    wide_card_top_accent(root, kCardX, kCardY, kCardW, accent);
    wide_icon_circle(root, "triangle-alert", 196, 246, 84, 42, accent);

    const char *title = p.error_title.empty()
                            ? (lang == AppLanguage::kFrench ? "Profil indisponible" : "Profile unavailable")
                            : p.error_title.c_str();
    const char *message = p.error_message.empty()
                              ? (lang == AppLanguage::kFrench ? "La réponse Pocket PSN n'est pas exploitable."
                                                               : "Pocket PSN response cannot be used.")
                              : p.error_message.c_str();
    label(root, title, &td_font_24, colors.text_primary, 276, 196, 306, LV_TEXT_ALIGN_LEFT);
    label(root, message, &td_font_16, colors.text_secondary, 278, 236, 396, LV_TEXT_ALIGN_LEFT);
    label(root, lang == AppLanguage::kFrench ? "Les anciennes données restent affichées."
                                             : "Previous data remains displayed.",
          &td_font_12, colors.text_muted, 278, 264, 390, LV_TEXT_ALIGN_LEFT);
    wide_state_button(root, 278, 288, accent, true);

    return root;
}

lv_obj_t *build_settings_screen_wide(const ProfileData &p, AppLanguage lang, WideSettingHandler handler,
                                      void *owner) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);
    label(root, tr(lang, Str::kSettingsTitle), &td_font_28, colors.text_primary, 112, 64, 210, LV_TEXT_ALIGN_LEFT);
    label(root, lang == AppLanguage::kFrench ? "Contrôles rapides" : "Quick controls", &td_font_16,
          colors.text_secondary, 114, 100, 250, LV_TEXT_ALIGN_LEFT);
    wide_bg_rect(root, 114, 125, 198, 2, colors.accent_cyan, LV_OPA_COVER, colors.accent_cyan, LV_OPA_TRANSP, 0, 1);
    make_status_badge(root, p.updated.c_str(), colors.success, 650, 67);

    const bool fr = lang == AppLanguage::kFrench;
    const char *on = fr ? "ON" : "ON";
    const char *off = fr ? "OFF" : "OFF";

    char brightness[16];
    std::snprintf(brightness, sizeof(brightness), "%d %%", std::clamp(p.brightness, 0, 100));
    settings_row_wide(root, 133, "sun", tr(lang, Str::kBrightnessLabel), brightness, colors.accent_cyan,
                      WideSettingAction::kBrightness, handler, owner, std::clamp(p.brightness, 0, 100));

    settings_row_wide(root, 186, "rotate-ccw", tr(lang, Str::kAutoRotateLabel), p.auto_rotate ? on : off,
                      lv_color_hex(0x52C6FF), WideSettingAction::kAutoRotate, handler, owner, -1, p.auto_rotate);

    settings_row_wide(root, 239, "sparkles", tr(lang, Str::kAnimationsLabel), p.reduced_motion ? off : on,
                      colors.accent_violet, WideSettingAction::kAnimations, handler, owner, -1, !p.reduced_motion);

    char refresh[16];
    std::snprintf(refresh, sizeof(refresh), "%d min", p.sync_interval_minutes);
    settings_row_wide(root, 292, "refresh-cw", tr(lang, Str::kRefreshLabel), refresh, colors.gold,
                      WideSettingAction::kSyncInterval, handler, owner);

    // Remplace l'ancienne ligne "A propos" : l'ecran credits est deja dans
    // le defilement automatique (slide 8), la dupliquer ici ne servait a
    // rien -- alors qu'une bascule de langue accessible sans passer par le
    // portail web change tout pour qui ne parle pas francais.
    settings_row_wide(root, 345, "languages", tr(lang, Str::kLanguageLabel),
                      tr(lang, fr ? Str::kLanguageFrench : Str::kLanguageEnglish), colors.accent_violet,
                      WideSettingAction::kLanguage, handler, owner);

    settings_row_wide(root, 398, "gamepad-pro", tr(lang, Str::kProfileLabel),
                      p.username.empty() ? "-" : p.username.c_str(), colors.success, WideSettingAction::kNone,
                      handler, owner);

    return root;
}

// Carte horizontale (pas un anneau circulaire, contrairement aux ecrans
// Trophees) : decision utilisateur du 2026-07-28 apres comparaison des
// deux styles ("trophee rond, stats rectangle") -- utilise
// intentionnellement toute la largeur du canevas 800px (icone dans un
// badge carre a gauche, valeur/legende alignees a gauche a droite) plutot
// qu'une composition centree qui laissait les cotes vides.
lv_obj_t *build_stat_screen_wide(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                  int sub_count) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_violet);

    constexpr int kCardX = WIDE_PAD;
    constexpr int kCardY = 90;
    constexpr int kCardW = WIDE_WIDTH - 2 * WIDE_PAD;
    constexpr int kCardH = 300;
    // Zone icone/texte alignee sur la specification donnee lors de la generation pour
    // generer le fond (voir assets/stat_card_background.cpp) : carre
    // ~180x180 debutant a x=76, centre verticalement.
    constexpr int kIconZoneX = 76;
    constexpr int kIconZoneSize = 180;
    // 64, PAS 84 : c'est la taille reelle des assets stat_icons.cpp (voir
    // leur en-tete lv_image_dsc_t, w=h=64) -- une valeur perimee (84) faisait
    // deborder le calcul de centrage de 10px en haut a gauche par rapport au
    // carre du fond (mesure precisement sur les captures) -- retour
    // utilisateur du 2026-07-29 ("aucune des icones n'est au centre").
    constexpr int kIconGlyph = 64;
    constexpr int kTextGap = 40;
    constexpr int kTextPad = 32;

    // Fond de carte complet (bordure degradee, texture, bandeau, bracket
    // de coin -- tout compose en une seule image generee par IA) : remplace
    // les assets separes precedents (accent_bar/badge_texture/corner/
    // bordure en code) -- retour utilisateur du 2026-07-28 ("l'IA te
    // genere le fond avec tout... et toi tu rajoutes sur le fond"), bien
    // plus simple et fidele qu'un reassemblage manuel piece par piece.
    lv_obj_t *panel = lv_obj_create(root);
    lv_obj_remove_style_all(panel);
    lv_obj_set_size(panel, kCardW, kCardH);
    lv_obj_set_pos(panel, kCardX, kCardY);
    lv_obj_set_style_radius(panel, 24, 0);
    lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_clip_corner(panel, true, 0);

    lv_obj_t *background = lv_image_create(panel);
    lv_image_set_src(background, stat_card_background_descriptor());
    lv_obj_set_pos(background, 0, 0);

    lv_obj_t *icon = lv_image_create(panel);
    lv_image_set_src(icon, stat_icon_descriptor(icon_kind));
    lv_obj_set_style_image_recolor(icon, colors.accent_violet, 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_set_pos(icon, kIconZoneX + (kIconZoneSize - kIconGlyph) / 2, (kCardH - kIconGlyph) / 2);

    const int text_x = kIconZoneX + kIconZoneSize + kTextGap;
    const int text_w = kCardW - text_x - kTextPad;
    const bool short_value = std::strlen(value) <= 4;
    const lv_font_t *value_font = short_value ? &td_font_96 : &td_font_48;
    // Bloc valeur+legende centre verticalement sur l'icone (centre carte
    // y=150), calcule a partir de la vraie hauteur de la police -- retour
    // utilisateur du 2026-07-29 ("on modifie les ecritures plus centrer",
    // puis "c'est surtout pour les heures et le rangs... faut vraiment les
    // rapprocher") : avec un y de legende fixe, "rang mondial"/"temps de
    // jeu" (valeurs longues, td_font_48, line_height=55) laissaient un
    // grand vide avant la legende par rapport a "jeux termines"/
    // "completion" (valeurs courtes, td_font_96, line_height=107) -- fige a
    // 68/202 (cas court), ces deux ecrans avaient donc un ecart bien plus
    // grand entre valeur et legende que les deux autres.
    constexpr int kValueCaptionGap = 27;
    constexpr int kCaptionHeight = 31; // td_font_28
    const int value_height = short_value ? 107 : 55; // td_font_96 / td_font_48
    const int block_height = value_height + kValueCaptionGap + kCaptionHeight;
    const int value_y = (kCardH - block_height) / 2;
    const int caption_y = value_y + value_height + kValueCaptionGap;
    lv_obj_t *value_lbl =
        label(panel, value, value_font, colors.text_primary, text_x, value_y, text_w, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *caption_lbl =
        label(panel, caption, &td_font_28, colors.text_secondary, text_x, caption_y, text_w, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *dots = make_glow_dots(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    // Pas d'animation -- voir build_dashboard_screen_wide().

    return root;
}

// Refait entierement au code (pas d'asset image) : meme anneau double-ton
// que les ecrans Trophees (make_dual_ring, deja eprouve/deja paye en flash)
// autour de l'icone wifi Lucide, meme style de carte que le reste de l'app
// pour la capsule SSID -- coherent avec l'identite visuelle existante
// plutot qu'une nouvelle image. Retour utilisateur du 2026-07-29 ("fait le
// toi meme... rend le beau").
lv_obj_t *build_wifi_setup_screen_wide(const std::string &ssid, const std::string &ip_address, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    constexpr int kRingY = WIDE_HERO_TROPHY_HALO_Y; // 146, meme cadence que les ecrans Trophees
    constexpr int kRingSize = 220;
    lv_obj_t *halo = make_dual_ring(root, kRingSize, colors.accent_cyan, colors.accent_violet, 3, LV_OPA_70);
    lv_obj_set_pos(halo, wide_center_x(kRingSize), kRingY - kRingSize / 2);
    lv_obj_t *halo2 =
        make_dual_ring(root, kRingSize - 24, colors.accent_cyan, colors.accent_violet, 1, LV_OPA_40, 20);
    lv_obj_set_pos(halo2, wide_center_x(kRingSize - 24), kRingY - (kRingSize - 24) / 2);
    add_slider_deco(root, wide_center_x(kRingSize) - 40, kRingY - 13, colors.accent_violet);
    add_slider_deco(root, wide_center_x(kRingSize) + kRingSize + 24, kRingY - 13, colors.accent_cyan);

    // QR d'appairage, a la place de l'ancienne icone Wi-Fi : les anneaux
    // lui servent de cadre, la composition ne bouge pas.
    //
    // Format "WIFI:" reconnu nativement par l'appareil photo d'Android et
    // d'iOS : scanner rejoint le reseau, puis le portail captif s'ouvre
    // seul. L'utilisateur n'a plus ni SSID ni adresse IP a recopier, ce
    // qui etait la marche exacte que ratait quelqu'un de non technique.
    // Le point d'acces est ouvert (WiFiManager.cpp appelle softAP() sans
    // mot de passe), d'ou "T:nopass".
    char wifi_payload[128];
    std::snprintf(wifi_payload, sizeof(wifi_payload), "WIFI:T:nopass;S:%s;;", ssid.c_str());

    // Fond blanc et modules noirs, avec une marge blanche autour : un QR
    // clair sur fond sombre ou colle au decor devient difficile a lire
    // pour beaucoup de lecteurs. C'est le seul element volontairement non
    // AMOLED de l'interface, et il n'apparait que tant que l'appareil
    // n'est pas configure.
    // La carte (192) est volontairement bien plus grande que le code
    // (164) : c'est la marge blanche qui compte, pas la taille demandee.
    //
    // Mesure, en decodant les captures rendues : passer de 176/148 a
    // 192/164 n'a PAS agrandi le code (143 -> 144 px -- lv_qrcode_set_size
    // arrondit a un multiple entier de modules, la valeur demandee n'est
    // qu'un plafond). Seule la marge a grandi, de ~16 a ~24 px par cote,
    // et le seuil de lecture est passe de 72 px a 58 px de resolution de
    // capture. Ne pas rogner cette marge pour gagner de la place.
    constexpr int kQrCard = 192;
    constexpr int kQrSize = 164;
    lv_obj_t *qr_card = card(root, wide_center_x(kQrCard), kRingY - kQrCard / 2, kQrCard, kQrCard, 14, LV_OPA_COVER);
    lv_obj_set_style_bg_color(qr_card, lv_color_white(), 0);
    lv_obj_set_style_border_width(qr_card, 0, 0);

    lv_obj_t *qr = lv_qrcode_create(qr_card);
    lv_qrcode_set_size(qr, kQrSize);
    lv_qrcode_set_dark_color(qr, lv_color_black());
    lv_qrcode_set_light_color(qr, lv_color_white());
    lv_qrcode_update(qr, wifi_payload, static_cast<uint32_t>(std::strlen(wifi_payload)));
    lv_obj_center(qr);

    // Capsule SSID : meme style de carte que le reste de l'app (voir card(),
    // deja utilise pour l'avatar/les mini-cartes du dashboard) -- largeur au
    // contenu plutot que fixe pour rester lisible avec un SSID plus long.
    constexpr int kPillY = 300;
    constexpr int kPillH = 64;
    constexpr int kPillW = 460;
    lv_obj_t *pill = card(root, wide_center_x(kPillW), kPillY, kPillW, kPillH, kPillH / 2, LV_OPA_70);
    lv_obj_set_style_border_width(pill, 1, 0);
    lv_obj_set_style_border_color(pill, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(pill, LV_OPA_50, 0);
    lv_obj_t *ssid_lbl = label(pill, ssid.c_str(), &td_font_28, colors.text_primary, 0, (kPillH - 31) / 2, kPillW,
                                LV_TEXT_ALIGN_CENTER);

    // Trois niveaux, du geste a faire au recours : scanner, ce qui se
    // passe ensuite, et quoi faire si le scan echoue.
    label(root, tr(lang, Str::kWifiSetupScan), &td_font_22, colors.text_primary,
          wide_center_x(WIDE_HERO_VALUE_W), kPillY + kPillH + 22, WIDE_HERO_VALUE_W);
    lv_obj_t *step_lbl = label(root, tr(lang, Str::kWifiSetupInstruction), &td_font_14, colors.text_secondary,
                                wide_center_x(WIDE_HERO_VALUE_W), kPillY + kPillH + 54, WIDE_HERO_VALUE_W);
    char fallback[96];
    std::snprintf(fallback, sizeof(fallback), tr(lang, Str::kWifiSetupFallbackFormat), ip_address.c_str());
    lv_obj_t *fallback_lbl = label(root, fallback, &td_font_14, colors.text_muted, wide_center_x(WIDE_HERO_VALUE_W),
                                    kPillY + kPillH + 78, WIDE_HERO_VALUE_W);

    // Pas d'animation -- voir build_dashboard_screen_wide().

    return root;
}

namespace {

// Echelle LVGL : 256 = taille reelle. Le pivot par defaut etant le coin
// haut-gauche, il faut le recentrer, sinon l'objet grandit en glissant
// vers le bas-droite au lieu de s'epanouir sur place (c'est le defaut du
// pulse_obj partage, acceptable pour un battement discret, pas pour une
// entree).
void celebration_pop_in(lv_obj_t *obj, int size, uint32_t delay, uint32_t duration = 520) {
    lv_obj_set_style_transform_pivot_x(obj, size / 2, 0);
    lv_obj_set_style_transform_pivot_y(obj, size / 2, 0);
    lv_obj_set_style_transform_scale_x(obj, 0, 0);
    lv_obj_set_style_transform_scale_y(obj, 0, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, 0, 256);
    lv_anim_set_delay(&a, delay);
    lv_anim_set_duration(&a, duration);
    // Depassement volontaire : l'objet franchit sa taille finale puis
    // revient. C'est ce petit rebond qui fait la difference entre "ca
    // apparait" et "ca arrive".
    lv_anim_set_path_cb(&a, lv_anim_path_overshoot);
    lv_anim_set_exec_cb(&a, [](void *target, int32_t v) {
        lv_obj_set_style_transform_scale_x(static_cast<lv_obj_t *>(target), v, 0);
        lv_obj_set_style_transform_scale_y(static_cast<lv_obj_t *>(target), v, 0);
    });
    lv_anim_start(&a);
}

// Rotation lente et continue. Empeche l'ecran de retomber en photo une
// fois l'entree jouee : il reste 10 secondes a l'affichage.
void celebration_spin(lv_obj_t *obj, int size, uint32_t period_ms, bool reverse) {
    lv_obj_set_style_transform_pivot_x(obj, size / 2, 0);
    lv_obj_set_style_transform_pivot_y(obj, size / 2, 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, obj);
    lv_anim_set_values(&a, reverse ? 3600 : 0, reverse ? 0 : 3600);
    lv_anim_set_duration(&a, period_ms);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_set_exec_cb(&a, [](void *target, int32_t v) {
        lv_obj_set_style_transform_rotation(static_cast<lv_obj_t *>(target), v, 0);
    });
    lv_anim_start(&a);
}

} // namespace

lv_obj_t *build_celebration_screen_wide(const TrophyStats &gained, AppLanguage lang, bool animate) {
    // Palier mis en avant : le plus prestigieux parmi ceux reellement
    // gagnes. Quand on decroche un Platine et trois Bronze dans la meme
    // synchro, l'evenement c'est le Platine.
    TrophyKind hero = TrophyKind::Bronze;
    if(gained.platinum > 0) hero = TrophyKind::Platinum;
    else if(gained.gold > 0) hero = TrophyKind::Gold;
    else if(gained.silver > 0) hero = TrophyKind::Silver;

    const int tiers = (gained.platinum > 0) + (gained.gold > 0) + (gained.silver > 0) + (gained.bronze > 0);
    const int total = std::max(1, gained.total);
    const lv_color_t accent = trophy_color_wide(hero);

    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, accent);

    // Composition centree, volontairement differente des ecrans de
    // donnees : ici il n'y a rien a lire ni a comparer, juste un moment.
    constexpr int kCx = WIDE_WIDTH / 2;
    constexpr int kCy = 170;
    constexpr int kTrophy = 160;
    lv_obj_t *halo = make_dual_ring(root, 258, accent, colors.accent_violet, 3, LV_OPA_70);
    lv_obj_set_pos(halo, kCx - 129, kCy - 129);
    lv_obj_t *halo2 = make_dual_ring(root, 224, accent, colors.accent_violet, 1, LV_OPA_40, 20);
    lv_obj_set_pos(halo2, kCx - 112, kCy - 112);
    lv_obj_t *trophy = make_trophy_icon(root, hero, kTrophy);
    lv_obj_set_pos(trophy, kCx - kTrophy / 2, kCy - kTrophy / 2);

    if(animate) {
        // Cascade : les anneaux s'ouvrent, le trophee arrive dessus avec
        // un rebond, puis le texte monte. L'ordre et les decalages font
        // tout -- joues ensemble, les trois lectures se marchent dessus.
        // Les anneaux sont des groupes de 28 arcs vectoriels : un scale
        // dessus obligerait LVGL a rasteriser puis transformer toute la
        // zone a chaque image. Ils entrent donc en fondu, ce qui ne
        // touche que l'alpha. Seul le trophee, qui est une image, recoit
        // un scale : c'est natif et bon marche.
        fade_in(halo, 0, 420);
        fade_in(halo2, 90, 420);
        celebration_pop_in(trophy, kTrophy, 200, 560);
        // Les deux anneaux tournent en sens inverse, a des vitesses
        // differentes : un seul anneau qui tourne se lit comme un
        // chargement, deux qui se croisent se lisent comme une mise en
        // valeur.
        // Faire tourner les anneaux etait la piste initiale : abandonnee,
        // meme cause que ci-dessus. Une rotation de ces groupes d'arcs
        // rendait le simulateur inutilisable (plusieurs secondes par
        // image, mesure) -- sur l'ESP32-S3 c'aurait ete pire encore.
        //
        // L'ecran garde donc sa vie par un battement lent du trophee, la
        // meme technique que la celebration de l'ecran rond, deja
        // eprouvee : un scale sur une image, ca LVGL sait faire. Le pivot
        // a deja ete recentre par celebration_pop_in juste au-dessus.
        pulse_obj(trophy, 820);
    }

    // Accord singulier/pluriel : "Nouveau trophee" au-dessus de "+7
    // trophees" ferait faute.
    lv_obj_t *title = label(root, tr(lang, total > 1 ? Str::kNewTrophiesTitle : Str::kNewTrophyTitle), &td_font_28,
                            colors.text_primary, wide_center_x(WIDE_HERO_VALUE_W), 306, WIDE_HERO_VALUE_W);
    if(animate) float_in(title, 420, 14);

    char gain[64];
    if(tiers > 1) {
        std::snprintf(gain, sizeof(gain), tr(lang, Str::kMultipleTrophiesFormat), static_cast<unsigned>(total));
    } else {
        std::snprintf(gain, sizeof(gain), "+%d %s", total, trophy_kind_label(hero, lang));
    }
    lv_obj_t *gain_lbl =
        label(root, gain, &td_font_48, accent, wide_center_x(WIDE_HERO_VALUE_W), 344, WIDE_HERO_VALUE_W);
    if(animate) float_in(gain_lbl, 520, 18);

    // Detail par palier uniquement s'il y en a plusieurs : avec un seul,
    // il repeterait mot pour mot la ligne au-dessus.
    if(tiers > 1) {
        struct Entry {
            TrophyKind kind;
            int count;
        };
        const Entry entries[] = {
            {TrophyKind::Platinum, gained.platinum},
            {TrophyKind::Gold, gained.gold},
            {TrophyKind::Silver, gained.silver},
            {TrophyKind::Bronze, gained.bronze},
        };
        constexpr int kChipW = 150;
        constexpr int kChipH = 40;
        constexpr int kChipGap = 12;
        int x = (WIDE_WIDTH - (tiers * kChipW + (tiers - 1) * kChipGap)) / 2;
        int shown = 0;
        for(const Entry &e : entries) {
            if(e.count <= 0) continue;
            const lv_color_t chip_color = trophy_color_wide(e.kind);
            lv_obj_t *chip = card(root, x, 418, kChipW, kChipH, kChipH / 2, LV_OPA_70);
            lv_obj_set_style_border_color(chip, chip_color, 0);
            lv_obj_set_style_border_opa(chip, wide_opa_pct(55), 0);
            lv_obj_set_style_border_width(chip, 1, 0);
            char txt[32];
            std::snprintf(txt, sizeof(txt), "%s +%d", trophy_kind_label(e.kind, lang), e.count);
            label(chip, txt, &td_font_16, chip_color, 0, (kChipH - 20) / 2, kChipW, LV_TEXT_ALIGN_CENTER);
            if(animate) float_in(chip, 640 + shown * 90, 12);
            ++shown;
            x += kChipW + kChipGap;
        }
    }

    return root;
}

// --- Couche de navigation tactile -------------------------------------

namespace {

// 44 px : plus petite cible tactile confortable a viser au doigt sans
// viser precisement (recommandation WCAG 2.5.5 / Apple HIG, retenue ici
// parce que l'appareil s'utilise debout, a bout de bras, pas de pres).
constexpr int kWideNavTouchSize = 44;
constexpr int kWideNavButtonSize = 56;
constexpr int kWideNavMargin = 10;

struct WideNavTouchContext {
    WideNavAction action;
    int target;
    WideNavHandler handler;
    void *owner;
};

void wide_nav_touch_event(lv_event_t *e) {
    auto *ctx = static_cast<WideNavTouchContext *>(lv_event_get_user_data(e));
    if(ctx == nullptr) return;
    const lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_DELETE) {
        delete ctx;
        return;
    }
    if(code == LV_EVENT_CLICKED && ctx->handler != nullptr) {
        ctx->handler(ctx->action, ctx->target, ctx->owner);
    }
}

// Rend un sous-arbre transparent au toucher : sans ca, le glyphe dessine
// au centre d'un bouton capte le clic a la place du bouton (lv_obj_create
// et lv_line_create sont CLICKABLE par defaut), et taper pile au milieu
// d'une fleche ne fait rien.
void wide_nav_passthrough(lv_obj_t *obj) {
    if(obj == nullptr) return;
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    const uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; ++i) wide_nav_passthrough(lv_obj_get_child(obj, i));
}

void wide_nav_set_gesture_bubble(lv_obj_t *obj) {
    if(obj == nullptr) return;
    lv_obj_add_flag(obj, LV_OBJ_FLAG_GESTURE_BUBBLE);
    const uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; ++i) wide_nav_set_gesture_bubble(lv_obj_get_child(obj, i));
}

// Prepare l'arbre pour que LVGL livre LV_EVENT_GESTURE a l'ecran.
//
// Attention au sens exact du drapeau (lv_indev.c, indev_gesture) : LVGL
// part de l'objet sous le doigt et remonte TANT QUE l'objet porte
// LV_OBJ_FLAG_GESTURE_BUBBLE ; l'evenement va au premier objet qui ne le
// porte PAS. Il faut donc l'inverse de l'intuition :
//   - tous les descendants le portent, sinon un geste parti d'une carte
//     s'arrete sur cette carte ;
//   - l'ecran ne le porte surtout PAS, sinon la remontee le depasse,
//     atteint nullptr, et l'evenement n'est envoye a personne.
void wide_nav_enable_gesture_bubbling(lv_obj_t *screen) {
    if(screen == nullptr) return;
    const uint32_t count = lv_obj_get_child_count(screen);
    for(uint32_t i = 0; i < count; ++i) wide_nav_set_gesture_bubble(lv_obj_get_child(screen, i));
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_GESTURE_BUBBLE);
}

lv_obj_t *wide_nav_overlay_root(lv_obj_t *screen) {
    lv_obj_t *overlay = lv_obj_create(screen);
    lv_obj_remove_style_all(overlay);
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
    // La couche ne capte rien elle-meme : seuls ses boutons sont
    // cliquables, les gestes continuent d'atteindre l'ecran en dessous.
    lv_obj_clear_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(overlay, WIDE_WIDTH, WIDE_HEIGHT);
    lv_obj_set_pos(overlay, 0, 0);
    lv_obj_set_style_bg_opa(overlay, LV_OPA_TRANSP, 0);
    return overlay;
}

lv_obj_t *wide_nav_button(lv_obj_t *parent, int x, int y, const char *icon, int glyph, WideNavAction action,
                          WideNavHandler handler, void *owner) {
    lv_obj_t *btn = lv_obj_create(parent);
    lv_obj_remove_style_all(btn);
    lv_obj_clear_flag(btn, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(btn, kWideNavButtonSize, kWideNavButtonSize);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, colors.surface_primary, 0);
    lv_obj_set_style_bg_opa(btn, wide_opa_pct(78), 0);
    lv_obj_set_style_border_color(btn, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(btn, wide_opa_pct(45), 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    // Retour visuel immediat a l'appui, gere par les etats LVGL plutot que
    // par un callback : rien a remettre a zero en cas de PRESS_LOST.
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
    lv_obj_set_style_border_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);

    lv_obj_t *box = make_icon_box(btn, icon, glyph, glyph, colors.text_primary);
    lv_obj_center(box);
    wide_nav_passthrough(box);

    auto *ctx = new WideNavTouchContext{action, 0, handler, owner};
    lv_obj_add_event_cb(btn, wide_nav_touch_event, LV_EVENT_ALL, ctx);
    return btn;
}

void wide_nav_dot(lv_obj_t *parent, int x, int y, bool active, int index, WideNavHandler handler, void *owner) {
    // Point discret (8/12 px) mais cible tactile pleine taille : le
    // reperage visuel n'a pas a dicter la difficulte de visee.
    lv_obj_t *hit = lv_obj_create(parent);
    lv_obj_remove_style_all(hit);
    lv_obj_clear_flag(hit, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(hit, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(hit, kWideNavTouchSize, kWideNavTouchSize);
    lv_obj_set_pos(hit, x, y);
    lv_obj_set_style_bg_opa(hit, LV_OPA_TRANSP, 0);

    lv_obj_t *dot = lv_obj_create(hit);
    lv_obj_remove_style_all(dot);
    lv_obj_clear_flag(dot, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(dot, active ? 12 : 8, active ? 12 : 8);
    lv_obj_center(dot);
    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(dot, active ? colors.accent_cyan : colors.text_secondary, 0);
    lv_obj_set_style_bg_opa(dot, active ? LV_OPA_COVER : wide_opa_pct(55), 0);

    auto *ctx = new WideNavTouchContext{WideNavAction::kGoTo, index, handler, owner};
    lv_obj_add_event_cb(hit, wide_nav_touch_event, LV_EVENT_ALL, ctx);
}

} // namespace

lv_obj_t *attach_wide_nav_overlay(lv_obj_t *screen, int index, int count, WideNavHandler handler, void *owner) {
    if(screen == nullptr || count <= 0) return nullptr;

    lv_obj_t *overlay = wide_nav_overlay_root(screen);

    const int dots_y = WIDE_HEIGHT - kWideNavTouchSize - 6;
    const int dots_x = (WIDE_WIDTH - count * kWideNavTouchSize) / 2;

    // Fond en pilule derriere la rangee de points, pour deux raisons :
    // les ecrans trophee/statistiques dessinent deja leur propre
    // indicateur de page a WIDE_HERO_DOTS_Y (442), soit exactement dans
    // cette bande -- sans ce fond, les deux paginations se superposent ;
    // et les controles restent lisibles quel que soit le contenu dessous.
    lv_obj_t *tray = lv_obj_create(overlay);
    lv_obj_remove_style_all(tray);
    lv_obj_clear_flag(tray, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(tray, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_size(tray, count * kWideNavTouchSize + 20, kWideNavTouchSize + 4);
    lv_obj_set_pos(tray, dots_x - 10, dots_y - 2);
    lv_obj_set_style_radius(tray, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(tray, colors.background_primary, 0);
    // Opaque, pas 92% : l'indicateur natif dessous est cyan vif, le moindre
    // reste de transparence le laissait percer en filet sous la rangee.
    lv_obj_set_style_bg_opa(tray, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(tray, colors.accent_cyan, 0);
    lv_obj_set_style_border_opa(tray, wide_opa_pct(30), 0);
    lv_obj_set_style_border_width(tray, 1, 0);

    for(int i = 0; i < count; ++i) {
        wide_nav_dot(overlay, dots_x + i * kWideNavTouchSize, dots_y, i == index, i, handler, owner);
    }

    const int arrow_y = (WIDE_HEIGHT - kWideNavButtonSize) / 2;
    wide_nav_button(overlay, kWideNavMargin, arrow_y, "chevron-left", 26, WideNavAction::kPrev, handler, owner);
    wide_nav_button(overlay, WIDE_WIDTH - kWideNavButtonSize - kWideNavMargin, arrow_y, "chevron-right", 26,
                    WideNavAction::kNext, handler, owner);
    // Engrenage en bas a gauche : la bande de points occupe le centre bas,
    // les fleches le milieu des bords -- aucun chevauchement.
    wide_nav_button(overlay, kWideNavMargin, WIDE_HEIGHT - kWideNavButtonSize - kWideNavMargin, "settings", 24,
                    WideNavAction::kOpenSettings, handler, owner);
    // En dernier : l'arbre doit etre complet, couche de navigation
    // comprise, pour qu'un geste parti de n'importe ou remonte a l'ecran.
    wide_nav_enable_gesture_bubbling(screen);
    return overlay;
}

lv_obj_t *attach_wide_back_overlay(lv_obj_t *screen, WideNavHandler handler, void *owner, AppLanguage lang) {
    if(screen == nullptr) return nullptr;

    lv_obj_t *overlay = wide_nav_overlay_root(screen);
    // En haut a gauche, la ou l'oeil cherche un retour. Les lignes de
    // reglages occupent x=77..723 sur toute la hauteur utile : la colonne
    // de gauche est le seul endroit ou poser ce controle sans recouvrir
    // quoi que ce soit (la derniere ligne descend jusqu'a y=444, il ne
    // reste rien d'exploitable en bas).
    const int y = 64;
    wide_nav_button(overlay, kWideNavMargin, y, "arrow-left", 26, WideNavAction::kCloseSettings, handler, owner);
    // Libelle sous l'icone : une fleche seule se comprend, mais le mot
    // leve tout doute pour quelqu'un qui decouvre l'appareil sans notice.
    label(overlay, tr(lang, Str::kBackButton), &td_font_12, colors.text_secondary, 2,
          y + kWideNavButtonSize + 4, 72, LV_TEXT_ALIGN_CENTER);
    wide_nav_enable_gesture_bubbling(screen);
    return overlay;
}

} // namespace trophy
