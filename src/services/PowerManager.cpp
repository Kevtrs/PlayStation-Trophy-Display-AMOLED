#include "services/PowerManager.h"

namespace {
int clampPercent(int v) {
  if (v < 0) return 0;
  if (v > 100) return 100;
  return v;
}
}  // namespace

PowerManager::PowerManager(IBrightnessBackend& backend) : backend_(backend) {}

void PowerManager::configure(int brightnessPercent, int sleepTimeoutSeconds) {
  configuredBrightnessPercent_ = clampPercent(brightnessPercent);
  sleepTimeoutSeconds_ = sleepTimeoutSeconds < 0 ? 0 : sleepTimeoutSeconds;
  if (state_ == PowerState::kAwake) {
    applyBrightness(configuredBrightnessPercent_);
  }
}

void PowerManager::notifyActivity(uint32_t nowMillis) {
  lastActivityMillis_ = nowMillis;
  if (state_ != PowerState::kAwake) {
    state_ = PowerState::kAwake;
    applyBrightness(configuredBrightnessPercent_);
  }
}

void PowerManager::poll(uint32_t nowMillis) {
  if (sleepTimeoutSeconds_ <= 0) {
    // Veille desactivee (mode toujours allume) : toujours reveille.
    if (state_ != PowerState::kAwake) {
      state_ = PowerState::kAwake;
      applyBrightness(configuredBrightnessPercent_);
    }
    return;
  }

  // Tolerant au retournement de millis() (~49 jours) : une soustraction
  // non signee produit toujours une petite valeur juste apres le
  // retournement, ce qui redemarre simplement le decompte d'inactivite.
  uint32_t elapsedSeconds = (nowMillis - lastActivityMillis_) / 1000u;
  uint32_t dimThreshold = static_cast<uint32_t>(sleepTimeoutSeconds_) / 2u;
  uint32_t sleepThreshold = static_cast<uint32_t>(sleepTimeoutSeconds_);

  if (elapsedSeconds >= sleepThreshold) {
    if (state_ != PowerState::kAsleep) {
      state_ = PowerState::kAsleep;
      applyBrightness(0);
    }
  } else if (elapsedSeconds >= dimThreshold) {
    if (state_ != PowerState::kDimmed) {
      state_ = PowerState::kDimmed;
      int dimmed = configuredBrightnessPercent_ / 4;
      applyBrightness(dimmed < 5 ? 5 : dimmed);
    }
  } else if (state_ != PowerState::kAwake) {
    state_ = PowerState::kAwake;
    applyBrightness(configuredBrightnessPercent_);
  }
}

void PowerManager::applyBrightness(int percent) {
  currentBrightnessPercent_ = clampPercent(percent);
  backend_.apply(currentBrightnessPercent_);
}
