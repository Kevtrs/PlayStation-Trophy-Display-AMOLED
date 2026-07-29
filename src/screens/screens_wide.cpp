#include "screens/screens_wide.hpp"

#include "assets/fonts/td_fonts.hpp"
#include "assets/pocketpsn_logo.hpp"
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

void ambient_rings(lv_obj_t *root, lv_color_t accent) {
    lv_obj_t *far = make_glow_arc_top(root, 760, accent, 1, LV_OPA_10);
    lv_obj_set_pos(far, wide_center_x(760), WIDE_HERO_HALO_Y - 380);
    lv_obj_t *outer = make_glow_arc_top(root, 620, colors.accent_cyan, 1, LV_OPA_20);
    lv_obj_set_pos(outer, wide_center_x(620), WIDE_HERO_HALO_Y - 310);
    lv_obj_t *inner = make_glow_arc_top(root, 480, accent, 1, LV_OPA_20);
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

} // namespace

lv_obj_t *build_dashboard_screen_wide(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    make_status_badge(root, p.offline ? tr(lang, Str::kOffline) : sync_state_label(p.sync, lang), colors.success,
                       WIDE_DASH_STATUS_X, WIDE_DASH_STATUS_Y, "wifi");

    // --- Colonne gauche : profil + niveau + mini-cartes ---
    lv_obj_t *avatar =
        card(root, WIDE_DASH_LEFT_X, WIDE_DASH_AVATAR_Y, WIDE_DASH_AVATAR_SIZE, WIDE_DASH_AVATAR_SIZE, LV_RADIUS_CIRCLE, LV_OPA_70);
    lv_obj_set_style_border_color(avatar, colors.accent_cyan, 0);
    lv_obj_t *avatar_icon = make_icon_box(avatar, "circle-user-round", 40, 26, colors.accent_cyan);
    lv_obj_center(avatar_icon);
    label(root, p.username.c_str(), &td_font_20, colors.text_primary, WIDE_DASH_NAME_X, WIDE_DASH_NAME_Y, 300,
          LV_TEXT_ALIGN_LEFT);
    label(root, p.updated.c_str(), &td_font_12, colors.text_muted, WIDE_DASH_NAME_X, WIDE_DASH_META_Y, 300,
          LV_TEXT_ALIGN_LEFT);

    char level[32];
    std::snprintf(level, sizeof(level), "%d", p.level);
    lv_obj_t *level_lbl = label(root, level, &td_font_96, colors.text_primary, WIDE_DASH_LEFT_X, WIDE_DASH_LEVEL_Y,
                                 WIDE_DASH_LEVEL_W, LV_TEXT_ALIGN_LEFT);
    lv_obj_set_style_text_align(level_lbl, LV_TEXT_ALIGN_LEFT, 0);
    label(root, tr(lang, Str::kLevelLabel), &td_font_12, colors.accent_cyan, WIDE_DASH_LEVEL_META_X,
          WIDE_DASH_LEVEL_TAG_Y, 200, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *xp = lv_bar_create(root);
    lv_obj_set_size(xp, WIDE_DASH_XP_TRACK_W, 8);
    lv_obj_set_pos(xp, WIDE_DASH_LEVEL_META_X, WIDE_DASH_XP_TRACK_Y);
    lv_bar_set_range(xp, 0, 100);
    lv_bar_set_value(xp, p.progress, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(xp, colors.surface_secondary, LV_PART_MAIN);
    lv_obj_set_style_radius(xp, 4, LV_PART_MAIN);
    lv_obj_set_style_bg_color(xp, colors.accent_cyan, LV_PART_INDICATOR);
    lv_obj_set_style_radius(xp, 4, LV_PART_INDICATOR);

    char xp_caption[48];
    std::snprintf(xp_caption, sizeof(xp_caption), tr(lang, Str::kDashProgressToLevelFormat), p.progress,
                  p.level + 1);
    label(root, xp_caption, &td_font_12, colors.text_muted, WIDE_DASH_LEVEL_META_X, WIDE_DASH_XP_CAPTION_Y,
          WIDE_DASH_XP_CAPTION_W,
          LV_TEXT_ALIGN_LEFT);

    lv_obj_t *total_card = card(root, WIDE_DASH_LEFT_X, WIDE_DASH_CARDS_Y, WIDE_DASH_CARD_W, WIDE_DASH_CARD_H);
    // Icone generique (pas make_trophy_icon(..., TrophyKind::Multiple, ...)
    // -- cette valeur retombe sur le meme asset que TrophyKind::Platinum,
    // voir premium_trophies.cpp) : les deux mini-cartes "Trophees" et
    // "Platine" affichaient donc visuellement la meme icone platine, pretant
    // a confusion. Retour utilisateur du 2026-07-29 apres le premier flash
    // reel.
    lv_obj_t *total_icon = make_icon_box(total_card, "trophy", 40, 26, colors.accent_cyan);
    lv_obj_set_pos(total_icon, 16, 26);
    label(total_card, format_number(p.total).c_str(), &td_font_22, colors.text_primary, 66, 20, 112, LV_TEXT_ALIGN_LEFT);
    label(total_card, tr(lang, Str::kTrophiesUnit), &td_font_10, colors.text_muted, 66, 50, 112, LV_TEXT_ALIGN_LEFT);
    // Petite barre de pied de carte, purement decorative (ne represente
    // aucune progression reelle -- pas de valeur "sur 100" pour un total
    // de trophees) : meme inspiration que la maquette generee, comble le
    // bas de carte vide.
    lv_obj_t *total_bar = lv_obj_create(total_card);
    lv_obj_remove_style_all(total_bar);
    lv_obj_clear_flag(total_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(total_bar, WIDE_DASH_CARD_W - 32, 3);
    lv_obj_set_pos(total_bar, 16, WIDE_DASH_CARD_H - 16);
    lv_obj_set_style_radius(total_bar, 2, 0);
    lv_obj_set_style_bg_color(total_bar, colors.accent_cyan, 0);
    lv_obj_set_style_bg_opa(total_bar, LV_OPA_60, 0);

    const int plat_card_x = WIDE_DASH_LEFT_X + WIDE_DASH_CARD_W + WIDE_DASH_CARD_GAP;
    lv_obj_t *plat_card = card(root, plat_card_x, WIDE_DASH_CARDS_Y, WIDE_DASH_CARD_W, WIDE_DASH_CARD_H);
    lv_obj_t *plat_icon = make_trophy_icon(plat_card, TrophyKind::Platinum, 40);
    lv_obj_set_pos(plat_icon, 16, 26);
    label(plat_card, format_number(p.platinum).c_str(), &td_font_22, colors.platinum, 66, 20, 112, LV_TEXT_ALIGN_LEFT);
    label(plat_card, tr(lang, Str::kTrophyPlatinum), &td_font_10, colors.text_muted, 66, 50, 112,
          LV_TEXT_ALIGN_LEFT);
    lv_obj_t *plat_bar = lv_obj_create(plat_card);
    lv_obj_remove_style_all(plat_bar);
    lv_obj_clear_flag(plat_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(plat_bar, WIDE_DASH_CARD_W - 32, 3);
    lv_obj_set_pos(plat_bar, 16, WIDE_DASH_CARD_H - 16);
    lv_obj_set_style_radius(plat_bar, 2, 0);
    lv_obj_set_style_bg_color(plat_bar, colors.accent_violet, 0);
    lv_obj_set_style_bg_opa(plat_bar, LV_OPA_60, 0);

    // --- Colonne droite : trophee platine en grand ---
    // Meme texture de fond (asset genere par IA) que les ecrans Trophees, reduite a
    // l'echelle du halo du dashboard (WIDE_DASH_HALO_SIZE=260 vs 340 natif) --
    // aucun cout flash supplementaire, meme lv_image_dsc_t reutilise. Toujours
    // SOUS l'anneau colore (make_dual_ring, code) pour la meme raison que sur
    // les ecrans Trophees : ne pas rivaliser avec le degrade cyan/violet.
    constexpr int kDashRingAssetSize = 340;
    constexpr int kDashRingScale = static_cast<int>(256.0 * WIDE_DASH_HALO_SIZE / kDashRingAssetSize + 0.5);
    lv_obj_t *ring_bg = lv_image_create(root);
    lv_image_set_src(ring_bg, trophy_ring_descriptor());
    lv_image_set_scale(ring_bg, kDashRingScale);
    lv_obj_set_size(ring_bg, WIDE_DASH_HALO_SIZE, WIDE_DASH_HALO_SIZE);
    lv_obj_set_pos(ring_bg, WIDE_DASH_RIGHT_X + (WIDE_DASH_RIGHT_W - WIDE_DASH_HALO_SIZE) / 2,
                   wide_center_y(WIDE_DASH_HALO_SIZE));

    lv_obj_t *halo = make_dual_ring(root, WIDE_DASH_HALO_SIZE, colors.accent_cyan, colors.accent_violet, 3,
                                     LV_OPA_60);
    lv_obj_set_pos(halo, WIDE_DASH_RIGHT_X + (WIDE_DASH_RIGHT_W - WIDE_DASH_HALO_SIZE) / 2,
                   wide_center_y(WIDE_DASH_HALO_SIZE));
    lv_obj_t *halo2 = make_dual_ring(root, WIDE_DASH_HALO_SIZE - 40, colors.accent_cyan, colors.accent_violet, 1,
                                      LV_OPA_40, 20);
    lv_obj_set_pos(halo2, WIDE_DASH_RIGHT_X + (WIDE_DASH_RIGHT_W - (WIDE_DASH_HALO_SIZE - 40)) / 2,
                   wide_center_y(WIDE_DASH_HALO_SIZE - 40));
    lv_obj_t *trophy = make_trophy_icon(root, TrophyKind::Platinum, WIDE_DASH_TROPHY_SIZE);
    lv_obj_set_pos(trophy, WIDE_DASH_RIGHT_X + (WIDE_DASH_RIGHT_W - WIDE_DASH_TROPHY_SIZE) / 2,
                   wide_center_y(WIDE_DASH_TROPHY_SIZE));

    // PAS d'entree animee ni de pulsation continue ici (contrairement a
    // l'ecran rond, voir widgets.hpp) : fade_in/float_in/pulse_obj rendaient
    // le board 7" reellement saccade au premier flash materiel -- la
    // pulsation en boucle infinie (pulse_obj, transform_scale a chaque
    // frame) coutait particulierement cher sur ce panneau RGB 800x480 sans
    // acceleration materielle du blending. Retour utilisateur du
    // 2026-07-29 apres le tout premier flash reel ("zero animation ca fait
    // tout ramer").

    return root;
}

lv_obj_t *build_trophy_screen_wide(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = trophy_color_wide(kind);
    ambient_rings(root, accent);

    // Texture de fond (tirets, glow, degrade violet/cyan) issue d'une planche
    // IA -- volontairement SOUS l'anneau colore par palier (make_dual_ring
    // ci-dessous, code) plutot qu'a sa place : recolorier cette image en teinte
    // plate aurait fait perdre la distinction bronze/argent/or/platine
    // (trophy_color_wide) -- retour utilisateur du 2026-07-28.
    //
    // Anneau recentre sur le TROPHEE (WIDE_HERO_TROPHY_HALO_Y/SIZE), pas sur
    // l'atmosphere de fond generique (WIDE_HERO_HALO_Y/SIZE, ambient_rings
    // ci-dessus, inchangee) -- voir audit du 2026-07-29 dans layout_wide.hpp.
    constexpr int kRingAssetNativeSize = 340;
    constexpr int kRingAssetScale =
        static_cast<int>(256.0 * WIDE_HERO_TROPHY_HALO_SIZE / kRingAssetNativeSize + 0.5);
    lv_obj_t *ring_bg = lv_image_create(root);
    lv_image_set_src(ring_bg, trophy_ring_descriptor());
    lv_image_set_scale(ring_bg, kRingAssetScale);
    lv_obj_set_size(ring_bg, WIDE_HERO_TROPHY_HALO_SIZE, WIDE_HERO_TROPHY_HALO_SIZE);
    lv_obj_set_pos(ring_bg, wide_center_x(WIDE_HERO_TROPHY_HALO_SIZE),
                   WIDE_HERO_TROPHY_HALO_Y - WIDE_HERO_TROPHY_HALO_SIZE / 2);

    lv_obj_t *halo =
        make_dual_ring(root, WIDE_HERO_TROPHY_HALO_SIZE, accent, colors.accent_violet, 3, LV_OPA_70);
    lv_obj_set_pos(halo, wide_center_x(WIDE_HERO_TROPHY_HALO_SIZE),
                   WIDE_HERO_TROPHY_HALO_Y - WIDE_HERO_TROPHY_HALO_SIZE / 2);
    lv_obj_t *halo2 = make_dual_ring(root, WIDE_HERO_TROPHY_HALO_SIZE - 24, accent, colors.accent_violet, 1,
                                      LV_OPA_40, 20);
    lv_obj_set_pos(halo2, wide_center_x(WIDE_HERO_TROPHY_HALO_SIZE - 24),
                   WIDE_HERO_TROPHY_HALO_Y - (WIDE_HERO_TROPHY_HALO_SIZE - 24) / 2);
    add_slider_deco(root, wide_center_x(WIDE_HERO_TROPHY_HALO_SIZE) - 40, WIDE_HERO_TROPHY_HALO_Y - 13, accent);
    add_slider_deco(root, wide_center_x(WIDE_HERO_TROPHY_HALO_SIZE) + WIDE_HERO_TROPHY_HALO_SIZE + 24,
                     WIDE_HERO_TROPHY_HALO_Y - 13, colors.accent_violet);

    lv_obj_t *trophy = make_trophy_icon(root, kind, WIDE_HERO_TROPHY_SIZE);
    lv_obj_set_pos(trophy, wide_center_x(WIDE_HERO_TROPHY_SIZE), WIDE_HERO_TROPHY_TOP);

    lv_obj_t *value_lbl = label(root, format_number(value).c_str(), &td_font_96, colors.text_primary,
                                 wide_center_x(WIDE_HERO_VALUE_W), WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, trophy_kind_label(kind, lang), &td_font_28,
                                   colors.text_secondary, wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y,
                                   WIDE_HERO_CAPTION_W);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    // Pas d'animation (entree ni pulsation continue) -- voir
    // build_dashboard_screen_wide() ci-dessus, meme retour utilisateur du
    // 2026-07-29 apres le premier flash reel.

    return root;
}

lv_obj_t *build_credits_screen_wide(AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    ambient_rings(root, colors.accent_cyan);

    // Logo a 2x sa taille native (44x23 -> 88x46) : au-dela, le bitmap
    // source devient visiblement flou -- la lisibilite de loin repose donc
    // sur le texte "Pocket PSN" ci-dessous (grande police), pas sur le
    // logo lui-meme. Centre verticalement sur la meme bande que le disque
    // d'icone des ecrans Statistiques (WIDE_HERO_ICON_TOP + ICON_BOX/2) --
    // meme cadence visuelle sur les 5 ecrans "hero".
    constexpr int kLogoW = assets::kPocketPsnLogoWidth * 2;
    constexpr int kLogoH = assets::kPocketPsnLogoHeight * 2;
    constexpr int kLogoCenterY = WIDE_HERO_ICON_TOP + WIDE_HERO_ICON_BOX / 2;

    // Anneau recentre sur le LOGO (kLogoCenterY=150), pas sur l'atmosphere
    // generique (WIDE_HERO_HALO_Y=210) -- meme bug/meme correction que les
    // ecrans Trophees (voir WIDE_HERO_TROPHY_HALO_Y/SIZE, audit du
    // 2026-07-29) : l'ancien anneau (centre 210) laissait le logo (centre
    // 150) colle en haut et le texte "Pocket PSN" (y=268) chevauchait le
    // bas de l'anneau. Taille 220 (legerement plus petite que les 230px
    // des ecrans Trophees) : le logo est plus petit qu'un trophee, et le
    // centre du logo (150) est plus bas que celui du trophee (146), moins
    // de marge disponible avant le texte.
    constexpr int kCreditsHaloY = kLogoCenterY;
    constexpr int kCreditsHaloSize = 220;
    constexpr int kRingAssetNativeSize = 340;
    constexpr int kCreditsRingScale = static_cast<int>(256.0 * kCreditsHaloSize / kRingAssetNativeSize + 0.5);
    lv_obj_t *ring_bg = lv_image_create(root);
    lv_image_set_src(ring_bg, trophy_ring_descriptor());
    lv_image_set_scale(ring_bg, kCreditsRingScale);
    lv_obj_set_size(ring_bg, kCreditsHaloSize, kCreditsHaloSize);
    lv_obj_set_pos(ring_bg, wide_center_x(kCreditsHaloSize), kCreditsHaloY - kCreditsHaloSize / 2);

    lv_obj_t *halo = make_dual_ring(root, kCreditsHaloSize, colors.accent_cyan, colors.accent_violet, 3, LV_OPA_70);
    lv_obj_set_pos(halo, wide_center_x(kCreditsHaloSize), kCreditsHaloY - kCreditsHaloSize / 2);
    lv_obj_t *halo2 =
        make_dual_ring(root, kCreditsHaloSize - 24, colors.accent_cyan, colors.accent_violet, 1, LV_OPA_40, 20);
    lv_obj_set_pos(halo2, wide_center_x(kCreditsHaloSize - 24), kCreditsHaloY - (kCreditsHaloSize - 24) / 2);
    add_slider_deco(root, wide_center_x(kCreditsHaloSize) - 40, kCreditsHaloY - 13, colors.accent_violet);
    add_slider_deco(root, wide_center_x(kCreditsHaloSize) + kCreditsHaloSize + 24, kCreditsHaloY - 13,
                     colors.accent_cyan);

    lv_obj_t *logo = lv_image_create(root);
    lv_image_set_src(logo, assets::pocketpsn_logo_descriptor());
    lv_image_set_scale(logo, 512);
    lv_obj_set_size(logo, kLogoW, kLogoH);
    lv_obj_set_pos(logo, wide_center_x(kLogoW), kLogoCenterY - kLogoH / 2);

    lv_obj_t *name_lbl = label(root, "Pocket PSN", &td_font_48, colors.text_primary, wide_center_x(WIDE_HERO_VALUE_W),
                                WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, tr(lang, Str::kCreditsDataProvidedBy), &td_font_28, colors.text_secondary,
                                   wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y, WIDE_HERO_CAPTION_W);
    lv_obj_t *credit_lbl = label(root, "Trophy Display -- Kevin Torres", &td_font_12, colors.text_muted,
                                  wide_center_x(400), WIDE_HERO_DOTS_Y, 400);

    // Pas d'animation -- voir build_dashboard_screen_wide().

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

    constexpr int kIconGlyph = 96;
    lv_obj_t *icon = make_icon_box(root, "wifi", kIconGlyph, kIconGlyph - 20, colors.text_primary);
    lv_obj_set_pos(icon, wide_center_x(kIconGlyph), kRingY - kIconGlyph / 2);

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

    lv_obj_t *step_lbl = label(root, tr(lang, Str::kWifiSetupInstruction), &td_font_22, colors.text_secondary,
                                wide_center_x(WIDE_HERO_VALUE_W), kPillY + kPillH + 30, WIDE_HERO_VALUE_W);
    char fallback[96];
    std::snprintf(fallback, sizeof(fallback), tr(lang, Str::kWifiSetupFallbackFormat), ip_address.c_str());
    lv_obj_t *fallback_lbl = label(root, fallback, &td_font_14, colors.text_muted, wide_center_x(WIDE_HERO_VALUE_W),
                                    kPillY + kPillH + 68, WIDE_HERO_VALUE_W);

    // Pas d'animation -- voir build_dashboard_screen_wide().

    return root;
}

// TEST -- direction epuree, voir screens_wide.hpp. Retire : anneaux/halos
// degrades, textures de fond, cartes remplies, tout glow -- ne garde que
// la typographie, l'espacement et UNE seule teinte d'accent (cyan). Le
// trophee reste seul, sans cadre, a droite.
lv_obj_t *build_dashboard_screen_wide_v2(const ProfileData &p, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();

    // Statut Wi-Fi discret : petit point + texte, pas de pill/badge.
    lv_obj_t *status_dot = lv_obj_create(root);
    lv_obj_remove_style_all(status_dot);
    lv_obj_set_size(status_dot, 8, 8);
    lv_obj_set_style_radius(status_dot, 4, 0);
    lv_obj_set_style_bg_color(status_dot, p.offline ? colors.text_muted : colors.success, 0);
    lv_obj_set_style_bg_opa(status_dot, LV_OPA_COVER, 0);
    lv_obj_set_pos(status_dot, 706, 34);
    label(root, p.offline ? tr(lang, Str::kOffline) : sync_state_label(p.sync, lang), &td_font_12,
          colors.text_muted, 480, 27, 210, LV_TEXT_ALIGN_RIGHT);

    // Avatar : simple cercle contour fin, pas de glow.
    lv_obj_t *avatar = lv_obj_create(root);
    lv_obj_remove_style_all(avatar);
    lv_obj_set_size(avatar, 56, 56);
    lv_obj_set_style_radius(avatar, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(avatar, 1, 0);
    lv_obj_set_style_border_color(avatar, colors.text_muted, 0);
    lv_obj_set_style_border_opa(avatar, LV_OPA_60, 0);
    lv_obj_set_pos(avatar, 40, 40);
    lv_obj_t *avatar_icon = make_icon_box(avatar, "circle-user-round", 32, 20, colors.text_secondary);
    lv_obj_center(avatar_icon);

    label(root, p.username.c_str(), &td_font_22, colors.text_primary, 108, 46, 300, LV_TEXT_ALIGN_LEFT);
    label(root, p.updated.c_str(), &td_font_12, colors.text_muted, 108, 74, 300, LV_TEXT_ALIGN_LEFT);

    // Niveau : gros nombre aligne a gauche, sans anneau ni halo.
    char level[32];
    std::snprintf(level, sizeof(level), "%d", p.level);
    label(root, level, &td_font_96, colors.text_primary, 40, 150, 300, LV_TEXT_ALIGN_LEFT);
    label(root, tr(lang, Str::kLevelLabel), &td_font_12, colors.accent_cyan, 44, 266, 200, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *xp = lv_bar_create(root);
    lv_obj_set_size(xp, 260, 4);
    lv_obj_set_pos(xp, 44, 294);
    lv_bar_set_range(xp, 0, 100);
    lv_bar_set_value(xp, p.progress, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(xp, colors.surface_secondary, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(xp, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_radius(xp, 2, LV_PART_MAIN);
    lv_obj_set_style_bg_color(xp, colors.accent_cyan, LV_PART_INDICATOR);
    lv_obj_set_style_radius(xp, 2, LV_PART_INDICATOR);

    char xp_caption[48];
    std::snprintf(xp_caption, sizeof(xp_caption), tr(lang, Str::kDashProgressToLevelFormat), p.progress,
                  p.level + 1);
    label(root, xp_caption, &td_font_12, colors.text_muted, 44, 306, 300, LV_TEXT_ALIGN_LEFT);

    // Mini stats : typographie seule, pas de carte remplie -- juste de
    // l'espacement pour separer les colonnes.
    label(root, format_number(p.total).c_str(), &td_font_28, colors.text_primary, 40, 372, 160,
          LV_TEXT_ALIGN_LEFT);
    label(root, tr(lang, Str::kTrophiesUnit), &td_font_12, colors.text_muted, 40, 412, 160, LV_TEXT_ALIGN_LEFT);

    label(root, format_number(p.platinum).c_str(), &td_font_28, colors.platinum, 220, 372, 160,
          LV_TEXT_ALIGN_LEFT);
    label(root, tr(lang, Str::kTrophyPlatinum), &td_font_12, colors.text_muted, 220, 412, 160,
          LV_TEXT_ALIGN_LEFT);

    // Trophee seul a droite, sans cadre/anneau/halo.
    constexpr int kTrophySize = 220;
    lv_obj_t *trophy = make_trophy_icon(root, TrophyKind::Platinum, kTrophySize);
    lv_obj_set_pos(trophy, WIDE_WIDTH - 40 - kTrophySize, wide_center_y(kTrophySize));

    return root;
}

// TEST -- meme direction epuree que build_dashboard_screen_wide_v2 (voir
// screens_wide.hpp) : trophee seul, sans anneau/halo/texture. Garde la
// couleur par palier (bronze/argent/or/platine) sur la legende -- c'est de
// l'information, pas de la decoration.
lv_obj_t *build_trophy_screen_wide_v2(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = trophy_color_wide(kind);

    lv_obj_t *trophy = make_trophy_icon(root, kind, WIDE_HERO_TROPHY_SIZE);
    lv_obj_set_pos(trophy, wide_center_x(WIDE_HERO_TROPHY_SIZE), WIDE_HERO_TROPHY_TOP);

    lv_obj_t *value_lbl = label(root, format_number(value).c_str(), &td_font_96, colors.text_primary,
                                 wide_center_x(WIDE_HERO_VALUE_W), WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, trophy_kind_label(kind, lang), &td_font_28, accent,
                                   wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y, WIDE_HERO_CAPTION_W);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    LV_UNUSED(value_lbl);
    LV_UNUSED(caption_lbl);
    return root;
}

// TEST -- meme direction epuree : icone seule (pas de carte, pas de
// texture), meme cadence verticale que build_trophy_screen_wide_v2 pour
// rester coherent entre les deux types d'ecran "hero".
lv_obj_t *build_stat_screen_wide_v2(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                     int sub_count) {
    lv_obj_t *root = wide_screen_root();

    constexpr int kIconGlyph = 84;
    lv_obj_t *icon = lv_image_create(root);
    lv_image_set_src(icon, stat_icon_descriptor(icon_kind));
    lv_obj_set_style_image_recolor(icon, colors.accent_violet, 0);
    lv_obj_set_style_image_recolor_opa(icon, LV_OPA_COVER, 0);
    lv_obj_set_pos(icon, wide_center_x(kIconGlyph), WIDE_HERO_TROPHY_TOP + (WIDE_HERO_TROPHY_SIZE - kIconGlyph) / 2);

    const lv_font_t *value_font = std::strlen(value) <= 4 ? &td_font_96 : &td_font_48;
    lv_obj_t *value_lbl = label(root, value, value_font, colors.text_primary, wide_center_x(WIDE_HERO_VALUE_W),
                                 WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, caption, &td_font_28, colors.accent_violet,
                                   wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y, WIDE_HERO_CAPTION_W);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    LV_UNUSED(value_lbl);
    LV_UNUSED(caption_lbl);
    return root;
}

// TEST -- scene complete generee par IA (anneau + decorations laterales, PAS
// juste l'anneau recadre) : retour utilisateur du 2026-07-29 ("pour les
// trophee jaurais tout pris ca") apres avoir vu l'image source complete.
// Redimensionnee a 600x360 (meme ratio 1.667 que l'ecran 800x480) pour
// tenir dans le budget flash restant -- voir trophy_scene_wide.cpp.
lv_obj_t *build_trophy_screen_wide_v4(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = trophy_color_wide(kind);

    constexpr int kSceneW = 600;
    constexpr int kSceneH = 360;
    constexpr int kSceneX = wide_center_x(kSceneW);
    constexpr int kSceneY = 24;
    lv_obj_t *scene = lv_image_create(root);
    lv_image_set_src(scene, trophy_scene_wide_descriptor());
    lv_obj_set_pos(scene, kSceneX, kSceneY);

    // Centre de l'anneau mesure dans l'asset 600x360 : (299.5, 125.5) --
    // voir scratchpad, analyse pixel precise.
    constexpr int kRingCenterX = kSceneX + 300;
    constexpr int kRingCenterY = kSceneY + 126;

    constexpr int kTrophySize = 190;
    lv_obj_t *trophy = make_trophy_icon(root, kind, kTrophySize);
    lv_obj_set_pos(trophy, kRingCenterX - kTrophySize / 2, kRingCenterY - kTrophySize / 2);

    lv_obj_t *value_lbl = label(root, format_number(value).c_str(), &td_font_96, colors.text_primary,
                                 wide_center_x(WIDE_HERO_VALUE_W), WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, trophy_kind_label(kind, lang), &td_font_28, accent,
                                   wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y, WIDE_HERO_CAPTION_W);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    LV_UNUSED(value_lbl);
    LV_UNUSED(caption_lbl);
    return root;
}

// TEST -- "juste avec les assets generes par IA et rien d'autre" (retour
// utilisateur du 2026-07-29) : SEUL l'asset genere par IA decore l'ecran, a pleine
// intensite (trophy_ring_full, pas d'attenuation) -- aucun anneau code
// (make_dual_ring), aucune atmosphere de fond (ambient_rings), aucun ticks
// (add_slider_deco). Le trophee (deja colore par palier) et la legende
// (teintee par palier) restent les seuls porteurs de la distinction
// bronze/argent/or/platine, l'anneau lui-meme reste fixe violet/cyan.
lv_obj_t *build_trophy_screen_wide_v3(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang) {
    lv_obj_t *root = wide_screen_root();
    const lv_color_t accent = trophy_color_wide(kind);

    // 230, pas plus : au-dela, l'anneau chevauche WIDE_HERO_VALUE_Y (268) --
    // meme contrainte mesuree et validee pour l'anneau code (voir
    // WIDE_HERO_TROPHY_HALO_SIZE, audit du 2026-07-29).
    constexpr int kRingSize = 230;
    lv_obj_t *ring = lv_image_create(root);
    lv_image_set_src(ring, trophy_ring_full_descriptor());
    lv_obj_set_size(ring, kRingSize, kRingSize);
    lv_image_set_scale(ring, static_cast<int>(256.0 * kRingSize / 340.0 + 0.5));
    lv_obj_set_pos(ring, wide_center_x(kRingSize), WIDE_HERO_TROPHY_HALO_Y - kRingSize / 2);

    lv_obj_t *trophy = make_trophy_icon(root, kind, WIDE_HERO_TROPHY_SIZE);
    lv_obj_set_pos(trophy, wide_center_x(WIDE_HERO_TROPHY_SIZE), WIDE_HERO_TROPHY_TOP);

    lv_obj_t *value_lbl = label(root, format_number(value).c_str(), &td_font_96, colors.text_primary,
                                 wide_center_x(WIDE_HERO_VALUE_W), WIDE_HERO_VALUE_Y, WIDE_HERO_VALUE_W);
    lv_obj_t *caption_lbl = label(root, trophy_kind_label(kind, lang), &td_font_28, accent,
                                   wide_center_x(WIDE_HERO_CAPTION_W), WIDE_HERO_CAPTION_Y, WIDE_HERO_CAPTION_W);

    lv_obj_t *dots = make_page_indicator(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    LV_UNUSED(value_lbl);
    LV_UNUSED(caption_lbl);
    return root;
}

// TEST -- meme principe : le fond de carte genere par IA (deja tout compose --
// bordure, texture, bandeau) reste le seul element decoratif, SANS
// ambient_rings() derriere (contrairement a build_stat_screen_wide en
// production).
lv_obj_t *build_stat_screen_wide_v3(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                     int sub_count) {
    lv_obj_t *root = wide_screen_root();

    constexpr int kCardX = WIDE_PAD;
    constexpr int kCardY = 90;
    constexpr int kCardW = WIDE_WIDTH - 2 * WIDE_PAD;
    constexpr int kCardH = 300;
    constexpr int kIconZoneX = 76;
    constexpr int kIconZoneSize = 180;
    constexpr int kIconGlyph = 64;
    constexpr int kTextGap = 40;
    constexpr int kTextPad = 32;

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
    const lv_font_t *value_font = std::strlen(value) <= 4 ? &td_font_96 : &td_font_48;
    lv_obj_t *value_lbl =
        label(panel, value, value_font, colors.text_primary, text_x, 56, text_w, LV_TEXT_ALIGN_LEFT);
    lv_obj_t *caption_lbl =
        label(panel, caption, &td_font_28, colors.text_secondary, text_x, 190, text_w, LV_TEXT_ALIGN_LEFT);

    lv_obj_t *dots = make_glow_dots(root, sub_index, sub_count);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, WIDE_HERO_DOTS_Y);

    LV_UNUSED(value_lbl);
    LV_UNUSED(caption_lbl);
    return root;
}

} // namespace trophy
