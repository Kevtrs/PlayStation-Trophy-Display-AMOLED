#pragma once

#include <cstdint>
#include <string>

#include "data/ProfileData.h"
#include "data/TrophyStats.h"
#include "storage/IPersistentStore.h"

// Cache hors-ligne des dernieres statistiques valides (voir docs/CACHE.md).
// Portable : ne depend que d'IPersistentStore. Regle absolue : une reponse
// invalide ou partielle ne doit jamais effacer un cache valide existant --
// seul save() avec des donnees pleinement validees remplace le cache.
class TrophyCache {
 public:
  explicit TrophyCache(IPersistentStore& store);

  // Charge le cache au demarrage (avant toute tentative reseau). Renvoie
  // false si aucun cache exploitable n'existe (premiere utilisation ou
  // fichier corrompu) -- dans ce cas hasData() reste false.
  bool load();

  // Sauvegarde atomique de nouvelles donnees *deja validees* par
  // l'appelant (TrophyRepository). fetchEpoch = date de recuperation
  // (epoch secondes, 0 si horloge non synchronisee).
  bool save(const ProfileData& profile, const TrophyStats& stats, uint32_t fetchEpoch);

  // Efface le cache persistant et en memoire (voir POST /api/reset --
  // reinitialisation complete demandee explicitement par l'utilisateur,
  // seul cas ou l'on efface volontairement un cache valide).
  void clear();

  bool hasData() const { return hasData_; }
  const ProfileData& profile() const { return profile_; }
  const TrophyStats& stats() const { return stats_; }
  uint32_t fetchEpoch() const { return fetchEpoch_; }

  // Age des donnees en secondes par rapport a "now" (epoch secondes). Si
  // fetchEpoch_ ou now valent 0 (horloge jamais synchronisee), renvoie -1
  // (age inconnu, voir TimeService pour le formatage "jamais synchronise").
  int32_t dataAgeSeconds(uint32_t nowEpoch) const;

  static constexpr const char* kStoreKey = "trophy_cache";
  static constexpr int kCurrentSchemaVersion = 1;

 private:
  IPersistentStore& store_;
  bool hasData_ = false;
  ProfileData profile_;
  TrophyStats stats_;
  uint32_t fetchEpoch_ = 0;
};
