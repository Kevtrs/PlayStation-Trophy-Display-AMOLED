#include "ui/ui_fixtures.hpp"

#include "ui/strings.hpp"

namespace trophy {
namespace {

constexpr TrophyEntry kNormalEntries[] = {
    {TrophyKind::Platinum, "Collectionneur ultime", "Astro Bot", "aujourd'hui", "0,8 %"},
    {TrophyKind::Gold, "Victoire parfaite", "Gran Turismo 7", "hier", "3,2 %"},
    {TrophyKind::Silver, "Mission accomplie", "Horizon Forbidden West", "18 juil.", "14 %"},
    {TrophyKind::Bronze, "Premier secret", "Ratchet & Clank", "16 juil.", "42 %"},
    {TrophyKind::Gold, "Sans faute", "Returnal", "12 juil.", "5,5 %"},
};

constexpr TrophyEntry kLongEntries[] = {
    {TrophyKind::Platinum, "The Ultimate PlayStation Collector", "The Legend of the Infinite Trophy Journey", "aujourd'hui", "0,1 %"},
    {TrophyKind::Gold, "Synchronisation héroïque terminée sans erreur", "Marvel's Spider-Man 2 Édition Complète", "hier", "2,4 %"},
    {TrophyKind::Silver, "Données enregistrées après reconnexion", "Horizon Forbidden West", "18 juillet", "12,8 %"},
    {TrophyKind::Bronze, "Célébration de découverte", "Astro's Playroom", "16 juillet", "64 %"},
};

constexpr TrophyEntry kHugeEntries[] = {
    {TrophyKind::Platinum, "Trophée platine numéro 1248", "Catalogue complet", "maintenant", "0,01 %"},
    {TrophyKind::Gold, "Défi mondial terminé", "Gran Turismo 7", "22 juil.", "1,1 %"},
    {TrophyKind::Silver, "Série de 24 510 trophées", "Bibliothèque PSN", "21 juil.", "8,7 %"},
    {TrophyKind::Bronze, "Bronze massif", "Archives PlayStation", "20 juil.", "71 %"},
};

} // namespace

ProfileData profile_for_fixture(UiFixture fixture, AppLanguage lang) {
    ProfileData p;
    p.username = "Kevin_Trophies";
    p.level = 327;
    p.progress = 72;
    p.total = 4286;
    p.platinum = 58;
    p.gold = 214;
    p.silver = 876;
    p.bronze = 3138;
    p.games_completed = 142;
    p.completion = 78;
    p.world_rank = "#12 483";
    p.play_time = "3 426 h";
    p.updated = tr(lang, Str::kFixtureNormalUpdated);
    p.offline_message = tr(lang, Str::kFixtureNormalOfflineMessage);
    p.error_title = tr(lang, Str::kFixtureNormalErrorTitle);
    p.error_message = tr(lang, Str::kFixtureNormalErrorMessage);
    p.brightness = 82;

    switch(fixture) {
        case UiFixture::Normal:
            break;
        case UiFixture::Empty:
            p.username = tr(lang, Str::kFixtureEmptyUsername);
            p.level = 0;
            p.progress = 0;
            p.total = 0;
            p.platinum = 0;
            p.gold = 0;
            p.silver = 0;
            p.bronze = 0;
            p.games_completed = 0;
            p.completion = 0;
            p.world_rank = "-";
            p.play_time = "0 h";
            p.updated = tr(lang, Str::kFixtureEmptyUpdated);
            p.offline_message = tr(lang, Str::kFixtureEmptyOfflineMessage);
            p.error_title = tr(lang, Str::kFixtureEmptyErrorTitle);
            p.error_message = tr(lang, Str::kFixtureEmptyErrorMessage);
            break;
        case UiFixture::LongText:
            p.username = "TheUltimatePlayStationCollector";
            p.updated = tr(lang, Str::kFixtureLongTextUpdated);
            p.world_rank = tr(lang, Str::kFixtureLongTextWorldRank);
            p.play_time = tr(lang, Str::kFixtureLongTextPlayTime);
            p.offline_message = tr(lang, Str::kFixtureLongTextOfflineMessage);
            p.error_title = tr(lang, Str::kFixtureLongTextErrorTitle);
            p.error_message = tr(lang, Str::kFixtureLongTextErrorMessage);
            break;
        case UiFixture::HugeValues:
            p.username = "Legend_999";
            p.level = 999;
            p.progress = 98;
            p.total = 137240;
            p.platinum = 1248;
            p.gold = 8642;
            p.silver = 24510;
            p.bronze = 102840;
            p.games_completed = 10743;
            p.completion = 100;
            p.world_rank = "#42";
            p.play_time = tr(lang, Str::kFixtureHugeValuesPlayTime);
            p.updated = tr(lang, Str::kFixtureHugeValuesUpdated);
            break;
        case UiFixture::OfflineCache:
            p.offline = true;
            p.sync = SyncState::Idle;
            p.updated = tr(lang, Str::kFixtureOfflineCacheUpdated);
            p.offline_message = tr(lang, Str::kFixtureOfflineCacheOfflineMessage);
            break;
        case UiFixture::OfflineNoCache:
            p.username = tr(lang, Str::kFixtureOfflineNoCacheUsername);
            p.level = 0;
            p.progress = 0;
            p.total = 0;
            p.platinum = 0;
            p.gold = 0;
            p.silver = 0;
            p.bronze = 0;
            p.games_completed = 0;
            p.completion = 0;
            p.world_rank = "-";
            p.play_time = "0 h";
            p.updated = tr(lang, Str::kFixtureOfflineNoCacheUpdated);
            p.offline = true;
            p.offline_message = tr(lang, Str::kFixtureOfflineNoCacheOfflineMessage);
            break;
        case UiFixture::ErrorState:
            p.sync = SyncState::Error;
            p.error_title = tr(lang, Str::kFixtureErrorStateTitle);
            p.error_message = tr(lang, Str::kFixtureErrorStateMessage);
            break;
    }
    return p;
}

const TrophyEntry *trophy_entries_for_fixture(UiFixture fixture, std::size_t *count) {
    if(count) *count = 0;
    switch(fixture) {
        case UiFixture::Empty:
        case UiFixture::OfflineNoCache:
            return nullptr;
        case UiFixture::LongText:
            if(count) *count = sizeof(kLongEntries) / sizeof(kLongEntries[0]);
            return kLongEntries;
        case UiFixture::HugeValues:
            if(count) *count = sizeof(kHugeEntries) / sizeof(kHugeEntries[0]);
            return kHugeEntries;
        default:
            if(count) *count = sizeof(kNormalEntries) / sizeof(kNormalEntries[0]);
            return kNormalEntries;
    }
}

const char *fixture_label(UiFixture fixture, AppLanguage lang) {
    switch(fixture) {
        case UiFixture::Normal: return tr(lang, Str::kFixtureNormal);
        case UiFixture::Empty: return tr(lang, Str::kFixtureEmpty);
        case UiFixture::LongText: return tr(lang, Str::kFixtureLongText);
        case UiFixture::HugeValues: return tr(lang, Str::kFixtureHugeValues);
        case UiFixture::OfflineCache: return tr(lang, Str::kFixtureOfflineCache);
        case UiFixture::OfflineNoCache: return tr(lang, Str::kFixtureOfflineNoCache);
        case UiFixture::ErrorState: return tr(lang, Str::kFixtureError);
    }
    return tr(lang, Str::kFixtureError);
}

UiFixture fixture_from_index(int index) {
    switch(index) {
        case 1: return UiFixture::Empty;
        case 2: return UiFixture::LongText;
        case 3: return UiFixture::HugeValues;
        case 4: return UiFixture::OfflineCache;
        case 5: return UiFixture::OfflineNoCache;
        case 6: return UiFixture::ErrorState;
        default: return UiFixture::Normal;
    }
}

int fixture_index(UiFixture fixture) {
    switch(fixture) {
        case UiFixture::Normal: return 0;
        case UiFixture::Empty: return 1;
        case UiFixture::LongText: return 2;
        case UiFixture::HugeValues: return 3;
        case UiFixture::OfflineCache: return 4;
        case UiFixture::OfflineNoCache: return 5;
        case UiFixture::ErrorState: return 6;
    }
    return 0;
}

} // namespace trophy
