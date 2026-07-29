#pragma once

#include <cstdint>
#include <string>

// Gestion du temps, portable (voir docs/IMPLEMENTATION_PLAN.md). Sur
// firmware, synchronise l'horloge systeme via NTP (configTzTime) puis
// s'appuie sur le RTC interne de l'ESP32 entre deux synchronisations ; sur
// simulateur/desktop, s'appuie directement sur l'horloge du PC (consideree
// deja synchronisee). Aucun ecran ne doit appeler time()/millis()
// directement : tout passe par cette classe.
class TimeService {
 public:
  // ianaTimezone : nom convivial style IANA (ex: "Europe/Paris", tel que
  // stocke dans AppSettings::timezone). Converti en interne vers une
  // chaine POSIX TZ (voir toPosixTz() dans TimeService.cpp et
  // docs/CONFIGURATION.md pour la table de correspondance -- volontairement
  // limitee aux fuseaux exposes par le web UI, a completer si la liste
  // s'allonge).
  void begin(const std::string& ianaTimezone);

  // A appeler periodiquement (non bloquant) : no-op aujourd'hui (NTP est
  // asynchrone cote firmware), reserve pour une future logique de
  // resynchronisation periodique.
  void poll();

  // Demande une (re)synchronisation immediate (ex: apres connexion
  // Wi-Fi). Non bloquant.
  void requestSync();

  bool isSynced() const;

  // Epoch secondes courant, ou 0 si l'horloge n'a jamais ete synchronisee.
  uint32_t nowEpoch() const;

  // "Il y a 8 min" / "Il y a 2 h" / "Donnees anciennes" (> 24h) / "Jamais
  // synchronise" (epoch=0). frenchLocale=true pour le francais.
  static std::string formatRelative(uint32_t epoch, uint32_t nowEpoch, bool frenchLocale);

  // "HH:MM" (24h) a partir d'un epoch, "--:--" si epoch=0.
  static std::string formatClock(uint32_t epoch);

  // Point d'injection pour les tests/simulateur (voir simulator/DebugPanel) :
  // force l'etat "synchronise" sans dependre du reseau.
  void setSyncedForTesting(bool synced) {
    forcedSynced_ = synced;
    forceOverride_ = true;
  }

 private:
  std::string timezone_;
  bool ntpRequested_ = false;
  bool forceOverride_ = false;
  bool forcedSynced_ = false;
};
