#pragma once

#include "network/IPocketPsnHttpClient.h"

// Implementation firmware de IPocketPsnHttpClient : requete HTTPS reelle
// via WiFiClientSecure/HTTPClient (Arduino/ESP32). Extraction mecanique de
// ce qui etait auparavant inline dans PocketPsnProvider.cpp -- aucun
// changement de comportement, seule la couche transport est isolee ici
// pour rendre PocketPsnProvider portable (voir simulator/src/PocketPsnHttpClientStub.h).
class PocketPsnHttpClient : public IPocketPsnHttpClient {
 public:
  Response post(const std::string& url, const std::string& body, const std::string& contentType,
                int timeoutMs) override;
};
