#pragma once

#include <string>

// Identite du profil PSN, separee des statistiques de trophees (voir
// docs/IMPLEMENTATION_PLAN.md). Rempli par un TrophyDataProvider.
struct ProfileData {
  std::string username;
  std::string displayName;  // nom d'affichage public, distinct du pseudo PSN (source: page publique Pocket PSN)
  std::string country;

  int level = 0;
  int levelProgressPercent = 0;  // "PSN Level Progress"
  int levelRemainingPoints = 0;  // "PSN Level Remaining"

  bool hasPsPlus = false;      // "Plus" (entier 0/1 dans la vraie reponse, observe le 2026-07-21)
  std::string avatarFileName;  // "Avatar" (ex. "A2117_l.png"), observe le 2026-07-21, jamais obligatoire
};
