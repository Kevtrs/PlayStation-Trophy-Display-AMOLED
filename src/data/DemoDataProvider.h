#pragma once

#include "data/TrophyDataProvider.h"

// Fournisseur de demonstration : donnees fictives, disponibles instantanement,
// sans reseau. Utilise les memes structures que PocketPsnProvider (voir
// docs/IMPLEMENTATION_PLAN.md) -- l'UI ne peut pas distinguer les deux.
class DemoDataProvider : public TrophyDataProvider {
 public:
  DemoDataProvider();

  void requestRefresh() override;
  Status poll() override;
  const ProfileData& profile() const override { return profile_; }
  const TrophyStats& stats() const override { return stats_; }
  int lastErrorCode() const override { return 0; }
  std::string lastErrorMessage() const override { return ""; }
  bool isVerified() const override { return true; }

  // Simule l'obtention d'un nouveau trophee (utilise par la touche N du
  // simulateur / les tests de detection de nouveaux trophees).
  void simulateNewTrophy();

  // Simule une erreur reseau/API lors de la prochaine actualisation.
  void simulateNextRefreshError(bool shouldFail) { nextRefreshShouldFail_ = shouldFail; }

  // Acces direct pour le panneau de debug du simulateur (outil de
  // developpement uniquement -- jamais utilise par le firmware).
  ProfileData& mutableProfile() { return profile_; }
  TrophyStats& mutableStats() { return stats_; }

 private:
  ProfileData profile_;
  TrophyStats stats_;
  Status status_ = Status::kIdle;
  bool nextRefreshShouldFail_ = false;
};
