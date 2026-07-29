#pragma once

// Version firmware partagee (voir src/ui/screens/SettingsScreen.cpp pour
// l'affichage LVGL -- non modifie ici, voir consigne "ne pas toucher au
// design LVGL" ; ce fichier existe pour que la reponse GET /api/diagnostics
// n'ait pas sa propre valeur divergente de celle affichee a l'ecran).
inline constexpr const char* kFirmwareVersion = "0.2.0-redesign";
