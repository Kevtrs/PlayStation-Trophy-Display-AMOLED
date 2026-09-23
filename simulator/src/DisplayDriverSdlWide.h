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

// Pointeur LVGL alimente par la souris SDL, pour que la couche tactile du
// board 7" (GT911 sur le vrai materiel, voir main_7inch.cpp) soit testable
// sans carte : la souris joue le doigt. Sans ca, aucun contributeur ne
// peut verifier la navigation tactile avant d'avoir le materiel en main.
void registerPointer();

// A appeler depuis la boucle d'evenements SDL. LVGL relit cet etat a son
// rythme (lv_timer_handler), il doit donc rester valable entre deux
// evenements, pas etre consomme une seule fois.
void setPointerState(int x, int y, bool pressed);

}  // namespace DisplayDriverSdlWide
