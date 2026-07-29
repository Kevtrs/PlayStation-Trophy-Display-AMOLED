#pragma once

#include "config/AppSettings.h"
#include "ui/ui_model.hpp"

#include <cstdint>

namespace trophy {

enum class UiPage {
    Boot,
    Welcome,
    Dashboard,
    Trophies,
    Statistics,
    Sync,
    Celebration,
    Settings,
    About,
    Offline,
    Error
};

void ui_init();
void ui_tick(uint32_t elapsed_ms);

// Extension ajoutee lors de l'integration firmware (voir HANDOFF_DESIGN.md :
// "Toute mise a jour doit passer par cette facade ou par des extensions
// equivalentes ajoutees dans trophy_display_ui.*") : le projet cible pilote
// deja lv_tick_inc()/lv_timer_handler() lui-meme (tick materiel precis cote
// ESP32-S3, voir src/main.cpp) -- appeler ui_tick() en plus ferait un double
// tick LVGL (risque explicitement signale dans les "risques de
// fusion"). ui_app_tick() n'avance que la logique applicative de l'ecran
// (boot auto-advance, showroom, debug overlay), jamais LVGL lui-meme.
void ui_app_tick(uint32_t now_ms);

void ui_set_profile(const ProfileData &profile);
void ui_set_trophy_stats(const TrophyStats &stats);
void ui_set_sync_state(SyncState state);
void ui_set_network_state(NetworkState state);
void ui_set_last_update(const char *text);

void ui_show_page(UiPage page);

// Extension (meme justification que ui_app_tick ci-dessus) : bascule de page
// sans l'animation de fondu par defaut (220 ms). Reservee aux transitions
// automatiques (etat reseau/synchronisation), jamais aux gestes utilisateur
// -- voir RoundUiBridge, seul appelant. Evite un bug reel trouve le
// 2026-07-22 : si une deuxieme transition survient avant la fin de
// l'animation de la premiere (lv_timer_handler() pas encore repasse), l'etat
// interne de transition d'ecran de LVGL se corrompt et un ecran ulterieur
// supprime declenche un crash.
void ui_show_page_immediate(UiPage page);

void ui_show_new_trophy(TrophyType type, uint32_t count = 1);
void ui_show_error(const char *title, const char *message);
void ui_show_offline();

void ui_set_brightness(uint8_t value);
void ui_set_animations_enabled(bool enabled);
void ui_set_language(AppLanguage lang);
void ui_set_auto_rotation(bool enabled, uint16_t interval_seconds);

ProfileData ui_get_profile();
UiPage ui_get_page();

// Passerelle navigation/interaction (extension, meme justification que
// ui_app_tick ci-dessus) : le firmware/simulateur pilote son propre tactile
// reel (voir src/main.cpp / simulator/src/main.cpp) et doit pouvoir
// declencher exactement les memes transitions que le simulateur de
// reference du design (swipe, tap, appui long), sans jamais manipuler
// d'objet LVGL interne directement.
void ui_swipe_left();
void ui_swipe_right();
void ui_activate();
void ui_long_press();

// Passerelle progression de demarrage (extension, meme justification que
// ui_app_tick ci-dessus) : App::boot_screen_set_progress/set_status/finish()
// existent deja sur App (voir ui/app.hpp, ecran Boot B1 deja valide,
// design inchange) mais n'etaient pas exposes par la facade -- necessaire
// pour qu'AppController (voir src/app/AppController.cpp), qui ne connait pas
// App/LVGL, puisse piloter la vraie progression de demarrage via UiBridge/
// RoundUiBridge plutot que l'ancienne simulation par ecoulement du temps.
void ui_boot_set_progress(uint8_t percent);
void ui_boot_set_status(const char *text);
void ui_boot_finish();
uint8_t ui_boot_progress();

} // namespace trophy
