#include "TouchDriverSdl.h"

namespace TouchDriverSdl {

namespace {
int lastX_ = 0;
int lastY_ = 0;
bool pressed_ = false;

void readCb(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
  data->point.x = lastX_;
  data->point.y = lastY_;
  data->state = pressed_ ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}
}  // namespace

void init() {
  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, readCb);
}

void setPointerState(int x, int y, bool pressed) {
  lastX_ = x;
  lastY_ = y;
  pressed_ = pressed;
}

}  // namespace TouchDriverSdl
