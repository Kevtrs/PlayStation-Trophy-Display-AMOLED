#pragma once

#include <SDL.h>
#include <lvgl.h>

// Variante de DisplayDriverSdl.h SANS masque circulaire, pour le mini-
// simulateur du board Waveshare ESP32-S3-Touch-LCD-7 (800x480, rectangulaire
// -- voir simulator/src/main_wide.cpp). Copie deliberement isolee plutot que
// de modifier DisplayDriverSdl.h/.cpp (partage avec le simulateur principal,
// ecran rond en production) : les deux cibles restent independantes tant
// que le nouveau board n'a pas remplace l'ancien.
namespace DisplayDriverSdlWide {

void init(SDL_Renderer* renderer, int widthPx, int heightPx);
void present(SDL_Renderer* renderer);
bool saveScreenshotPng(const char* path);

}  // namespace DisplayDriverSdlWide
