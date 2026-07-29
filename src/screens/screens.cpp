#include "screens/screens.hpp"

#include "assets/fonts/td_fonts.hpp"
#include "assets/lucide_icons.hpp"
#include "assets/pocketpsn_logo.hpp"
#include "theme/theme.hpp"
#include "ui/app.hpp"
#include "ui/layout.hpp"
#include "ui/strings.hpp"
#include "ui/ui_fixtures.hpp"
#include "widgets/widgets.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>

#if defined(TROPHY_DESIGN_SIMULATOR)
#include <SDL.h>
#endif

namespace trophy {
namespace {

constexpr int kScreenW = DISPLAY_WIDTH;
constexpr int kCenter = DISPLAY_CENTER_X;
constexpr int kTitleY = 42;
constexpr int kSubtitleY = 68;
constexpr int kProductDotsY = 420;
constexpr int kActionH = 48;
constexpr int kActionW = 184;
constexpr int kActionX = center_x(kActionW);

lv_color_t trophy_color(TrophyKind kind) {
    switch(kind) {
        case TrophyKind::Platinum: return colors.platinum;
        case TrophyKind::Gold: return colors.gold;
        case TrophyKind::Silver: return colors.silver;
        case TrophyKind::Bronze: return colors.bronze;
        case TrophyKind::Multiple: return colors.accent_cyan;
    }
    return colors.accent_cyan;
}

lv_obj_t *base_screen(lv_color_t accent, bool rings = true) {
    lv_obj_t *root = screen_root();
    if(rings) {
        lv_obj_t *outer = make_glow_ring(root, 444, accent, 1, LV_OPA_30);
        lv_obj_center(outer);
        lv_obj_t *content = make_glow_ring(root, 362, colors.accent_violet, 1, LV_OPA_10);
        lv_obj_center(content);
    }
    lv_obj_t *notch = lv_obj_create(root);
    lv_obj_clear_flag(notch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(notch, 76, 3);
    lv_obj_set_pos(notch, center_x(76), 28);
    lv_obj_set_style_radius(notch, 2, 0);
    lv_obj_set_style_border_width(notch, 0, 0);
    lv_obj_set_style_bg_color(notch, accent, 0);
    lv_obj_set_style_bg_opa(notch, LV_OPA_70, 0);
    return root;
}

void title_block(lv_obj_t *root, const char *title, const char *subtitle, lv_color_t accent) {
    lv_obj_t *t = make_label(root, title, &style_title, 72, kTitleY, 322);
    lv_obj_set_style_text_font(t, &td_font_22, 0);
    lv_obj_set_style_text_color(t, colors.text_primary, 0);
    if(subtitle && subtitle[0]) {
        lv_obj_t *s = make_label(root, subtitle, &style_micro, 78, kSubtitleY, 310);
        lv_obj_set_style_text_font(s, &td_font_12, 0);
        lv_obj_set_style_text_color(s, accent, 0);
    }
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

lv_obj_t *label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color, int x, int y, int w, lv_text_align_t align = LV_TEXT_ALIGN_CENTER) {
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

lv_obj_t *wrapped(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color, int x, int y, int w, int h) {
    lv_obj_t *obj = label(parent, text, font, color, x, y, w, LV_TEXT_ALIGN_CENTER);
    lv_obj_set_height(obj, h);
    lv_label_set_long_mode(obj, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_line_space(obj, 2, 0);
    return obj;
}

lv_obj_t *button(lv_obj_t *parent, const char *text, int x, int y, int w, lv_color_t accent, lv_event_cb_t cb, void *user) {
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_add_style(btn, &style_button, 0);
    lv_obj_add_style(btn, &style_button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(btn, w, kActionH);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_style_bg_color(btn, colors.accent_blue, 0);
    lv_obj_set_style_border_color(btn, accent, 0);
    if(cb) lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, user);
    lv_obj_t *txt = label(btn, text, &td_font_16, colors.text_primary, 10, 14, w - 20);
    lv_obj_center(txt);
    return btn;
}

void page_dots(lv_obj_t *root, int active) {
    lv_obj_t *wrap = make_page_indicator(root, active, PRODUCT_PAGE_COUNT);
    lv_obj_set_y(wrap, kProductDotsY);
}

void go_dashboard_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->show_page(Page::Dashboard);
}

void go_settings_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->show_page(Page::Settings);
}

void go_about_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->show_page(Page::About);
}

void sync_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->simulate_sync();
}

void celebration_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->simulate_new_trophy(TrophyKind::Platinum);
}

void profile_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->cycle_fixture();
}

void motion_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->toggle_animations_enabled();
}

void open_pocketpsn_cb(lv_event_t *) {
#if defined(TROPHY_DESIGN_SIMULATOR)
    SDL_OpenURL("https://pocketpsn.com");
#endif
}

void add_pocketpsn_wordmark(lv_obj_t *root, int x, int y, int w, const lv_font_t *text_font) {
    lv_obj_t *logo = lv_image_create(root);
    lv_image_set_src(logo, assets::pocketpsn_logo_descriptor());
    lv_obj_set_size(logo, assets::kPocketPsnLogoWidth, assets::kPocketPsnLogoHeight);
    lv_obj_set_pos(logo, x, y);
    const int text_y = y + 1;
    lv_obj_t *name = label(root,
                           "Pocket PSN",
                           text_font,
                           colors.text_primary,
                           x + assets::kPocketPsnLogoWidth + BOOT_LOGO_GAP,
                           text_y,
                           w - assets::kPocketPsnLogoWidth - BOOT_LOGO_GAP,
                           LV_TEXT_ALIGN_LEFT);
    lv_obj_set_height(name, 24);
    lv_label_set_long_mode(name, LV_LABEL_LONG_CLIP);
}

void add_boot_wordmark(lv_obj_t *root) {
    add_pocketpsn_wordmark(root, BOOT_WORDMARK_X, BOOT_WORDMARK_Y, BOOT_WORDMARK_W, &td_font_16);
}

void add_trophy_stat(lv_obj_t *parent, TrophyKind kind, int value, int x, int y, AppLanguage lang) {
    lv_obj_t *icon = make_trophy_icon(parent, kind, 32);
    lv_obj_set_pos(icon, x, y);
    std::string n = format_number(value);
    label(parent, n.c_str(), &td_font_18, colors.text_primary, x + 38, y + 2, 76, LV_TEXT_ALIGN_LEFT);
    label(parent, trophy_kind_label(kind, lang), &td_font_10, colors.text_muted, x + 38, y + 24, 76, LV_TEXT_ALIGN_LEFT);
}

void settings_row(lv_obj_t *root, const char *icon, const char *name, const char *value, int y, lv_event_cb_t cb, App *app) {
    constexpr int row_x = 96;
    constexpr int row_w = 274;
    lv_obj_t *row = lv_button_create(root);
    lv_obj_add_style(row, &style_surface, 0);
    lv_obj_add_style(row, &style_button_pressed, LV_STATE_PRESSED);
    lv_obj_set_size(row, row_w, 44);
    lv_obj_set_pos(row, row_x, y);
    lv_obj_set_style_radius(row, 18, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    if(cb) lv_obj_add_event_cb(row, cb, LV_EVENT_CLICKED, app);
    lv_obj_t *sym = make_icon_box(row, icon, 32, 22, colors.accent_cyan);
    lv_obj_set_pos(sym, 10, 6);
    label(row, name, &td_font_14, colors.text_primary, 52, 13, 118, LV_TEXT_ALIGN_LEFT);
    label(row, value, &td_font_10, colors.text_secondary, 184, 16, 74, LV_TEXT_ALIGN_RIGHT);
}

} // namespace

lv_obj_t *build_boot_screen(App &app) {
    lv_obj_t *root = screen_root();
    lv_obj_t *outer = make_glow_ring(root, BOOT_OUTER_RING_SIZE, colors.accent_blue, 1, LV_OPA_40);
    lv_obj_center(outer);
    lv_obj_t *inner = make_glow_ring(root, BOOT_INNER_RING_SIZE, colors.accent_violet, 1, LV_OPA_20);
    lv_obj_center(inner);

    lv_obj_t *track = lv_arc_create(root);
    lv_obj_remove_style_all(track);
    lv_obj_set_size(track, BOOT_PROGRESS_RING_SIZE, BOOT_PROGRESS_RING_SIZE);
    lv_obj_set_pos(track, BOOT_PROGRESS_RING_X, BOOT_PROGRESS_RING_Y);
    lv_arc_set_range(track, 0, 100);
    lv_arc_set_value(track, app.boot_progress());
    lv_arc_set_bg_angles(track, 0, 360);
    lv_arc_set_rotation(track, 270);
    lv_obj_set_style_arc_color(track, lv_color_hex(0x08162A), LV_PART_MAIN);
    lv_obj_set_style_arc_opa(track, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_arc_width(track, 8, LV_PART_MAIN);
    lv_obj_set_style_arc_rounded(track, true, LV_PART_MAIN);
    lv_obj_set_style_arc_color(track, colors.accent_cyan, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(track, LV_OPA_90, LV_PART_INDICATOR);
    lv_obj_set_style_arc_width(track, 11, LV_PART_INDICATOR);
    lv_obj_set_style_arc_rounded(track, true, LV_PART_INDICATOR);
    lv_obj_clear_flag(track, LV_OBJ_FLAG_CLICKABLE);
    app.boot_progress_arc_ = track;

    lv_obj_t *halo = make_glow_ring(root, 142, colors.accent_cyan, 8, LV_OPA_20);
    lv_obj_set_pos(halo, center_x(142), 104);
    lv_obj_t *icon = make_trophy_icon(root, TrophyKind::Platinum, BOOT_TROPHY_SIZE);
    lv_obj_set_pos(icon, BOOT_TROPHY_X, BOOT_TROPHY_Y);

    label(root, "Trophy Display", &td_font_24, colors.text_primary, BOOT_TITLE_X, BOOT_TITLE_Y, BOOT_TITLE_W);
    char pct[8];
    std::snprintf(pct, sizeof(pct), "%u %%", static_cast<unsigned>(app.boot_progress()));
    app.boot_percent_label_ = label(root, pct, &td_font_12, colors.text_primary, BOOT_PERCENT_X, BOOT_PERCENT_Y, BOOT_PERCENT_W);
    label(root, tr(app.language(), Str::kPlayStationDataProvidedBy), &td_font_12, lv_color_hex(0xC7D4E8), BOOT_ATTR_X, BOOT_ATTR_Y, BOOT_ATTR_W);
    add_boot_wordmark(root);

    if(app.animations_enabled()) {
        pulse_obj(halo);
        float_in(icon, 80, 8);
    }
    return root;
}

lv_obj_t *build_welcome_screen(App &app) {
    lv_obj_t *root = base_screen(colors.accent_blue);
    lv_obj_t *orb = make_glow_ring(root, 232, colors.accent_violet, 8, LV_OPA_20);
    lv_obj_set_pos(orb, center_x(232), 90);
    lv_obj_t *trophy = make_trophy_icon(root, TrophyKind::Gold, 96);
    lv_obj_set_pos(trophy, center_x(96), 106);
    label(root, tr(app.language(), Str::kWelcomeTitle), &td_font_28, colors.text_primary, 80, 236, 306);
    wrapped(root, tr(app.language(), Str::kWelcomeSubtitle), &td_font_12, colors.text_secondary, 82, 272, 302, 36);
    button(root, tr(app.language(), Str::kExploreButton), kActionX, 314, kActionW, colors.accent_cyan, go_dashboard_cb, &app);
    add_pocketpsn_wordmark(root, center_x(166), 374, 166, &td_font_16);
    page_dots(root, 0);
    return root;
}

lv_obj_t *build_dashboard_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(p.offline ? colors.warning : colors.accent_blue);
    make_status_badge(root, p.offline ? tr(app.language(), Str::kOffline) : sync_state_label(p.sync, app.language()), p.offline ? colors.warning : colors.success, 174, 36, p.offline ? "wifi-off" : "wifi");

    lv_obj_t *avatar = card(root, 74, 80, 42, 42, LV_RADIUS_CIRCLE, LV_OPA_70);
    lv_obj_set_style_border_color(avatar, colors.accent_cyan, 0);
    lv_obj_t *avatar_icon = make_icon_box(avatar, "circle-user-round", 30, 20, colors.accent_cyan);
    lv_obj_center(avatar_icon);
    label(root, p.username.c_str(), p.username.size() > 24 ? &td_font_14 : &td_font_18, colors.text_primary, 122, 80, 268, LV_TEXT_ALIGN_LEFT);
    label(root, p.updated.c_str(), &td_font_10, colors.text_muted, 122, 104, 250, LV_TEXT_ALIGN_LEFT);

    // Anneau circulaire retire (etait ici) : chevauchement avec le badge
    // "% vers +1" une fois corrige, un artefact triangulaire persistant
    // est apparu au demarrage de l'animation de remplissage, jamais
    // reproductible dans le simulateur (probable bug de rafraichissement
    // partiel LVGL specifique a cet ecran/pilote reel, meme famille que le
    // bug de padding trouve plus tot dans la session) -- decision
    // utilisateur du 2026-07-27 : retirer le widget plutot que continuer a
    // chasser un rendu non reproductible localement.
    //
    // Halo decoratif statique a la place (demande utilisateur : l'ecran est
    // petit et rond, l'espace laisse vide par l'anneau ne doit pas rester
    // perdu) -- ring FIXE (valeur 100/360 constante, jamais anime), meme
    // widget deja utilise sans souci sur Welcome/Boot/Celebration : n'a
    // jamais ete concerne par le bug de rafraichissement partiel de
    // l'anneau de progression (qui necessitait une valeur variable/animee).
    lv_obj_t *halo = make_glow_ring(root, 260, colors.accent_cyan, 10, LV_OPA_10);
    lv_obj_set_pos(halo, center_x(260), 132);

    char level[32];
    std::snprintf(level, sizeof(level), "%d", p.level);
    label(root, level, &td_font_48, colors.text_primary, 142, 176, 182);
    label(root, tr(app.language(), Str::kLevelLabel), &td_font_10, colors.accent_cyan, 166, 233, 134);
    char prog[24];
    std::snprintf(prog, sizeof(prog), tr(app.language(), Str::kProgressToNextFormat), p.progress);
    make_chip(root, prog, 176, 264, 114);

    lv_obj_t *total = card(root, 82, 318, 148, 72, 18, LV_OPA_80);
    label(total, format_number(p.total).c_str(), &td_font_24, colors.text_primary, 14, 14, 124);
    label(total, tr(app.language(), Str::kTrophiesUnit), &td_font_10, colors.text_muted, 14, 46, 124);
    lv_obj_t *plat = card(root, 240, 318, 148, 72, 18, LV_OPA_80);
    add_trophy_stat(plat, TrophyKind::Platinum, p.platinum, 14, 20, app.language());
    page_dots(root, 0);
    return root;
}

lv_obj_t *build_trophies_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(colors.gold);
    // Vitrine "derniers gains" (fixture de demonstration) retiree
    // definitivement (decision utilisateur, 2026-07-27) : Pocket PSN ne
    // fournit jamais de liste detaillee des derniers trophees (voir
    // PocketPsnParser.cpp), donc cet ecran n'affiche plus que le resume
    // reel Plat/Or/Argent/Bronze, y compris en mode demo -- plus de
    // branche conditionnelle a maintenir (voir aussi
    // ProfileData::trophy_feed_available, conserve pour compatibilite
    // mais plus lu ici).
    // Une seule categorie a la fois, en grand, avec defilement automatique
    // (voir App::content_cycle_index()) : les 4 lignes serrees d'avant
    // etaient peu lisibles de loin sur ce petit ecran rond -- demande
    // utilisateur du 2026-07-28.
    struct TrophyEntry { TrophyKind kind; int value; };
    const TrophyEntry entries[4] = {
        {TrophyKind::Platinum, p.platinum},
        {TrophyKind::Gold, p.gold},
        {TrophyKind::Silver, p.silver},
        {TrophyKind::Bronze, p.bronze},
    };
    const int idx = app.content_cycle_index() % 4;
    const TrophyEntry &current = entries[idx];
    const lv_color_t accent = trophy_color(current.kind);

    title_block(root, tr(app.language(), Str::kTrophiesTitle), "", colors.gold);
    lv_obj_t *halo = make_glow_ring(root, 250, accent, 8, LV_OPA_10);
    lv_obj_set_pos(halo, center_x(250), 116);
    lv_obj_t *disc = card(root, center_x(140), 124, 140, 140, LV_RADIUS_CIRCLE, LV_OPA_60);
    lv_obj_set_style_border_color(disc, accent, 0);
    lv_obj_t *icon = make_trophy_icon(disc, current.kind, 92);
    lv_obj_center(icon);

    label(root, format_number(current.value).c_str(), &td_font_48, colors.text_primary, 63, 276, 340, LV_TEXT_ALIGN_CENTER);
    label(root, trophy_kind_label(current.kind, app.language()), &td_font_16, colors.text_secondary, 63, 334, 340, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *subdots = make_page_indicator(root, idx, 4);
    lv_obj_set_y(subdots, 376);
    page_dots(root, 1);
    return root;
}

lv_obj_t *build_statistics_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(colors.accent_violet);
    title_block(root, tr(app.language(), Str::kStatisticsTitle), tr(app.language(), Str::kStatisticsSubtitle), colors.accent_violet);

    char games[24];
    char completion[16];
    std::snprintf(games, sizeof(games), "%s", format_number(p.games_completed).c_str());
    std::snprintf(completion, sizeof(completion), "%d %%", p.completion);

    // Une seule info a la fois, en grand, avec defilement automatique (voir
    // App::content_cycle_index()) : les 4 tuiles serrees d'avant etaient
    // illisibles de loin sur ce petit ecran rond -- demande utilisateur du
    // 2026-07-28.
    struct StatEntry { const char *icon; const char *value; const char *caption; };
    const StatEntry entries[4] = {
        {"gamepad-2", games, tr(app.language(), Str::kGamesCompleted)},
        {"percent", completion, tr(app.language(), Str::kCompletion)},
        {"podium", p.world_rank.c_str(), tr(app.language(), Str::kWorldRank)},
        {"clock-3", p.play_time.c_str(), tr(app.language(), Str::kPlayTime)},
    };
    const int idx = app.content_cycle_index() % 4;
    const StatEntry &current = entries[idx];

    lv_obj_t *halo = make_glow_ring(root, 250, colors.accent_violet, 8, LV_OPA_10);
    lv_obj_set_pos(halo, center_x(250), 116);
    lv_obj_t *disc = card(root, center_x(112), 138, 112, 112, LV_RADIUS_CIRCLE, LV_OPA_60);
    lv_obj_set_style_border_color(disc, colors.accent_violet, 0);
    lv_obj_t *icon = make_icon_box(disc, current.icon, 76, 56, colors.accent_violet);
    lv_obj_center(icon);

    const std::size_t valueLen = std::strlen(current.value);
    const lv_font_t *valueFont = valueLen <= 4 ? &td_font_48 : (valueLen <= 7 ? &td_font_28 : &td_font_22);
    label(root, current.value, valueFont, colors.text_primary, 63, 268, 340, LV_TEXT_ALIGN_CENTER);
    label(root, current.caption, &td_font_14, colors.text_secondary, 63, 330, 340, LV_TEXT_ALIGN_CENTER);

    lv_obj_t *subdots = make_page_indicator(root, idx, 4);
    lv_obj_set_y(subdots, 372);
    page_dots(root, 2);
    return root;
}

lv_obj_t *build_sync_screen(App &app) {
    ProfileData &p = app.profile();
    if(p.sync == SyncState::Idle) p.sync = SyncState::Fetching;
    const bool done = p.sync == SyncState::Done;
    lv_color_t accent = done ? colors.success : (p.sync == SyncState::Error ? colors.error : colors.accent_cyan);
    lv_obj_t *root = base_screen(accent);
    make_status_badge(root, done ? tr(app.language(), Str::kDataReadyBadge) : tr(app.language(), Str::kSyncingBadge), accent, 154, 48, done ? "badge-check" : "refresh-cw");
    lv_obj_t *ring = make_circular_progress(root, done ? 100 : 68, 210, accent, 7, 11);
    lv_obj_set_pos(ring, center_x(210), 90);
    lv_obj_t *disc = card(root, center_x(92), 149, 92, 92, LV_RADIUS_CIRCLE, LV_OPA_60);
    lv_obj_set_style_border_color(disc, accent, 0);
    lv_obj_t *icon = make_icon_box(disc, done ? "badge-check" : "refresh-cw", 66, 50, accent);
    lv_obj_center(icon);
    label(root, done ? tr(app.language(), Str::kSyncedTitle) : tr(app.language(), Str::kSyncingTitle), &td_font_24, colors.text_primary, 74, 306, 318);
    label(root, sync_state_label(p.sync, app.language()), &td_font_14, colors.text_secondary, 92, 340, 282);
    wrapped(root, done ? tr(app.language(), Str::kDataUpToDate) : tr(app.language(), Str::kDataStaysReadable), &td_font_10, colors.text_muted, 82, 366, 302, 34);
    page_dots(root, 3);
    return root;
}

lv_obj_t *build_celebration_screen(App &app) {
    const ProfileData &p = app.profile();
    TrophyKind kind = p.celebration;
    lv_color_t accent = trophy_color(kind);
    lv_obj_t *root = base_screen(accent);
    lv_obj_t *halo = make_glow_ring(root, kind == TrophyKind::Platinum ? 224 : 210, accent, kind == TrophyKind::Platinum ? 18 : 14, kind == TrophyKind::Platinum ? LV_OPA_30 : LV_OPA_20);
    lv_obj_set_pos(halo, center_x(kind == TrophyKind::Platinum ? 224 : 210), 58);
    lv_obj_t *icon = make_trophy_icon(root, kind, 128);
    lv_obj_set_pos(icon, center_x(128), 92);
    label(root, tr(app.language(), Str::kNewTrophyTitle), &td_font_24, colors.text_primary, 70, 286, 326);
    char gain[48];
    if(kind == TrophyKind::Multiple) std::snprintf(gain, sizeof(gain), tr(app.language(), Str::kMultipleTrophiesFormat), static_cast<unsigned>(p.celebration_count));
    else std::snprintf(gain, sizeof(gain), "+%u %s", static_cast<unsigned>(std::max<uint32_t>(1, p.celebration_count)), trophy_kind_label(kind, app.language()));
    label(root, gain, &td_font_22, accent, 100, 324, 266);
    label(root, tr(app.language(), Str::kTapToReturn), &td_font_10, colors.text_muted, 126, 378, 214);
    page_dots(root, 3);
    if(app.animations_enabled()) pulse_obj(icon, 80);
    return root;
}

lv_obj_t *build_settings_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(colors.accent_blue);
    title_block(root, tr(app.language(), Str::kSettingsTitle), tr(app.language(), Str::kSettingsSubtitle), colors.accent_cyan);
    settings_row(root, "sun", tr(app.language(), Str::kBrightnessLabel), (std::to_string(p.brightness) + " %").c_str(), 96, nullptr, &app);
    lv_obj_t *bar = lv_bar_create(root);
    lv_obj_set_size(bar, 144, 5);
    lv_obj_set_pos(bar, 148, 126);
    lv_bar_set_range(bar, 0, 100);
    lv_bar_set_value(bar, p.brightness, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(bar, colors.surface_secondary, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, colors.accent_cyan, LV_PART_INDICATOR);
    settings_row(root, "sparkles", "Animations", p.reduced_motion ? tr(app.language(), Str::kAnimationsReduced) : tr(app.language(), Str::kAnimationsActive), 146, motion_cb, &app);
    settings_row(root, "circle-user-round", tr(app.language(), Str::kProfileLabel), fixture_label(app.fixture(), app.language()), 196, profile_cb, &app);
    settings_row(root, "refresh-cw", tr(app.language(), Str::kRefreshLabel), tr(app.language(), Str::kSimulateValue), 246, sync_cb, &app);
    settings_row(root, "party-popper", tr(app.language(), Str::kCelebrationLabel), tr(app.language(), Str::kTestValue), 296, celebration_cb, &app);
    settings_row(root, "info", tr(app.language(), Str::kAboutLabel), tr(app.language(), Str::kOpenValue), 346, go_about_cb, &app);
    page_dots(root, 3);
    return root;
}

lv_obj_t *build_about_screen(App &app) {
    lv_obj_t *root = base_screen(colors.accent_cyan);
    title_block(root, tr(app.language(), Str::kAboutLabel), "", colors.accent_cyan);
    label(root, "Trophy Display", &td_font_24, colors.text_primary, 82, 102, 302);
    label(root, "Version 1.0", &td_font_14, colors.text_secondary, 92, 130, 282);
    wrapped(root, tr(app.language(), Str::kPlayStationDataProvidedBy), &td_font_14, colors.text_secondary, 72, 176, 322, 20);
    label(root, "Pocket PSN", &td_font_20, colors.text_primary, 92, 204, 282);
    button(root, "pocketpsn.com", center_x(196), 232, 196, colors.accent_cyan, open_pocketpsn_cb, nullptr);
    label(root, tr(app.language(), Str::kDesignAndDevelopment), &td_font_12, colors.text_secondary, 74, 296, 318);
    label(root, "Kevin Torres", &td_font_16, colors.text_primary, 92, 320, 282);
    button(root, tr(app.language(), Str::kBackToSettings), center_x(180), 354, 180, colors.accent_cyan, go_settings_cb, &app);
    page_dots(root, 4);
    return root;
}

lv_obj_t *build_icon_gallery_screen(App &) {
    lv_obj_t *root = base_screen(colors.accent_cyan);
    title_block(root, "Debug UI", "icônes et typographie", colors.accent_cyan);
    label(root, "Aa 0123456789 % # / -", &td_font_24, colors.text_primary, 60, 104, 346);
    label(root, "Éé Àà Çç Synchronisation", &td_font_18, colors.text_secondary, 60, 146, 346);
    label(root, "Données enregistrées · Actualisation · Déconnexion", &td_font_14, colors.text_secondary, 62, 184, 342);
    const char *icons[] = {"wifi", "wifi-off", "refresh-cw", "triangle-alert", "settings", "gamepad-2", "podium", "clock-3"};
    for(int i = 0; i < 8; ++i) {
        lv_obj_t *box = card(root, 76 + (i % 4) * 80, 238 + (i / 4) * 58, 44, 44, 12, LV_OPA_70);
        lv_obj_t *sym = make_icon_box(box, icons[i], 32, 24, colors.accent_cyan);
        lv_obj_center(sym);
    }
    label(root, "F2 zone · F3 boîtes · F4 grille · F5 coordonnées", &td_font_10, colors.text_muted, 52, 374, 362);
    return root;
}

lv_obj_t *build_offline_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(colors.warning);
    make_status_badge(root, p.total > 0 ? tr(app.language(), Str::kDataAvailable) : tr(app.language(), Str::kOffline), colors.warning, 142, 50, "wifi-off");
    lv_obj_t *disc = card(root, center_x(104), 118, 104, 104, LV_RADIUS_CIRCLE, LV_OPA_60);
    lv_obj_set_style_bg_color(disc, lv_color_mix(colors.warning, colors.background_primary, 46), 0);
    lv_obj_set_style_border_color(disc, colors.warning, 0);
    lv_obj_t *sym = make_icon_box(disc, "wifi-off", 70, 54, colors.warning);
    lv_obj_center(sym);
    label(root, tr(app.language(), Str::kOffline), &td_font_24, colors.text_primary, 78, 236, 310);
    wrapped(root, p.offline_message.c_str(), &td_font_12, colors.text_secondary, 76, 272, 314, 52);
    button(root, tr(app.language(), Str::kBackToDashboard), kActionX, 344, kActionW, colors.warning, go_dashboard_cb, &app);
    return root;
}

lv_obj_t *build_error_screen(App &app) {
    const ProfileData &p = app.profile();
    lv_obj_t *root = base_screen(colors.error);
    make_status_badge(root, tr(app.language(), Str::kErrorBadge), colors.error, 164, 50, "triangle-alert");
    lv_obj_t *disc = card(root, center_x(104), 118, 104, 104, LV_RADIUS_CIRCLE, LV_OPA_60);
    lv_obj_set_style_bg_color(disc, lv_color_mix(colors.error, colors.background_primary, 54), 0);
    lv_obj_set_style_border_color(disc, colors.error, 0);
    lv_obj_t *sym = make_icon_box(disc, "triangle-alert", 70, 54, colors.error);
    lv_obj_center(sym);
    label(root, p.error_title.c_str(), &td_font_24, colors.text_primary, 70, 236, 326);
    wrapped(root, p.error_message.c_str(), &td_font_12, colors.text_secondary, 76, 272, 314, 54);
    button(root, tr(app.language(), Str::kBackButton), kActionX, 344, kActionW, colors.error, go_dashboard_cb, &app);
    return root;
}

} // namespace trophy
