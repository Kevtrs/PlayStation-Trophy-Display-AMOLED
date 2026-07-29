#pragma once

#include "config/AppSettings.h"
#include "data/NetworkStatus.h"
#include "data/ProfileData.h"
#include "data/SyncStatus.h"
#include "data/TrophyStats.h"

// Seule donnee lue par l'interface (voir docs/IMPLEMENTATION_PLAN.md) :
// agregat de tout ce qui peut etre affiche. Ecrit uniquement par
// AppController -- jamais par un ecran LVGL.
struct AppState {
  ProfileData profile;
  TrophyStats stats;
  SyncStatus sync;
  NetworkStatus network;
  AppSettings settings;
};
