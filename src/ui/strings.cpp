#include "ui/strings.hpp"

#include <cstddef>

namespace trophy {
namespace {

struct Entry {
    const char *fr;
    const char *en;
};

// L'ordre DOIT correspondre exactement a l'enum Str (voir static_assert plus
// bas) : chaque ligne est commentee avec le nom de la valeur qu'elle couvre.
constexpr Entry kEntries[] = {
    /* kOffline */                        {"Hors ligne", "Offline"},
    /* kDataAvailable */                  {"Données disponibles", "Data available"},
    /* kPlayStationDataProvidedBy */      {"Données PlayStation fournies par", "PlayStation data provided by"},
    /* kTrophiesUnit */                   {"trophées", "trophies"},
    /* kReady */                          {"Prêt", "Ready"},
    /* kConnectingLower */                {"connexion", "connecting"},
    /* kProcessingLower */                {"traitement", "processing"},

    /* kTrophyPlatinum */                 {"Platine", "Platinum"},
    /* kTrophyGold */                     {"Or", "Gold"},
    /* kTrophySilver */                   {"Argent", "Silver"},
    /* kTrophyBronze */                   {"Bronze", "Bronze"},
    /* kTrophyMultiple */                 {"Trophées", "Trophies"},
    /* kTrophyGeneric */                  {"Trophée", "Trophy"},

    /* kSyncIdle */                       {"Synchro", "Sync"},
    /* kSyncConnectingTitle */            {"Connexion", "Connecting"},
    /* kSyncFetching */                   {"Récupération", "Fetching"},
    /* kSyncProcessingTitle */            {"Traitement", "Processing"},

    /* kBootInitializing */               {"Initialisation", "Starting up"},
    /* kBootLoadingProfile */             {"Chargement du profil", "Loading profile"},

    /* kWelcomeTitle */                   {"Bienvenue", "Welcome"},
    /* kWelcomeSubtitle */                {"Mode démo prêt. Configurez le profil depuis les réglages.",
                                            "Demo mode ready. Set up your profile from settings."},
    /* kExploreButton */                  {"Explorer", "Explore"},

    /* kLevelLabel */                     {"NIVEAU", "LEVEL"},
    /* kProgressToNextFormat */           {"%d %% vers +1", "%d%% to next"},

    /* kTrophiesTitle */                  {"Trophées", "Trophies"},

    /* kStatisticsTitle */                {"Statistiques", "Statistics"},
    /* kStatisticsSubtitle */             {"synthèse du profil", "profile summary"},
    /* kGamesCompleted */                 {"jeux terminés", "games completed"},
    /* kCompletion */                     {"complétion", "completion"},
    /* kWorldRank */                      {"rang mondial", "world rank"},
    /* kPlayTime */                       {"temps de jeu", "play time"},

    /* kDataReadyBadge */                 {"Données prêtes", "Data ready"},
    /* kSyncingBadge */                   {"Synchronisation", "Syncing"},
    /* kSyncedTitle */                    {"Synchronisé", "Synced"},
    /* kSyncingTitle */                   {"Synchronisation", "Syncing"},
    /* kDataUpToDate */                   {"Les données sont à jour.", "Data is up to date."},
    /* kDataStaysReadable */              {"Les données restent lisibles pendant la mise à jour.",
                                            "Data stays readable during the update."},

    /* kNewTrophyTitle */                 {"Nouveau trophée", "New trophy"},
    /* kMultipleTrophiesFormat */         {"+%u trophées", "+%u trophies"},
    /* kTapToReturn */                    {"Touchez pour revenir", "Tap to go back"},

    /* kSettingsTitle */                  {"Réglages", "Settings"},
    /* kSettingsSubtitle */               {"simulation tactile", "touch simulation"},
    /* kBrightnessLabel */                {"Luminosité", "Brightness"},
    /* kAnimationsReduced */              {"réduites", "reduced"},
    /* kAnimationsActive */               {"actives", "on"},
    /* kProfileLabel */                   {"Profil", "Profile"},
    /* kRefreshLabel */                   {"Actualisation", "Refresh"},
    /* kSimulateValue */                  {"simuler", "simulate"},
    /* kCelebrationLabel */               {"Célébration", "Celebration"},
    /* kTestValue */                      {"tester", "test"},
    /* kAboutLabel */                     {"À propos", "About"},
    /* kOpenValue */                      {"ouvrir", "open"},

    /* kDesignAndDevelopment */           {"Conception et développement", "Design and development"},
    /* kBackToSettings */                 {"Retour réglages", "Back to settings"},

    /* kBackToDashboard */                {"Retour dashboard", "Back to dashboard"},

    /* kErrorBadge */                     {"Erreur", "Error"},
    /* kBackButton */                     {"Revenir", "Back"},

    /* kShowroomConnecting */             {"connexion", "connecting"},
    /* kShowroomProcessing */             {"traitement", "processing"},
    /* kShowroomSyncedJustNow */          {"synchronisé à l'instant", "synced just now"},
    /* kShowroomLocalDataKept */          {"Données locales conservées. Reconnexion dès que le réseau revient.",
                                            "Local data kept. Reconnecting as soon as the network is back."},
    /* kShowroomReconnectedJustNow */     {"reconnecté à l'instant", "reconnected just now"},
    /* kShowroomServiceUnavailable */     {"Service indisponible", "Service unavailable"},
    /* kShowroomPocketPsnNotResponding */ {"Pocket PSN ne répond pas. Dernières données affichées en cache.",
                                            "Pocket PSN isn't responding. Last cached data is shown."},

    /* kErrAccountNotFoundTitle */        {"Compte Pocket PSN introuvable", "Pocket PSN account not found"},
    /* kErrAccountNotFoundMessage */      {"Vérifiez le pseudo PSN dans les réglages.",
                                            "Check the PSN username in settings."},
    /* kErrInvalidResponseTitle */        {"Réponse Pocket PSN invalide", "Invalid Pocket PSN response"},
    /* kErrInvalidResponseMessage */      {"Le service a renvoyé des données inattendues. Nouvelle tentative automatique.",
                                            "The service returned unexpected data. Retrying automatically."},
    /* kErrConnectionFailedTitle */       {"Connexion impossible", "Connection failed"},
    /* kErrConnectionFailedMessage */     {"Impossible de contacter Pocket PSN. Vérifiez le réseau.",
                                            "Couldn't reach Pocket PSN. Check the network."},
    /* kErrInconsistentDataTitle */       {"Données incohérentes reçues", "Inconsistent data received"},
    /* kErrInconsistentDataMessage */     {"Le profil précédent reste affiché par sécurité.",
                                            "The previous profile stays shown for safety."},
    /* kErrServiceUnavailableTitle */     {"Service Pocket PSN indisponible", "Pocket PSN service unavailable"},
    /* kErrServiceUnavailableMessage */   {"Le service ne répond pas correctement. Nouvelle tentative automatique.",
                                            "The service isn't responding correctly. Retrying automatically."},
    /* kErrSyncFailedTitle */             {"Synchronisation impossible", "Sync failed"},
    /* kErrSyncFailedMessage */           {"Les données locales restent disponibles. Nouvelle tentative automatique.",
                                            "Local data remains available. Retrying automatically."},

    /* kJustNow */                        {"à l'instant", "just now"},
    /* kMinuteAgoSingular */              {"il y a %u minute", "%u minute ago"},
    /* kMinutesAgoPlural */               {"il y a %u minutes", "%u minutes ago"},
    /* kHourAgoSingular */                {"il y a %u heure", "%u hour ago"},
    /* kHoursAgoPlural */                 {"il y a %u heures", "%u hours ago"},
    /* kDayAgoSingular */                 {"il y a %u jour", "%u day ago"},
    /* kDaysAgoPlural */                  {"il y a %u jours", "%u days ago"},

    /* kBootSystemStart */                {"Démarrage système", "System startup"},
    /* kBootConfigLoaded */               {"Chargement de la configuration", "Loading configuration"},
    /* kBootCacheLoaded */                {"Chargement du cache", "Loading cache"},
    /* kBootNetworkInit */                {"Initialisation réseau", "Initializing network"},
    /* kBootDataReady */                  {"Récupération des données Pocket PSN", "Fetching Pocket PSN data"},
    /* kBootUiReady */                    {"Préparation de l'interface", "Preparing interface"},

    /* kOfflineMessageDefault */          {"Dernières données conservées.\nSynchronisation relancée dès que le réseau revient.",
                                            "Last data kept.\nSync resumes as soon as the network is back."},

    /* kFixtureNormal */                  {"normal", "normal"},
    /* kFixtureEmpty */                   {"vide", "empty"},
    /* kFixtureLongText */                {"textes longs", "long text"},
    /* kFixtureHugeValues */              {"grandes valeurs", "huge values"},
    /* kFixtureOfflineCache */            {"hors ligne cache", "offline cached"},
    /* kFixtureOfflineNoCache */          {"hors ligne vide", "offline empty"},
    /* kFixtureError */                   {"erreur", "error"},

    /* kFixtureNormalUpdated */           {"il y a 8 min", "8 min ago"},
    /* kFixtureNormalOfflineMessage */    {"Données enregistrées. Le profil reste visible pendant la reconnexion.",
                                            "Data saved. Profile stays visible while reconnecting."},
    /* kFixtureNormalErrorTitle */        {"Synchro impossible", "Sync failed"},
    /* kFixtureNormalErrorMessage */      {"Pocket PSN ne répond pas. Les dernières statistiques restent affichées.",
                                            "Pocket PSN isn't responding. The latest stats stay shown."},

    /* kFixtureEmptyUsername */          {"Profil non configuré", "Profile not configured"},
    /* kFixtureEmptyUpdated */            {"aucune donnée", "no data"},
    /* kFixtureEmptyOfflineMessage */     {"Aucune donnée locale n'est encore disponible.",
                                            "No local data is available yet."},
    /* kFixtureEmptyErrorTitle */         {"Configuration requise", "Setup required"},
    /* kFixtureEmptyErrorMessage */       {"Ajoutez un profil PSN depuis l'interface de configuration.",
                                            "Add a PSN profile from the configuration page."},

    /* kFixtureLongTextUpdated */         {"données synchronisées aujourd'hui à 23:58", "data synced today at 23:58"},
    /* kFixtureLongTextWorldRank */       {"#928 104 777", "#928,104,777"},
    /* kFixtureLongTextPlayTime */        {"128 742 h", "128,742 h"},
    /* kFixtureLongTextOfflineMessage */  {"Données enregistrées localement. Synchronisation automatique dès que le réseau revient.",
                                            "Data saved locally. Sync resumes automatically once the network is back."},
    /* kFixtureLongTextErrorTitle */      {"Échec de synchronisation", "Sync failed"},
    /* kFixtureLongTextErrorMessage */    {"Profil conservé en cache. Vérifiez la connexion, puis relancez la synchronisation.",
                                            "Profile kept in cache. Check the connection, then retry the sync."},

    /* kFixtureHugeValuesPlayTime */      {"18 742 h", "18,742 h"},
    /* kFixtureHugeValuesUpdated */       {"maintenant", "now"},

    /* kFixtureOfflineCacheUpdated */     {"cache : il y a 2 h", "cached: 2 h ago"},
    /* kFixtureOfflineCacheOfflineMessage */ {"Données enregistrées disponibles. Reconnexion dès que le réseau revient.",
                                               "Saved data available. Reconnecting once the network is back."},

    /* kFixtureOfflineNoCacheUsername */  {"Profil indisponible", "Profile unavailable"},
    /* kFixtureOfflineNoCacheUpdated */   {"hors ligne", "offline"},
    /* kFixtureOfflineNoCacheOfflineMessage */ {"Aucune donnée locale. Connectez l'appareil pour charger le profil.",
                                                 "No local data. Connect the device to load the profile."},

    /* kFixtureErrorStateTitle */         {"Profil introuvable", "Profile not found"},
    /* kFixtureErrorStateMessage */       {"Vérifiez le pseudo PSN, puis relancez le test depuis les réglages.",
                                            "Check the PSN username, then retry the test from settings."},

    /* kWifiSetupTitle */                 {"Configuration requise", "Setup required"},
    /* kWifiSetupInstruction */           {"Une page de configuration s'ouvrira automatiquement",
                                            "A setup page will open automatically"},
    /* kWifiSetupFallbackFormat */        {"Sinon, ouvrez %s dans votre navigateur",
                                            "Otherwise, open %s in your browser"},

    /* kDashProgressToLevelFormat */      {"%d %% vers le niveau %d", "%d%% to level %d"},

    /* kStatCompletionCaption */          {"progression moyenne", "average progress"},

    /* kCreditsDataProvidedBy */          {"Donnees fournies par pocketpsn.com", "Data provided by pocketpsn.com"},
};

static_assert(sizeof(kEntries) / sizeof(kEntries[0]) == static_cast<std::size_t>(Str::kCount),
              "kEntries doit avoir exactement une ligne par valeur de Str, dans le meme ordre");

} // namespace

const char *tr(AppLanguage lang, Str id) {
    const Entry &e = kEntries[static_cast<std::size_t>(id)];
    return lang == AppLanguage::kEnglish ? e.en : e.fr;
}

} // namespace trophy
