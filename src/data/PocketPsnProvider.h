#pragma once

#include <string>

#include "data/TrophyDataProvider.h"
#include "network/IPocketPsnHttpClient.h"

// Endpoint, methode HTTP et noms de parametres confirmes par inspection
// passive du firmware officiel (voir docs/POCKETPSN_PROTOCOL.md et
// AUDIT.md sections 2-3) : un test reseau minimal a obtenu HTTP 200. Une
// cle API privee et legitime a ete obtenue directement aupres du
// proprietaire de Pocket PSN (voir AUDIT.md section 0ter) mais n'est pas
// encore presente dans cet environnement -- ajoutee localement par
// l'utilisateur uniquement, jamais committee/journalisee.
//
// isVerified() commence a false et le reste tant qu'un vrai
// requestRefresh() n'a pas reellement reussi (reponse HTTP 200 non vide,
// JSON parse avec succes, pseudo non vide) -- voir poll()/requestRefresh().
// Une fois devenu true, le reste (une panne reseau ponctuelle ulterieure
// ne "deverifie" pas un schema deja confirme reel). Ne jamais presenter ce
// fournisseur comme fonctionnel dans l'UI/les reglages tant que ceci n'a
// pas ete confirme par un vrai appel reussi.
//
// Portable : depend uniquement de IPocketPsnHttpClient (injecte), jamais
// directement de HTTPClient/WiFiClientSecure -- voir
// src/network/PocketPsnHttpClient.h (implementation firmware) et
// simulator/src/PocketPsnHttpClientStub.h (implementation simulateur).
class PocketPsnProvider : public TrophyDataProvider {
 public:
  PocketPsnProvider(std::string psnUsername, std::string apiKey, IPocketPsnHttpClient& httpClient);

  void requestRefresh() override;
  Status poll() override;
  const ProfileData& profile() const override { return profile_; }
  const TrophyStats& stats() const override { return stats_; }
  int lastErrorCode() const override { return lastErrorCode_; }
  std::string lastErrorMessage() const override { return lastError_; }
  bool isVerified() const override { return verified_; }

  // Codes d'erreur internes (pas des codes HTTP -- voir lastErrorCode()
  // qui renvoie le code HTTP brut quand la reponse a ete recue).
  static constexpr int kErrorNone = 0;
  static constexpr int kErrorHttpBeginFailed = -1;
  static constexpr int kErrorEmptyResponse = -2;
  static constexpr int kErrorInvalidJson = -3;

 private:
  bool parseResponse(const std::string& json, ProfileData& outProfile, TrophyStats& outStats);

  std::string psnUsername_;
  std::string apiKey_;
  IPocketPsnHttpClient& httpClient_;
  ProfileData profile_;
  TrophyStats stats_;
  Status status_ = Status::kIdle;
  int lastErrorCode_ = kErrorNone;
  std::string lastError_;
  bool verified_ = false;
};
