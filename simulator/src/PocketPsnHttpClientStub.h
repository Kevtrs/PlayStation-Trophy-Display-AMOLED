#pragma once

#include "network/IPocketPsnHttpClient.h"

// Simulateur : aucune requete reseau reelle. Renvoie la reponse mise en
// file par queueResponse() (utilise par --selftest, voir
// simulator/src/main.cpp runPocketPsnProviderSelfTest()) et enregistre la
// derniere requete recue pour verifier la construction de l'URL/du corps
// sans dependre d'un vrai serveur.
class PocketPsnHttpClientStub : public IPocketPsnHttpClient {
 public:
  Response post(const std::string& url, const std::string& body, const std::string& contentType,
                int timeoutMs) override;

  void queueResponse(const Response& response) { queued_ = response; }

  const std::string& lastUrl() const { return lastUrl_; }
  const std::string& lastBody() const { return lastBody_; }
  const std::string& lastContentType() const { return lastContentType_; }

 private:
  Response queued_;
  std::string lastUrl_;
  std::string lastBody_;
  std::string lastContentType_;
};
