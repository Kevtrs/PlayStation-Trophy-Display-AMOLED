#pragma once

#include <string>

#include "data/ProfileData.h"
#include "data/TrophyStats.h"

// Interface abstraite : l'UI et TrophyRepository ne doivent jamais dependre
// du format d'un fournisseur particulier (JSON Pocket PSN, donnees
// fictives, etc.).
class TrophyDataProvider {
 public:
  enum class Status { kIdle, kInProgress, kSuccess, kError };

  virtual ~TrophyDataProvider() = default;

  // Demarre une actualisation. Peut etre synchrone (DemoDataProvider) ou
  // bloquant sur le reseau (PocketPsnProvider) -- voir SyncService pour la
  // version non-bloquante utilisee par l'orchestrateur.
  virtual void requestRefresh() = 0;

  // A appeler periodiquement ; renvoie l'etat courant de la derniere
  // actualisation demandee.
  virtual Status poll() = 0;

  virtual const ProfileData& profile() const = 0;
  virtual const TrophyStats& stats() const = 0;

  // Erreur structuree de la derniere tentative (code=0 si aucune erreur).
  virtual int lastErrorCode() const = 0;
  virtual std::string lastErrorMessage() const = 0;

  // false pour tout fournisseur dont le comportement n'a pas ete confirme
  // par un test reel reussi (voir AUDIT.md). L'UI/les reglages doivent
  // afficher un avertissement si cet indicateur est faux.
  virtual bool isVerified() const = 0;
};
