#pragma once

// Statistiques de trophees et de jeux, independantes du profil (voir
// ProfileData.h) et du fournisseur (voir docs/POCKETPSN_PROTOCOL.md pour les
// noms de champs JSON d'origine, cites en commentaire).
struct TrophyStats {
  int platinum = 0;
  int gold = 0;
  int silver = 0;
  int bronze = 0;
  int totalTrophies = 0;

  int trophyPoints = 0;  // "Trophy Points" (score PSN)
  int pocketPoints = 0;  // "Pocket Points" (score propre a Pocket PSN)

  int totalGames = 0;
  int worldRank = 0;
  int nationalRank = 0;  // "Country Rank" cote Pocket PSN

  // Sous-objet "Quick Stats" (agregat, pas per-jeu -- voir AUDIT.md section 2)
  int gamesCompleted = 0;
  float completionRatePercent = 0.0f;
  float averageRarityPercent = 0.0f;
  int unearnedTrophies = 0;
  float playtimeHours = 0.0f;
  float trophiesPerDay = 0.0f;  // source: page publique Pocket PSN uniquement (absent du JSON prive)
};
