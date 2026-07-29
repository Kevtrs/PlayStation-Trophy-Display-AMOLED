#include "ui/ui_model.hpp"

#include "ui/strings.hpp"

#include <algorithm>
#include <cstdlib>

namespace trophy {

std::string format_number(int value) {
    const bool negative = value < 0;
    std::string digits = std::to_string(std::abs(value));
    std::string out;
    int group = 0;
    for(auto it = digits.rbegin(); it != digits.rend(); ++it) {
        if(group == 3) {
            out.push_back(' ');
            group = 0;
        }
        out.push_back(*it);
        group++;
    }
    if(negative) out.push_back('-');
    std::reverse(out.begin(), out.end());
    return out;
}

const char *trophy_kind_label(TrophyKind kind, AppLanguage lang) {
    switch(kind) {
        case TrophyKind::Platinum: return tr(lang, Str::kTrophyPlatinum);
        case TrophyKind::Gold: return tr(lang, Str::kTrophyGold);
        case TrophyKind::Silver: return tr(lang, Str::kTrophySilver);
        case TrophyKind::Bronze: return tr(lang, Str::kTrophyBronze);
        case TrophyKind::Multiple: return tr(lang, Str::kTrophyMultiple);
    }
    return tr(lang, Str::kTrophyGeneric);
}

const char *sync_state_label(SyncState state, AppLanguage lang) {
    switch(state) {
        case SyncState::Idle: return tr(lang, Str::kSyncIdle);
        case SyncState::Connecting: return tr(lang, Str::kSyncConnectingTitle);
        case SyncState::Fetching: return tr(lang, Str::kSyncFetching);
        case SyncState::Processing: return tr(lang, Str::kSyncProcessingTitle);
        case SyncState::Done: return tr(lang, Str::kReady);
        case SyncState::Error: return tr(lang, Str::kErrorBadge);
    }
    return tr(lang, Str::kSyncIdle);
}

} // namespace trophy
