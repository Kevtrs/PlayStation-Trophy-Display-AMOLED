// Simulateur PC du firmware PlayStation Trophy Display AMOLED.
// Partage src/ui/ avec le firmware ESP32-S3 ; seule cette couche (SDL2 +
// pilotes) est specifique au simulateur.

#define SDL_MAIN_HANDLED  // application console classique, pas de wrapper WinMain
#include <ArduinoJson.h>
#include <SDL.h>
#include <lvgl.h>

#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <string>
#include <vector>

#include "DebugPanel.h"
#include "DisplayDriverSdl.h"
#include "NullBrightnessBackend.h"
#include "TouchDriverSdl.h"
#include "WiFiManagerStub.h"
#include "PocketPsnHttpClientStub.h"
#include "ShowroomScenario.h"
#include "app/AppController.h"
#include "config/ConfigManager.h"
#include "data/DemoDataProvider.h"
#include "data/PocketPsnHtmlParser.h"
#include "data/PocketPsnParser.h"
#include "data/PocketPsnProvider.h"
#include "data/ProviderFactory.h"
#include "data/TrophyRepository.h"
#include "storage/FilePersistentStore.h"
#include "storage/TrophyCache.h"
#include "services/PendingRestart.h"
#include "services/SyncService.h"
#include "ui/RoundUiBridge.h"
#include "ui/GestureRecognizer.h"
#include "ui/Layout.h"
#include "ui/trophy_display_ui.hpp"
#include "web/WebApiHandlers.h"

namespace {

constexpr int kDebugPanelWidth = 340;
constexpr int kDebugPanelHeight = 720;

// Localise un fichier du depot depuis quelques chemins relatifs plausibles
// selon le repertoire de travail au lancement (simulator/build/, racine du
// depot...) -- meme logique defensive que pour les dossiers de captures
// d'ecran.
std::string findRepoFile(const char* relativePath) {
  const std::vector<std::string> prefixes = {"", "../", "../../", "../../../"};
  for (const auto& prefix : prefixes) {
    std::string candidate = prefix + relativePath;
    if (std::filesystem::exists(candidate)) return candidate;
  }
  return "";
}

// Extrait les noms de champs lus par data/app.js sous la forme
// "diagnostics.xxx"/"diagnostics?.xxx" (voir renderDiagnostics()) --
// verification statique (voir --selftest) que ces champs existent bien
// dans la reponse GET /api/diagnostics, sans dupliquer la liste a la main
// (qui se desynchroniserait silencieusement du vrai fichier si app.js
// change plus tard).
std::set<std::string> extractDiagnosticsFieldsReadByAppJs(const std::string& appJsContent) {
  std::set<std::string> fields;
  std::regex fieldRegex(R"(diagnostics(?:\?)?\.([A-Za-z_][A-Za-z0-9_]*))");
  for (std::sregex_iterator it(appJsContent.begin(), appJsContent.end(), fieldRegex), end; it != end; ++it) {
    fields.insert((*it)[1].str());
  }
  return fields;
}

// Teste PocketPsnParser (endpoint prive JSON) sur la vraie reponse obtenue
// le 2026-07-21 (voir test/fixtures/pocketpsn_response_real.json, cle
// privee de l'utilisateur -- jamais dans ce depot, corps deja anonymise)
// et sur des variantes construites a la main pour les cas de repli/erreur
// -- logique de parsing pure, ne necessite ni UiManager ni AppController.
bool runPocketPsnParserSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  auto readFile = [](const std::string& path) -> std::string {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  };

  // 1. Vraie reponse (format tableau, virgule trainante, Plus/Avatar
  // presents, pas de Status) -- confirmee stable sur deux requetes reelles
  // independantes le 2026-07-21 (voir docs/POCKETPSN_PROTOCOL.md).
  {
    std::string fixturePath = findRepoFile("test/fixtures/pocketpsn_response_real.json");
    check("PocketPsnParser fixture reelle : fichier localise", !fixturePath.empty());
    if (!fixturePath.empty()) {
      std::string json = readFile(fixturePath);
      PocketPsnParser::ParseResult r = PocketPsnParser::parse(json);
      check("PocketPsnParser fixture reelle : parse() reussit malgre la virgule trainante", r.ok());
      check("PocketPsnParser fixture reelle : username", r.profile.username == "TEST_PSN_USERNAME");
      check("PocketPsnParser fixture reelle : level=314", r.profile.level == 314);
      check("PocketPsnParser fixture reelle : hasPsPlus=true ('Plus':1)", r.profile.hasPsPlus);
      check("PocketPsnParser fixture reelle : avatarFileName", r.profile.avatarFileName == "A2117_l.png");
      check("PocketPsnParser fixture reelle : totalTrophies=3376", r.stats.totalTrophies == 3376);
      check("PocketPsnParser fixture reelle : worldRank=284939", r.stats.worldRank == 284939);
      check("PocketPsnParser fixture reelle : gamesCompleted=1 (Quick Stats tableau, 'Stat':\"1\")",
            r.stats.gamesCompleted == 1);
      check("PocketPsnParser fixture reelle : completionRatePercent=22 ('Stat':\"22%\")",
            std::abs(r.stats.completionRatePercent - 22.0f) < 0.01f);
      check("PocketPsnParser fixture reelle : averageRarityPercent≈38.76 ('Stat':\"38.76%\")",
            std::abs(r.stats.averageRarityPercent - 38.76f) < 0.01f);
      check("PocketPsnParser fixture reelle : unearnedTrophies=10743 (virgule de milliers geree)",
            r.stats.unearnedTrophies == 10743);
      check("PocketPsnParser fixture reelle : playtimeHours=2582 (virgule de milliers geree)",
            std::abs(r.stats.playtimeHours - 2582.0f) < 0.01f);
    }
  }

  // 2. Ancien format suppose ("Quick Stats" objet, jamais confirme par une
  // vraie reponse avant le 2026-07-21) -- doit rester supporte par
  // compatibilite.
  {
    const std::string oldFormatJson = R"({
      "Username": "OldFormatUser", "Country": "fr", "PSN Level": 10,
      "PSN Level Progress": 5, "PSN Level Remaining": 90,
      "Trophies Plats": 1, "Trophies Gold": 2, "Trophies Silver": 3, "Trophies Bronze": 4, "Trophies Total": 10,
      "Trophy Points": 100, "Pocket Points": 50, "Total Games": 5,
      "World Rank": 999, "Country Rank": 99,
      "Quick Stats": {"Games Completed": 2, "Completion Average": 50.0, "Average Rarity": 12.5,
                       "Unearned Trophies": 7, "Hours Played": 33.5}
    })";
    PocketPsnParser::ParseResult r = PocketPsnParser::parse(oldFormatJson);
    check("PocketPsnParser ancien format (Quick Stats objet) : parse() reussit", r.ok());
    check("PocketPsnParser ancien format : gamesCompleted=2", r.stats.gamesCompleted == 2);
    check("PocketPsnParser ancien format : playtimeHours=33.5", std::abs(r.stats.playtimeHours - 33.5f) < 0.01f);
    check("PocketPsnParser ancien format : hasPsPlus=false ('Plus' absent, valeur par defaut)",
          !r.profile.hasPsPlus);
    check("PocketPsnParser ancien format : avatarFileName vide ('Avatar' absent)", r.profile.avatarFileName.empty());
  }

  // 3. Reponse sans 'Plus' ni 'Avatar' (champs optionnels) -- ne doit pas
  // faire echouer le parsing, valeurs par defaut attendues.
  {
    const std::string noOptionalFieldsJson = R"({
      "Username": "NoOptionalFieldsUser",
      "Trophies Plats": 0, "Trophies Gold": 0, "Trophies Silver": 0, "Trophies Bronze": 1, "Trophies Total": 1,
      "Quick Stats": [{"Title": "Games Completed", "Stat": "0", "Percentile": 0.0}]
    })";
    PocketPsnParser::ParseResult r = PocketPsnParser::parse(noOptionalFieldsJson);
    check("PocketPsnParser sans Plus/Avatar : parse() reussit quand meme", r.ok());
    check("PocketPsnParser sans Plus/Avatar : hasPsPlus=false (valeur par defaut)", !r.profile.hasPsPlus);
    check("PocketPsnParser sans Plus/Avatar : avatarFileName vide (valeur par defaut)",
          r.profile.avatarFileName.empty());
    check("PocketPsnParser sans Plus/Avatar : username correct malgre champs optionnels absents",
          r.profile.username == "NoOptionalFieldsUser");
  }

  // 4. Reponse d'erreur avec 'Status' (aucun 'Username') -- 'Status' n'est
  // jamais exige en cas de succes, mais doit apparaitre dans le message
  // d'erreur pour faciliter le diagnostic quand Username est absent.
  {
    const std::string errorWithStatusJson = R"({"Status": "Not on PocketPSN"})";
    PocketPsnParser::ParseResult r = PocketPsnParser::parse(errorWithStatusJson);
    check("PocketPsnParser erreur avec Status : parse() echoue proprement", !r.ok());
    check("PocketPsnParser erreur avec Status : erreur = champs manquants",
          r.error == PocketPsnParser::ParseError::kMissingRequiredFields);
    check("PocketPsnParser erreur avec Status : message cite le Status recu",
          r.errorMessage.find("Not on PocketPSN") != std::string::npos);
  }

  // 5. Virgule trainante illegale isolee (cas minimal, independant de la
  // fixture reelle du test 1) -- doit etre nettoyee explicitement avant
  // parsing, jamais une simple esperance de tolerance d'ArduinoJson.
  {
    const std::string trailingCommaJson = R"({"Username": "TrailingCommaUser", "Trophies Total": 5,})";
    PocketPsnParser::ParseResult r = PocketPsnParser::parse(trailingCommaJson);
    check("PocketPsnParser virgule trainante isolee : parse() reussit (nettoyee avant parsing)", r.ok());
    check("PocketPsnParser virgule trainante isolee : donnees correctement lues malgre la virgule",
          r.profile.username == "TrailingCommaUser" && r.stats.totalTrophies == 5);
  }

  return allPassed;
}

// Teste le pipeline complet (PocketPsnProvider -> TrophyRepository ->
// TrophyCache -> SyncService) avec des donnees realistes, sans materiel :
// succes + persistance a travers un "redemarrage" simule (nouvelle
// instance de TrophyCache sur le meme fichier), reponse invalide rejetee
// sans corrompre le cache, panne de transport reseau, mode hors-ligne via
// SyncService, et un cycle repete pour la stabilite. Utilise un repertoire
// de stockage dedie (efface a la fin de chaque bloc) pour ne jamais
// interferer avec le cache demo de l'execution normale du simulateur.
bool runPocketPsnIntegrationSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  auto readFile = [](const std::string& path) -> std::string {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  };

  // Sous-dossier de simulator/.simulator_data/ (deja ignore par .gitignore)
  // plutot qu'un nouveau dossier a sa racine, pour rester couvert sans
  // ajouter de regle .gitignore dediee.
  const std::string kTestStoreDir = "simulator/.simulator_data/selftest_pocketpsn";

  std::string fixturePath = findRepoFile("test/fixtures/pocketpsn_response_real.json");
  check("integration : fixture reelle localisee", !fixturePath.empty());
  if (fixturePath.empty()) return false;
  std::string realResponseJson = readFile(fixturePath);

  // 1. Succes + persistance a travers un "redemarrage" simule.
  {
    FilePersistentStore store(kTestStoreDir);
    TrophyCache cache(store);
    cache.clear();  // depart propre, meme si un run precedent a laisse des donnees

    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 200;
    response.contentType = "application/json";
    response.body = realResponseJson;
    stub.queueResponse(response);

    PocketPsnProvider provider("gaz91610", "test-key-not-real", stub);
    TrophyRepository repo(provider, cache);
    repo.loadFromCache();
    check("integration succes : pas de donnees avant la premiere synchro", !repo.hasData());

    repo.requestRefresh();
    TrophyDataProvider::Status status = repo.poll(1700000000);
    check("integration succes : poll() -> kSuccess", status == TrophyDataProvider::Status::kSuccess);
    check("integration succes : profil correct (fixture reelle)",
          repo.profile().username == "TEST_PSN_USERNAME" && repo.profile().avatarFileName == "A2117_l.png" &&
              repo.profile().hasPsPlus);
    check("integration succes : stats correctes (fixture reelle)",
          repo.stats().totalTrophies == 3376 && repo.stats().unearnedTrophies == 10743);

    // "Redemarrage" simule : nouvelle instance de TrophyCache sur le meme
    // fichier, nouveau TrophyRepository (provider factice, jamais appele).
    FilePersistentStore store2(kTestStoreDir);
    TrophyCache cache2(store2);
    DemoDataProvider dummyProvider;
    TrophyRepository repo2(dummyProvider, cache2);
    repo2.loadFromCache();
    check("integration succes : donnees relues apres redemarrage simule", repo2.hasData());
    check("integration succes : avatarFileName survit au redemarrage (bug reel corrige)",
          repo2.profile().avatarFileName == "A2117_l.png");
    check("integration succes : hasPsPlus survit au redemarrage (bug reel corrige)", repo2.profile().hasPsPlus);
    check("integration succes : totalTrophies identique apres redemarrage",
          repo2.stats().totalTrophies == repo.stats().totalTrophies);

    cache2.clear();
  }

  // 2. Reponse invalide (incoherence detectee par TrophyRepository::validate())
  // ne doit jamais ecraser un cache valide existant.
  {
    FilePersistentStore store(kTestStoreDir);
    TrophyCache cache(store);
    cache.clear();

    ProfileData goodProfile;
    goodProfile.username = "GoodUser";
    TrophyStats goodStats;
    goodStats.totalTrophies = 50;
    goodStats.platinum = 1;
    goodStats.gold = 10;
    goodStats.silver = 15;
    goodStats.bronze = 24;
    cache.save(goodProfile, goodStats, 1700000000);

    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 200;
    response.contentType = "application/json";
    // Username present mais incoherence totalTrophies>0 / detail=0 (reponse
    // partielle plausible) -> doit etre rejete par validate().
    response.body =
        R"({"Username": "BadUser", "Trophies Total": 100, "Trophies Plats": 0, "Trophies Gold": 0, "Trophies Silver": 0, "Trophies Bronze": 0})";
    stub.queueResponse(response);

    PocketPsnProvider provider("BadUser", "test-key-not-real", stub);
    TrophyRepository repo(provider, cache);
    repo.loadFromCache();
    check("integration invalide : cache pre-rempli charge (GoodUser)", repo.profile().username == "GoodUser");

    repo.requestRefresh();
    TrophyDataProvider::Status status = repo.poll(1700003600);
    check("integration invalide : poll() -> kError (rejete par validate())",
          status == TrophyDataProvider::Status::kError);
    check("integration invalide : profil en memoire inchange (toujours GoodUser)",
          repo.profile().username == "GoodUser");
    check("integration invalide : message d'erreur explicite",
          repo.lastError().message.find("rejetees") != std::string::npos);

    FilePersistentStore store2(kTestStoreDir);
    TrophyCache cache2(store2);
    cache2.load();
    check("integration invalide : cache persiste toujours GoodUser (non corrompu)",
          cache2.profile().username == "GoodUser");

    cache.clear();
  }

  // 3. Panne de transport reseau (DNS/TLS/timeout) -- ne doit pas non plus
  // corrompre un cache existant, et doit remonter une erreur distincte.
  {
    FilePersistentStore store(kTestStoreDir);
    TrophyCache cache(store);
    cache.clear();

    ProfileData goodProfile;
    goodProfile.username = "OfflineCacheUser";
    TrophyStats goodStats;
    goodStats.totalTrophies = 20;
    goodStats.gold = 5;
    goodStats.silver = 5;
    goodStats.bronze = 10;
    cache.save(goodProfile, goodStats, 1700000000);

    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = false;  // panne DNS/TLS/timeout
    stub.queueResponse(response);

    PocketPsnProvider provider("OfflineCacheUser", "test-key-not-real", stub);
    TrophyRepository repo(provider, cache);
    repo.loadFromCache();
    repo.requestRefresh();
    TrophyDataProvider::Status status = repo.poll(1700003600);
    check("integration panne reseau : poll() -> kError", status == TrophyDataProvider::Status::kError);
    check("integration panne reseau : dernieres donnees valides toujours servies (cache)",
          repo.profile().username == "OfflineCacheUser" && repo.stats().totalTrophies == 20);

    cache.clear();
  }

  // 4. Mode hors-ligne via SyncService (aucune tentative reseau, dernieres
  // donnees en cache toujours servies).
  {
    FilePersistentStore store(kTestStoreDir);
    TrophyCache cache(store);
    cache.clear();

    ProfileData profile;
    profile.username = "SyncOfflineUser";
    TrophyStats stats;
    stats.totalTrophies = 5;
    stats.gold = 2;
    stats.silver = 1;
    stats.bronze = 2;
    cache.save(profile, stats, 1700000000);

    DemoDataProvider dummyProvider;  // jamais appele : reseau indisponible
    TrophyRepository repo(dummyProvider, cache);
    repo.loadFromCache();

    SyncService sync(repo);
    sync.configure(30);
    sync.setNetworkAvailable(false);
    sync.requestManualSync();  // sans effet : aucune tentative tant que hors-ligne
    sync.poll(0, 1700003600);

    check("integration hors-ligne : SyncStatus.state == kOffline", sync.status().state == SyncState::kOffline);
    check("integration hors-ligne : isOffline == true", sync.status().isOffline);
    check("integration hors-ligne : donnees en cache toujours servies malgre l'absence de reseau",
          repo.profile().username == "SyncOfflineUser" && repo.hasData());

    cache.clear();
  }

  // 5. Stabilite : repete le cycle parse+sauvegarde+relecture N fois avec
  // la vraie fixture -- verifie l'absence de plantage/derive sur des
  // executions repetees (sans outil de profilage memoire, verification
  // minimale possible sans materiel).
  {
    constexpr int kIterations = 30;
    FilePersistentStore store(kTestStoreDir);
    TrophyCache cache(store);
    cache.clear();
    bool allIterationsConsistent = true;

    for (int i = 0; i < kIterations; ++i) {
      PocketPsnHttpClientStub stub;
      IPocketPsnHttpClient::Response response;
      response.transportOk = true;
      response.httpStatus = 200;
      response.contentType = "application/json";
      response.body = realResponseJson;
      stub.queueResponse(response);

      PocketPsnProvider provider("gaz91610", "test-key-not-real", stub);
      TrophyRepository repo(provider, cache);
      repo.requestRefresh();
      TrophyDataProvider::Status status = repo.poll(1700000000u + static_cast<uint32_t>(i));
      if (status != TrophyDataProvider::Status::kSuccess || repo.stats().totalTrophies != 3376) {
        allIterationsConsistent = false;
        break;
      }
    }
    check("integration stabilite : 30 cycles parse+cache repetes, resultats stables", allIterationsConsistent);
    cache.clear();
  }

  return allPassed;
}

// Implementation vide de UiBridge pour les tests headless ci-dessous :
// aucune dependance LVGL/design, uniquement pour satisfaire le constructeur
// d'AppController (voir ui/UiBridge.h -- interface pure, prevue pour etre
// implementee par un futur design sans toucher a AppController).
struct NullUiBridge : public UiBridge {
  void setAppState(const AppState&) override {}
  void showSyncState(SyncState) override {}
  void showTrophyDelta(const TrophyDelta&) override {}
  void showError(const AppError&) override {}
  void showBootProgress(BootStep) override {}
};

// Meme role que NullUiBridge, mais enregistre chaque etape Boot B1 recue
// (voir BootStep.h) pour verifier, sans aucune dependance LVGL/design, que
// la progression reste strictement croissante et atteint bien kAppReady
// dans les scenarios de demarrage reels ci-dessous (avec cache, sans
// reseau, avec succes/erreur Pocket PSN) -- meme verification que
// verifyBootSequence() (mode interactif), mais reproductible et rapide en
// --selftest.
struct SpyUiBridge : public UiBridge {
  void setAppState(const AppState&) override {}
  void showSyncState(SyncState) override {}
  void showTrophyDelta(const TrophyDelta&) override {}
  void showError(const AppError&) override {}
  void showBootProgress(BootStep step) override {
    int percent = bootStepPercent(step);
    if (percent < lastPercent) regressed = true;
    lastPercent = percent;
    if (step == BootStep::kAppReady) reachedAppReady = true;
  }

  static int bootStepPercent(BootStep step) {
    switch (step) {
      case BootStep::kSystemStart: return 0;
      case BootStep::kConfigLoaded: return 15;
      case BootStep::kCacheLoaded: return 30;
      case BootStep::kNetworkInit: return 45;
      case BootStep::kProfileLoaded: return 60;
      case BootStep::kDataReady: return 75;
      case BootStep::kUiReady: return 90;
      case BootStep::kAppReady: return 100;
    }
    return -1;
  }

  int lastPercent = -1;
  bool regressed = false;
  bool reachedAppReady = false;
};

// Simulation longue duree du flux complet AppController (WiFiManagerStub +
// PocketPsnProvider/PocketPsnHttpClientStub + TrophyCache/TrophyRepository/
// SyncService reels), sans materiel et sans aucune dependance LVGL/design
// (NullUiBridge ci-dessus, NullBrightnessBackend deja utilise ailleurs) --
// voir demande explicite de l'utilisateur (design reserve a une passe ulterieure).
// Chaque scenario utilise un sous-dossier dedie de
// simulator/.simulator_data/ (deja ignore par Git), efface avant/apres.
bool runAppControllerLongRunSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  auto readFile = [](const std::string& path) -> std::string {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  };
  std::string fixturePath = findRepoFile("test/fixtures/pocketpsn_response_real.json");
  check("AppController long-run : fixture reelle localisee", !fixturePath.empty());
  if (fixturePath.empty()) return false;
  std::string realResponseJson = readFile(fixturePath);

  // Corps synthetiques (jamais une vraie reponse Pocket PSN, clairement
  // nommes) pour les scenarios narratifs ou plusieurs syncs distinctes sont
  // necessaires -- totaux croissants pour rester acceptes par
  // TrophyRepository::validate() (qui rejette toute baisse).
  const std::string kSyntheticUserA =
      R"({"Username": "UserA", "Trophies Total": 10, "Trophies Plats": 1, "Trophies Gold": 2, "Trophies Silver": 3, "Trophies Bronze": 4})";
  const std::string kSyntheticUserB =
      R"({"Username": "UserB", "Trophies Total": 20, "Trophies Plats": 2, "Trophies Gold": 3, "Trophies Silver": 5, "Trophies Bronze": 10})";
  const std::string kSyntheticUserC =
      R"({"Username": "UserC", "Trophies Total": 30, "Trophies Plats": 3, "Trophies Gold": 4, "Trophies Silver": 8, "Trophies Bronze": 15})";
  const std::string kSyntheticInvalid =
      R"({"Username": "UserInvalid", "Trophies Total": 999, "Trophies Plats": 0, "Trophies Gold": 0, "Trophies Silver": 0, "Trophies Bronze": 0})";

  auto makeSuccessResponse = [](const std::string& body) {
    IPocketPsnHttpClient::Response r;
    r.transportOk = true;
    r.httpStatus = 200;
    r.contentType = "application/json";
    r.body = body;
    return r;
  };

  uint32_t nowMillis = 0;
  uint32_t nowEpoch = 1700000000;
  auto advance = [&](uint32_t deltaMs) {
    nowMillis += deltaMs;
    nowEpoch += deltaMs / 1000;
  };

  // 1. Demarrage avec Internet : synchro automatique dès le premier tick
  // une fois le Wi-Fi connecte (voir SyncService -- everAttempted_ force la
  // toute premiere synchro sans attendre un intervalle complet).
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_1";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    TrophyCache(cacheStore).clear();  // depart propre

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(realResponseJson));
    PocketPsnProvider provider("gaz91610", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    SpyUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    check("scenario 1 (demarrage avec Internet) : hors-ligne juste apres begin()", app.state().sync.isOffline);
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 1 : synchro reussie automatiquement une fois connecte",
          app.state().sync.state == SyncState::kSuccess);
    check("scenario 1 : profil reel charge (fixture Pocket PSN)",
          app.state().profile.username == "TEST_PSN_USERNAME" && app.state().stats.totalTrophies == 3376);
    check("scenario 1 : plus hors-ligne", !app.state().sync.isOffline);
    check("scenario 1 (Boot B1, premier demarrage + succes Pocket PSN) : progression croissante",
          !ui.regressed);
    check("scenario 1 (Boot B1) : atteint kAppReady (100 %)", ui.reachedAppReady);

    TrophyCache(cacheStore).clear();
  }

  // 2. Demarrage hors ligne avec cache : donnees precedemment persistees
  // servies immediatement, sans jamais se connecter.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_2";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    {
      TrophyCache preload(cacheStore);
      preload.clear();
      ProfileData p;
      p.username = "CachedOfflineUser";
      TrophyStats s;
      s.totalTrophies = 42;
      s.platinum = 1;
      s.gold = 10;
      s.silver = 15;
      s.bronze = 16;
      preload.save(p, s, nowEpoch);
    }

    PocketPsnHttpClientStub stub;  // jamais interrogee : reseau jamais connecte
    PocketPsnProvider provider("CachedOfflineUser", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    SpyUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    check("scenario 2 (demarrage hors ligne) : donnees en cache servies des begin()",
          app.state().profile.username == "CachedOfflineUser" && app.state().stats.totalTrophies == 42);

    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);  // Wi-Fi jamais connecte (WiFiManagerStub reste en point d'acces par defaut)
    }
    check("scenario 2 : reste hors-ligne (aucune connexion simulee)", app.state().sync.isOffline);
    check("scenario 2 : donnees en cache toujours affichees apres plusieurs tick()",
          app.state().profile.username == "CachedOfflineUser");
    check("scenario 2 (Boot B1, avec cache + sans reseau) : progression croissante", !ui.regressed);
    check("scenario 2 (Boot B1) : atteint kAppReady (100 %) sans attendre un reseau absent",
          ui.reachedAppReady);

    TrophyCache(cacheStore).clear();
  }

  // 3+4. Perte du Wi-Fi puis reconnexion : une synchro reussit, le Wi-Fi
  // tombe (aucune nouvelle synchro tant qu'il est coupe -- sur ce
  // simulateur synchrone, sans tache de fond FreeRTOS, une synchro
  // s'execute integralement en un seul poll() : la coupure est donc
  // testee ENTRE deux tentatives, pas au milieu d'une requete en vol, ce
  // qui reste le comportement reellement observable ici), puis la
  // reconnexion relance une synchro reussie avec de nouvelles donnees.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_3";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    TrophyCache(cacheStore).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kSyntheticUserA));
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    NullUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 3/4 : premiere synchro reussie (UserA)",
          app.state().sync.state == SyncState::kSuccess && app.state().profile.username == "UserA");
    int syncCountAfterFirst = app.syncTotalCount();

    // Coupure Wi-Fi : queue une reponse que la synchro ne doit JAMAIS
    // consommer tant que le reseau est indisponible.
    stub.queueResponse(makeSuccessResponse(kSyntheticUserB));
    wifiStub.simulateDisconnected();
    for (int i = 0; i < 10; ++i) {
      advance(500);
      app.tick(nowMillis);
    }
    check("scenario 3 : hors-ligne detecte apres la coupure Wi-Fi", app.state().sync.isOffline);
    check("scenario 3 : aucune nouvelle synchro tant que hors-ligne (toujours UserA)",
          app.state().profile.username == "UserA" && app.syncTotalCount() == syncCountAfterFirst);

    // Reconnexion : constat reel important (voir HANDOFF_PROGRESS.md) --
    // SyncService ne relance PAS automatiquement de synchro sur la seule
    // reconnexion : everAttempted_ reste vrai (deja mis a true par la
    // premiere synchro reussie) et aucun echec n'a ete comptabilise pendant
    // la coupure (le retour "hors-ligne" court-circuite avant tout calcul
    // de backoff), donc ni l'intervalle complet (30 min par defaut) ni le
    // backoff ne s'appliquent. Une action explicite (synchro manuelle,
    // ex: l'utilisateur rouvre l'app, ou un futur appel automatique a
    // ajouter cote produit) est necessaire ici -- signale a l'utilisateur.
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 4 : reconnexion seule ne relance PAS de synchro (comportement reel actuel)",
          app.syncTotalCount() == syncCountAfterFirst);

    app.requestManualSync();
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 4 : synchro manuelle apres reconnexion reussit (UserB)",
          app.state().sync.state == SyncState::kSuccess && app.state().profile.username == "UserB");
    check("scenario 4 : compteur de synchros incremente", app.syncTotalCount() == syncCountAfterFirst + 1);

    TrophyCache(cacheStore).clear();
  }

  // 5. Reponse API invalide : ne doit jamais ecraser le cache existant.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_5";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    TrophyCache(cacheStore).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kSyntheticUserA));
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    NullUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 5 : premiere synchro valide reussie (UserA)", app.state().profile.username == "UserA");

    stub.queueResponse(makeSuccessResponse(kSyntheticInvalid));
    app.requestManualSync();
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 5 : synchro invalide rejetee (kError)", app.state().sync.state == SyncState::kError);
    check("scenario 5 : profil INCHANGE malgre la reponse invalide (toujours UserA)",
          app.state().profile.username == "UserA" && app.state().stats.totalTrophies == 10);

    TrophyCache reload(cacheStore);
    reload.load();
    check("scenario 5 : cache persistant non corrompu (toujours UserA)", reload.profile().username == "UserA");

    TrophyCache(cacheStore).clear();
  }

  // 6. Plusieurs synchronisations successives (manuelles), donnees et
  // compteur mis a jour a chaque fois.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_6";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    TrophyCache(cacheStore).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kSyntheticUserA));
    PocketPsnProvider provider("Multi", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    NullUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 6 : synchro 1/3 (UserA, 10 trophees)",
          app.state().profile.username == "UserA" && app.state().stats.totalTrophies == 10);

    stub.queueResponse(makeSuccessResponse(kSyntheticUserB));
    app.requestManualSync();
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 6 : synchro 2/3 (UserB, 20 trophees)",
          app.state().profile.username == "UserB" && app.state().stats.totalTrophies == 20);

    stub.queueResponse(makeSuccessResponse(kSyntheticUserC));
    app.requestManualSync();
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 6 : synchro 3/3 (UserC, 30 trophees)",
          app.state().profile.username == "UserC" && app.state().stats.totalTrophies == 30);
    check("scenario 6 : compteur total = 3 synchros reussies", app.syncTotalCount() == 3);
    check("scenario 6 : aucun echec comptabilise", app.syncFailureCount() == 0);

    TrophyCache(cacheStore).clear();
  }

  // 7. Redemarrage simule entre deux synchronisations : les donnees de la
  // premiere synchro survivent (nouvelle instance d'AppController sur les
  // memes fichiers), la seconde synchro s'enchaine normalement ensuite.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_7";
    FilePersistentStore configStore1(dir);
    FilePersistentStore cacheStore1(dir);
    TrophyCache(cacheStore1).clear();

    {
      PocketPsnHttpClientStub stub;
      stub.queueResponse(makeSuccessResponse(kSyntheticUserA));
      PocketPsnProvider provider("UserA", "test-key-not-real", stub);
      WiFiManagerStub wifiStub;
      NullBrightnessBackend brightness;
      NullUiBridge ui;
      AppController app(configStore1, cacheStore1, provider, brightness, wifiStub, ui);

      app.begin();
      wifiStub.simulateConnected("Home");
      for (int i = 0; i < 5; ++i) {
        advance(200);
        app.tick(nowMillis);
      }
      check("scenario 7 : premiere synchro reussie avant redemarrage (UserA)",
            app.state().profile.username == "UserA" && app.syncTotalCount() == 1);
    }  // "extinction" : toutes les instances sont detruites ici

    // "Redemarrage" : nouvelles instances sur les memes fichiers de
    // persistance (config_/cache_).
    FilePersistentStore configStore2(dir);
    FilePersistentStore cacheStore2(dir);
    PocketPsnHttpClientStub stub2;
    stub2.queueResponse(makeSuccessResponse(kSyntheticUserB));
    PocketPsnProvider provider2("UserB", "test-key-not-real", stub2);
    WiFiManagerStub wifiStub2;
    NullBrightnessBackend brightness2;
    NullUiBridge ui2;
    AppController app2(configStore2, cacheStore2, provider2, brightness2, wifiStub2, ui2);

    app2.begin();
    check("scenario 7 : donnees de la premiere synchro relues apres redemarrage (UserA)",
          app2.state().profile.username == "UserA");
    check("scenario 7 : compteur de synchros reinitialise apres redemarrage (non persiste, attendu)",
          app2.syncTotalCount() == 0);

    wifiStub2.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app2.tick(nowMillis);
    }
    check("scenario 7 : deuxieme synchro reussie apres redemarrage (UserB)",
          app2.state().profile.username == "UserB" && app2.syncTotalCount() == 1);

    TrophyCache(cacheStore2).clear();
  }

  // 8. Boot B1 avec erreur Pocket PSN des la toute premiere synchro (reseau
  // connecte, mais la reponse recue est invalide) -- distinct du scenario 5
  // ci-dessus, ou l'echec ne survient qu'a une DEUXIEME synchro, apres un
  // demarrage deja reussi : ici, aucune synchro n'a encore jamais abouti
  // quand l'erreur survient, donc profile()/stats() restent aux valeurs par
  // defaut (jamais de donnees a afficher). Le demarrage doit malgre tout se
  // terminer normalement (atteindre kAppReady) plutot que rester bloque en
  // attente d'une premiere synchro qui n'arrivera jamais.
  {
    const std::string dir = "simulator/.simulator_data/selftest_appcontroller_8";
    FilePersistentStore configStore(dir);
    FilePersistentStore cacheStore(dir);
    TrophyCache(cacheStore).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kSyntheticInvalid));
    PocketPsnProvider provider("UserInvalid", "test-key-not-real", stub);
    WiFiManagerStub wifiStub;
    NullBrightnessBackend brightness;
    SpyUiBridge ui;
    AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

    app.begin();
    wifiStub.simulateConnected("Home");
    for (int i = 0; i < 5; ++i) {
      advance(200);
      app.tick(nowMillis);
    }
    check("scenario 8 (erreur Pocket PSN des le premier demarrage) : synchro rejetee (kError)",
          app.state().sync.state == SyncState::kError);
    check("scenario 8 (Boot B1) : progression croissante malgre l'erreur", !ui.regressed);
    check("scenario 8 (Boot B1) : atteint kAppReady (100 %) sans rester bloque en attente de donnees",
          ui.reachedAppReady);

    TrophyCache(cacheStore).clear();
  }

  return allPassed;
}

// Teste le debounce de reconnexion de SyncService (voir
// kReconnectStabilizationMs) directement (sans AppController/WiFiManagerStub,
// controle precis des millis simules) : une reconnexion stable declenche
// une seule synchro apres le delai de stabilisation, les reconnexions
// rapides et repetees n'en declenchent aucune tant que la connexion n'est
// pas restee stable, le cache reste affiche pendant l'attente, et le
// backoff existant n'est jamais contourne (ni declenche en double) si des
// echecs sont deja en cours -- demande explicite de l'utilisateur.
bool runSyncServiceReconnectDebounceSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  auto makeSuccessResponse = [](const std::string& body) {
    IPocketPsnHttpClient::Response r;
    r.transportOk = true;
    r.httpStatus = 200;
    r.contentType = "application/json";
    r.body = body;
    return r;
  };
  auto makeFailureResponse = []() {
    IPocketPsnHttpClient::Response r;
    r.transportOk = false;
    return r;
  };

  const std::string kUserA =
      R"({"Username": "UserA", "Trophies Total": 10, "Trophies Plats": 1, "Trophies Gold": 2, "Trophies Silver": 3, "Trophies Bronze": 4})";
  const std::string kUserB =
      R"({"Username": "UserB", "Trophies Total": 20, "Trophies Plats": 2, "Trophies Gold": 3, "Trophies Silver": 5, "Trophies Bronze": 10})";

  // 1. Reconnexion stable (>= 4s) : une seule synchro, declenchee
  // uniquement une fois le seuil depasse ; le cache reste affiche pendant
  // toute la fenetre de stabilisation.
  {
    const std::string dir = "simulator/.simulator_data/selftest_reconnect_1";
    FilePersistentStore store(dir);
    TrophyCache(store).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kUserA));
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    TrophyCache cache(store);
    TrophyRepository repo(provider, cache);
    SyncService sync(repo);
    sync.configure(30);

    uint32_t nowMillis = 0;
    sync.setNetworkAvailable(true);
    sync.poll(nowMillis, 1700000000);  // premiere synchro (bypass everAttempted_, pas le debounce)
    check("reconnect-debounce 1 : premiere synchro reussie (UserA)",
          sync.status().state == SyncState::kSuccess && repo.profile().username == "UserA");

    stub.queueResponse(makeSuccessResponse(kUserB));
    sync.setNetworkAvailable(false);
    nowMillis += 1000;
    sync.poll(nowMillis, 1700000001);
    check("reconnect-debounce 1 : hors-ligne detecte", sync.status().isOffline);

    sync.setNetworkAvailable(true);
    nowMillis += 100;
    sync.poll(nowMillis, 1700000002);
    check("reconnect-debounce 1 : pas de synchro immediate a la reconnexion (cache -- UserA -- toujours affiche)",
          repo.profile().username == "UserA");
    check("reconnect-debounce 1 : state ne reste PAS a kOffline une fois reconnecte (kIdle, pas encore de synchro)",
          sync.status().state == SyncState::kIdle && !sync.status().isOffline);

    nowMillis += 2000;  // ~2.1s depuis la reconnexion : encore sous le seuil de 4s
    sync.poll(nowMillis, 1700000004);
    check("reconnect-debounce 1 : toujours pas de synchro a ~2.1s (sous le seuil, cache toujours affiche)",
          repo.profile().username == "UserA");
    check("reconnect-debounce 1 : state toujours kIdle pendant l'attente (jamais kOffline, jamais de synchro forcee)",
          sync.status().state == SyncState::kIdle);

    nowMillis += 2500;  // ~4.6s depuis la reconnexion : seuil de stabilisation depasse
    sync.poll(nowMillis, 1700000007);
    check("reconnect-debounce 1 : synchro declenchee une fois le seuil depasse (UserB)",
          sync.status().state == SyncState::kSuccess && repo.profile().username == "UserB");

    TrophyCache(store).clear();
  }

  // 2. Reconnexions rapides et repetees (flapping) : aucune synchro tant
  // que la connexion n'est pas restee stable, une seule au final.
  {
    const std::string dir = "simulator/.simulator_data/selftest_reconnect_2";
    FilePersistentStore store(dir);
    TrophyCache(store).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kUserA));
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    TrophyCache cache(store);
    TrophyRepository repo(provider, cache);
    SyncService sync(repo);
    sync.configure(30);

    uint32_t nowMillis = 0;
    sync.setNetworkAvailable(true);
    sync.poll(nowMillis, 1700000000);
    int syncCountAfterFirst = sync.status().totalSyncCount;
    check("reconnect-debounce 2 : premiere synchro comptee", syncCountAfterFirst == 1);

    stub.queueResponse(makeSuccessResponse(kUserB));
    for (int i = 0; i < 4; ++i) {
      sync.setNetworkAvailable(false);
      nowMillis += 300;
      sync.poll(nowMillis, 1700000000 + nowMillis / 1000);
      sync.setNetworkAvailable(true);
      nowMillis += 300;
      sync.poll(nowMillis, 1700000000 + nowMillis / 1000);
    }
    check("reconnect-debounce 2 : aucune synchro pendant le flapping (reconnexions < 4s a chaque fois)",
          sync.status().totalSyncCount == syncCountAfterFirst);
    check("reconnect-debounce 2 : profil toujours UserA pendant le flapping", repo.profile().username == "UserA");

    nowMillis += 4200;  // reste connecte, stable, au-dela du seuil cette fois
    sync.poll(nowMillis, 1700000000 + nowMillis / 1000);
    check("reconnect-debounce 2 : une seule synchro apres stabilisation finale (UserB)",
          sync.status().totalSyncCount == syncCountAfterFirst + 1 && repo.profile().username == "UserB");

    TrophyCache(store).clear();
  }

  // 3. Un echec deja en cours (backoff actif) n'est jamais contourne par
  // une reconnexion stabilisee -- seul le backoff existant gere la reprise.
  {
    const std::string dir = "simulator/.simulator_data/selftest_reconnect_3";
    FilePersistentStore store(dir);
    TrophyCache(store).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeFailureResponse());
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    TrophyCache cache(store);
    TrophyRepository repo(provider, cache);
    SyncService sync(repo);
    sync.configure(30);

    uint32_t nowMillis = 0;
    sync.setNetworkAvailable(true);
    sync.poll(nowMillis, 1700000000);
    check("reconnect-debounce 3 : premier echec comptabilise",
          sync.status().state == SyncState::kError && sync.status().consecutiveFailures == 1);

    stub.queueResponse(makeSuccessResponse(kUserA));
    sync.setNetworkAvailable(false);
    nowMillis += 100;
    sync.poll(nowMillis, 1700000000);
    sync.setNetworkAvailable(true);
    nowMillis += 100;
    sync.poll(nowMillis, 1700000000);
    nowMillis += 4200;  // stabilisation depassee, mais AVANT l'expiration du backoff (5000ms)
    sync.poll(nowMillis, 1700000004);
    // "state" reflete maintenant la reconnexion (kIdle, plus kOffline)
    // sans qu'aucune synchro supplementaire n'ait ete forcee -- voir le
    // fix dedie (state ne reste plus jamais bloque a kOffline une fois
    // reconnecte).
    check("reconnect-debounce 3 : aucune tentative forcee par la reconnexion (echec en cours, backoff respecte)",
          sync.status().totalSyncCount == 1 && sync.status().consecutiveFailures == 1 && !sync.status().isOffline);
    check("reconnect-debounce 3 : state reflete la reconnexion (kIdle) malgre l'echec anterieur, jamais kOffline",
          sync.status().state == SyncState::kIdle);

    nowMillis = 5100;  // le backoff EXISTANT (5000ms) est desormais ecoule depuis le premier echec
    sync.poll(nowMillis, 1700000005);
    check("reconnect-debounce 3 : le backoff existant (pas le debounce) finit par relancer une tentative (UserA)",
          sync.status().totalSyncCount == 2 && sync.status().state == SyncState::kSuccess &&
              repo.profile().username == "UserA");

    TrophyCache(store).clear();
  }

  // 4. Si la synchro declenchee par la reconnexion echoue elle-meme,
  // aucune boucle de nouvelles tentatives immediates -- seul le backoff
  // normal reprend ensuite.
  {
    const std::string dir = "simulator/.simulator_data/selftest_reconnect_4";
    FilePersistentStore store(dir);
    TrophyCache(store).clear();

    PocketPsnHttpClientStub stub;
    stub.queueResponse(makeSuccessResponse(kUserA));
    PocketPsnProvider provider("UserA", "test-key-not-real", stub);
    TrophyCache cache(store);
    TrophyRepository repo(provider, cache);
    SyncService sync(repo);
    sync.configure(30);

    uint32_t nowMillis = 0;
    sync.setNetworkAvailable(true);
    sync.poll(nowMillis, 1700000000);
    check("reconnect-debounce 4 : premiere synchro reussie", sync.status().state == SyncState::kSuccess);

    stub.queueResponse(makeFailureResponse());
    sync.setNetworkAvailable(false);
    nowMillis += 100;
    sync.poll(nowMillis, 1700000000);
    sync.setNetworkAvailable(true);
    nowMillis += 100;
    sync.poll(nowMillis, 1700000000);  // enregistre l'instant de reconnexion (debut de la stabilisation)
    nowMillis += 4200;
    sync.poll(nowMillis, 1700000004);
    check("reconnect-debounce 4 : synchro post-reconnexion tentee et echouee",
          sync.status().state == SyncState::kError && sync.status().consecutiveFailures == 1);
    int totalAfterReconnectFailure = sync.status().totalSyncCount;

    for (int i = 0; i < 10; ++i) {
      nowMillis += 100;
      sync.poll(nowMillis, 1700000004 + static_cast<uint32_t>(i));
    }
    check("reconnect-debounce 4 : aucune tentative supplementaire avant l'expiration du backoff (pas de boucle)",
          sync.status().totalSyncCount == totalAfterReconnectFailure);

    TrophyCache(store).clear();
  }

  return allPassed;
}

// Test de stabilite longue duree (sans materiel) : plusieurs centaines de
// cycles connexion/synchro/coupure/reponse-invalide avec le pipeline
// AppController complet, pour detecter un plantage, une derive d'etat ou
// une croissance non bornee du fichier de cache -- preparation avant
// validation materielle reelle (voir demande explicite de l'utilisateur).
bool runLongDurationStressSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  const std::string dir = "simulator/.simulator_data/selftest_stress";
  FilePersistentStore configStore(dir);
  FilePersistentStore cacheStore(dir);
  TrophyCache(cacheStore).clear();

  const std::string kStressResponse =
      R"({"Username": "StressUser", "Trophies Total": 50, "Trophies Plats": 5, "Trophies Gold": 10, "Trophies Silver": 15, "Trophies Bronze": 20, "Quick Stats": [{"Title": "Games Completed", "Stat": "3", "Percentile": 0.5}]})";
  const std::string kInvalidResponse =
      R"({"Username": "Bad", "Trophies Total": 999, "Trophies Plats": 0, "Trophies Gold": 0, "Trophies Silver": 0, "Trophies Bronze": 0})";

  PocketPsnHttpClientStub stub;
  IPocketPsnHttpClient::Response okResponse;
  okResponse.transportOk = true;
  okResponse.httpStatus = 200;
  okResponse.contentType = "application/json";
  okResponse.body = kStressResponse;
  stub.queueResponse(okResponse);

  PocketPsnProvider provider("StressUser", "test-key-not-real", stub);
  WiFiManagerStub wifiStub;
  NullBrightnessBackend brightness;
  NullUiBridge ui;
  AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);

  app.begin();
  wifiStub.simulateConnected("StressNet");

  constexpr int kCycles = 500;
  uint32_t nowMillis = 0;
  bool inconsistentState = false;
  auto stressStart = std::chrono::steady_clock::now();

  for (int i = 0; i < kCycles; ++i) {
    // Coupure breve toutes les 7 iterations et reponse invalide toutes
    // les 11 : maximise les chemins de code exerces sur une meme
    // execution prolongee (coupures, reconnexions, rejets de validate()).
    if (i % 7 == 0) {
      wifiStub.simulateDisconnected();
      nowMillis += 300;
      app.tick(nowMillis);
      wifiStub.simulateConnected("StressNet");
    }
    IPocketPsnHttpClient::Response response = okResponse;
    if (i > 0 && i % 11 == 0) response.body = kInvalidResponse;  // jamais a i=0 : etablit d'abord un profil valide
    stub.queueResponse(response);

    app.requestManualSync();
    for (int t = 0; t < 3; ++t) {
      nowMillis += 200;
      app.tick(nowMillis);
    }
    if (app.state().profile.username.empty()) {
      inconsistentState = true;
      break;
    }
  }
  auto stressEnd = std::chrono::steady_clock::now();
  double elapsedSeconds = std::chrono::duration<double>(stressEnd - stressStart).count();

  check("stress longue duree : 500 cycles executes sans plantage ni profil vide", !inconsistentState);
  check("stress longue duree : profil final coherent (les reponses invalides n'ont jamais ecrase StressUser)",
        app.state().profile.username == "StressUser");
  check("stress longue duree : au moins une synchro reussie comptabilisee", app.syncTotalCount() > 0);
  std::printf("[selftest] stress longue duree : %d cycles executes en %.2f s\n", kCycles, elapsedSeconds);
  check("stress longue duree : temps d'execution raisonnable (< 5s pour 500 cycles)", elapsedSeconds < 5.0);

  // Le fichier de cache doit rester borne (ecriture atomique par
  // ecrasement, voir FilePersistentStore::save() -- jamais d'accumulation
  // au fil des cycles).
  std::string cacheFilePath = dir + "/trophy_cache.json";
  std::ifstream cacheFile(cacheFilePath, std::ios::binary | std::ios::ate);
  bool cacheFileFound = cacheFile.is_open();
  check("stress longue duree : fichier de cache localise sur disque", cacheFileFound);
  if (cacheFileFound) {
    std::streamsize size = cacheFile.tellg();
    check("stress longue duree : taille du fichier de cache bornee (pas d'accumulation, < 10 Ko)",
          size > 0 && size < 10 * 1024);
  }

  TrophyCache(cacheStore).clear();
  return allPassed;
}

// Verifie que ShowroomScenario (voir simulator/src/ShowroomScenario.h)
// traverse bien les 12 etapes attendues dans l'ordre exact, en pilotant les
// vrais services (AppController/SyncService/TrophyRepository) via des
// transports simules controles -- aucune horloge reelle ici (nowMillis
// avance par pas fixes), le test s'execute donc instantanement malgre les
// delais de maintien penses pour une demonstration humaine (voir les
// constantes kHoldXxxMs de ShowroomScenario.cpp).
bool runShowroomScenarioSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  const std::string dir = "simulator/.simulator_data/selftest_showroom";
  FilePersistentStore configStore(dir);
  FilePersistentStore cacheStore(dir);
  TrophyCache(cacheStore).clear();

  PocketPsnHttpClientStub stub;
  PocketPsnProvider provider("ShowroomUser", "test-key-not-real", stub);
  WiFiManagerStub wifiStub;
  NullBrightnessBackend brightness;
  NullUiBridge ui;
  AppController app(configStore, cacheStore, provider, brightness, wifiStub, ui);
  app.begin();

  ShowroomScenario scenario(app, wifiStub, stub);
  scenario.start(0);

  std::vector<ShowroomScenario::Step> visited;
  visited.push_back(scenario.currentStep());
  bool cacheStayedIntactDuringApiError = true;
  bool everBlankAfterFirstSync = false;

  uint32_t nowMillis = 0;
  constexpr uint32_t kStepMs = 100;
  constexpr int kMaxIterations = 600;  // 60 s simulees : tres large marge de securite
  bool reachedDone = false;

  for (int i = 0; i < kMaxIterations; ++i) {
    nowMillis += kStepMs;
    bool stillRunning = scenario.tick(nowMillis);
    app.tick(nowMillis);

    ShowroomScenario::Step step = scenario.currentStep();
    if (step != visited.back()) visited.push_back(step);

    if (step == ShowroomScenario::Step::kApiError &&
        (app.state().profile.username.empty() || app.state().profile.username != "ShowroomUser")) {
      cacheStayedIntactDuringApiError = false;
    }
    if (step != ShowroomScenario::Step::kStartup && step != ShowroomScenario::Step::kLoading &&
        step != ShowroomScenario::Step::kSyncing && app.state().profile.username.empty()) {
      everBlankAfterFirstSync = true;
    }

    if (!stillRunning) {
      reachedDone = true;
      break;
    }
  }

  check("showroom : la sequence automatique atteint l'etat termine avant le plafond de securite", reachedDone);

  static const ShowroomScenario::Step kExpectedOrder[] = {
      ShowroomScenario::Step::kStartup,      ShowroomScenario::Step::kLoading,
      ShowroomScenario::Step::kSyncing,      ShowroomScenario::Step::kProfileDisplay,
      ShowroomScenario::Step::kUsingCache,   ShowroomScenario::Step::kNetworkLost,
      ShowroomScenario::Step::kOffline,      ShowroomScenario::Step::kReconnecting,
      ShowroomScenario::Step::kResyncing,    ShowroomScenario::Step::kApiError,
      ShowroomScenario::Step::kBackToNormal, ShowroomScenario::Step::kDone,
  };
  bool orderMatches = visited.size() == sizeof(kExpectedOrder) / sizeof(kExpectedOrder[0]);
  if (orderMatches) {
    for (size_t i = 0; i < visited.size(); ++i) {
      if (visited[i] != kExpectedOrder[i]) {
        orderMatches = false;
        break;
      }
    }
  }
  check("showroom : les 12 etapes sont traversees dans l'ordre exact attendu, une seule fois chacune", orderMatches);

  check("showroom : synchronisation Pocket PSN reellement verifiee (isVerified) a la fin de la sequence",
        app.state().sync.pocketPsnVerified);
  check("showroom : profil jamais vide une fois la premiere synchronisation passee", !everBlankAfterFirstSync);
  check("showroom : le profil affiche reste intact pendant toute l'etape 'erreur API simulee' (cache non ecrase)",
        cacheStayedIntactDuringApiError);
  check("showroom : profil final coherent (ShowroomUser)", app.state().profile.username == "ShowroomUser");
  check("showroom : au moins une synchronisation reussie comptabilisee", app.syncTotalCount() > 0);

  TrophyCache(cacheStore).clear();
  return allPassed;
}

// Verifie la rotation automatique Dashboard/Trophees/Statistiques (voir
// App::tick()/set_auto_rotation(), branchee depuis AppSettings::
// autoRotateEnabled/rotationIntervalSeconds via RoundUiBridge -- reglage
// deja expose dans le portail captif mais jamais reellement applique avant
// le 2026-07-28). Utilise directement la facade trophy::ui_* (App/LVGL
// deja initialises par uiManager.begin() plus haut dans main()) : pas
// besoin d'AppController ici, seulement de faire avancer le temps via
// ui_tick() (lv_tick_inc + lv_timer_handler + App::tick(lv_tick_get()),
// seule maniere de garder lv_tick_get() synchronise avec le temps simule --
// ui_app_tick(now_ms) desynchroniserait le minuteur, remis a zero via
// lv_tick_get() dans App::show_page()).
bool runAutoRotationSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] rotation automatique : %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  trophy::ui_set_auto_rotation(true, 2);  // 2 secondes, pour un test rapide
  // Etat de depart neutre (hors rotation) : garantit une vraie transition
  // vers Dashboard juste apres, donc une remise a zero fiable du minuteur,
  // quel que soit l'etat laisse par un self-test precedent.
  trophy::ui_show_page_immediate(trophy::UiPage::Settings);
  trophy::ui_tick(10);
  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  trophy::ui_tick(10);
  check("etat initial : Dashboard", trophy::ui_get_page() == trophy::UiPage::Dashboard);

  trophy::ui_tick(1500);
  check("avant l'echeance (2 s) : toujours Dashboard", trophy::ui_get_page() == trophy::UiPage::Dashboard);

  // Sequence lineaire unifiee (voir App::tick()/rotation_slide_for()) :
  // Dashboard (1 diapositive) -> Trophees (4 sous-vues) -> Statistiques
  // (4 sous-vues) -> retour Dashboard, TOUTES affichees exactement le meme
  // intervalle -- avant cette correction, Dashboard restait affiche tout
  // l'intervalle configure pendant que chaque sous-vue de Trophees/
  // Statistiques ne durait que 3 s fixes, incoherent (signale par
  // l'utilisateur le 2026-07-28).
  trophy::ui_tick(600);  // ~2110 ms ecoulees depuis l'entree sur Dashboard : 1re echeance
  check("echeance 1 : Dashboard -> Trophees (sous-vue 1/4)", trophy::ui_get_page() == trophy::UiPage::Trophies);

  trophy::ui_tick(2100);
  check("echeance 2 : toujours Trophees (sous-vue 2/4)", trophy::ui_get_page() == trophy::UiPage::Trophies);
  trophy::ui_tick(2100);
  check("echeance 3 : toujours Trophees (sous-vue 3/4)", trophy::ui_get_page() == trophy::UiPage::Trophies);
  trophy::ui_tick(2100);
  check("echeance 4 : toujours Trophees (sous-vue 4/4)", trophy::ui_get_page() == trophy::UiPage::Trophies);

  trophy::ui_tick(2100);
  check("echeance 5 : Trophees -> Statistiques (sous-vue 1/4), meme duree que Trophees",
        trophy::ui_get_page() == trophy::UiPage::Statistics);
  trophy::ui_tick(2100);
  check("echeance 6 : toujours Statistiques (sous-vue 2/4)", trophy::ui_get_page() == trophy::UiPage::Statistics);
  trophy::ui_tick(2100);
  check("echeance 7 : toujours Statistiques (sous-vue 3/4)", trophy::ui_get_page() == trophy::UiPage::Statistics);
  trophy::ui_tick(2100);
  check("echeance 8 : toujours Statistiques (sous-vue 4/4)", trophy::ui_get_page() == trophy::UiPage::Statistics);

  trophy::ui_tick(2100);
  check("echeance 9 : boucle complete, retour a Dashboard, meme duree que les autres",
        trophy::ui_get_page() == trophy::UiPage::Dashboard);

  trophy::ui_set_auto_rotation(false, 2);
  trophy::ui_tick(10000);
  check("desactivee : reste sur Dashboard malgre un long delai", trophy::ui_get_page() == trophy::UiPage::Dashboard);

  trophy::ui_set_auto_rotation(true, 2);
  trophy::ui_show_page_immediate(trophy::UiPage::Settings);
  trophy::ui_tick(2100);
  check("jamais de rotation automatique depuis Reglages", trophy::ui_get_page() == trophy::UiPage::Settings);

  // Retour a un etat neutre pour ne pas influencer un self-test suivant.
  trophy::ui_set_auto_rotation(true, 10);
  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  trophy::ui_tick(10);

  return allPassed;
}

// Teste PocketPsnHtmlParser sur des fixtures reelles (voir
// test/fixtures/pocketpsn_profile_public.html, obtenu manuellement dans un
// navigateur par l'utilisateur puis nettoye de toute donnee de compte, et
// test/fixtures/pocketpsn_cloudflare_challenge.html, capture reelle du
// defi anti-robot via tools/pocketpsn_public_probe/) et sur des variantes
// derivees en memoire pour les cas d'erreur -- ne necessite ni UiManager
// ni AppController, logique de parsing pure.
bool runPocketPsnHtmlParserSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  auto readFile = [](const std::string& path) -> std::string {
    std::ifstream in(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  };

  std::string profilePath = findRepoFile("test/fixtures/pocketpsn_profile_public.html");
  std::string challengePath = findRepoFile("test/fixtures/pocketpsn_cloudflare_challenge.html");
  if (profilePath.empty() || challengePath.empty()) {
    check("test/fixtures/pocketpsn_profile_public.html et pocketpsn_cloudflare_challenge.html localises",
          false);
    return allPassed;
  }
  const std::string realProfileHtml = readFile(profilePath);
  const std::string cloudflareHtml = readFile(challengePath);

  // 1. Fixture reelle (gaz91610, nettoyee/anonymisee) : toutes les valeurs
  // doivent correspondre exactement a ce qui a ete observe dans le vrai
  // navigateur (voir le message utilisateur source de cette fixture).
  {
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(realProfileHtml, 200);
    check("fixture reelle : parse() reussit", r.ok());
    check("fixture reelle : username", r.profile.username == "TEST_PSN_USERNAME");
    check("fixture reelle : displayName", r.profile.displayName == "TestDisplayName");
    check("fixture reelle : level=314", r.profile.level == 314);
    check("fixture reelle : levelProgressPercent=48", r.profile.levelProgressPercent == 48);
    check("fixture reelle : totalTrophies=3381 (virgule de milliers geree)", r.stats.totalTrophies == 3381);
    check("fixture reelle : platinum=1 gold=185 silver=540 bronze=2655",
          r.stats.platinum == 1 && r.stats.gold == 185 && r.stats.silver == 540 && r.stats.bronze == 2655);
    check("fixture reelle : worldRank=282072 (virgule de milliers geree)", r.stats.worldRank == 282072);
    check("fixture reelle : countryRank=13195", r.stats.nationalRank == 13195);
    check("fixture reelle : pocketPoints=15024", r.stats.pocketPoints == 15024);
    check("fixture reelle : gamesCompleted=1", r.stats.gamesCompleted == 1);
    check("fixture reelle : completionRatePercent≈21.95",
          std::abs(r.stats.completionRatePercent - 21.95f) < 0.01f);
    check("fixture reelle : averageRarityPercent≈38.77",
          std::abs(r.stats.averageRarityPercent - 38.77f) < 0.01f);
    check("fixture reelle : unearnedTrophies=10782", r.stats.unearnedTrophies == 10782);
    check("fixture reelle : trophiesPerDay≈0.62", std::abs(r.stats.trophiesPerDay - 0.62f) < 0.01f);
    check("fixture reelle : playtimeHours≈2568.28", std::abs(r.stats.playtimeHours - 2568.28f) < 0.01f);
  }

  // 2. HTML tronque : coupe avant le libelle "World Rank" -> ne doit
  // jamais planter, doit renvoyer une erreur structuree (champs manquants).
  {
    size_t cutPoint = realProfileHtml.find("World Rank");
    check("HTML tronque : point de coupe trouve dans la fixture", cutPoint != std::string::npos);
    std::string truncated = realProfileHtml.substr(0, cutPoint);
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(truncated, 200);
    check("HTML tronque : parse() echoue proprement (pas d'exception, erreur structuree)", !r.ok());
    check("HTML tronque : erreur = champs manquants",
          r.error == PocketPsnHtmlParser::ParseError::kMissingRequiredFields);
  }

  // 3. Profil introuvable : page HTML valide (site reel) mais qui ne
  // represente pas une page de profil (ex: page d'accueil generique).
  {
    std::string notFoundHtml =
        "<!DOCTYPE html><html><head><title>PocketPSN.com</title></head>"
        "<body><div class=\"card\"><h1>Bienvenue sur PocketPSN</h1>"
        "<p>Recherchez un profil PSN ci-dessus.</p></div></body></html>";
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(notFoundHtml, 200);
    check("profil introuvable : parse() echoue proprement", !r.ok());
    check("profil introuvable : erreur = profil introuvable",
          r.error == PocketPsnHtmlParser::ParseError::kProfileNotFound);
  }

  // 4. Page Cloudflare : capture reelle (voir tools/pocketpsn_public_probe/),
  // detectee via le code HTTP 403 ET via le contenu seul (code inconnu).
  {
    PocketPsnHtmlParser::ParseResult withStatus = PocketPsnHtmlParser::parse(cloudflareHtml, 403);
    check("page Cloudflare : detectee via le code HTTP 403",
          withStatus.error == PocketPsnHtmlParser::ParseError::kCloudflareChallenge);
    PocketPsnHtmlParser::ParseResult bodyOnly = PocketPsnHtmlParser::parse(cloudflareHtml, 0);
    check("page Cloudflare : detectee via le contenu seul (code HTTP inconnu)",
          bodyOnly.error == PocketPsnHtmlParser::ParseError::kCloudflareChallenge);
  }

  // 5. Nombres avec separateur de milliers en espace ("2 655" au lieu de
  // "2,655") : doit rester interprete comme 2655, pas 2 ni 655.
  {
    std::string spaced = realProfileHtml;
    size_t pos = spaced.find("2,655");
    check("nombres avec espace : occurrence de '2,655' trouvee dans la fixture", pos != std::string::npos);
    spaced.replace(pos, std::string("2,655").size(), "2 655");
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(spaced, 200);
    check("nombres avec espace : parse() reussit toujours", r.ok());
    check("nombres avec espace : bronze=2655 (separateur espace correctement ignore)", r.stats.bronze == 2655);
  }

  // 6. Champ manquant : supprime le bloc "Average Rarity" -> erreur
  // explicite nommant le champ, pas un plantage ni une valeur par defaut
  // silencieuse.
  {
    std::string missingField = realProfileHtml;
    size_t labelPos = missingField.find("Average Rarity");
    check("champ manquant : libelle 'Average Rarity' trouve dans la fixture", labelPos != std::string::npos);
    size_t blockStart = missingField.rfind("<div class=\"col-sm-6 col-lg-2\"", labelPos);
    size_t blockEnd = missingField.find("<div class=\"col-sm-6 col-lg-2\"", blockStart + 1);
    check("champ manquant : bloc englobant delimite", blockStart != std::string::npos && blockEnd != std::string::npos);
    missingField.erase(blockStart, blockEnd - blockStart);
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(missingField, 200);
    check("champ manquant : parse() echoue proprement", !r.ok());
    check("champ manquant : erreur = champs manquants",
          r.error == PocketPsnHtmlParser::ParseError::kMissingRequiredFields);
    check("champ manquant : message nomme le champ concerne",
          r.errorMessage.find("Average Rarity") != std::string::npos);
  }

  // 7. Incoherence du total : modifie un compteur de trophee sans changer
  // le total affiche -> doit etre detecte, pas silencieusement accepte.
  {
    std::string mismatched = realProfileHtml;
    size_t pos = mismatched.find("style=\"color: #667FB2;\"></i><span class=\"fw-medium\">1</span>");
    check("incoherence du total : occurrence du compteur platine trouvee", pos != std::string::npos);
    mismatched.replace(pos, std::string("style=\"color: #667FB2;\"></i><span class=\"fw-medium\">1</span>").size(),
                        "style=\"color: #667FB2;\"></i><span class=\"fw-medium\">2</span>");
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(mismatched, 200);
    check("incoherence du total : parse() echoue proprement", !r.ok());
    check("incoherence du total : erreur = incoherence du total",
          r.error == PocketPsnHtmlParser::ParseError::kTotalMismatch);
  }

  // 8. Variante d'espaces/retours a la ligne : compresse tous les espaces
  // multiples en un seul -> l'extraction par libelle doit rester robuste
  // (elle ne doit pas dependre d'une mise en forme HTML exacte).
  {
    std::string reformatted = std::regex_replace(realProfileHtml, std::regex("[ \\t]+"), " ");
    PocketPsnHtmlParser::ParseResult r = PocketPsnHtmlParser::parse(reformatted, 200);
    check("variante d'espaces : parse() reussit toujours", r.ok());
    check("variante d'espaces : valeurs inchangees (totalTrophies=3381, worldRank=282072)",
          r.stats.totalTrophies == 3381 && r.stats.worldRank == 282072);
  }

  return allPassed;
}

// Teste PocketPsnProvider (portable, voir IPocketPsnHttpClient) et
// ProviderFactory/configPatchRequiresRestart via PocketPsnHttpClientStub --
// aucune vraie cle/reponse Pocket PSN n'est utilisee ici : chaque succes
// est explicitement une fixture synthetique construite a la main pour
// exercer le parsing/la machine a etats, jamais une preuve que le vrai
// schema Pocket PSN est confirme (voir docs/POCKETPSN_PROTOCOL.md et
// AUDIT.md section 0ter -- la validation reelle attend une vraie cle
// fournie localement par l'utilisateur, jamais dans ce depot).
bool runPocketPsnProviderSelfTest() {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  // Fixture JSON synthetique (jamais une vraie reponse Pocket PSN) au
  // format documente dans docs/POCKETPSN_PROTOCOL.md.
  const std::string kSyntheticSuccessJson = R"({
    "Username": "SelfTestUser", "Country": "FR",
    "PSN Level": 42, "PSN Level Progress": 50, "PSN Level Remaining": 100,
    "Trophies Plats": 2, "Trophies Gold": 10, "Trophies Silver": 20, "Trophies Bronze": 30, "Trophies Total": 62,
    "Trophy Points": 5000, "Pocket Points": 1200, "Total Games": 15,
    "World Rank": 12345, "Country Rank": 678,
    "Quick Stats": {"Games Completed": 5, "Completion Average": 33.3, "Average Rarity": 20.5,
                     "Unearned Trophies": 40, "Hours Played": 123.4}
  })";

  // 1. Succes (fixture synthetique) : parse complet + isVerified() devient true.
  {
    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 200;
    response.contentType = "application/json";
    response.body = kSyntheticSuccessJson;
    stub.queueResponse(response);

    PocketPsnProvider provider("SelfTestUser", "test-key-not-real", stub);
    check("PocketPsnProvider : isVerified() commence a false", !provider.isVerified());
    provider.requestRefresh();
    check("PocketPsnProvider : succes (fixture synthetique) -> kSuccess",
          provider.poll() == TrophyDataProvider::Status::kSuccess);
    check("PocketPsnProvider : profil extrait (fixture synthetique, pas une vraie reponse)",
          provider.profile().username == "SelfTestUser");
    check("PocketPsnProvider : isVerified() devient true apres un vrai succes de parsing",
          provider.isVerified());

    // 2. Reponse vide + HTTP 200 (comportement reel documente pour une cle
    // invalide) -> kErrorEmptyResponse, isVerified() ne redevient jamais false.
    IPocketPsnHttpClient::Response emptyResponse;
    emptyResponse.transportOk = true;
    emptyResponse.httpStatus = 200;
    emptyResponse.body = "";
    stub.queueResponse(emptyResponse);
    provider.requestRefresh();
    check("PocketPsnProvider : corps vide + HTTP 200 -> kErrorEmptyResponse",
          provider.poll() == TrophyDataProvider::Status::kError &&
              provider.lastErrorCode() == PocketPsnProvider::kErrorEmptyResponse);
    check("PocketPsnProvider : isVerified() reste true apres un echec ulterieur (semantique \"sticky\")",
          provider.isVerified());
  }

  // 3. Code HTTP non-200.
  {
    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 403;
    response.body = "peu importe";
    stub.queueResponse(response);
    PocketPsnProvider provider("u", "k", stub);
    provider.requestRefresh();
    check("PocketPsnProvider : HTTP 403 -> kError, lastErrorCode()=403",
          provider.poll() == TrophyDataProvider::Status::kError && provider.lastErrorCode() == 403);
    check("PocketPsnProvider : isVerified() reste false (jamais reussi)", !provider.isVerified());
  }

  // 4. Echec de transport (DNS/TLS/timeout).
  {
    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = false;
    stub.queueResponse(response);
    PocketPsnProvider provider("u", "k", stub);
    provider.requestRefresh();
    check("PocketPsnProvider : echec de transport -> kError",
          provider.poll() == TrophyDataProvider::Status::kError);
    check("PocketPsnProvider : isVerified() reste false (echec de transport)", !provider.isVerified());
  }

  // 5. JSON invalide.
  {
    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 200;
    response.contentType = "application/json";
    response.body = "ceci n'est pas du JSON";
    stub.queueResponse(response);
    PocketPsnProvider provider("u", "k", stub);
    provider.requestRefresh();
    check("PocketPsnProvider : JSON invalide -> kErrorInvalidJson",
          provider.poll() == TrophyDataProvider::Status::kError &&
              provider.lastErrorCode() == PocketPsnProvider::kErrorInvalidJson);
  }

  // 7. Construction de la requete : verifie l'URL et le corps exacts
  // (psn_name=...&key=...), sans dependre d'un vrai serveur.
  {
    PocketPsnHttpClientStub stub;
    IPocketPsnHttpClient::Response response;
    response.transportOk = true;
    response.httpStatus = 200;
    response.body = "";  // le contenu de la reponse n'importe pas ici
    stub.queueResponse(response);
    PocketPsnProvider provider("MonPseudo", "placeholder-key", stub);
    provider.requestRefresh();
    check("PocketPsnProvider : URL de requete correcte",
          stub.lastUrl() == "https://api.pocketpsn.com/PSTrophyDisplay/");
    check("PocketPsnProvider : corps de requete correct (psn_name=...&key=...)",
          stub.lastBody() == "psn_name=MonPseudo&key=placeholder-key");
  }

  // 8. ProviderFactory::shouldUsePocketPsn() / effectiveApiKey().
  //
  // Adaptatif au contenu local de include/secrets.h (voir AUDIT.md section
  // 0quater) : POCKETPSN_SHARED_API_KEY peut etre vide (aucune cle
  // compilee) ou renseigne (vraie cle de l'utilisateur) selon la machine
  // qui compile -- les deux sont des configurations valides, donc ce test
  // verifie la coherence entre effectiveApiKey() et shouldUsePocketPsn()
  // plutot que de figer un resultat suppose.
  {
    AppSettings emptySettings;
    const bool hasSharedKey = !ProviderFactory::effectiveApiKey(emptySettings).empty();
    check("ProviderFactory : les deux champs vides -> false", !ProviderFactory::shouldUsePocketPsn(emptySettings));
    check("effectiveApiKey : rien de rempli -> vide sauf si une cle partagee est compilee",
          ProviderFactory::effectiveApiKey(emptySettings).empty() == !hasSharedKey);

    AppSettings onlyUsername;
    onlyUsername.psnUsername = "MonPseudo";
    check("ProviderFactory : seul le pseudo rempli -> depend uniquement de la cle partagee compilee",
          ProviderFactory::shouldUsePocketPsn(onlyUsername) == hasSharedKey);

    AppSettings onlyKey;
    onlyKey.pocketPsnApiKey = "une-cle";
    check("ProviderFactory : seule la cle remplie -> false", !ProviderFactory::shouldUsePocketPsn(onlyKey));

    AppSettings both;
    both.psnUsername = "MonPseudo";
    both.pocketPsnApiKey = "une-cle";
    check("ProviderFactory : pseudo et cle remplis -> true", ProviderFactory::shouldUsePocketPsn(both));
    check("effectiveApiKey : la cle manuelle est prioritaire sur la cle partagee",
          ProviderFactory::effectiveApiKey(both) == "une-cle");
  }

  // 9. WebApiHandlers::configPatchRequiresRestart().
  {
    check("configPatchRequiresRestart : patch avec psnUsername -> true",
          WebApiHandlers::configPatchRequiresRestart(R"({"psnUsername":"MonPseudo"})"));
    check("configPatchRequiresRestart : patch avec pocketPsnApiKey -> true",
          WebApiHandlers::configPatchRequiresRestart(R"({"pocketPsnApiKey":"une-cle"})"));
    check("configPatchRequiresRestart : patch sans ces champs -> false",
          !WebApiHandlers::configPatchRequiresRestart(R"({"brightnessPercent":50})"));
  }

  return allPassed;
}

// --- Second affichage LVGL pour le panneau de debug (fenetre separee) ---
SDL_Renderer* debugRenderer_ = nullptr;
SDL_Texture* debugTexture_ = nullptr;
// Pixels RGB565 bruts, voir le commentaire equivalent dans DisplayDriverSdl.cpp
// (lv_color_t n'est plus un type pixel depuis LVGL 9).
std::vector<uint16_t> debugFramebuffer_;
std::vector<uint16_t> debugLvglBuf_;
int debugMouseX_ = 0, debugMouseY_ = 0;
bool debugMousePressed_ = false;

void debugFlushCb(lv_display_t* display, const lv_area_t* area, uint8_t* pxMap) {
  auto* colorP = reinterpret_cast<uint16_t*>(pxMap);
  for (int y = area->y1; y <= area->y2 && y < kDebugPanelHeight; ++y) {
    for (int x = area->x1; x <= area->x2 && x < kDebugPanelWidth; ++x) {
      debugFramebuffer_[static_cast<size_t>(y) * kDebugPanelWidth + x] = *colorP;
      ++colorP;
    }
  }
  lv_display_flush_ready(display);
}

void debugIndevReadCb(lv_indev_t*, lv_indev_data_t* data) {
  data->point.x = debugMouseX_;
  data->point.y = debugMouseY_;
  data->state = debugMousePressed_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void presentDebugWindow() {
  void* pixels = nullptr;
  int pitch = 0;
  SDL_LockTexture(debugTexture_, nullptr, &pixels, &pitch);
  for (int y = 0; y < kDebugPanelHeight; ++y) {
    auto* row = reinterpret_cast<uint16_t*>(static_cast<uint8_t*>(pixels) + y * pitch);
    for (int x = 0; x < kDebugPanelWidth; ++x) {
      row[x] = debugFramebuffer_[static_cast<size_t>(y) * kDebugPanelWidth + x];
    }
  }
  SDL_UnlockTexture(debugTexture_);
  SDL_RenderCopy(debugRenderer_, debugTexture_, nullptr, nullptr);
  SDL_RenderPresent(debugRenderer_);
}

Uint32 lastTickMs_ = 0;

// LVGL a besoin qu'on lui signale explicitement le temps ecoule
// (lv_tick_inc) pour que ses timers periodiques (rafraichissement d'ecran,
// animations...) se declenchent. Sans cet appel, l'affichage reste fige sur
// le tout premier rendu -- bug reel rencontre et corrige le 2026-07-14 (tous
// les changements d'ecran semblaient ignores lors de l'export des captures).
void advanceLvglTick() {
  Uint32 now = SDL_GetTicks();
  if (lastTickMs_ == 0) lastTickMs_ = now;
  lv_tick_inc(now - lastTickMs_);
  lastTickMs_ = now;
}

void pumpFrames(RoundUiBridge& ui, AppController& appController, SDL_Renderer* mainRenderer, int count) {
  for (int i = 0; i < count; ++i) {
    SDL_Delay(16);
    advanceLvglTick();
    appController.tick(SDL_GetTicks());
    // uiManager.tick() (voir main(), boucle interactive reelle) fait
    // avancer l'horloge interne de RoundUiBridge et son minuteur de retour
    // automatique au Dashboard apres un succes de synchro affiche sur Sync
    // (voir kSyncDwellMs) -- l'omettre ici (comme c'etait le cas avant)
    // bloquait indefiniment ce retour naturel des que exportScreenshots()/
    // verifyBootSequence() etc. dependaient d'une transition NON forcee par
    // ui_show_page_immediate() -- bug reel trouve le 2026-07-23 lors de la
    // verification du demarrage reel (Boot B1) : la progression atteignait
    // bien 100 % mais restait bloquee sur Offline/Sync, jamais sur
    // Dashboard, dans cette seule fonction de pompage de frames.
    ui.tick(SDL_GetTicks());
    lv_timer_handler();
    DisplayDriverSdl::present(mainRenderer);
    presentDebugWindow();
  }
}

// Verifie que l'ecran Boot progresse a partir des vraies etapes de
// demarrage (voir AppController::begin()/tick(), src/ui/BootStep.h), pas
// d'une simulation par ecoulement du temps (l'ancien App::boot_auto_advance(),
// desormais reserve au mode replay debug -- voir App::replay_boot()) :
// progression jamais decroissante, jamais bloquee meme si la connexion
// reseau prend un moment (voir WiFiManagerStub::kSimulatedConnectDelayMs),
// et transition propre vers le Dashboard une fois terminee. Remplace
// l'ancien warmup fixe de 60 ticks (voir historique de ce fichier) : sert le
// meme role (laisser la synchro initiale se stabiliser avant toute capture)
// tout en verifiant reellement le comportement demande plutot que de
// supposer un delai fixe suffisant.
bool verifyBootSequence(RoundUiBridge& ui, AppController& appController, SDL_Renderer* mainRenderer,
                         const char* screenshotDir) {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::fprintf(stderr, "[boot-check] %s: %s\n", label, condition ? "PASS" : "FAIL");
    std::fflush(stderr);
    if (!condition) allPassed = false;
  };

  // Capture le Boot B1 avant tout appController.tick() : AppController::
  // begin() a deja signale 0/15/30/45/60 % de maniere synchrone (voir
  // AppController.cpp) -- l'ecran affiche ici montre donc une vraie
  // progression. Pomper uniquement l'animation LVGL ici (sans tick()) laisse
  // les entrees en fondu du Boot B1 (fade_in/float_in, voir
  // build_boot_screen()) se terminer proprement avant la capture, sans
  // risquer de declencher RoundUiBridge::showSyncState() (appele
  // uniquement depuis AppController::tick(), jamais depuis
  // lv_timer_handler()) qui redirigerait sinon immediatement vers Hors-
  // ligne/Sync des que l'etat de sync change, independamment de la
  // progression Boot elle-meme.
  for (int i = 0; i < 20; ++i) {
    SDL_Delay(16);
    advanceLvglTick();
    lv_timer_handler();
    DisplayDriverSdl::present(mainRenderer);
    presentDebugWindow();
  }
  {
    std::error_code ec;
    std::filesystem::create_directories(screenshotDir, ec);
    char bootPath[512];
    std::snprintf(bootPath, sizeof(bootPath), "%s/boot.png", screenshotDir);
    DisplayDriverSdl::saveScreenshotPng(bootPath);
  }

  uint8_t lastProgress = trophy::ui_boot_progress();
  bool regressed = false;
  bool reachedDashboard = false;
  constexpr int kMaxIterations = 300;  // ~4.8 s a 16 ms/frame : marge tres large
                                        // au-dela du delai de connexion simule
                                        // (600 ms) + synchro demo.
  int iterationsUsed = 0;
  for (int i = 0; i < kMaxIterations; ++i) {
    SDL_Delay(16);
    advanceLvglTick();
    appController.tick(SDL_GetTicks());
    ui.tick(SDL_GetTicks());  // voir pumpFrames() : fait avancer le minuteur
                              // de retour automatique au Dashboard.
    lv_timer_handler();
    DisplayDriverSdl::present(mainRenderer);
    presentDebugWindow();

    uint8_t progress = trophy::ui_boot_progress();
    if (progress < lastProgress) regressed = true;
    lastProgress = progress;
    iterationsUsed = i + 1;
    if (trophy::ui_get_page() == trophy::UiPage::Dashboard) {
      reachedDashboard = true;
      break;
    }
  }

  check("progression jamais decroissante (0/15/30/45/60/75/90/100 %)", !regressed);
  check("demarrage termine sans blocage (transition atteinte avant la limite)", reachedDashboard);
  check("transition propre vers le Dashboard", trophy::ui_get_page() == trophy::UiPage::Dashboard);
  std::fprintf(stderr,
               "[boot-check] progression terminee en %d frames (~%d ms) ; page finale=%d progres=%d "
               "reseau=%d sync=%d\n",
               iterationsUsed, iterationsUsed * 16, static_cast<int>(trophy::ui_get_page()), lastProgress,
               static_cast<int>(appController.state().network.state),
               static_cast<int>(appController.state().sync.state));
  std::fflush(stderr);

  // Laisse quelques frames supplementaires pour que la premiere synchro
  // demo finisse de peupler l'AppState affiche sur le Dashboard.
  pumpFrames(ui, appController, mainRenderer, 10);
  return allPassed;
}

// Capture chaque ecran du design final avec les vraies donnees
// affichees a cet instant (voir RoundUiBridge) -- reecrit contre
// trophy::UiPage a l'etape d'integration des ecrans (l'ancien UiManager::
// ScreenId n'existe plus, voir HANDOFF_PROGRESS.md).
void exportScreenshots(RoundUiBridge& ui, AppController& appController, SDL_Renderer* mainRenderer, const char* dir) {
  struct Entry {
    trophy::UiPage page;
    const char* filename;
  };
  const Entry entries[] = {
      {trophy::UiPage::Welcome, "welcome.png"},     {trophy::UiPage::Dashboard, "dashboard.png"},
      {trophy::UiPage::Trophies, "trophies.png"},   {trophy::UiPage::Statistics, "statistics.png"},
      {trophy::UiPage::Sync, "sync.png"},           {trophy::UiPage::Offline, "offline.png"},
      {trophy::UiPage::Error, "error.png"},         {trophy::UiPage::Settings, "settings.png"},
      {trophy::UiPage::About, "about.png"},
  };
  // Le repertoire est relatif au repertoire courant au lancement, qui varie
  // selon comment l'executable est invoque (run.ps1, double-clic, autre
  // cwd...) -- on le cree systematiquement plutot que de supposer son
  // existence (bug reel rencontre et corrige le 2026-07-14 : la capture
  // echouait silencieusement quand "screenshots/" n'existait pas encore).
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
  if (ec) {
    std::fprintf(stderr, "Impossible de creer le dossier de captures '%s': %s\n", dir, ec.message().c_str());
  }

  for (const Entry& entry : entries) {
    trophy::ui_show_page_immediate(entry.page);
    pumpFrames(ui, appController, mainRenderer, 20);  // laisse les animations d'entree se stabiliser avant la capture
    char path[512];
    std::snprintf(path, sizeof(path), "%s/%s", dir, entry.filename);
    if (DisplayDriverSdl::saveScreenshotPng(path)) {
      std::printf("Capture enregistree : %s\n", path);
    } else {
      std::fprintf(stderr, "Echec de capture : %s\n", path);
    }
  }

  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  pumpFrames(ui, appController, mainRenderer, 10);
}

// Verifie le cycle de navigation par geste (swipe gauche/droite, voir
// GestureRecognizer + App::next_page()/previous_page()) sur le vrai chemin
// anime (animate=true, celui reellement utilise par un geste utilisateur --
// contrairement a ui_show_page_immediate(), reserve aux transitions
// automatiques). --selftest ne peut pas verifier ceci : il n'appelle jamais
// lv_timer_handler(), donc l'animation de fondu (220 ms) ne se termine
// jamais et l'etat de transition de LVGL reste incomplet (c'est exactement
// le bug reel trouve et corrige le 2026-07-22 sur l'ecran Sync -- voir
// HANDOFF_PROGRESS.md). Cette verification pompe donc de vraies frames
// entre chaque swipe, comme le ferait la boucle principale.
bool verifyScreenNavigation(RoundUiBridge& ui, AppController& appController, SDL_Renderer* mainRenderer) {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::fprintf(stderr, "[nav-check] %s: %s\n", label, condition ? "PASS" : "FAIL");
    std::fflush(stderr);
    if (!condition) allPassed = false;
  };

  // Sync n'est plus dans le carrousel manuel (voir PRODUCT_PAGE_COUNT et
  // App::next_page()/previous_page(), retire le 2026-07-27 : contenu
  // statique redondant avec le badge du Dashboard quand on y swipe
  // manuellement -- voir AUDIT.md). Reste affiche automatiquement pendant
  // une vraie synchronisation, verifie plus bas via un declenchement
  // direct plutot qu'un swipe.
  const trophy::UiPage forwardCycle[] = {
      trophy::UiPage::Trophies, trophy::UiPage::Statistics, trophy::UiPage::Settings,
      trophy::UiPage::About,    trophy::UiPage::Dashboard,
  };
  const trophy::UiPage backwardCycle[] = {
      trophy::UiPage::About,      trophy::UiPage::Settings, trophy::UiPage::Statistics,
      trophy::UiPage::Trophies,   trophy::UiPage::Dashboard,
  };

  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  pumpFrames(ui, appController, mainRenderer, 20);
  check("etat initial : Dashboard", trophy::ui_get_page() == trophy::UiPage::Dashboard);

  ui.swipeLeft();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe gauche -> Trophies", trophy::ui_get_page() == trophy::UiPage::Trophies);
  ui.swipeLeft();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe gauche -> Statistics", trophy::ui_get_page() == trophy::UiPage::Statistics);
  ui.swipeLeft();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe gauche -> Settings", trophy::ui_get_page() == trophy::UiPage::Settings);
  ui.swipeLeft();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe gauche -> About", trophy::ui_get_page() == trophy::UiPage::About);
  ui.swipeLeft();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe gauche -> retour Dashboard (boucle complete)", trophy::ui_get_page() == trophy::UiPage::Dashboard);

  ui.swipeRight();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe droite -> About", trophy::ui_get_page() == trophy::UiPage::About);
  ui.swipeRight();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe droite -> Settings", trophy::ui_get_page() == trophy::UiPage::Settings);
  ui.swipeRight();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("swipe droite -> Statistics", trophy::ui_get_page() == trophy::UiPage::Statistics);

  // Sync n'est plus atteignable par swipe -- verifie le declenchement
  // direct (celui qu'utilise une vraie synchronisation) et le retour
  // automatique au Dashboard apres kSyncDwellMs (voir RoundUiBridge.cpp).
  // Meme motif de test que precedemment (bug de test trouve le
  // 2026-07-23) : on isole ce minuteur dans sa propre verification,
  // plutot que de le laisser se declencher au milieu d'un arret sans
  // rapport.
  trophy::ui_show_page_immediate(trophy::UiPage::Sync);
  pumpFrames(ui, appController, mainRenderer, 20);
  check("declenchement direct -> Sync", trophy::ui_get_page() == trophy::UiPage::Sync);
  pumpFrames(ui, appController, mainRenderer, 110);  // >1.5 s : laisse le minuteur de retour se declencher ici
  check("retour automatique Dashboard apres Sync (kSyncDwellMs)",
        trophy::ui_get_page() == trophy::UiPage::Dashboard);

  // Verification memoire (voir mandat : la memoire ne doit pas diminuer en
  // continu apres plusieurs centaines de changements d'ecran) -- 30 cycles
  // complets aller-retour (360 changements d'ecran) via lv_mem_monitor(),
  // pool LVGL_STDLIB_BUILTIN (voir include/lv_conf.h). Simple balayage, sans
  // re-verifier chaque arret (deja fait ci-dessus) : le retour automatique
  // depuis Sync peut se declencher n'importe quand pendant ce balayage, ce
  // qui est sans consequence ici (on ne verifie que la memoire, jamais la
  // position exacte). Le premier cycle n'est pas compte dans la
  // comparaison : les toutes premieres constructions d'ecran peuvent
  // legitimement allouer des caches (styles, glyphes) jamais liberes mais
  // reutilises ensuite -- seule une baisse apres stabilisation indiquerait
  // une vraie fuite.
  constexpr int kMemCycles = 30;
  size_t freeAfterFirstCycle = 0;
  size_t freeAfterLastCycle = 0;
  for (int cycle = 0; cycle < kMemCycles; ++cycle) {
    for (trophy::UiPage expected : forwardCycle) {
      (void)expected;
      ui.swipeLeft();
      pumpFrames(ui, appController, mainRenderer, 20);  // laisse le fondu de 220 ms se terminer
    }
    for (trophy::UiPage expected : backwardCycle) {
      (void)expected;
      ui.swipeRight();
      pumpFrames(ui, appController, mainRenderer, 20);
    }

    lv_mem_monitor_t mon;
    lv_mem_monitor(&mon);
    std::fprintf(stderr, "[mem-check] cycle %d/%d : %zu octets libres (%zu utilises)\n", cycle + 1, kMemCycles,
                 mon.free_size, mon.total_size - mon.free_size);
    std::fflush(stderr);
    if (cycle == 0) freeAfterFirstCycle = mon.free_size;
    if (cycle == kMemCycles - 1) freeAfterLastCycle = mon.free_size;
  }
  // Tolerance de 4 Ko : marge pour la fragmentation normale du TLSF sans
  // masquer une vraie fuite (qui, sur 29 cycles supplementaires, se
  // compterait en dizaines/centaines de Ko, pas quelques octets).
  char memLabel[96];
  std::snprintf(memLabel, sizeof(memLabel), "memoire stable sur %d cycles complets (%d changements d'ecran)",
                kMemCycles, kMemCycles * 10);
  check(memLabel, freeAfterLastCycle + 4096 >= freeAfterFirstCycle);

  // Ne doit jamais crasher/bloquer. Sur Dashboard, un vrai tap declenche
  // appController.requestManualSync() (voir le dispatch de geste dans
  // main()/src/main.cpp firmware), jamais ui.activate() directement --
  // App::activate()'s propre gestion de Dashboard (simulate_sync(), un
  // mecanisme de demo) n'est jamais atteinte en usage reel. Naviguer
  // vers Sync est le comportement attendu, pas une regression.
  appController.requestManualSync();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("tap sur Dashboard (synchro manuelle) : pas de blocage",
        trophy::ui_get_page() == trophy::UiPage::Dashboard ||
            trophy::ui_get_page() == trophy::UiPage::Sync);
  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  pumpFrames(ui, appController, mainRenderer, 20);
  ui.longPress();
  pumpFrames(ui, appController, mainRenderer, 20);
  check("appui long : pas de blocage", true);

  trophy::ui_show_page_immediate(trophy::UiPage::Dashboard);
  pumpFrames(ui, appController, mainRenderer, 10);
  return allPassed;
}

// Verifie que les trois voies d'ecriture du panneau de debug
// (debugApplySettings/WiFiManagerStub/debugSetDisplayedData, voir
// simulator/src/DebugPanel.cpp) survivent a plusieurs tick() consecutifs --
// c'est exactement le bug corrige (avant : ecriture directe dans
// UiManager::state(), ecrasee par le tick suivant). Verifie egalement les
// scenarios Wi-Fi de base (connexion, perte reseau, bascule point d'acces,
// voir tache #26). Ne remplace pas un test d'interaction reelle (clics
// souris), mais verifie la garantie de fond sans dependre d'un
// environnement graphique interactif.
bool runDebugPanelSelfTest(RoundUiBridge& uiManager, AppController& appController, WiFiManagerStub& wifiStub) {
  bool allPassed = true;
  auto check = [&](const char* label, bool condition) {
    std::printf("[selftest] %s: %s\n", label, condition ? "PASS" : "FAIL");
    if (!condition) allPassed = false;
  };

  AppSettings settings = appController.state().settings;
  settings.brightnessPercent = 37;
  appController.debugApplySettings(settings);
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("reglages (luminosite=37) persistent apres 5 tick()", uiManager.state().settings.brightnessPercent == 37);

  wifiStub.simulateConnected("SelfTestSSID");
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("Wi-Fi connecte (SelfTestSSID) persiste apres 5 tick()",
        uiManager.state().network.state == WifiState::kConnected &&
            uiManager.state().network.ssid == "SelfTestSSID");

  wifiStub.simulateDisconnected();
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("Wi-Fi deconnecte persiste apres 5 tick()", uiManager.state().network.state == WifiState::kDisconnected);

  wifiStub.simulateAccessPoint();
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("bascule point d'acces persiste apres 5 tick()",
        uiManager.state().network.state == WifiState::kAccessPoint);

  // begin() avec un SSID : passe par kConnecting puis, apres le delai
  // simule, kConnected (contrairement aux simulateXxx() ci-dessus qui sont
  // instantanes) -- verifie la transition asynchrone non bloquante.
  wifiStub.begin("AsyncTestSSID", "pw");
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("Wi-Fi en cours de connexion (kConnecting) juste apres begin()",
        uiManager.state().network.state == WifiState::kConnecting);
  Uint32 asyncStart = SDL_GetTicks();
  while (SDL_GetTicks() - asyncStart < WiFiManagerStub::kSimulatedConnectDelayMs + 200) {
    appController.tick(SDL_GetTicks());
  }
  check("Wi-Fi connecte (AsyncTestSSID) apres le delai simule",
        uiManager.state().network.state == WifiState::kConnected &&
            uiManager.state().network.ssid == "AsyncTestSSID");

  ProfileData profile = appController.state().profile;
  TrophyStats stats = appController.state().stats;
  profile.username = "SelfTestUser";
  stats.platinum = 12345;
  appController.debugSetDisplayedData(profile, stats);
  for (int i = 0; i < 5; ++i) appController.tick(SDL_GetTicks());
  check("donnees de debug (SelfTestUser, 12345 platine) persistent apres 5 tick()",
        uiManager.state().profile.username == "SelfTestUser" && uiManager.state().stats.platinum == 12345);

  // --- WebApiHandlers (logique JSON portable du portail captif, voir
  // src/web/WebApiHandlers.h) : pas de vrai socket ici, mais la
  // construction/analyse JSON exacte utilisee par CaptivePortalServer
  // (firmware) est verifiee sans dependre de materiel.
  {
    std::string statusJson = WebApiHandlers::buildStatusJson(appController);
    JsonDocument statusDoc;
    bool statusParsed = deserializeJson(statusDoc, statusJson) == DeserializationError::Ok;
    check("GET /api/status produit un JSON valide avec les champs attendus (contrat web)",
          statusParsed && statusDoc["network"]["connected"].is<bool>() &&
              statusDoc["sync"]["state"].is<const char*>() && statusDoc["configured"].is<bool>());
  }

  {
    std::string configJson = WebApiHandlers::buildConfigJson(appController);
    JsonDocument configDoc;
    bool configParsed = deserializeJson(configDoc, configJson) == DeserializationError::Ok;
    check("GET /api/config produit un JSON valide avec les champs attendus (contrat web)",
          configParsed && configDoc["ssid"].is<const char*>() && configDoc["brightness"].is<int>() &&
              configDoc["syncInterval"].is<int>());

    std::string internalJson, warning, error;
    bool translated = WebApiHandlers::translateConfigPatch(
        R"({"ssid":"Foo","brightness":55,"sleepEnabled":true,"sleepDelay":3,"autoRotation":false,)"
        R"("rotationDelay":30,"animations":true,"language":"es","syncInterval":180})",
        internalJson, warning, error);
    JsonDocument internalDoc;
    deserializeJson(internalDoc, internalJson);
    check("POST /api/config : traduction des champs web vers le format interne",
          translated && std::string(internalDoc["wifiSsid"].as<const char*>()) == "Foo" &&
              internalDoc["brightnessPercent"] == 55 && internalDoc["sleepTimeoutSeconds"] == 180 &&
              internalDoc["rotationIntervalSeconds"] == 30 && internalDoc["syncIntervalMinutes"] == 180);
    check("POST /api/config : langue non supportee ('es') signalee sans faire echouer la requete",
          translated && !warning.empty() && !internalDoc["language"].is<const char*>());
  }

  {
    wifiStub.requestScan();
    std::string scanningJson = WebApiHandlers::buildWifiScanJson(wifiStub);
    JsonDocument scanningDoc;
    deserializeJson(scanningDoc, scanningJson);
    check("GET /api/wifi/scan (en cours) renvoie status=scanning",
          std::string(scanningDoc["status"].as<const char*>()) == "scanning");

    Uint32 scanStart = SDL_GetTicks();
    while (SDL_GetTicks() - scanStart < WiFiManagerStub::kSimulatedScanDelayMs + 200) {
      wifiStub.poll(SDL_GetTicks());
    }
    std::string doneJson = WebApiHandlers::buildWifiScanJson(wifiStub);
    JsonDocument doneDoc;
    deserializeJson(doneDoc, doneJson);
    check("GET /api/wifi/scan (termine) renvoie au moins un reseau",
          std::string(doneDoc["status"].as<const char*>()) == "done" && doneDoc["networks"].size() > 0);
  }

  {
    std::string ssid, password, error;
    bool okValid = WebApiHandlers::parseWifiConnectRequest(R"({"ssid":"MonReseau","password":"secret"})", ssid,
                                                            password, error);
    check("POST /api/wifi/connect : corps valide analyse correctement",
          okValid && ssid == "MonReseau" && password == "secret");

    std::string ssid2, password2, error2;
    bool okInvalid = WebApiHandlers::parseWifiConnectRequest(R"({"password":"secret"})", ssid2, password2, error2);
    check("POST /api/wifi/connect : ssid absent correctement rejete", !okInvalid && !error2.empty());
  }

  // --- POST /api/sync : decision pure, testee sans avoir besoin d'un vrai
  // sync en cours (voir WebApiHandlers::shouldAcceptSyncRequest()). ---
  {
    check("POST /api/sync : accepte quand aucune synchronisation en cours",
          WebApiHandlers::shouldAcceptSyncRequest(SyncState::kIdle));
    check("POST /api/sync : refuse quand une synchronisation est deja en cours",
          !WebApiHandlers::shouldAcceptSyncRequest(SyncState::kDownloading));
  }

  // --- POST /api/reboot : la minuterie portable PendingRestart (utilisee
  // par CaptivePortalServer) est testee directement, sans HTTP reel. ---
  {
    PendingRestart restart;
    check("reboot : rien de programme initialement", !restart.isPending() && !restart.consumeDue(0));
    Uint32 t0 = SDL_GetTicks();
    restart.schedule(t0, 500);
    check("reboot programme : pas encore du avant le delai", restart.isPending() && !restart.consumeDue(t0 + 100));
    check("reboot programme : declenche une fois le delai atteint",
          restart.consumeDue(t0 + 500) && !restart.isPending());
    check("reboot programme : ne se redeclenche pas une deuxieme fois", !restart.consumeDue(t0 + 600));
  }

  // --- POST /api/reset : refus sans confirmation, acceptation avec
  // confirmation, puis effet reel sur AppController (config + cache). ---
  {
    bool confirmed = true;
    std::string err;
    bool parsedMissing = WebApiHandlers::parseResetConfirmation("{}", confirmed, err);
    check("reset refuse quand \"confirm\" est absent", parsedMissing && !confirmed);

    confirmed = true;
    bool parsedFalse = WebApiHandlers::parseResetConfirmation(R"({"confirm":false})", confirmed, err);
    check("reset refuse quand \"confirm\":false", parsedFalse && !confirmed);

    confirmed = false;
    bool parsedTrue = WebApiHandlers::parseResetConfirmation(R"({"confirm":true})", confirmed, err);
    check("reset accepte quand \"confirm\":true", parsedTrue && confirmed);

    AppSettings before = appController.state().settings;
    before.wifiSsid = "ResetTestSSID";
    before.psnUsername = "ResetTestUser";
    appController.debugApplySettings(before);
    check("reset (preparation) : SSID de test bien enregistre avant reinitialisation",
          appController.state().settings.wifiSsid == "ResetTestSSID");

    appController.factoryReset();
    check("reset accepte : configuration revenue aux valeurs par defaut (SSID efface)",
          appController.state().settings.wifiSsid.empty() && appController.state().settings.psnUsername.empty());
    check("reset accepte : cache de trophees efface", !appController.hasCachedData());
  }

  // --- GET /api/diagnostics : jamais de mot de passe Wi-Fi, champs
  // materiel absents ("non mesurable dans le simulateur"). ---
  {
    AppSettings withPassword = appController.state().settings;
    withPassword.wifiSsid = "DiagTestSSID";
    withPassword.wifiPassword = "SuperSecretPassword123";
    appController.debugApplySettings(withPassword);
    // state().network.ssid ne se met a jour qu'une fois le WiFiManagerStub
    // reellement "connecte" (delai simule, voir kSimulatedConnectDelayMs) --
    // voir AppController::tick().
    Uint32 diagStart = SDL_GetTicks();
    while (SDL_GetTicks() - diagStart < WiFiManagerStub::kSimulatedConnectDelayMs + 200) {
      appController.tick(SDL_GetTicks());
    }

    DiagnosticsSnapshot snapshot = WebApiHandlers::buildDiagnosticsSnapshot(appController, appController.time().nowEpoch());
    std::string diagJson = WebApiHandlers::buildDiagnosticsJson(snapshot);
    check("GET /api/diagnostics : le mot de passe Wi-Fi n'apparait jamais dans la reponse",
          diagJson.find("SuperSecretPassword123") == std::string::npos &&
              diagJson.find("wifiPassword") == std::string::npos);
    check("GET /api/diagnostics : champs materiel absents dans le simulateur (non mesurables)",
          !snapshot.uptimeSeconds.has_value() && !snapshot.freeHeapBytes.has_value());
    check("GET /api/diagnostics : ssid present, champs reseau/sync coherents",
          snapshot.ssid == "DiagTestSSID" && !snapshot.lastSyncState.empty());

    // --- Test statique : tous les champs "diagnostics.xxx" reellement lus
    // par data/app.js (renderDiagnostics()) doivent exister dans la reponse
    // GET /api/diagnostics -- lit le vrai fichier, ne duplique pas de
    // liste a la main (voir docs/WEB_UI_GAP_ANALYSIS.md).
    std::string appJsPath = findRepoFile("data/app.js");
    if (appJsPath.empty()) {
      check("test statique : data/app.js localise (verification des champs diagnostics)", false);
    } else {
      std::ifstream in(appJsPath, std::ios::binary);
      std::string appJsContent((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
      std::set<std::string> fieldsReadByAppJs = extractDiagnosticsFieldsReadByAppJs(appJsContent);

      check("test statique : au moins un champ 'diagnostics.xxx' trouve dans data/app.js",
            !fieldsReadByAppJs.empty());

      JsonDocument diagDoc;
      deserializeJson(diagDoc, diagJson);

      // Cle presente (meme avec une valeur null, ex: champs materiel non
      // mesurables dans le simulateur) vs cle absente (typo) : on liste les
      // cles reellement presentes dans l'objet plutot que de tester
      // isNull() (indistinguable entre "absent" et "present mais null").
      std::set<std::string> presentKeys;
      for (JsonPairConst kv : diagDoc.as<JsonObjectConst>()) {
        presentKeys.insert(kv.key().c_str());
      }

      std::vector<std::string> missingFields;
      for (const std::string& field : fieldsReadByAppJs) {
        if (presentKeys.find(field) == presentKeys.end()) {
          missingFields.push_back(field);
        }
      }
      bool allFieldsPresent = missingFields.empty();
      if (!allFieldsPresent) {
        std::string joined;
        for (const std::string& f : missingFields) joined += f + " ";
        std::printf("[selftest]   champs manquants dans /api/diagnostics : %s\n", joined.c_str());
      }
      check("test statique : tous les champs lus par data/app.js existent dans /api/diagnostics",
            allFieldsPresent);
    }
  }

  return allPassed;
}

}  // namespace

int main(int argc, char** argv) {
  // Ligne par ligne plutot que pleinement bufferise : la narration console
  // du scenario showroom (voir ShowroomScenario.h) n'a d'interet que si elle
  // s'affiche en temps reel, y compris quand la sortie standard est
  // redirigee vers un fichier (ex: pour archiver une demonstration).
  std::setvbuf(stdout, nullptr, _IOLBF, 0);

  const char* screenshotDir = "screenshots";
  bool skipScreenshots = false;
  bool recordGif = false;
  bool selfTest = false;
  bool showroomMode = false;
  const char* gifFramesDir = "gif_frames";
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    std::string argLower = arg;
    for (char& c : argLower) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

    if (arg == "--no-screenshots") skipScreenshots = true;
    if (arg == "--screenshot-dir" && i + 1 < argc) screenshotDir = argv[++i];
    if (arg == "--record-gif") recordGif = true;
    if (arg == "--gif-dir" && i + 1 < argc) gifFramesDir = argv[++i];
    if (arg == "--selftest") selfTest = true;
    // Accepte -Showroom/--showroom/-showroom (insensible a la casse) :
    // simulator/run.ps1 -Showroom transmet l'argument tel quel (voir
    // simulator/README.md), sans convention imposee de tiret simple/double.
    if (argLower == "-showroom" || argLower == "--showroom") showroomMode = true;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* mainWindow =
      SDL_CreateWindow("PlayStation Trophy Display -- Simulateur", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                        CircleLayout::kScreenWidth, CircleLayout::kScreenHeight, SDL_WINDOW_SHOWN);
  SDL_Renderer* mainRenderer = SDL_CreateRenderer(mainWindow, -1, SDL_RENDERER_ACCELERATED);
  Uint32 mainWindowId = SDL_GetWindowID(mainWindow);

  SDL_Window* debugWindow =
      SDL_CreateWindow("Debug -- Trophy Display", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, kDebugPanelWidth,
                        kDebugPanelHeight, SDL_WINDOW_SHOWN);
  debugRenderer_ = SDL_CreateRenderer(debugWindow, -1, SDL_RENDERER_ACCELERATED);
  Uint32 debugWindowId = SDL_GetWindowID(debugWindow);

  lv_init();

  DisplayDriverSdl::init(mainRenderer, CircleLayout::kScreenWidth, CircleLayout::kScreenHeight);
  lv_display_t* mainDisp = lv_display_get_default();
  TouchDriverSdl::init();

  // --- Second affichage (panneau de debug), enregistre apres le principal ---
  debugFramebuffer_.assign(static_cast<size_t>(kDebugPanelWidth) * kDebugPanelHeight, 0);
  debugTexture_ = SDL_CreateTexture(debugRenderer_, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING,
                                    kDebugPanelWidth, kDebugPanelHeight);
  debugLvglBuf_.resize(static_cast<size_t>(kDebugPanelWidth) * 60);
  lv_display_t* debugDisp = lv_display_create(kDebugPanelWidth, kDebugPanelHeight);
  lv_display_set_flush_cb(debugDisp, debugFlushCb);
  lv_display_set_buffers(debugDisp, debugLvglBuf_.data(), nullptr,
                         static_cast<uint32_t>(debugLvglBuf_.size() * sizeof(uint16_t)),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t* debugIndev = lv_indev_create();
  lv_indev_set_type(debugIndev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(debugIndev, debugIndevReadCb);
  lv_indev_set_display(debugIndev, debugDisp);

  lv_disp_set_default(mainDisp);

  DemoDataProvider provider;
  NullBrightnessBackend brightnessBackend;
  WiFiManagerStub wifiStub;
  // Design final, branche sur les vraies donnees via RoundUiBridge
  // (voir src/ui/RoundUiBridge.h) -- remplace l'ecran minimal de validation
  // de l'etape socle (voir HANDOFF_PROGRESS.md).
  RoundUiBridge uiManager;
  GestureRecognizer gestureRecognizer;
  uiManager.begin();

  // Config et cache partagent le meme repertoire local (cles distinctes,
  // voir ConfigManager::kStoreKey / TrophyCache::kStoreKey) -- ignore par
  // .gitignore (voir simulator/.simulator_data/).
  FilePersistentStore persistentStore("simulator/.simulator_data");

  // Selection du provider actif au demarrage uniquement (voir
  // ProviderFactory.h et AUDIT.md section 0ter) : lecture anticipee de la
  // config, avant la construction d'AppController (qui rechargera la
  // config une seconde fois en interne via begin() -- leger cout accepte,
  // evite de restructurer la propriete de ConfigManager par AppController).
  ConfigManager earlyConfig(persistentStore);
  earlyConfig.load();
  PocketPsnHttpClientStub pocketPsnHttpClientStub;
  PocketPsnProvider pocketPsnProvider(earlyConfig.settings().psnUsername,
                                      ProviderFactory::effectiveApiKey(earlyConfig.settings()),
                                      pocketPsnHttpClientStub);
  TrophyDataProvider& activeProvider =
      ProviderFactory::shouldUsePocketPsn(earlyConfig.settings())
          ? static_cast<TrophyDataProvider&>(pocketPsnProvider)
          : static_cast<TrophyDataProvider&>(provider);

  AppController appController(persistentStore, persistentStore, activeProvider, brightnessBackend, wifiStub,
                               uiManager);
  appController.begin();
  // Config fraiche = aucun SSID enregistre -> begin() ci-dessus a bascule
  // le stub en point d'acces (comportement reel attendu, voir
  // WiFiManager::startAccessPoint()). Pour la commodite du mode demo, on
  // enregistre ici un reseau deja configure (comme si l'utilisateur avait
  // deja termine la configuration Wi-Fi) -- via debugApplySettings() (donc
  // via ConfigManager, jamais en pokant le stub directement) : une ecriture
  // directe dans le stub sans passer par la config serait desynchronisee
  // et ecrasee par le prochain appel a wifi_.begin() avec les reglages
  // (vides) de ConfigManager -- bug reel rencontre et corrige le
  // 2026-07-15 (voir HANDOFF_PROGRESS.md).
  AppSettings demoWifiSettings = appController.state().settings;
  demoWifiSettings.wifiSsid = "Simulateur";
  demoWifiSettings.wifiPassword = "demo";
  appController.debugApplySettings(demoWifiSettings);

  // Orchestrateur du scenario de demonstration ("showroom", voir
  // ShowroomScenario.h et simulator/README.md) : toujours construit en mode
  // interactif (pas seulement quand --showroom est passe), pour que les
  // etats individuels restent declenchables a tout moment en mode manuel
  // (raccourcis clavier ci-dessous et boutons DebugPanel) sans avoir besoin
  // de relancer l'executable.
  ShowroomScenario showroomScenario(appController, wifiStub, pocketPsnHttpClientStub);

  if (selfTest) {
    bool passed = runDebugPanelSelfTest(uiManager, appController, wifiStub);
    passed = runPocketPsnParserSelfTest() && passed;
    passed = runPocketPsnIntegrationSelfTest() && passed;
    passed = runAppControllerLongRunSelfTest() && passed;
    passed = runSyncServiceReconnectDebounceSelfTest() && passed;
    passed = runLongDurationStressSelfTest() && passed;
    passed = runPocketPsnHtmlParserSelfTest() && passed;
    passed = runPocketPsnProviderSelfTest() && passed;
    passed = runShowroomScenarioSelfTest() && passed;
    passed = runAutoRotationSelfTest() && passed;
    SDL_Quit();
    return passed ? 0 : 1;
  }

  lv_disp_set_default(debugDisp);
  lv_obj_t* debugScreen = DebugPanel::create(&appController, &provider, &wifiStub, &showroomScenario);
  lv_scr_load(debugScreen);
  lv_disp_set_default(mainDisp);

  // Laisse les transitions d'ecran initiales (meme a duree nulle) se
  // terminer proprement avant tout appel supplementaire a lv_scr_load(),
  // et verifie la vraie progression de l'ecran Boot (voir
  // verifyBootSequence() : remplace l'ancien warmup a duree fixe qui
  // supposait simplement que 60 ticks suffisaient, sans jamais verifier
  // que la connexion Wi-Fi simulee (600 ms, voir
  // WiFiManagerStub::kSimulatedConnectDelayMs) et la premiere synchronisation
  // demo avaient bien eu le temps de se terminer -- resultat reel observe le
  // 2026-07-22 : dashboard.png capturait en fait l'ecran Offline quand ce
  // delai n'etait pas respecte).
  bool bootPassed = verifyBootSequence(uiManager, appController, mainRenderer, screenshotDir);
  std::fprintf(stderr, "Verification du demarrage (Boot B1, etapes reelles) : %s\n",
               bootPassed ? "PASS" : "FAIL");
  std::fflush(stderr);

  if (!skipScreenshots) {
    exportScreenshots(uiManager, appController, mainRenderer, screenshotDir);
    bool navPassed = verifyScreenNavigation(uiManager, appController, mainRenderer);
    std::fprintf(stderr, "Verification de navigation (swipe complet, aller-retour) : %s\n",
                 navPassed ? "PASS" : "FAIL");
    std::fflush(stderr);
  }
  if (recordGif) {
    std::printf("Enregistrement GIF non reimplemente pour ce design (hors perimetre de cette etape).\n");
  }

  std::printf(
      "Raccourcis showroom (mode manuel) : 1-9,0,- = declencher un etat precis, Espace = lancer la sequence "
      "automatique complete. Voir simulator/README.md.\n");
  if (showroomMode) {
    std::printf("Mode --showroom actif : lancement automatique de la sequence de demonstration.\n");
    showroomScenario.start(SDL_GetTicks());
  }

  bool running = true;
  bool showFps = false;
  Uint32 lastFpsPrint = SDL_GetTicks();
  int frameCount = 0;

  // Position/etat du pointeur sur la fenetre principale, mis a jour par les
  // evenements SDL ci-dessous puis consommes une fois par frame par
  // gestureRecognizer (voir src/ui/GestureRecognizer.h) -- meme modele que
  // le firmware (tactile interroge une fois par tick, pas par evenement).
  int pointerX = 0, pointerY = 0;
  bool pointerPressed = false;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        running = false;
      } else if (event.type == SDL_KEYDOWN) {
        switch (event.key.keysym.sym) {
          case SDLK_LEFT:
            // swipe_right() = page precedente (voir App::swipe_right()),
            // convention clavier historique de ce simulateur (fleche gauche
            // = ecran precedent).
            uiManager.swipeRight();
            break;
          case SDLK_RIGHT:
            uiManager.swipeLeft();
            break;
          case SDLK_r:
            appController.requestManualSync();
            break;
          case SDLK_e:
            provider.simulateNextRefreshError(true);
            appController.requestManualSync();
            break;
          case SDLK_n:
            provider.simulateNewTrophy();
            break;
          case SDLK_d: {
            bool nowConnected = appController.state().network.state != WifiState::kConnected;
            if (nowConnected) {
              wifiStub.simulateConnected();
            } else {
              wifiStub.simulateDisconnected();
            }
            std::printf("Reseau simule : %s\n", nowConnected ? "connecte" : "deconnecte");
            break;
          }
          case SDLK_f:
            showFps = !showFps;
            std::printf("Debug FPS: %s\n", showFps ? "actif" : "inactif");
            break;
          // --- Equivalents clavier des gestes tactiles (voir aussi la
          // souris, gerre plus bas via gestureRecognizer) : pratiques pour
          // naviguer sans souris pendant la verification ecran par ecran.
          case SDLK_a:
            if (uiManager.onDashboard()) {
              appController.requestManualSync();
            } else {
              uiManager.activate();
            }
            break;
          case SDLK_l:
            uiManager.longPress();
            break;
          // --- Mode manuel showroom : declenche directement l'action reelle
          // d'un etat donne, independamment de la sequence automatique (voir
          // ShowroomScenario::triggerStep()) -- utile pour capturer chaque
          // etat individuellement.
          case SDLK_1:
            showroomScenario.triggerStep(ShowroomScenario::Step::kStartup, SDL_GetTicks());
            break;
          case SDLK_2:
            showroomScenario.triggerStep(ShowroomScenario::Step::kLoading, SDL_GetTicks());
            break;
          case SDLK_3:
            showroomScenario.triggerStep(ShowroomScenario::Step::kSyncing, SDL_GetTicks());
            break;
          case SDLK_4:
            showroomScenario.triggerStep(ShowroomScenario::Step::kProfileDisplay, SDL_GetTicks());
            break;
          case SDLK_5:
            showroomScenario.triggerStep(ShowroomScenario::Step::kUsingCache, SDL_GetTicks());
            break;
          case SDLK_6:
            showroomScenario.triggerStep(ShowroomScenario::Step::kNetworkLost, SDL_GetTicks());
            break;
          case SDLK_7:
            showroomScenario.triggerStep(ShowroomScenario::Step::kOffline, SDL_GetTicks());
            break;
          case SDLK_8:
            showroomScenario.triggerStep(ShowroomScenario::Step::kReconnecting, SDL_GetTicks());
            break;
          case SDLK_9:
            showroomScenario.triggerStep(ShowroomScenario::Step::kResyncing, SDL_GetTicks());
            break;
          case SDLK_0:
            showroomScenario.triggerStep(ShowroomScenario::Step::kApiError, SDL_GetTicks());
            break;
          case SDLK_MINUS:
            showroomScenario.triggerStep(ShowroomScenario::Step::kBackToNormal, SDL_GetTicks());
            break;
          case SDLK_SPACE:
            std::printf("Lancement de la sequence showroom automatique.\n");
            showroomScenario.start(SDL_GetTicks());
            break;
          default:
            break;
        }
      } else if (event.type == SDL_MOUSEBUTTONDOWN || event.type == SDL_MOUSEBUTTONUP ||
                 event.type == SDL_MOUSEMOTION) {
        int x, y;
        bool pressed;
        if (event.type == SDL_MOUSEMOTION) {
          x = event.motion.x;
          y = event.motion.y;
          pressed = (event.motion.state & SDL_BUTTON_LMASK) != 0;
        } else {
          x = event.button.x;
          y = event.button.y;
          pressed = event.type == SDL_MOUSEBUTTONDOWN;
        }
        Uint32 windowId = event.type == SDL_MOUSEMOTION ? event.motion.windowID : event.button.windowID;
        if (windowId == mainWindowId) {
          TouchDriverSdl::setPointerState(x, y, pressed);
          pointerX = x;
          pointerY = y;
          pointerPressed = pressed;
        } else if (windowId == debugWindowId) {
          debugMouseX_ = x;
          debugMouseY_ = y;
          debugMousePressed_ = pressed;
        }
      }
    }

    // Avant appController.tick() : toute action declenchee par le scenario
    // ce cycle (connexion/deconnexion simulee, reponse HTTP mise en file,
    // synchronisation demandee) doit etre traitee des l'appel a
    // AppController::tick() qui suit immediatement, dans la meme frame.
    showroomScenario.tick(SDL_GetTicks());
    advanceLvglTick();
    appController.tick(SDL_GetTicks());
    uiManager.tick(SDL_GetTicks());

    // Meme garde que le firmware reel (src/main.cpp) : un geste survenant
    // pendant que l'ecran est attenue/en veille ne doit jamais AUSSI
    // declencher une action (sync, navigation...), seulement reveiller
    // l'ecran -- sinon le tout premier tap apres une veille relance une
    // synchronisation que l'utilisateur ne voit meme pas venir (bug reel
    // signale le 2026-07-28). Capture AVANT notifyTouchActivity() (plus
    // bas), qui reveille l'ecran.
    const bool wasFullyAwake = appController.isDisplayAwake();

    GestureRecognizer::Gesture gesture = gestureRecognizer.update(pointerX, pointerY, pointerPressed, SDL_GetTicks());

    if (pointerPressed) {
      appController.notifyTouchActivity(SDL_GetTicks());
    }

    if (wasFullyAwake) {
      switch (gesture) {
        case GestureRecognizer::Gesture::kSwipeLeft:
          uiManager.swipeLeft();
          break;
        case GestureRecognizer::Gesture::kSwipeRight:
          uiManager.swipeRight();
          break;
        case GestureRecognizer::Gesture::kTap:
          if (uiManager.onDashboard()) {
            appController.requestManualSync();
          } else {
            uiManager.activate();
          }
          break;
        case GestureRecognizer::Gesture::kLongPress:
          uiManager.longPress();
          break;
        case GestureRecognizer::Gesture::kNone:
          break;
      }
    }

    lv_timer_handler();
    DisplayDriverSdl::present(mainRenderer);
    presentDebugWindow();

    ++frameCount;
    Uint32 now = SDL_GetTicks();
    if (showFps && now - lastFpsPrint >= 1000) {
      std::printf("FPS: %d\n", frameCount);
      frameCount = 0;
      lastFpsPrint = now;
    }

    SDL_Delay(8);
  }

  SDL_Quit();
  return 0;
}
