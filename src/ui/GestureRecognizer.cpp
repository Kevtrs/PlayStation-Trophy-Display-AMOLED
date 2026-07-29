#include "ui/GestureRecognizer.h"

#include <cstdlib>

GestureRecognizer::Gesture GestureRecognizer::update(int x, int y, bool pressed, uint32_t nowMs) {
  Gesture result = Gesture::kNone;

  if (pressed && !wasPressed_) {
    downX_ = x;
    downY_ = y;
    downMs_ = nowMs;
    longPressFired_ = false;
  } else if (pressed && wasPressed_) {
    if (!longPressFired_ && nowMs - downMs_ > 760) {
      longPressFired_ = true;
      result = Gesture::kLongPress;
    }
  } else if (!pressed && wasPressed_) {
    int dx = x - downX_;
    int dy = y - downY_;
    uint32_t held = nowMs - downMs_;
    if (!longPressFired_ && std::abs(dx) > 54 && std::abs(dy) < 72) {
      result = dx < 0 ? Gesture::kSwipeLeft : Gesture::kSwipeRight;
    } else if (!longPressFired_ && held < 520 && std::abs(dx) < 18 && std::abs(dy) < 18) {
      result = Gesture::kTap;
    }
  }

  wasPressed_ = pressed;
  return result;
}
