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
lv_obj_t *build_trophies_overview_screen_wide(const ProfileData &p, AppLanguage lang);
lv_obj_t *build_trophy_screen_wide(TrophyKind kind, int value, int total, int sub_index, int sub_count,
                                    AppLanguage lang);
lv_obj_t *build_statistics_overview_screen_wide(const ProfileData &p, AppLanguage lang);
lv_obj_t *build_sync_screen_wide(SyncState state, int progress, AppLanguage lang);
lv_obj_t *build_offline_screen_wide(const ProfileData &p, AppLanguage lang);
lv_obj_t *build_error_screen_wide(const ProfileData &p, AppLanguage lang);
// --- Ecran Reglages : actions tactiles --------------------------------
//
// Chaque ligne modifiable declenche une action ; le bridge la traduit en
// patch de configuration (voir AppController::applyConfigPatch) et
// reconstruit l'ecran pour afficher la nouvelle valeur. Les lignes
// informatives (profil) portent kNone.
enum class WideSettingAction : uint8_t {
    kNone,
    kBrightness,    // cycle 25 / 50 / 75 / 100 %
    kAutoRotate,    // bascule
    kAnimations,    // bascule
    kSyncInterval,  // cycle 15 / 30 / 60 / 120 min
    kLanguage,      // bascule francais / anglais
};

using WideSettingHandler = void (*)(WideSettingAction action, void *owner);

lv_obj_t *build_settings_screen_wide(const ProfileData &p, AppLanguage lang, WideSettingHandler handler,
                                      void *owner);
lv_obj_t *build_stat_screen_wide(StatIconKind icon_kind, const char *value, const char *caption, int sub_index,
                                  int sub_count);

// Ecran de credits (logo + mention Pocket PSN, source des donnees de
// trophees) : le board rond a un ecran "A propos" equivalent (voir
// build_about_screen() dans screens.cpp). Garde dans le defilement
// automatique meme depuis l'ajout du tactile : la mention de la source des
// donnees doit rester visible sans qu'on ait a la chercher.
// Retour utilisateur du 2026-07-28.
lv_obj_t *build_credits_screen_wide(AppLanguage lang);

// Ecran de celebration : affiche apres une synchro qui a rapporte au
// moins un trophee (voir TrophyDelta / UiBridge::showTrophyDelta). Prend
// les gains, pas les totaux -- "gained.total" est le nombre de nouveaux
// trophees, pas la collection.
//
// Ne prend volontairement aucun type de la couche data : TrophyStats est
// le type UI (ui_model.hpp), le bridge fait la conversion.
// animate : false quand l'utilisateur a demande moins de mouvement
// (reduced_motion). L'ecran reste alors parfaitement lisible, fige.
lv_obj_t *build_celebration_screen_wide(const TrophyStats &gained, AppLanguage lang, bool animate);

// --- Couche de navigation tactile (board 7", GT911) -------------------
//
// Le defilement automatique reste le mode par defaut : l'appareil est un
// objet pose sur un bureau, il doit etre lisible sans qu'on le touche. Le
// tactile ne remplace pas ce defilement, il le met en pause et donne la
// main -- voir WideUiBridge pour la machine a etats.
enum class WideNavAction : uint8_t {
    kPrev,
    kNext,
    kGoTo,           // "target" porte l'index de destination
    kOpenSettings,
    kCloseSettings,
};

// Pointeur de fonction libre plutot que std::function : pas d'allocation
// dynamique cachee dans le chemin d'interaction sur ESP32-S3.
using WideNavHandler = void (*)(WideNavAction action, int target, void *owner);

// Pose la couche de navigation par-dessus un ecran du defilement et
// renvoie sa racine, a masquer/afficher avec LV_OBJ_FLAG_HIDDEN (masquee,
// elle ne recoit plus aucun evenement : le mode ambiant est donc inerte au
// toucher sans code supplementaire). Renvoie nullptr si screen == nullptr.
//
// A appeler APRES avoir dessine le contenu de l'ecran : LVGL empile dans
// l'ordre de creation, la couche doit rester au-dessus.
lv_obj_t *attach_wide_nav_overlay(lv_obj_t *screen, int index, int count, WideNavHandler handler, void *owner);

// Bouton de retour pose sur l'ecran Reglages (icone + libelle traduit :
// un pictogramme seul n'est pas evident pour quelqu'un qui decouvre
// l'objet sans notice).
lv_obj_t *attach_wide_back_overlay(lv_obj_t *screen, WideNavHandler handler, void *owner, AppLanguage lang);

// Ecran affiche tant que le board n'a ni Wi-Fi ni compte Pocket PSN
// configures (WifiState::kAccessPoint, voir AppController::tick()) : sans
// cet ecran, un utilisateur non-technique qui flashe l'appareil n'a AUCUN
// moyen de savoir qu'une configuration est possible -- il ne verrait que
// des trophees de demonstration qui donnent l'impression que "ca marche
// deja". Remplace entierement le defilement normal tant que ce n'est pas
// configure, et ne recoit volontairement aucune couche de navigation : il
// n'y a rien d'autre a consulter tant que l'appareil n'est pas configure
// (voir WideUiBridge::tick()).
// Retour utilisateur du 2026-07-29 ("ta tout prevu... l'ecran de
// connexion... tout ?").
lv_obj_t *build_wifi_setup_screen_wide(const std::string &ssid, const std::string &ip_address, AppLanguage lang);

} // namespace trophy
