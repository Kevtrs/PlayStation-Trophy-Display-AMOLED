#pragma once

#include <string>

#include "config/AppSettings.h"

// Decision pure (aucune E/S, aucune construction d'objet) de savoir si
// PocketPsnProvider peut etre utilise au demarrage -- voir
// docs/IMPLEMENTATION_PLAN.md. AppController reste totalement agnostique
// (il ne prend qu'un TrophyDataProvider&) ; c'est l'appelant (main.cpp
// firmware/simulateur) qui choisit quel provider construire et passer,
// une seule fois au demarrage (voir AUDIT.md section 0ter et decision
// utilisateur : pas de bascule a chaud, un redemarrage est necessaire
// apres changement de la cle/du pseudo -- voir
// WebApiHandlers::configPatchRequiresRestart()).
namespace ProviderFactory {

// Cle API effective : celle saisie manuellement si presente, sinon la cle
// partagee compilee dans le firmware (POCKETPSN_SHARED_API_KEY, voir
// include/secrets.example.h et AUDIT.md section 0quater).
std::string effectiveApiKey(const AppSettings& settings);

bool shouldUsePocketPsn(const AppSettings& settings);

}  // namespace ProviderFactory
