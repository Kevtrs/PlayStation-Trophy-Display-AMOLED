#pragma once

#include <lvgl.h>

// Pilote tactile simulateur (equivalent du TouchDriver ESP32/CST9217) :
// utilise l'etat de la souris SDL comme point de contact unique.
namespace TouchDriverSdl {
void init();
void setPointerState(int x, int y, bool pressed);
}  // namespace TouchDriverSdl
