#include "ui/app.hpp"

#include "screens/screens.hpp"
#include "theme/theme.hpp"
#include "ui/strings.hpp"
#include "ui/ui_fixtures.hpp"
#include "ui/layout.hpp"
#include "widgets/widgets.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace trophy {

// Position (0..kRotationSlideCount-1) de l'ecran/sous-vue courant dans la
// sequence lineaire de rotation automatique (voir App::tick()) : slide 0 =
// Dashboard, slides 1-4 = Trophees (Platine/Or/Argent/Bronze), slides 5-8 =
// Statistiques (les 4 stats).
static int rotation_slide_for(Page page, int content_index) {
    if(page == Page::Trophies) return 1 + content_index;
    if(page == Page::Statistics) return 1 + App::kContentCycleCount + content_index;
    return 0;
}

static void goto_dashboard_cb(lv_event_t *e) {
    auto *app = static_cast<App *>(lv_event_get_user_data(e));
    if(app) app->show_page(Page::Dashboard);
}

App::App() = default;

static void clear_obj_base(lv_obj_t *obj) {
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(obj, 0, 0);
    lv_obj_set_style_pad_all(obj, 0, 0);
}

static void add_debug_box(lv_obj_t *overlay, const lv_area_t &area, lv_color_t color, lv_opa_t opa = LV_OPA_70) {
    const int w = std::max(1, static_cast<int>(lv_area_get_width(&area)));
    const int h = std::max(1, static_cast<int>(lv_area_get_height(&area)));
    lv_obj_t *box = lv_obj_create(overlay);
    clear_obj_base(box);
    lv_obj_set_pos(box, area.x1, area.y1);
    lv_obj_set_size(box, w, h);
    lv_obj_set_style_border_width(box, 1, 0);
    lv_obj_set_style_border_color(box, color, 0);
    lv_obj_set_style_border_opa(box, opa, 0);
}

static void add_debug_hline(lv_obj_t *overlay, int y, lv_color_t color, lv_opa_t opa = LV_OPA_60) {
    lv_obj_t *line = lv_obj_create(overlay);
    clear_obj_base(line);
    lv_obj_set_pos(line, EDGE_MARGIN, y);
    lv_obj_set_size(line, DISPLAY_WIDTH - EDGE_MARGIN * 2, 1);
    lv_obj_set_style_bg_color(line, color, 0);
    lv_obj_set_style_bg_opa(line, opa, 0);
}

static void add_debug_text(lv_obj_t *overlay, const char *text, int x, int y, lv_color_t color) {
    lv_obj_t *label = lv_label_create(overlay);
    lv_obj_add_style(label, &style_debug, 0);
    lv_obj_set_style_bg_opa(label, LV_OPA_80, 0);
    lv_obj_set_style_pad_all(label, 2, 0);
    lv_obj_set_style_text_color(label, color, 0);
    lv_obj_set_pos(label, x, y);
    lv_label_set_text(label, text);
}

static bool outside_screen(const lv_area_t &area) {
    return area.x1 < 0 || area.y1 < 0 || area.x2 >= DISPLAY_WIDTH || area.y2 >= DISPLAY_HEIGHT;
}

static bool outside_safe_corner(const lv_area_t &area) {
    return !inside_circle(area.x1, area.y1, SAFE_RADIUS) ||
           !inside_circle(area.x2, area.y1, SAFE_RADIUS) ||
           !inside_circle(area.x1, area.y2, SAFE_RADIUS) ||
           !inside_circle(area.x2, area.y2, SAFE_RADIUS);
}

static bool same_profile(const ProfileData &a, const ProfileData &b) {
    return a.username == b.username &&
           a.level == b.level &&
           a.progress == b.progress &&
           a.total == b.total &&
           a.platinum == b.platinum &&
           a.gold == b.gold &&
           a.silver == b.silver &&
           a.bronze == b.bronze &&
           a.games_completed == b.games_completed &&
           a.completion == b.completion &&
           a.world_rank == b.world_rank &&
           a.play_time == b.play_time &&
           a.updated == b.updated &&
           a.offline_message == b.offline_message &&
           a.error_title == b.error_title &&
           a.error_message == b.error_message &&
           a.offline == b.offline &&
           a.sync == b.sync &&
           a.celebration == b.celebration &&
           a.celebration_count == b.celebration_count &&
           a.trophy_feed_available == b.trophy_feed_available &&
           a.reduced_motion == b.reduced_motion &&
           a.brightness == b.brightness;
}

static void draw_child_boxes(lv_obj_t *overlay, lv_obj_t *obj, int depth = 0) {
    const uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; ++i) {
        lv_obj_t *child = lv_obj_get_child(obj, static_cast<int32_t>(i));
        if(!child || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        lv_area_t area;
        lv_obj_get_coords(child, &area);
        const lv_color_t color = depth < 1 ? colors.accent_cyan : (depth < 3 ? colors.accent_violet : colors.warning);
        add_debug_box(overlay, area, color, depth < 1 ? LV_OPA_60 : LV_OPA_40);
        draw_child_boxes(overlay, child, depth + 1);
    }
}

static void draw_child_coordinates(lv_obj_t *overlay, lv_obj_t *obj, int depth = 0) {
    const uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; ++i) {
        lv_obj_t *child = lv_obj_get_child(obj, static_cast<int32_t>(i));
        if(!child || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        lv_area_t area;
        lv_obj_get_coords(child, &area);
        const int w = static_cast<int>(lv_area_get_width(&area));
        const int h = static_cast<int>(lv_area_get_height(&area));
        if(w > 16 && h > 8 && depth <= 2) {
            char buf[48];
            std::snprintf(buf, sizeof(buf), "%d,%d %dx%d", area.x1, area.y1, w, h);
            add_debug_text(overlay, buf, std::max(2, static_cast<int>(area.x1)),
                           std::max(2, static_cast<int>(area.y1) - 11), colors.text_primary);
        }
        draw_child_coordinates(overlay, child, depth + 1);
    }
}

static void validate_child_layout(lv_obj_t *obj, const char *page_name, int depth = 0) {
    const uint32_t count = lv_obj_get_child_count(obj);
    for(uint32_t i = 0; i < count; ++i) {
        lv_obj_t *child = lv_obj_get_child(obj, static_cast<int32_t>(i));
        if(!child || lv_obj_has_flag(child, LV_OBJ_FLAG_HIDDEN)) continue;
        lv_area_t area;
        lv_obj_get_coords(child, &area);
        const int w = static_cast<int>(lv_area_get_width(&area));
        const int h = static_cast<int>(lv_area_get_height(&area));
        if(w <= 1 || h <= 1) {
            validate_child_layout(child, page_name, depth + 1);
            continue;
        }
        if(outside_screen(area)) {
            std::printf("[layout] %s object depth=%d outside screen: x=%d y=%d w=%d h=%d\n",
                        page_name, depth, area.x1, area.y1, w, h);
        }
        const bool compact_content = w <= 220 && h <= 140 && area.y1 >= CONTENT_TOP && area.y1 < PAGE_INDICATOR_Y - 4;
        if(compact_content && outside_safe_corner(area)) {
            std::printf("[layout] %s compact object depth=%d outside safe radius: x=%d y=%d w=%d h=%d\n",
                        page_name, depth, area.x1, area.y1, w, h);
        }
        validate_child_layout(child, page_name, depth + 1);
    }
}

void App::init() {
    init_theme();
    fixture_ = UiFixture::Normal;
    profile_ = profile_for_fixture(fixture_, language_);
    boot_progress_ = 0;
    boot_status_ = tr(language_, Str::kBootInitializing);
    boot_finished_ = false;
    boot_started_ms_ = lv_tick_get();
    show_page(Page::Boot, false);
}

void App::tick(uint32_t now_ms) {
    if(showroom_active_) tick_showroom(now_ms);
    if(page_ == Page::Boot && !capture_mode_ && !showroom_active_) boot_auto_advance(now_ms);
    // Rotation automatique : une seule sequence lineaire de 9 "diapositives"
    // (Dashboard, puis Platine/Or/Argent/Bronze sur Trophees, puis les 4
    // stats sur Statistiques), toutes affichees exactement la meme duree
    // (rotation_interval_seconds_, voir set_auto_rotation()) -- avant cette
    // correction, Dashboard restait affiche tout l'intervalle configure
    // pendant que chaque sous-vue de Trophees/Statistiques ne durait que 3 s
    // fixes, sans rapport avec ce reglage (incoherent, signale par
    // l'utilisateur le 2026-07-28). Uniquement entre ces 3 ecrans "passifs"
    // a regarder de loin -- jamais vers Reglages/A propos (menus qu'on doit
    // atteindre deliberement) ni pendant Sync/Celebration/Hors ligne/Erreur
    // (etats pilotes par de vrais evenements, voir RoundUiBridge). Desactive
    // pendant le showroom (qui pilote deja ses propres transitions).
    if(!showroom_active_ && auto_rotate_enabled_ &&
       (page_ == Page::Dashboard || page_ == Page::Trophies || page_ == Page::Statistics) &&
       now_ms - auto_rotate_started_ms_ >= static_cast<uint32_t>(rotation_interval_seconds_) * 1000u) {
        auto_rotate_started_ms_ = now_ms;
        const int current_slide = rotation_slide_for(page_, content_cycle_index_);
        const int next_slide = (current_slide + 1) % kRotationSlideCount;
        Page next_page = Page::Dashboard;
        int next_content_index = 0;
        if(next_slide == 0) {
            next_page = Page::Dashboard;
        } else if(next_slide <= kContentCycleCount) {
            next_page = Page::Trophies;
            next_content_index = next_slide - 1;
        } else {
            next_page = Page::Statistics;
            next_content_index = next_slide - 1 - kContentCycleCount;
        }
        if(next_page == page_) {
            // Meme ecran, sous-vue suivante (ex: Or -> Argent) : pas
            // d'animation, comme un simple rafraichissement de contenu.
            content_cycle_index_ = next_content_index;
            show_page(page_, false);
        } else {
            // content_cycle_index_ est remis a 0 par show_page() en entrant
            // sur Trophees/Statistiques, ce qui correspond deja a
            // next_content_index (toujours 0 au premier slide d'un ecran).
            show_page(next_page);
        }
    }
    if((debug_visible_ || layout_debug_visible_) && now_ms - last_debug_ms_ > 250) {
        if(debug_visible_) update_debug();
        if(layout_debug_visible_) refresh_layout_debug();
        last_debug_ms_ = now_ms;
    }
}

void App::set_auto_rotation(bool enabled, uint16_t interval_seconds) {
    const bool was_enabled = auto_rotate_enabled_;
    auto_rotate_enabled_ = enabled;
    rotation_interval_seconds_ = interval_seconds > 0 ? interval_seconds : 10;
    // Repart d'un delai complet des l'activation (pas de saut immediat sur
    // le premier tick si le reglage vient d'etre allume, ou si l'intervalle
    // vient de changer).
    if(enabled && !was_enabled) auto_rotate_started_ms_ = lv_tick_get();
}

void App::boot_auto_advance(uint32_t now_ms) {
    if(boot_started_ms_ == 0) boot_started_ms_ = now_ms;
    const uint32_t elapsed = now_ms - boot_started_ms_;
    constexpr uint32_t progress_ms = 2350;
    const uint8_t percent = static_cast<uint8_t>(std::clamp<uint32_t>(elapsed * 100 / progress_ms, 0, 100));
    boot_screen_set_progress(percent);
    if(elapsed > 720 && elapsed <= 1720) {
        boot_screen_set_status(tr(language_, Str::kBootLoadingProfile));
    } else if(elapsed > 1720) {
        boot_screen_set_status(tr(language_, Str::kReady));
    }
    if(elapsed > 2550 || boot_finished_) {
        boot_screen_finish();
    }
}

lv_obj_t *App::build(Page page) {
    switch(page) {
        case Page::Boot: return build_boot_screen(*this);
        case Page::Welcome: return build_welcome_screen(*this);
        case Page::Dashboard: return build_dashboard_screen(*this);
        case Page::Trophies: return build_trophies_screen(*this);
        case Page::Statistics: return build_statistics_screen(*this);
        case Page::Sync: return build_sync_screen(*this);
        case Page::Celebration: return build_celebration_screen(*this);
        case Page::Settings: return build_settings_screen(*this);
        case Page::About: return build_about_screen(*this);
        case Page::IconGallery: return build_icon_gallery_screen(*this);
        case Page::Offline: return build_offline_screen(*this);
        case Page::Error: return build_error_screen(*this);
    }
    return build_dashboard_screen(*this);
}

void App::show_page(Page page, bool animate) {
    // Reinitialise le defilement de sous-vues uniquement en ENTRANT sur
    // Statistiques/Trophees (pas a chaque rebuild periodique du meme ecran
    // depuis tick(), qui appelle show_page(page_, ...) avec la meme page --
    // sinon l'index avancerait puis serait aussitot efface). Correspond
    // toujours au premier slide de la sequence de rotation pour cet ecran
    // (voir rotation_slide_for()), donc pas de resynchronisation supplementaire
    // necessaire ici.
    if(page != page_ && (page == Page::Statistics || page == Page::Trophies)) {
        content_cycle_index_ = 0;
    }
    // Repart d'un delai complet a chaque entree sur une des 3 pages de la
    // rotation automatique (que ce soit par un swipe manuel ou par la
    // rotation elle-meme) : sans ca, revenir sur Dashboard juste avant
    // l'echeance ferait basculer trop vite vers la page suivante.
    if(page != page_ && (page == Page::Dashboard || page == Page::Trophies || page == Page::Statistics)) {
        auto_rotate_started_ms_ = lv_tick_get();
    }
    page_ = page;
    lv_obj_t *previous = lv_scr_act();
    if(page != Page::Boot) {
        boot_progress_arc_ = nullptr;
        boot_percent_label_ = nullptr;
    } else {
        boot_progress_ = 0;
        boot_status_ = tr(language_, Str::kBootInitializing);
        boot_finished_ = false;
        boot_started_ms_ = lv_tick_get();
    }
    lv_obj_t *screen = build(page);
    if(animate && !profile_.reduced_motion) {
        lv_scr_load_anim(screen, LV_SCR_LOAD_ANIM_FADE_IN, 220, 0, true);
    } else {
        lv_scr_load(screen);
        if(previous && previous != screen) {
            lv_obj_delete(previous);
        }
    }
    ensure_debug();
    update_debug();
    validate_layout(screen);
    refresh_layout_debug();
}

void App::replay_boot() {
    stop_showroom();
    boot_progress_ = 0;
    boot_status_ = tr(language_, Str::kBootInitializing);
    boot_finished_ = false;
    boot_started_ms_ = lv_tick_get();
    show_page(Page::Boot, false);
}

void App::boot_screen_set_progress(uint8_t percent) {
    const uint8_t next = static_cast<uint8_t>(std::min<int>(percent, 100));
    boot_progress_ = next;
    if(boot_progress_arc_) {
        lv_arc_set_value(boot_progress_arc_, next);
    }
    if(boot_percent_label_) {
        char buf[8];
        std::snprintf(buf, sizeof(buf), "%u %%", static_cast<unsigned>(next));
        lv_label_set_text(boot_percent_label_, buf);
    }
}

void App::boot_screen_set_status(const char *text) {
    boot_status_ = text ? text : "";
}

void App::boot_screen_finish() {
    boot_finished_ = true;
    boot_screen_set_progress(100);
    if(page_ == Page::Boot) {
        show_page(Page::Welcome, false);
    }
}

void App::next_page() {
    if(showroom_active_) stop_showroom();
    switch(page_) {
        case Page::Dashboard: show_page(Page::Trophies); break;
        case Page::Trophies: show_page(Page::Statistics); break;
        case Page::Statistics: show_page(Page::Settings); break;
        // Sync n'est plus dans le carrousel manuel (voir PRODUCT_PAGE_COUNT) --
        // si l'utilisateur swipe alors qu'une synchronisation reelle l'a
        // affiche automatiquement, on revient simplement au Dashboard.
        case Page::Sync: show_page(Page::Dashboard); break;
        case Page::Settings: show_page(Page::About); break;
        case Page::About: show_page(Page::Dashboard); break;
        default: show_page(Page::Dashboard); break;
    }
}

void App::previous_page() {
    if(showroom_active_) stop_showroom();
    switch(page_) {
        case Page::Dashboard: show_page(Page::About); break;
        case Page::Trophies: show_page(Page::Dashboard); break;
        case Page::Statistics: show_page(Page::Trophies); break;
        // Voir next_page() : meme motif, Sync n'est plus dans le carrousel.
        case Page::Sync: show_page(Page::Dashboard); break;
        case Page::Settings: show_page(Page::Statistics); break;
        case Page::About: show_page(Page::Settings); break;
        default: show_page(Page::Dashboard); break;
    }
}

void App::activate() {
    if(showroom_active_) stop_showroom();
    if(page_ == Page::Welcome) show_page(Page::Dashboard);
    else if(page_ == Page::Dashboard) simulate_sync();
    else if(page_ == Page::Sync) show_page(Page::Dashboard);
    else if(page_ == Page::Celebration) show_page(Page::Dashboard);
    else if(page_ == Page::About) show_page(Page::Settings);
    else if(page_ == Page::Offline || page_ == Page::Error) show_page(Page::Dashboard);
}

void App::long_press() {
    if(showroom_active_) stop_showroom();
    show_page(Page::Settings);
}

void App::swipe_left() {
    next_page();
}

void App::swipe_right() {
    previous_page();
}

void App::toggle_debug() {
    debug_visible_ = !debug_visible_;
    ensure_debug();
    set_obj_hidden(debug_panel_, !debug_visible_);
    update_debug();
}

void App::toggle_layout_debug() {
    const bool next = !layout_debug_visible_;
    safe_overlay_visible_ = next;
    box_overlay_visible_ = next;
    grid_overlay_visible_ = next;
    coordinate_overlay_visible_ = next;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::set_layout_debug(bool value) {
    safe_overlay_visible_ = value;
    box_overlay_visible_ = value;
    grid_overlay_visible_ = value;
    coordinate_overlay_visible_ = value;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::toggle_safe_overlay() {
    safe_overlay_visible_ = !safe_overlay_visible_;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::toggle_box_overlay() {
    box_overlay_visible_ = !box_overlay_visible_;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::toggle_grid_overlay() {
    grid_overlay_visible_ = !grid_overlay_visible_;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::toggle_coordinate_overlay() {
    coordinate_overlay_visible_ = !coordinate_overlay_visible_;
    update_overlay_visibility();
    refresh_layout_debug();
}

void App::update_overlay_visibility() {
    layout_debug_visible_ = safe_overlay_visible_ || box_overlay_visible_ || grid_overlay_visible_ || coordinate_overlay_visible_;
}

void App::toggle_reduced_motion() {
    toggle_animations_enabled();
}

void App::toggle_animations_enabled() {
    profile_.reduced_motion = !profile_.reduced_motion;
    show_page(page_, false);
}

void App::set_fixture(UiFixture fixture) {
    fixture_ = fixture;
    const bool keep_motion = profile_.reduced_motion;
    profile_ = profile_for_fixture(fixture_, language_);
    profile_.reduced_motion = keep_motion;
    show_page(page_, false);
}

void App::cycle_fixture() {
    const int next = (fixture_index(fixture_) + 1) % 7;
    set_fixture(fixture_from_index(next));
}

void App::cycle_profile() {
    cycle_fixture();
}

void App::simulate_sync() {
    if(showroom_active_) stop_showroom();
    profile_.sync = SyncState::Fetching;
    show_page(Page::Sync);
}

void App::simulate_error() {
    if(showroom_active_) stop_showroom();
    profile_.sync = SyncState::Error;
    show_page(Page::Error);
}

void App::simulate_offline() {
    if(showroom_active_) stop_showroom();
    profile_.offline = true;
    show_page(Page::Offline);
}

void App::simulate_new_trophy(TrophyKind kind) {
    if(showroom_active_) stop_showroom();
    profile_.celebration = kind;
    profile_.celebration_count = kind == TrophyKind::Multiple ? 7 : 1;
    switch(kind) {
        case TrophyKind::Platinum: profile_.platinum++; profile_.total++; break;
        case TrophyKind::Gold: profile_.gold++; profile_.total++; break;
        case TrophyKind::Silver: profile_.silver++; profile_.total++; break;
        case TrophyKind::Bronze: profile_.bronze++; profile_.total++; break;
        case TrophyKind::Multiple:
            profile_.gold += 1;
            profile_.silver += 2;
            profile_.bronze += 4;
            profile_.total += 7;
            break;
    }
    show_page(Page::Celebration);
}

void App::start_showroom() {
    showroom_active_ = true;
    showroom_step_ = 0;
    showroom_step_started_ms_ = lv_tick_get();
    apply_showroom_step(showroom_step_, false);
}

void App::stop_showroom() {
    showroom_active_ = false;
}

void App::toggle_showroom() {
    if(showroom_active_) stop_showroom();
    else start_showroom();
}

void App::tick_showroom(uint32_t now_ms) {
    constexpr uint32_t durations[] = {
        1500, 1500, 1300, 1300, 1700, 1300, 1300,
        1400, 1300, 1500, 1500, 1300, 1900
    };
    constexpr int step_count = static_cast<int>(sizeof(durations) / sizeof(durations[0]));
    if(showroom_step_started_ms_ == 0) showroom_step_started_ms_ = now_ms;
    const uint32_t elapsed = now_ms - showroom_step_started_ms_;
    if(page_ == Page::Boot) {
        const uint8_t percent = static_cast<uint8_t>(std::clamp<uint32_t>(elapsed * 100 / durations[showroom_step_], 0, 100));
        boot_screen_set_progress(percent);
    }
    if(elapsed < durations[showroom_step_]) return;
    showroom_step_ = (showroom_step_ + 1) % step_count;
    showroom_step_started_ms_ = now_ms;
    apply_showroom_step(showroom_step_, true);
}

void App::apply_showroom_step(int step, bool animate) {
    ProfileData p;
    p.username = "Kevin_Trophies";
    p.level = 327;
    p.progress = 72;
    p.total = 4286;
    p.platinum = 58;
    p.gold = 214;
    p.silver = 876;
    p.bronze = 3138;
    p.games_completed = 142;
    p.completion = 78;
    p.world_rank = "#12 483";
    p.play_time = "3 426 h";
    p.updated = tr(language_, Str::kFixtureNormalUpdated);
    p.brightness = profile_.brightness;
    p.reduced_motion = false;

    switch(step) {
        case 0:
            profile_ = p;
            show_page(Page::Boot, animate);
            break;
        case 1:
            profile_ = p;
            show_page(Page::Welcome, animate);
            break;
        case 2:
            p.sync = SyncState::Connecting;
            p.updated = tr(language_, Str::kShowroomConnecting);
            profile_ = p;
            show_page(Page::Sync, animate);
            break;
        case 3:
            p.sync = SyncState::Processing;
            p.updated = tr(language_, Str::kShowroomProcessing);
            profile_ = p;
            show_page(Page::Sync, animate);
            break;
        case 4:
            p.sync = SyncState::Done;
            p.updated = tr(language_, Str::kShowroomSyncedJustNow);
            profile_ = p;
            show_page(Page::Dashboard, animate);
            break;
        case 5:
            profile_ = p;
            show_page(Page::Trophies, animate);
            break;
        case 6:
            profile_ = p;
            show_page(Page::Statistics, animate);
            break;
        case 7:
            p.offline = true;
            p.offline_message = tr(language_, Str::kShowroomLocalDataKept);
            profile_ = p;
            show_page(Page::Offline, animate);
            break;
        case 8:
            p.sync = SyncState::Done;
            p.updated = tr(language_, Str::kShowroomReconnectedJustNow);
            profile_ = p;
            show_page(Page::Sync, animate);
            break;
        case 9:
            p.sync = SyncState::Error;
            p.error_title = tr(language_, Str::kShowroomServiceUnavailable);
            p.error_message = tr(language_, Str::kShowroomPocketPsnNotResponding);
            profile_ = p;
            show_page(Page::Error, animate);
            break;
        case 10:
            p.celebration = TrophyKind::Platinum;
            p.celebration_count = 1;
            p.platinum += 1;
            p.total += 1;
            profile_ = p;
            show_page(Page::Celebration, animate);
            break;
        case 11:
            profile_ = p;
            show_page(Page::Settings, animate);
            break;
        default:
            profile_ = p;
            show_page(Page::About, animate);
            break;
    }
}

void App::adjust_progress(int delta) {
    profile_.progress = std::clamp(profile_.progress + delta, 0, 100);
    show_page(page_, false);
}

void App::adjust_trophies(int delta) {
    profile_.bronze = std::max(0, profile_.bronze + delta);
    profile_.total = std::max(0, profile_.platinum + profile_.gold + profile_.silver + profile_.bronze);
    show_page(page_, false);
}

void App::set_profile(const ProfileData &profile, bool refresh) {
    if(same_profile(profile_, profile)) return;
    profile_ = profile;
    if(refresh) show_page(page_, false);
}

void App::set_trophy_stats(const TrophyStats &stats, bool refresh) {
    if(profile_.total == stats.total &&
       profile_.platinum == stats.platinum &&
       profile_.gold == stats.gold &&
       profile_.silver == stats.silver &&
       profile_.bronze == stats.bronze) {
        return;
    }
    profile_.total = stats.total;
    profile_.platinum = stats.platinum;
    profile_.gold = stats.gold;
    profile_.silver = stats.silver;
    profile_.bronze = stats.bronze;
    if(refresh) show_page(page_, false);
}

void App::set_sync_state(SyncState state, bool refresh) {
    if(profile_.sync == state) return;
    profile_.sync = state;
    if(refresh) show_page(page_, false);
}

void App::set_network_state(NetworkState state, bool refresh) {
    const bool offline = state == NetworkState::Offline;
    if(profile_.offline == offline) return;
    profile_.offline = offline;
    if(refresh) show_page(page_, false);
}

void App::set_last_update(const char *text, bool refresh) {
    const std::string next = text ? text : "";
    if(profile_.updated == next) return;
    profile_.updated = next;
    if(refresh) show_page(page_, false);
}

void App::set_error(const char *title, const char *message) {
    profile_.error_title = title ? title : "";
    profile_.error_message = message ? message : "";
    profile_.sync = SyncState::Error;
    show_page(Page::Error);
}

void App::ensure_debug() {
    if(debug_panel_) return;
    debug_panel_ = lv_obj_create(lv_layer_top());
    lv_obj_add_style(debug_panel_, &style_debug, 0);
    lv_obj_clear_flag(debug_panel_, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(debug_panel_, 178, 78);
    lv_obj_align(debug_panel_, LV_ALIGN_TOP_MID, 0, 8);
    debug_label_ = lv_label_create(debug_panel_);
    lv_obj_add_style(debug_label_, &style_debug, 0);
    lv_obj_set_style_bg_opa(debug_label_, LV_OPA_TRANSP, 0);
    lv_obj_set_width(debug_label_, 160);
    lv_obj_set_pos(debug_label_, 6, 4);
    set_obj_hidden(debug_panel_, !debug_visible_);
}

void App::update_debug() {
    if(!debug_label_) return;
    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "%s | %s | fps cible 60\nlvl %d  %d%%  total %s\nP/O/A/B %d/%d/%d/%d\n1-9 pages  N/E/L/H fixtures",
                  showroom_active_ ? "Showroom" : page_name(page_), fixture_label(fixture_, language_),
                  profile_.level, profile_.progress, format_number(profile_.total).c_str(),
                  profile_.platinum, profile_.gold, profile_.silver, profile_.bronze);
    lv_label_set_text(debug_label_, buf);
}

void App::refresh_layout_debug() {
    if(layout_debug_overlay_) {
        lv_obj_delete(layout_debug_overlay_);
        layout_debug_overlay_ = nullptr;
    }
    if(!layout_debug_visible_) return;

    lv_obj_update_layout(lv_scr_act());
    layout_debug_overlay_ = lv_obj_create(lv_layer_top());
    clear_obj_base(layout_debug_overlay_);
    lv_obj_set_size(layout_debug_overlay_, DISPLAY_WIDTH, DISPLAY_HEIGHT);
    lv_obj_set_pos(layout_debug_overlay_, 0, 0);

    if(safe_overlay_visible_) {
        lv_obj_t *physical = lv_obj_create(layout_debug_overlay_);
        clear_obj_base(physical);
        lv_obj_set_size(physical, DISPLAY_RADIUS * 2, DISPLAY_RADIUS * 2);
        lv_obj_set_pos(physical, 0, 0);
        lv_obj_set_style_radius(physical, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(physical, 1, 0);
        lv_obj_set_style_border_color(physical, colors.accent_blue, 0);
        lv_obj_set_style_border_opa(physical, LV_OPA_70, 0);

        lv_obj_t *safe = lv_obj_create(layout_debug_overlay_);
        clear_obj_base(safe);
        lv_obj_set_size(safe, SAFE_RADIUS * 2, SAFE_RADIUS * 2);
        lv_obj_set_pos(safe, DISPLAY_CENTER_X - SAFE_RADIUS, DISPLAY_CENTER_Y - SAFE_RADIUS);
        lv_obj_set_style_radius(safe, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(safe, 1, 0);
        lv_obj_set_style_border_color(safe, colors.success, 0);
        lv_obj_set_style_border_opa(safe, LV_OPA_70, 0);
    }

    if(grid_overlay_visible_) {
        for(int x = EDGE_MARGIN; x <= DISPLAY_WIDTH - EDGE_MARGIN; x += 32) {
            lv_obj_t *line = lv_obj_create(layout_debug_overlay_);
            clear_obj_base(line);
            lv_obj_set_pos(line, x, EDGE_MARGIN);
            lv_obj_set_size(line, 1, DISPLAY_HEIGHT - EDGE_MARGIN * 2);
            lv_obj_set_style_bg_color(line, colors.text_muted, 0);
            lv_obj_set_style_bg_opa(line, LV_OPA_20, 0);
        }
        add_debug_hline(layout_debug_overlay_, TOP_SAFE_Y, colors.success, LV_OPA_50);
        add_debug_hline(layout_debug_overlay_, CONTENT_TOP, colors.accent_blue, LV_OPA_50);
        add_debug_hline(layout_debug_overlay_, CONTENT_BOTTOM, colors.accent_blue, LV_OPA_50);
        add_debug_hline(layout_debug_overlay_, PAGE_INDICATOR_Y, colors.warning, LV_OPA_60);
        add_debug_hline(layout_debug_overlay_, BOTTOM_SAFE_Y, colors.success, LV_OPA_50);
    }

    if(safe_overlay_visible_ || grid_overlay_visible_) {
        lv_obj_t *v = lv_obj_create(layout_debug_overlay_);
        clear_obj_base(v);
        lv_obj_set_pos(v, DISPLAY_CENTER_X, EDGE_MARGIN);
        lv_obj_set_size(v, 1, DISPLAY_HEIGHT - EDGE_MARGIN * 2);
        lv_obj_set_style_bg_color(v, colors.error, 0);
        lv_obj_set_style_bg_opa(v, LV_OPA_70, 0);

        lv_obj_t *h = lv_obj_create(layout_debug_overlay_);
        clear_obj_base(h);
        lv_obj_set_pos(h, EDGE_MARGIN, DISPLAY_CENTER_Y);
        lv_obj_set_size(h, DISPLAY_WIDTH - EDGE_MARGIN * 2, 1);
        lv_obj_set_style_bg_color(h, colors.error, 0);
        lv_obj_set_style_bg_opa(h, LV_OPA_70, 0);
    }

    if(box_overlay_visible_) draw_child_boxes(layout_debug_overlay_, lv_scr_act());
    if(coordinate_overlay_visible_) draw_child_coordinates(layout_debug_overlay_, lv_scr_act());
}

void App::validate_layout(lv_obj_t *root) {
    if(!root) return;
    if(!layout_debug_visible_) return;
    lv_obj_update_layout(root);
    validate_child_layout(root, page_name(page_));
}

void App::close_toast() {
    profile_.sync = SyncState::Idle;
}

const char *page_name(Page page) {
    switch(page) {
        case Page::Boot: return "Boot";
        case Page::Welcome: return "Bienvenue";
        case Page::Dashboard: return "Dashboard";
        case Page::Trophies: return "Trophées";
        case Page::Statistics: return "Stats";
        case Page::Sync: return "Sync";
        case Page::Celebration: return "Célébration";
        case Page::Settings: return "Réglages";
        case Page::About: return "À propos";
        case Page::IconGallery: return "Lucide";
        case Page::Offline: return "Hors ligne";
        case Page::Error: return "Erreur";
    }
    return "Page";
}

int page_index(Page page) {
    return static_cast<int>(page);
}

Page page_from_capture_index(int index) {
    switch(index) {
        case 1: return Page::Boot;
        case 2: return Page::Welcome;
        case 3: return Page::Dashboard;
        case 4: return Page::Trophies;
        case 5: return Page::Statistics;
        case 6: return Page::Sync;
        case 7: return Page::Celebration;
        case 8: return Page::Celebration;
        case 9: return Page::Settings;
        case 10: return Page::Offline;
        case 11: return Page::Error;
        case 12: return Page::About;
        case 13: return Page::Celebration;
        case 14: return Page::Celebration;
        case 16: return Page::IconGallery;
        default: return Page::Dashboard;
    }
}

} // namespace trophy
