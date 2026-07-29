#include "ui/trophy_display_ui.hpp"

#include "ui/app.hpp"

#include <algorithm>

namespace trophy {

static App g_ui_app;
static bool g_ui_initialized = false;

static Page to_app_page(UiPage page) {
    switch(page) {
        case UiPage::Boot: return Page::Boot;
        case UiPage::Welcome: return Page::Welcome;
        case UiPage::Dashboard: return Page::Dashboard;
        case UiPage::Trophies: return Page::Trophies;
        case UiPage::Statistics: return Page::Statistics;
        case UiPage::Sync: return Page::Sync;
        case UiPage::Celebration: return Page::Celebration;
        case UiPage::Settings: return Page::Settings;
        case UiPage::About: return Page::About;
        case UiPage::Offline: return Page::Offline;
        case UiPage::Error: return Page::Error;
    }
    return Page::Dashboard;
}

static UiPage to_ui_page(Page page) {
    switch(page) {
        case Page::Boot: return UiPage::Boot;
        case Page::Welcome: return UiPage::Welcome;
        case Page::Dashboard: return UiPage::Dashboard;
        case Page::Trophies: return UiPage::Trophies;
        case Page::Statistics: return UiPage::Statistics;
        case Page::Sync: return UiPage::Sync;
        case Page::Celebration: return UiPage::Celebration;
        case Page::Settings: return UiPage::Settings;
        case Page::About: return UiPage::About;
        case Page::IconGallery: return UiPage::Dashboard;  // reserve simulateur, jamais utilise produit
        case Page::Offline: return UiPage::Offline;
        case Page::Error: return UiPage::Error;
    }
    return UiPage::Dashboard;
}

void ui_init() {
    if(g_ui_initialized) return;
    g_ui_app.init();
    g_ui_initialized = true;
}

void ui_tick(uint32_t elapsed_ms) {
    if(!g_ui_initialized) return;
    lv_tick_inc(elapsed_ms);
    lv_timer_handler();
    g_ui_app.tick(lv_tick_get());
}

void ui_app_tick(uint32_t now_ms) {
    if(!g_ui_initialized) return;
    g_ui_app.tick(now_ms);
}

UiPage ui_get_page() {
    if(!g_ui_initialized) ui_init();
    return to_ui_page(g_ui_app.page());
}

void ui_swipe_left() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.swipe_left();
}

void ui_swipe_right() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.swipe_right();
}

void ui_activate() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.activate();
}

void ui_long_press() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.long_press();
}

void ui_set_profile(const ProfileData &profile) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_profile(profile);
}

void ui_set_trophy_stats(const TrophyStats &stats) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_trophy_stats(stats);
}

void ui_set_sync_state(SyncState state) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_sync_state(state);
}

void ui_set_network_state(NetworkState state) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_network_state(state);
}

void ui_set_last_update(const char *text) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_last_update(text);
}

void ui_show_page(UiPage page) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.show_page(to_app_page(page));
}

void ui_show_page_immediate(UiPage page) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.show_page(to_app_page(page), false);
}

void ui_show_new_trophy(TrophyType type, uint32_t count) {
    if(!g_ui_initialized) ui_init();
    ProfileData &profile = g_ui_app.profile();
    profile.celebration = count > 1 ? TrophyKind::Multiple : type;
    profile.celebration_count = std::max<uint32_t>(1, count);
    g_ui_app.show_page(Page::Celebration);
}

void ui_show_error(const char *title, const char *message) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_error(title, message);
}

void ui_show_offline() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_network_state(NetworkState::Offline, false);
    g_ui_app.show_page(Page::Offline);
}

void ui_set_brightness(uint8_t value) {
    if(!g_ui_initialized) ui_init();
    const int next = std::clamp<int>(value, 0, 100);
    if(g_ui_app.profile().brightness == next) return;
    g_ui_app.profile().brightness = next;
    g_ui_app.show_page(g_ui_app.page(), false);
}

void ui_set_animations_enabled(bool enabled) {
    if(!g_ui_initialized) ui_init();
    const bool reduced = !enabled;
    if(g_ui_app.profile().reduced_motion == reduced) return;
    g_ui_app.profile().reduced_motion = reduced;
    g_ui_app.show_page(g_ui_app.page(), false);
}

void ui_set_language(AppLanguage lang) {
    if(!g_ui_initialized) ui_init();
    if(g_ui_app.language() == lang) return;
    g_ui_app.set_language(lang);
    g_ui_app.show_page(g_ui_app.page(), false);
}

void ui_set_auto_rotation(bool enabled, uint16_t interval_seconds) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.set_auto_rotation(enabled, interval_seconds);
}

ProfileData ui_get_profile() {
    if(!g_ui_initialized) ui_init();
    return g_ui_app.profile();
}

void ui_boot_set_progress(uint8_t percent) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.boot_screen_set_progress(percent);
}

void ui_boot_set_status(const char *text) {
    if(!g_ui_initialized) ui_init();
    g_ui_app.boot_screen_set_status(text);
}

void ui_boot_finish() {
    if(!g_ui_initialized) ui_init();
    g_ui_app.boot_screen_finish();
}

uint8_t ui_boot_progress() {
    if(!g_ui_initialized) ui_init();
    return g_ui_app.boot_progress();
}

} // namespace trophy
