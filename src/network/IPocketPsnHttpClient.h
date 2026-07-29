#pragma once

#include <string>

// Abstraction du transport HTTP pour PocketPsnProvider (voir
// docs/IMPLEMENTATION_PLAN.md) : la logique metier (construction de la
// requete, decision d'erreur, parsing via PocketPsnParser) ne connait
// jamais HTTPClient/WiFiClientSecure directement, uniquement cette
// interface -- meme principe que IWiFiManager pour le Wi-Fi. Deux
// implementations : PocketPsnHttpClient (firmware, requete HTTPS reelle)
// et PocketPsnHttpClientStub (simulateur, reponses en file d'attente).
class IPocketPsnHttpClient {
 public:
  virtual ~IPocketPsnHttpClient() = default;

  struct Response {
    bool transportOk = false;  // false = echec de transport (DNS/TLS/timeout), httpStatus/body non pertinents
    int httpStatus = 0;
    std::string contentType;
    std::string body;
  };

  // Requete POST bloquante (l'appelant -- PocketPsnProvider -- est deja
  // appele depuis un contexte non bloquant pour le reste de l'app ; voir
  // AppController/SyncService pour l'orchestration).
  virtual Response post(const std::string& url, const std::string& body, const std::string& contentType,
                        int timeoutMs) = 0;
};
