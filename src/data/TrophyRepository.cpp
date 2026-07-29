#include "data/TrophyRepository.h"

#include "utils/Logger.h"

TrophyRepository::TrophyRepository(TrophyDataProvider& provider, TrophyCache& cache)
    : provider_(provider), cache_(cache) {}

void TrophyRepository::loadFromCache() {
  if (cache_.load()) {
    profile_ = cache_.profile();
    stats_ = cache_.stats();
    hasData_ = true;
    Logger::info("TrophyRepository: donnees chargees depuis le cache hors-ligne");
  }
}

void TrophyRepository::requestRefresh() { provider_.requestRefresh(); }

bool TrophyRepository::validate(const ProfileData& profile, const TrophyStats& stats, std::string& outReason) const {
  if (profile.username.empty()) {
    outReason = "nom d'utilisateur absent";
    return false;
  }

  int detailedSum = stats.platinum + stats.gold + stats.silver + stats.bronze;
  if (stats.totalTrophies > 0 && detailedSum == 0) {
    outReason = "incoherence : totalTrophies > 0 mais aucun trophee detaille (reponse partielle ?)";
    return false;
  }

  if (hasData_) {
    // Rejette toute baisse anormale (reponse partielle, erreur serveur
    // renvoyant un profil vide/reinitialise) -- une baisse reelle (reset de
    // compte PSN) n'est pas distinguable d'une erreur ici, donc par
    // prudence on ne l'accepte jamais automatiquement (voir docs/CACHE.md).
    if (stats.totalTrophies < stats_.totalTrophies || stats.platinum < stats_.platinum ||
        stats.gold < stats_.gold || stats.silver < stats_.silver || stats.bronze < stats_.bronze) {
      outReason = "baisse anormale du nombre de trophees, reponse rejetee";
      return false;
    }
  }

  return true;
}

TrophyDataProvider::Status TrophyRepository::poll(uint32_t nowEpoch) {
  TrophyDataProvider::Status status = provider_.poll();

  if (status != TrophyDataProvider::Status::kSuccess) {
    if (status == TrophyDataProvider::Status::kError) {
      lastError_.code = provider_.lastErrorCode();
      lastError_.message = provider_.lastErrorMessage();
    }
    return status;
  }

  const ProfileData& newProfile = provider_.profile();
  const TrophyStats& newStats = provider_.stats();

  std::string reason;
  if (!validate(newProfile, newStats, reason)) {
    lastError_.code = -100;
    lastError_.message = "Donnees rejetees: " + reason;
    Logger::error("TrophyRepository: %s", lastError_.message.c_str());
    return TrophyDataProvider::Status::kError;
  }

  bool hadPreviousData = hasData_;
  TrophyStats previousStats = stats_;

  if (!cache_.save(newProfile, newStats, nowEpoch)) {
    // L'ecriture a echoue mais les donnees sont valides : on les garde en
    // memoire pour l'affichage courant sans considerer la sync en echec
    // vis-a-vis de l'utilisateur, tout en journalisant le probleme.
    Logger::error("TrophyRepository: sync valide mais cache non persiste");
  }

  profile_ = newProfile;
  stats_ = newStats;
  hasData_ = true;
  lastError_ = AppError{};

  if (hadPreviousData) {
    TrophyDelta delta = computeTrophyDelta(previousStats, newStats);
    if (delta.hasNewTrophy()) {
      pendingDelta_ = delta;
      deltaPending_ = true;
    }
  }

  return TrophyDataProvider::Status::kSuccess;
}

bool TrophyRepository::consumePendingDelta(TrophyDelta& outDelta) {
  if (!deltaPending_) return false;
  outDelta = pendingDelta_;
  deltaPending_ = false;
  return true;
}

void TrophyRepository::resetInMemoryState() {
  profile_ = ProfileData{};
  stats_ = TrophyStats{};
  hasData_ = false;
  lastError_ = AppError{};
  deltaPending_ = false;
  pendingDelta_ = TrophyDelta{};
}
