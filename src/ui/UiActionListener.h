#pragma once

// Canal UI -> logique applicative, symetrique de UiBridge (voir
// docs/IMPLEMENTATION_PLAN.md). L'UI n'appelle jamais directement
// TrophyRepository/SyncService/ConfigManager : elle notifie uniquement une
// intention via cette interface, implementee par AppController.
class UiActionListener {
 public:
  virtual ~UiActionListener() = default;

  // L'utilisateur a demande une synchronisation manuelle (bouton dedie ou
  // geste sur l'ecran Sync).
  virtual void onRequestManualSync() = 0;
};
