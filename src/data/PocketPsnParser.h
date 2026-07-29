#pragma once

#include <string>

#include "data/ProfileData.h"
#include "data/TrophyStats.h"

// Parsing pur (aucune dependance Arduino/HTTP) de la reponse Pocket PSN,
// extrait de PocketPsnProvider pour etre teste sans materiel (voir
// simulator/src/main.cpp runPocketPsnParserSelfTest() et
// test/fixtures/pocketpsn_*.json).
//
// Format confirme par une vraie reponse le 2026-07-21 (voir
// docs/POCKETPSN_PROTOCOL.md, section "Format reel observe") -- remplace
// l'ancien format suppose (jamais confirme) par inspection passive du
// binaire officiel. Deux formats "Quick Stats" sont geres : le tableau
// reellement observe ({"Title","Stat","Percentile"}) et l'ancien objet
// suppose, conserve par compatibilite au cas ou l'API y reviendrait.
namespace PocketPsnParser {

enum class ParseError {
  kNone,
  kInvalidJson,
  kEmptyInput,
  kMissingRequiredFields,
};

struct ParseResult {
  ParseError error = ParseError::kNone;
  std::string errorMessage;
  ProfileData profile;
  TrophyStats stats;

  bool ok() const { return error == ParseError::kNone; }
};

// Parse le corps JSON brut. Ne leve jamais d'exception, ne suppose jamais
// une valeur absente (les champs manquants restent a leur valeur par
// defaut de la structure -- voir ProfileData.h / TrophyStats.h). Nettoie
// explicitement une eventuelle virgule trainante avant accolade/crochet
// fermant (quirk reel du backend Pocket PSN, observe le 2026-07-21) avant
// meme de tenter le parsing JSON.
ParseResult parse(const std::string& jsonBody);

}  // namespace PocketPsnParser
