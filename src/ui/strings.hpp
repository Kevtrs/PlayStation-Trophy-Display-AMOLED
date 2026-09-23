#pragma once

#include "config/AppSettings.h"

// Table de traduction FR/EN pour tous les textes statiques/formates affiches
// a l'ecran (voir tache "Version anglaise : interface de l'ecran"). Ne
// couvre pas :
// - les marques/noms propres (Trophy Display, Pocket PSN, Kevin Torres,
//   pocketpsn.com) : identiques dans les deux langues ;
// - les logs Serial (Logger::info/warn) : jamais affiches a l'ecran ;
// - l'ecran IconGallery (Page::IconGallery) : reserve simulateur, jamais
//   utilise en produit (voir trophy_display_ui.cpp) ;
// - page_name() (src/ui/app.cpp) : etiquettes de debug interne uniquement.
namespace trophy {

enum class Str {
    // Communs / reutilises entre plusieurs ecrans.
    kOffline,
    kDataAvailable,
    kPlayStationDataProvidedBy,
    kTrophiesUnit,
    kReady,
    kConnectingLower,
    kProcessingLower,

    // ui_model.cpp : trophy_kind_label().
    kTrophyPlatinum,
    kTrophyGold,
    kTrophySilver,
    kTrophyBronze,
    kTrophyMultiple,
    kTrophyGeneric,

    // ui_model.cpp : sync_state_label().
    kSyncIdle,
    kSyncConnectingTitle,
    kSyncFetching,
    kSyncProcessingTitle,

    // Boot.
    kBootInitializing,
    kBootLoadingProfile,

    // Welcome.
    kWelcomeTitle,
    kWelcomeSubtitle,
    kExploreButton,

    // Dashboard.
    kLevelLabel,
    kProgressToNextFormat,

    // Trophies.
    kTrophiesTitle,

    // Statistics.
    kStatisticsTitle,
    kStatisticsSubtitle,
    kGamesCompleted,
    kCompletion,
    kWorldRank,
    kPlayTime,

    // Sync.
    kDataReadyBadge,
    kSyncingBadge,
    kSyncedTitle,
    kSyncingTitle,
    kDataUpToDate,
    kDataStaysReadable,

    // Celebration.
    kNewTrophyTitle,
    kNewTrophiesTitle, // pluriel : plusieurs trophees dans la meme synchro
    kMultipleTrophiesFormat,
    kTapToReturn,

    // Settings.
    kSettingsTitle,
    kSettingsSubtitle,
    kBrightnessLabel,
    kAnimationsReduced,
    kAnimationsActive,
    kProfileLabel,
    kRefreshLabel,
    kAutoRotateLabel,   // ecran Reglages 7" : etaient ecrits en dur en
    kAnimationsLabel,   // francais, donc intraduisibles depuis la bascule
    kLanguageLabel,     // de langue ajoutee sur ce meme ecran
    kLanguageFrench,
    kLanguageEnglish,
    kSimulateValue,
    kCelebrationLabel,
    kTestValue,
    kAboutLabel,
    kOpenValue,

    // About.
    kDesignAndDevelopment,
    kBackToSettings,

    // Offline.
    kBackToDashboard,

    // Error.
    kErrorBadge,
    kBackButton,

    // app.cpp : demo/showroom (simulateur de scenarios, Settings > Profil).
    kShowroomConnecting,
    kShowroomProcessing,
    kShowroomSyncedJustNow,
    kShowroomLocalDataKept,
    kShowroomReconnectedJustNow,
    kShowroomServiceUnavailable,
    kShowroomPocketPsnNotResponding,

    // RoundUiBridge.cpp : erreurs reelles traduites pour l'utilisateur.
    kErrAccountNotFoundTitle,
    kErrAccountNotFoundMessage,
    kErrInvalidResponseTitle,
    kErrInvalidResponseMessage,
    kErrConnectionFailedTitle,
    kErrConnectionFailedMessage,
    kErrInconsistentDataTitle,
    kErrInconsistentDataMessage,
    kErrServiceUnavailableTitle,
    kErrServiceUnavailableMessage,
    kErrSyncFailedTitle,
    kErrSyncFailedMessage,

    // RoundUiBridge.cpp : formatUpdated() (temps relatif).
    kJustNow,
    kMinuteAgoSingular,
    kMinutesAgoPlural,
    kHourAgoSingular,
    kHoursAgoPlural,
    kDayAgoSingular,
    kDaysAgoPlural,

    // RoundUiBridge.cpp : showBootProgress() (progression demarrage reel).
    kBootSystemStart,
    kBootConfigLoaded,
    kBootCacheLoaded,
    kBootNetworkInit,
    kBootDataReady,
    kBootUiReady,

    // RoundUiBridge.cpp : mapProfile() offline_message par defaut.
    kOfflineMessageDefault,

    // ui_fixtures.cpp : fixture_label() (Settings > Profil, cycle debug).
    kFixtureNormal,
    kFixtureEmpty,
    kFixtureLongText,
    kFixtureHugeValues,
    kFixtureOfflineCache,
    kFixtureOfflineNoCache,
    kFixtureError,

    // ui_fixtures.cpp : profile_for_fixture() -- valeurs par defaut (Normal).
    kFixtureNormalUpdated,
    kFixtureNormalOfflineMessage,
    kFixtureNormalErrorTitle,
    kFixtureNormalErrorMessage,

    // ui_fixtures.cpp : fixture Empty.
    kFixtureEmptyUsername,
    kFixtureEmptyUpdated,
    kFixtureEmptyOfflineMessage,
    kFixtureEmptyErrorTitle,
    kFixtureEmptyErrorMessage,

    // ui_fixtures.cpp : fixture LongText.
    kFixtureLongTextUpdated,
    kFixtureLongTextWorldRank,
    kFixtureLongTextPlayTime,
    kFixtureLongTextOfflineMessage,
    kFixtureLongTextErrorTitle,
    kFixtureLongTextErrorMessage,

    // ui_fixtures.cpp : fixture HugeValues.
    kFixtureHugeValuesPlayTime,
    kFixtureHugeValuesUpdated,

    // ui_fixtures.cpp : fixture OfflineCache.
    kFixtureOfflineCacheUpdated,
    kFixtureOfflineCacheOfflineMessage,

    // ui_fixtures.cpp : fixture OfflineNoCache.
    kFixtureOfflineNoCacheUsername,
    kFixtureOfflineNoCacheUpdated,
    kFixtureOfflineNoCacheOfflineMessage,

    // ui_fixtures.cpp : fixture ErrorState.
    kFixtureErrorStateTitle,
    kFixtureErrorStateMessage,

    // screens_wide.cpp : build_wifi_setup_screen_wide() (board 7").
    kWifiSetupTitle,
    kWifiSetupScan, // invite de scan du QR d'appairage Wi-Fi
    kWifiSetupInstruction,
    kWifiSetupFallbackFormat, // printf-style, %s = adresse IP (voir kProgressToNextFormat)

    // screens_wide.cpp : build_dashboard_screen_wide() (board 7", legende XP
    // -- plus de place que le kProgressToNextFormat de l'ecran rond, affiche
    // le numero du prochain niveau).
    kDashProgressToLevelFormat,

    // WideUiBridge.cpp : legende de l'ecran Statistiques "completion" (board
    // 7" uniquement -- pas kCompletion, "complétion" seul sonnait bizarre en
    // grand sous le pourcentage, retour utilisateur du 2026-07-29. Ecran
    // rond non touche, garde kCompletion.
    kStatCompletionCaption,

    // screens_wide.cpp : build_credits_screen_wide() (board 7" uniquement).
    // Chaine complete (domaine inclus, pas de concatenation a l'appel) --
    // "Donnees PlayStation fournies par pocketpsn.com" (kPlayStationData
    // ProvidedBy + domaine) depassait WIDE_HERO_CAPTION_W (700px) a
    // td_font_28 : LVGL casse les lignes sur "." par defaut
    // (LV_TXT_BREAK_CHARS), ".com" se retrouvait seul sur la ligne suivante
    // -- retour utilisateur du 2026-07-29. Raccourcie pour tenir sur une
    // ligne.
    kCreditsDataProvidedBy,

    kCount
};

const char *tr(AppLanguage lang, Str id);

} // namespace trophy
