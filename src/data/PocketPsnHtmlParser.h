#pragma once

#include <string>

#include "data/ProfileData.h"
#include "data/TrophyStats.h"

// Parsing pur (aucune dependance Arduino/HTTP) de la page HTML publique
// https://pocketpsn.com/<pseudo>, obtenue SANS cle privee et SANS
// authentification -- voir tools/pocketpsn_public_probe/ et
// test/test_pocketpsn_html_parser.cpp.
//
// Cette voie est distincte de PocketPsnParser (qui parse le JSON de
// l'endpoint prive api.pocketpsn.com/PSTrophyDisplay/, jamais confirme
// faute de cle -- voir AUDIT.md section 3). Ici, les noms de champs et la
// structure HTML sont repris d'une vraie page capturee manuellement par
// l'utilisateur dans son navigateur (test/fixtures/pocketpsn_profile_public.html),
// nettoyee de toute donnee de session/compte avant d'etre committee.
//
// Toute requete directe (curl/urllib) vers ce meme domaine depuis un
// script est bloquee par un defi anti-robot Cloudflare (403, voir
// tools/pocketpsn_public_probe/) : ce parser doit donc detecter ce cas et
// le signaler explicitement plutot que d'echouer silencieusement ou de
// tenter un contournement.
namespace PocketPsnHtmlParser {

enum class ParseError {
  kNone,
  kEmptyInput,
  kCloudflareChallenge,     // defi anti-robot detecte (403/429/503, page "Just a moment...", etc.)
  kProfileNotFound,         // page HTML valide mais sans bloc de profil identifiable
  kMissingRequiredFields,   // bloc de profil present mais un champ attendu est absent
  kInvalidNumber,           // un champ numerique attendu n'a pas pu etre converti
  kTotalMismatch,           // totalTrophies != platinum + gold + silver + bronze
  kInvalidLevelProgress,    // levelProgressPercent hors de l'intervalle [0, 100]
  kMetaDescriptionMismatch, // la balise meta description contredit le bloc de profil principal
};

struct ParseResult {
  ParseError error = ParseError::kNone;
  std::string errorMessage;
  ProfileData profile;
  TrophyStats stats;

  bool ok() const { return error == ParseError::kNone; }
};

// Parse le corps HTML brut d'une page de profil publique Pocket PSN.
// `httpStatusCode` est optionnel (0 = inconnu) : s'il vaut 403, 429 ou 503,
// la reponse est traitee comme un defi anti-robot sans meme inspecter le
// corps. Ne leve jamais d'exception, ne tente jamais de resoudre un defi.
ParseResult parse(const std::string& html, int httpStatusCode = 0);

// Expose separement pour les tests et pour un futur appelant HTTP qui
// voudrait decider tot (avant meme un parsing complet) si la reponse est
// exploitable.
bool isCloudflareChallenge(const std::string& html, int httpStatusCode);

}  // namespace PocketPsnHtmlParser
