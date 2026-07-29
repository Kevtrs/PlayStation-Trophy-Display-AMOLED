#pragma once

#include "data/TrophyStats.h"

// Difference entre deux syncronisations reussies, calculee par
// TrophyRepository. Sert a declencher l'evenement "nouveau trophee" cote UI
// (voir UiBridge::showTrophyDelta) sans jamais le rejouer au redemarrage.
struct TrophyDelta {
  int platinumDelta = 0;
  int goldDelta = 0;
  int silverDelta = 0;
  int bronzeDelta = 0;
  int totalDelta = 0;

  bool hasNewTrophy() const { return totalDelta > 0; }
};

// Calcule le delta entre deux etats. Les baisses (ex: reinitialisation de
// compte, reponse partielle) ne sont jamais reportees comme un delta
// negatif "recompense" -- voir TrophyRepository pour la logique de rejet.
inline TrophyDelta computeTrophyDelta(const TrophyStats& previous, const TrophyStats& current) {
  TrophyDelta delta;
  delta.platinumDelta = current.platinum - previous.platinum;
  delta.goldDelta = current.gold - previous.gold;
  delta.silverDelta = current.silver - previous.silver;
  delta.bronzeDelta = current.bronze - previous.bronze;
  delta.totalDelta = current.totalTrophies - previous.totalTrophies;
  return delta;
}
