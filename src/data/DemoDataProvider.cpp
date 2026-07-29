#include "data/DemoDataProvider.h"

namespace {
// Valeurs alignees sur la maquette de reference fournie par l'utilisateur.
void fillDefaultDemoProfile(ProfileData& p) {
  p.username = "Kevin_Trophies";
  p.country = "FR";
  p.level = 327;
  p.levelProgressPercent = 72;  // "XP 64 820 / 90 000" sur la maquette
  p.levelRemainingPoints = 90000 - 64820;
  p.hasPsPlus = true;
}

void fillDefaultDemoStats(TrophyStats& s) {
  s.platinum = 58;
  s.gold = 214;
  s.silver = 876;
  s.bronze = 3138;
  s.totalTrophies = 4286;

  s.trophyPoints = 0;  // non visible sur la maquette, laisse a 0 en demo
  s.pocketPoints = 0;

  s.totalGames = 142;
  s.worldRank = 12483;
  s.nationalRank = 0;  // non visible sur la maquette

  s.gamesCompleted = 142;
  s.completionRatePercent = 78.0f;
  s.averageRarityPercent = 0.0f;
  s.unearnedTrophies = 0;
  s.playtimeHours = 3426.0f;
}
}  // namespace

DemoDataProvider::DemoDataProvider() {
  fillDefaultDemoProfile(profile_);
  fillDefaultDemoStats(stats_);
}

void DemoDataProvider::requestRefresh() {
  if (nextRefreshShouldFail_) {
    status_ = Status::kError;
    nextRefreshShouldFail_ = false;
    return;
  }
  status_ = Status::kSuccess;
}

TrophyDataProvider::Status DemoDataProvider::poll() { return status_; }

void DemoDataProvider::simulateNewTrophy() {
  stats_.bronze += 1;
  stats_.totalTrophies += 1;
}
