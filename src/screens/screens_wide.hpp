#pragma once

#include "assets/stat_icons.hpp"
#include "lvgl.h"
#include "ui/ui_model.hpp"

#include <string>

namespace trophy {

// Ecrans pour le board Waveshare ESP32-S3-Touch-LCD-7 (800x480). Prennent
// des donnees pures (pas App&) : ne dependent pas encore du cycle
// applicatif complet (App::tick()/rotation_slide_for()), pour rester
// testables isolement dans le mini-simulateur de prevue tant que le
// materiel n'est pas en main -- voir simulator_wide/.
lv_obj_t *build_dashboard_screen_wide(const ProfileData &p, AppLanguage lang);
lv_obj_t *build_trophy_screen_wide(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang);
lv_obj_t *build_stat_screen_wide(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                  int sub_count);

// Ecran de credits (logo + mention Pocket PSN, source des donnees de
// trophees) : le board rond a un ecran "A propos" equivalent (voir
// build_about_screen() dans screens.cpp) mais accessible par tactile
// (Reglages -> A propos) -- ce board n'a pas de tactile, donc aucune
// mention Pocket PSN n'apparaissait nulle part avant son ajout ici, dans
// le defilement automatique. Retour utilisateur du 2026-07-28.
lv_obj_t *build_credits_screen_wide(AppLanguage lang);

// Ecran affiche tant que le board n'a ni Wi-Fi ni compte Pocket PSN
// configures (WifiState::kAccessPoint, voir AppController::tick()) : ce
// board n'a pas de tactile (voir commentaire d'en-tete de
// WideUiBridge.h), donc sans cet ecran un utilisateur non-technique qui
// flashe l'appareil n'a AUCUN moyen de savoir qu'une configuration est
// possible -- il ne verrait que des trophees de demonstration qui donnent
// l'impression que "ca marche deja". Remplace entierement le defilement
// normal tant que ce n'est pas configure (voir WideUiBridge::tick()).
// Retour utilisateur du 2026-07-29 ("ta tout prevu... l'ecran de
// connexion... tout ?").
lv_obj_t *build_wifi_setup_screen_wide(const std::string &ssid, const std::string &ip_address, AppLanguage lang);

// TEST -- direction epuree (retour utilisateur du 2026-07-29 "trop
// charge/sci-fi"), preview simulateur uniquement, jamais branchee sur le
// vrai firmware tant que non validee.
lv_obj_t *build_dashboard_screen_wide_v2(const ProfileData &p, AppLanguage lang);
lv_obj_t *build_trophy_screen_wide_v2(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang);
lv_obj_t *build_stat_screen_wide_v2(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                     int sub_count);
lv_obj_t *build_trophy_screen_wide_v3(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang);
lv_obj_t *build_trophy_screen_wide_v4(TrophyKind kind, int value, int sub_index, int sub_count, AppLanguage lang);
lv_obj_t *build_stat_screen_wide_v3(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                     int sub_count);

} // namespace trophy
