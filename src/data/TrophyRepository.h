#pragma once

#include <string>

#include "data/SyncStatus.h"
#include "data/TrophyDataProvider.h"
#include "data/TrophyDelta.h"
#include "storage/TrophyCache.h"

// Orchestre un TrophyDataProvider (Demo ou Pocket PSN) et un TrophyCache :
// c'est la seule classe qui decide si de nouvelles donnees sont assez
// valides pour remplacer le cache et pour publier un TrophyDelta (voir
// docs/IMPLEMENTATION_PLAN.md, flux AppController -> TrophyRepository ->
// PocketPsnProvider -> AppState -> UiBridge). Ne bloque jamais : poll() ne
// fait qu'interroger l'etat courant du provider.
class TrophyRepository {
 public:
  TrophyRepository(TrophyDataProvider& provider, TrophyCache& cache);

  // A appeler une fois au demarrage, avant toute tentative reseau : rend
  // immediatement disponibles les dernieres donnees valides connues.
  void loadFromCache();

  void requestRefresh();

  // A appeler periodiquement (non bloquant). nowEpoch : epoch courant
  // fourni par TimeService (0 si horloge non synchronisee).
  TrophyDataProvider::Status poll(uint32_t nowEpoch);

  const ProfileData& profile() const { return profile_; }
  const TrophyStats& stats() const { return stats_; }
  bool hasData() const { return hasData_; }
  uint32_t lastFetchEpoch() const { return cache_.fetchEpoch(); }

  const AppError& lastError() const { return lastError_; }

  // Relai vers TrophyDataProvider::isVerified() : ne devient true que si le
  // fournisseur courant a reellement ete valide par un test reel reussi
  // (voir AUDIT.md). Ne jamais deduire "verifie" du seul succes d'un sync.
  bool isProviderVerified() const { return provider_.isVerified(); }

  // Renvoie true et remplit outDelta si un nouveau trophee valide est en
  // attente de notification (consomme l'evenement -- ne sera plus renvoye
  // tant qu'aucun nouveau succes valide n'arrive ; jamais rejoue au
  // redemarrage puisque non persiste).
  bool consumePendingDelta(TrophyDelta& outDelta);

  // Efface le profil/stats en memoire (voir POST /api/reset --
  // TrophyCache::clear() efface deja le cache persistant separement ;
  // sans ceci, un ancien profil resterait affiche et un futur delta
  // serait calcule contre des valeurs perimees).
  void resetInMemoryState();

 private:
  bool validate(const ProfileData& profile, const TrophyStats& stats, std::string& outReason) const;

  TrophyDataProvider& provider_;
  TrophyCache& cache_;

  ProfileData profile_;
  TrophyStats stats_;
  bool hasData_ = false;

  AppError lastError_;

  bool deltaPending_ = false;
  TrophyDelta pendingDelta_;
};
