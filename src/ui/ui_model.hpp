#pragma once

#include "config/AppSettings.h"

#include <cstdint>
#include <string>

namespace trophy {

enum class TrophyKind {
    Platinum,
    Gold,
    Silver,
    Bronze,
    Multiple
};

using TrophyType = TrophyKind;

enum class SyncState {
    Idle,
    Connecting,
    Fetching,
    Processing,
    Done,
    Error
};

enum class NetworkState {
    Online,
    Offline
};

enum class UiFixture {
    Normal,
    Empty,
    LongText,
    HugeValues,
    OfflineCache,
    OfflineNoCache,
    ErrorState
};

struct TrophyStats {
    int total = 0;
    int platinum = 0;
    int gold = 0;
    int silver = 0;
    int bronze = 0;
};

struct ProfileData {
    std::string username;
    int level = 0;
    int progress = 0;
    int total = 0;
    int platinum = 0;
    int gold = 0;
    int silver = 0;
    int bronze = 0;
    int games_completed = 0;
    int completion = 0;
    std::string world_rank;
    std::string play_time;
    std::string updated;
    std::string offline_message = "Les données locales restent disponibles.";
    std::string error_title = "Synchro impossible";
    std::string error_message = "Profil conservé. Aucun appel API.";
    bool offline = false;
    SyncState sync = SyncState::Idle;
    TrophyKind celebration = TrophyKind::Platinum;
    uint32_t celebration_count = 1;
    bool trophy_feed_available = true;
    bool reduced_motion = false;
    int brightness = 82;
};

struct TrophyEntry {
    TrophyKind kind = TrophyKind::Bronze;
    const char *title = "";
    const char *game = "";
    const char *date = "";
    const char *rarity = "";
};

struct UiState {
    ProfileData profile;
    NetworkState network = NetworkState::Online;
};

std::string format_number(int value);
const char *trophy_kind_label(TrophyKind kind, AppLanguage lang);
const char *sync_state_label(SyncState state, AppLanguage lang);

} // namespace trophy
