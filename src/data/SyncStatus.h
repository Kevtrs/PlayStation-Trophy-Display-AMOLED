#pragma once

#include <cstdint>
#include <string>

// Etat de synchronisation, distinct des donnees elles-memes. Utilise par
// SyncService (src/services/SyncService.h) et affiche par l'UI (jamais
// modifie directement par l'UI).
enum class SyncState {
  kIdle,
  kWaitingForNetwork,
  kConnecting,
  kDownloading,
  kParsing,
  kSaving,
  kSuccess,
  kError,
  kOffline,
};

// Erreur structuree -- jamais de message invente : soit un code/texte
// provenant reellement d'un HTTPClient/parseur, soit un code interne
// documente (voir SyncService.h).
struct AppError {
  int code = 0;
  std::string message;

  bool hasError() const { return code != 0 || !message.empty(); }
};

// Vrai pour tous les etats intermediaires d'une synchronisation en cours
// (equivalent de l'ancien "kSyncing" -- voir docs/IMPLEMENTATION_PLAN.md).
inline bool isSyncActive(SyncState s) {
  return s == SyncState::kWaitingForNetwork || s == SyncState::kConnecting ||
         s == SyncState::kDownloading || s == SyncState::kParsing || s == SyncState::kSaving;
}

struct SyncStatus {
  SyncState state = SyncState::kIdle;

  uint32_t lastSyncEpoch = 0;  // 0 = jamais synchronise (voir TimeService)
  bool isOffline = false;      // aucune connexion reseau exploitable
  bool isDemo = true;          // donnees actuellement affichees = DemoDataProvider

  AppError lastError;

  int consecutiveFailures = 0;
  int totalSyncCount = 0;
  int totalFailureCount = 0;

  bool pocketPsnVerified = false;  // voir AUDIT.md -- toujours false hors test reel reussi
};
