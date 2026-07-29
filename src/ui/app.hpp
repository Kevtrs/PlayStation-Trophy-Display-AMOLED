#pragma once

#include "config/AppSettings.h"
#include "lvgl.h"
#include "ui/ui_model.hpp"

#include <cstdint>
#include <string>

namespace trophy {

enum class Page {
    Boot = 0,
    Welcome,
    Dashboard,
    Trophies,
    Statistics,
    Sync,
    Celebration,
    Settings,
    About,
    IconGallery,
    Offline,
    Error
};

constexpr int PAGE_COUNT = 12;
// Sync n'est plus dans le carrousel manuel (voir App::next_page()/
// previous_page()) -- reste affiche automatiquement pendant une vraie
// synchronisation (voir RoundUiBridge), sans y consacrer un swipe pour un
// contenu statique redondant avec le badge du Dashboard.
constexpr int PRODUCT_PAGE_COUNT = 5;

class App {
public:
    App();

    void init();
    void tick(uint32_t now_ms);
    void show_page(Page page, bool animate = true);
    void next_page();
    void previous_page();
    void activate();
    void long_press();
    void swipe_left();
    void swipe_right();
    void toggle_debug();
    void toggle_layout_debug();
    void toggle_safe_overlay();
    void toggle_box_overlay();
    void toggle_grid_overlay();
    void toggle_coordinate_overlay();
    void toggle_reduced_motion();
    void toggle_animations_enabled();
    void cycle_profile();
    void set_fixture(UiFixture fixture);
    void cycle_fixture();
    void simulate_sync();
    void simulate_error();
    void simulate_offline();
    void simulate_new_trophy(TrophyKind kind = TrophyKind::Platinum);
    void replay_boot();
    void boot_screen_set_progress(uint8_t percent);
    void boot_screen_set_status(const char *text);
    void boot_screen_finish();
    void start_showroom();
    void stop_showroom();
    void toggle_showroom();
    void adjust_progress(int delta);
    void adjust_trophies(int delta);
    void set_profile(const ProfileData &profile, bool refresh = true);
    void set_trophy_stats(const TrophyStats &stats, bool refresh = true);
    void set_sync_state(SyncState state, bool refresh = true);
    void set_network_state(NetworkState state, bool refresh = true);
    void set_last_update(const char *text, bool refresh = true);
    void set_error(const char *title, const char *message);
    void set_capture_mode(bool value) { capture_mode_ = value; }
    void set_layout_debug(bool value);

    ProfileData &profile() { return profile_; }
    const ProfileData &profile() const { return profile_; }
    Page page() const { return page_; }
    bool debug_visible() const { return debug_visible_; }
    bool layout_debug_visible() const { return layout_debug_visible_; }
    bool showroom_active() const { return showroom_active_; }
    uint8_t boot_progress() const { return boot_progress_; }
    const std::string &boot_status() const { return boot_status_; }
    UiFixture fixture() const { return fixture_; }
    bool animations_enabled() const { return !profile_.reduced_motion; }
    // Langue d'affichage (voir src/ui/strings.hpp) : source de verite unique
    // pour tous les build_*_screen() -- RoundUiBridge la synchronise depuis
    // AppSettings::language via ui_set_language() a chaque tick.
    AppLanguage language() const { return language_; }
    void set_language(AppLanguage lang) { language_ = lang; }
    // Rotation automatique Dashboard/Trophees/Statistiques (voir tick()) :
    // RoundUiBridge la synchronise depuis AppSettings::autoRotateEnabled/
    // rotationIntervalSeconds a chaque tick, comme la langue -- reglage
    // deja expose dans le portail captif ("Reglages avances") mais jamais
    // branche a un comportement reel jusqu'ici (demande utilisateur du
    // 2026-07-28, ecran petit et rond a regarder de loin sans interagir).
    void set_auto_rotation(bool enabled, uint16_t interval_seconds);
    // Index (0-3) de la sous-vue actuellement affichee en grand sur les
    // ecrans Statistiques/Trophees (voir tick() : defile automatiquement,
    // une info a la fois, pour rester lisible sur ce petit ecran rond --
    // demande utilisateur du 2026-07-28).
    int content_cycle_index() const { return content_cycle_index_; }
    // Nombre de sous-vues sur Trophees/Statistiques (Platine/Or/Argent/
    // Bronze, ou les 4 stats) : public pour rotation_slide_for() (app.cpp),
    // qui calcule la position dans la sequence lineaire de rotation
    // automatique a partir de la page et de content_cycle_index().
    static constexpr int kContentCycleCount = 4;

private:
    friend lv_obj_t *build_boot_screen(App &app);
    friend lv_obj_t *build_welcome_screen(App &app);
    friend lv_obj_t *build_dashboard_screen(App &app);
    friend lv_obj_t *build_trophies_screen(App &app);
    friend lv_obj_t *build_statistics_screen(App &app);
    friend lv_obj_t *build_sync_screen(App &app);
    friend lv_obj_t *build_celebration_screen(App &app);
    friend lv_obj_t *build_settings_screen(App &app);
    friend lv_obj_t *build_about_screen(App &app);
    friend lv_obj_t *build_icon_gallery_screen(App &app);
    friend lv_obj_t *build_offline_screen(App &app);
    friend lv_obj_t *build_error_screen(App &app);

    lv_obj_t *build(Page page);
    void update_debug();
    void ensure_debug();
    void refresh_layout_debug();
    void validate_layout(lv_obj_t *root);
    void boot_auto_advance(uint32_t now_ms);
    void tick_showroom(uint32_t now_ms);
    void apply_showroom_step(int step, bool animate);
    void close_toast();
    void update_overlay_visibility();

    ProfileData profile_;
    AppLanguage language_ = AppLanguage::kFrench;
    UiFixture fixture_ = UiFixture::Normal;
    Page page_ = Page::Boot;
    lv_obj_t *debug_panel_ = nullptr;
    lv_obj_t *debug_label_ = nullptr;
    lv_obj_t *layout_debug_overlay_ = nullptr;
    lv_obj_t *boot_progress_arc_ = nullptr;
    lv_obj_t *boot_percent_label_ = nullptr;
    uint32_t boot_started_ms_ = 0;
    uint32_t last_debug_ms_ = 0;
    int content_cycle_index_ = 0;
    // Compte les 9 "diapositives" de la rotation automatique : Dashboard(1)
    // + Trophees(kContentCycleCount) + Statistiques(kContentCycleCount).
    static constexpr int kRotationSlideCount = 1 + 2 * kContentCycleCount;
    bool auto_rotate_enabled_ = true;
    uint16_t rotation_interval_seconds_ = 10;
    uint32_t auto_rotate_started_ms_ = 0;
    uint8_t boot_progress_ = 0;
    int profile_variant_ = 0;
    int showroom_step_ = 0;
    uint32_t showroom_step_started_ms_ = 0;
    std::string boot_status_ = "Initialisation";
    bool boot_finished_ = false;
    bool debug_visible_ = false;
    bool layout_debug_visible_ = false;
    bool safe_overlay_visible_ = false;
    bool box_overlay_visible_ = false;
    bool grid_overlay_visible_ = false;
    bool coordinate_overlay_visible_ = false;
    bool capture_mode_ = false;
    bool showroom_active_ = false;
};

const char *page_name(Page page);
int page_index(Page page);
Page page_from_capture_index(int index);

} // namespace trophy
