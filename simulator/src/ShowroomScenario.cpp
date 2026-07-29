#include "ShowroomScenario.h"

#include <cstdio>

#include "PocketPsnHttpClientStub.h"
#include "WiFiManagerStub.h"
#include "app/AppController.h"
#include "data/NetworkStatus.h"
#include "data/SyncStatus.h"
#include "services/SyncService.h"

namespace {

// Delais de maintien par etape : penses pour rester observables a l'oeil nu
// pendant une demonstration, pas pour la vitesse d'execution (voir aussi
// runLongDurationStressSelfTest() pour un test de charge, sans affichage,
// qui lui n'a aucune raison d'etre ralenti).
constexpr uint32_t kHoldStartupMs = 2000;
constexpr uint32_t kHoldLoadingMs = 1500;
constexpr uint32_t kHoldProfileDisplayMs = 2500;
constexpr uint32_t kHoldUsingCacheMs = 2000;
constexpr uint32_t kHoldNetworkLostMs = 1500;
constexpr uint32_t kHoldOfflineMs = 2500;
// Marge au-dela de SyncService::kReconnectStabilizationMs (4000 ms) pour
// laisser le temps a la resynchronisation automatique de reellement
// s'executer (voir SyncService::poll()) avant de considerer l'etape
// "reconnexion" terminee.
constexpr uint32_t kReconnectingMarginMs = 800;
// Garde-fou du scenario de demonstration uniquement (pas du comportement
// applicatif reel, deja verifie par runSyncServiceReconnectDebounceSelfTest())
// : si la resynchronisation automatique n'a toujours pas reussi apres ce
// delai, on la force manuellement plutot que de bloquer indefiniment la
// demonstration.
constexpr uint32_t kResyncingTimeoutMs = 5000;
constexpr uint32_t kHoldApiErrorMs = 2000;
constexpr uint32_t kHoldBackToNormalMs = 2000;

// Reponses JSON synthetiques, au format reel confirme le 2026-07-21 (voir
// docs/POCKETPSN_PROTOCOL.md) -- jamais une vraie reponse Pocket PSN,
// uniquement des donnees de demonstration plausibles.
constexpr const char* kShowroomSyncResponse =
    R"({"Username": "ShowroomUser", "Country": "FR", "PSN Level": 42, "PSN Level Progress": 60, )"
    R"("PSN Level Remaining": 400, "Plus": 1, "Avatar": "A0001_l.png", "Trophies Plats": 12, )"
    R"("Trophies Gold": 58, "Trophies Silver": 120, "Trophies Bronze": 340, "Trophies Total": 530, )"
    R"("Trophy Points": 15230, "Pocket Points": 4210, "Total Games": 87, "World Rank": 152340, )"
    R"("Country Rank": 4210, "Quick Stats": [{"Title": "Games Completed", "Stat": "63", "Percentile": 0.72}, )"
    R"({"Title": "Completion Average", "Stat": "38.76%", "Percentile": 0.5}, )"
    R"({"Title": "Average Rarity", "Stat": "22%", "Percentile": 0.4}, )"
    R"({"Title": "Unearned Trophies", "Stat": "2,582", "Percentile": 0.3}, )"
    R"({"Title": "Hours Played", "Stat": "10,743", "Percentile": 0.9}]})";

// Meme profil, legerement enrichi (un trophee bronze de plus) : utilise pour
// la resynchronisation apres reconnexion, afin qu'un observateur voie
// concretement qu'une nouvelle synchronisation a bien eu lieu (pas la meme
// reponse rejouee a l'identique).
constexpr const char* kShowroomResyncResponse =
    R"({"Username": "ShowroomUser", "Country": "FR", "PSN Level": 42, "PSN Level Progress": 65, )"
    R"("PSN Level Remaining": 350, "Plus": 1, "Avatar": "A0001_l.png", "Trophies Plats": 12, )"
    R"("Trophies Gold": 58, "Trophies Silver": 120, "Trophies Bronze": 341, "Trophies Total": 531, )"
    R"("Trophy Points": 15245, "Pocket Points": 4215, "Total Games": 87, "World Rank": 152100, )"
    R"("Country Rank": 4205, "Quick Stats": [{"Title": "Games Completed", "Stat": "63", "Percentile": 0.72}, )"
    R"({"Title": "Completion Average", "Stat": "38.76%", "Percentile": 0.5}, )"
    R"({"Title": "Average Rarity", "Stat": "22%", "Percentile": 0.4}, )"
    R"({"Title": "Unearned Trophies", "Stat": "2,581", "Percentile": 0.3}, )"
    R"({"Title": "Hours Played", "Stat": "10,749", "Percentile": 0.9}]})";

IPocketPsnHttpClient::Response makeJsonResponse(const std::string& body) {
  IPocketPsnHttpClient::Response response;
  response.transportOk = true;
  response.httpStatus = 200;
  response.contentType = "application/json";
  response.body = body;
  return response;
}

// Comportement reellement observe pour une reponse invalide (voir
// PocketPsnProvider::requestRefresh()) : HTTP 200 + corps vide ->
// kErrorEmptyResponse, jamais un simple echec de transport.
IPocketPsnHttpClient::Response makeEmptyResponse() {
  IPocketPsnHttpClient::Response response;
  response.transportOk = true;
  response.httpStatus = 200;
  response.contentType = "application/json";
  response.body.clear();
  return response;
}

}  // namespace

ShowroomScenario::ShowroomScenario(AppController& appController, WiFiManagerStub& wifiStub,
                                    PocketPsnHttpClientStub& httpStub)
    : app_(appController), wifiStub_(wifiStub), httpStub_(httpStub) {}

const char* ShowroomScenario::stepLabel(Step step) {
  switch (step) {
    case Step::kStartup: return "1. Demarrage";
    case Step::kLoading: return "2. Chargement (connexion Wi-Fi)";
    case Step::kSyncing: return "3. Synchronisation PocketPSN";
    case Step::kProfileDisplay: return "4. Affichage du profil";
    case Step::kUsingCache: return "5. Utilisation du cache";
    case Step::kNetworkLost: return "6. Perte du reseau";
    case Step::kOffline: return "7. Ecran hors ligne";
    case Step::kReconnecting: return "8. Reconnexion (stabilisation en cours)";
    case Step::kResyncing: return "9. Nouvelle synchronisation";
    case Step::kApiError: return "10. Erreur API simulee (cache preserve)";
    case Step::kBackToNormal: return "11. Retour a un etat normal";
    case Step::kDone: return "Termine";
  }
  return "?";
}

void ShowroomScenario::start(uint32_t nowMillis) {
  autoAdvance_ = true;
  enterStep(Step::kStartup, nowMillis);
}

void ShowroomScenario::triggerStep(Step step, uint32_t nowMillis) {
  autoAdvance_ = false;
  enterStep(step, nowMillis);
}

void ShowroomScenario::enterStep(Step step, uint32_t nowMillis) {
  step_ = step;
  stepStartedMillis_ = nowMillis;
  std::printf("[showroom] %s\n", stepLabel(step));

  switch (step) {
    case Step::kStartup:
      // Rien a faire : AppController::begin() a deja ete appele par
      // l'appelant (voir simulator/src/main.cpp) -- cette etape ne fait
      // qu'observer l'etat initial (cache eventuel, Wi-Fi non connecte).
      break;

    case Step::kLoading:
      wifiStub_.simulateConnected("Showroom-WiFi");
      break;

    case Step::kSyncing:
      httpStub_.queueResponse(makeJsonResponse(kShowroomSyncResponse));
      app_.requestManualSync();
      break;

    case Step::kProfileDisplay:
      // Rien a faire : maintien du profil deja synchronise a l'ecran.
      break;

    case Step::kUsingCache:
      // Rien a faire : le profil affiche est deja servi par le cache local
      // (TrophyCache/TrophyRepository), pas re-telecharge -- voir
      // AppController::hasCachedData(), verifie par le test associe.
      break;

    case Step::kNetworkLost:
      wifiStub_.simulateDisconnected();
      break;

    case Step::kOffline:
      // Rien a faire : maintien de l'etat hors ligne (SyncStatus::kOffline)
      // pendant que le dernier profil connu reste affiche depuis le cache.
      break;

    case Step::kReconnecting:
      // La reponse de resynchronisation est mise en file des maintenant :
      // elle doit deja etre prete quand SyncService declenchera
      // automatiquement une resynchronisation unique, apres sa fenetre de
      // stabilisation (voir SyncService::kReconnectStabilizationMs) -- on ne
      // declenche jamais la synchro nous-memes ici, c'est tout l'interet de
      // la demonstration (comportement reel, pas simule au niveau UI).
      httpStub_.queueResponse(makeJsonResponse(kShowroomResyncResponse));
      wifiStub_.simulateConnected("Showroom-WiFi");
      break;

    case Step::kResyncing:
      // Rien a faire ici : l'action a deja eu lieu a l'entree de
      // kReconnecting (voir ci-dessus). Cette etape se contente d'observer
      // le resultat (voir exitConditionMet()).
      break;

    case Step::kApiError:
      preErrorProfile_ = app_.state().profile;
      preErrorStats_ = app_.state().stats;
      httpStub_.queueResponse(makeEmptyResponse());
      app_.requestManualSync();
      break;

    case Step::kBackToNormal:
      httpStub_.queueResponse(makeJsonResponse(kShowroomResyncResponse));
      app_.requestManualSync();
      break;

    case Step::kDone:
      std::printf("[showroom] Scenario termine.\n");
      break;
  }
}

bool ShowroomScenario::exitConditionMet(uint32_t nowMillis) const {
  uint32_t elapsed = nowMillis - stepStartedMillis_;
  const SyncStatus& sync = app_.state().sync;

  switch (step_) {
    case Step::kStartup:
      return elapsed >= kHoldStartupMs;
    case Step::kLoading:
      return elapsed >= kHoldLoadingMs && app_.state().network.state == WifiState::kConnected;
    case Step::kSyncing:
      return sync.state == SyncState::kSuccess || sync.state == SyncState::kError;
    case Step::kProfileDisplay:
      return elapsed >= kHoldProfileDisplayMs;
    case Step::kUsingCache:
      return elapsed >= kHoldUsingCacheMs;
    case Step::kNetworkLost:
      return elapsed >= kHoldNetworkLostMs && app_.state().network.state != WifiState::kConnected;
    case Step::kOffline:
      return elapsed >= kHoldOfflineMs;
    case Step::kReconnecting:
      return elapsed >= SyncService::kReconnectStabilizationMs + kReconnectingMarginMs;
    case Step::kResyncing:
      return sync.state == SyncState::kSuccess || elapsed >= kResyncingTimeoutMs;
    case Step::kApiError:
      return (elapsed >= kHoldApiErrorMs && sync.state == SyncState::kError) || elapsed >= kResyncingTimeoutMs;
    case Step::kBackToNormal:
      return (elapsed >= kHoldBackToNormalMs && sync.state == SyncState::kSuccess) || elapsed >= kResyncingTimeoutMs;
    case Step::kDone:
      return false;
  }
  return false;
}

bool ShowroomScenario::tick(uint32_t nowMillis) {
  if (step_ == Step::kDone) return false;
  if (!autoAdvance_) return true;  // en mode manuel, ne progresse jamais tout seul

  if (!exitConditionMet(nowMillis)) return true;

  switch (step_) {
    case Step::kStartup:
      enterStep(Step::kLoading, nowMillis);
      break;
    case Step::kLoading:
      enterStep(Step::kSyncing, nowMillis);
      break;
    case Step::kSyncing:
      enterStep(Step::kProfileDisplay, nowMillis);
      break;
    case Step::kProfileDisplay:
      enterStep(Step::kUsingCache, nowMillis);
      break;
    case Step::kUsingCache:
      enterStep(Step::kNetworkLost, nowMillis);
      break;
    case Step::kNetworkLost:
      enterStep(Step::kOffline, nowMillis);
      break;
    case Step::kOffline:
      enterStep(Step::kReconnecting, nowMillis);
      break;
    case Step::kReconnecting:
      enterStep(Step::kResyncing, nowMillis);
      break;
    case Step::kResyncing:
      // Garde-fou du scenario (voir kResyncingTimeoutMs) : la resynchro
      // automatique aurait normalement deja du reussir pendant toute la
      // duree de l'etape kReconnecting precedente (elle attend deja plus
      // longtemps que SyncService::kReconnectStabilizationMs, voir
      // exitConditionMet()) -- ce cas ne devrait donc jamais se produire en
      // pratique. On se contente de journaliser et de continuer plutot que
      // de forcer une action ici, pour ne jamais faire concurrence a la
      // reponse deja mise en file par la prochaine etape (kApiError).
      if (app_.state().sync.state != SyncState::kSuccess) {
        std::printf("[showroom] Avertissement : resynchronisation automatique non observee a temps.\n");
      }
      enterStep(Step::kApiError, nowMillis);
      break;
    case Step::kApiError:
      enterStep(Step::kBackToNormal, nowMillis);
      break;
    case Step::kBackToNormal:
      enterStep(Step::kDone, nowMillis);
      break;
    case Step::kDone:
      break;
  }
  return step_ != Step::kDone;
}
