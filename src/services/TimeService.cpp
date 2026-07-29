#include "services/TimeService.h"

#include <cstdio>
#include <ctime>

#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace {
// Seuil arbitraire (~nov. 2023) permettant de distinguer une horloge ESP32
// reellement synchronisee (epoch NTP moderne) d'une horloge non
// synchronisee (epoch proche de 0 au demarrage, avant tout appel NTP
// reussi). Sans pertinence sur desktop (l'horloge systeme est toujours
// plausible).
constexpr uint32_t kMinPlausibleEpoch = 1700000000u;

// Table volontairement restreinte aux fuseaux proposes par le web UI (voir
// docs/CONFIGURATION.md) : "UTC0" en repli sur pour tout nom non reconnu
// plutot que de transmettre un nom IANA invalide a configTzTime().
std::string toPosixTz(const std::string& ianaName) {
  if (ianaName == "Europe/Paris" || ianaName == "Europe/Berlin" || ianaName == "Europe/Madrid" ||
      ianaName == "Europe/Rome" || ianaName == "Europe/Brussels" || ianaName == "Europe/Amsterdam") {
    return "CET-1CEST,M3.5.0,M10.5.0/3";
  }
  if (ianaName == "Europe/London" || ianaName == "Europe/Lisbon") return "GMT0BST,M3.5.0/1,M10.5.0";
  if (ianaName == "America/New_York") return "EST5EDT,M3.2.0,M11.1.0";
  if (ianaName == "America/Chicago") return "CST6CDT,M3.2.0,M11.1.0";
  if (ianaName == "America/Los_Angeles") return "PST8PDT,M3.2.0,M11.1.0";
  if (ianaName == "Asia/Tokyo") return "JST-9";
  return "UTC0";
}
}  // namespace

void TimeService::begin(const std::string& ianaTimezone) {
  timezone_ = ianaTimezone;
#ifdef ARDUINO
  configTzTime(toPosixTz(timezone_).c_str(), "pool.ntp.org", "time.nist.gov");
#endif
  ntpRequested_ = true;
}

void TimeService::poll() {
  // Rien a faire ici : configTzTime() synchronise en arriere-plan de facon
  // asynchrone cote firmware ; isSynced()/nowEpoch() detectent l'etat
  // courant a la demande. Reserve pour une future resynchronisation
  // periodique (voir docs/IMPLEMENTATION_PLAN.md).
}

void TimeService::requestSync() {
#ifdef ARDUINO
  configTzTime(toPosixTz(timezone_).c_str(), "pool.ntp.org", "time.nist.gov");
#endif
  ntpRequested_ = true;
}

bool TimeService::isSynced() const {
  if (forceOverride_) return forcedSynced_;
#ifdef ARDUINO
  return ntpRequested_ && static_cast<uint32_t>(time(nullptr)) > kMinPlausibleEpoch;
#else
  return true;
#endif
}

uint32_t TimeService::nowEpoch() const {
  if (!isSynced()) return 0;
  return static_cast<uint32_t>(time(nullptr));
}

std::string TimeService::formatRelative(uint32_t epoch, uint32_t nowEpochValue, bool frenchLocale) {
  if (epoch == 0) return frenchLocale ? "Jamais synchronise" : "Never synced";

  uint32_t diff = (nowEpochValue > epoch) ? (nowEpochValue - epoch) : 0;

  if (diff > 24u * 3600u) return frenchLocale ? "Donnees anciennes" : "Data outdated";
  if (diff < 60u) return frenchLocale ? "A l'instant" : "Just now";

  char buf[32];
  if (diff < 3600u) {
    unsigned minutes = static_cast<unsigned>(diff / 60u);
    snprintf(buf, sizeof(buf), frenchLocale ? "Il y a %u min" : "%u min ago", minutes);
  } else {
    unsigned hours = static_cast<unsigned>(diff / 3600u);
    snprintf(buf, sizeof(buf), frenchLocale ? "Il y a %u h" : "%u h ago", hours);
  }
  return buf;
}

std::string TimeService::formatClock(uint32_t epoch) {
  if (epoch == 0) return "--:--";

  time_t t = static_cast<time_t>(epoch);
  struct tm* tmPtr = localtime(&t);
  if (!tmPtr) return "--:--";
  struct tm tmVal = *tmPtr;  // copie immediate : localtime() n'est pas reentrant

  char buf[8];
  snprintf(buf, sizeof(buf), "%02d:%02d", tmVal.tm_hour, tmVal.tm_min);
  return buf;
}
