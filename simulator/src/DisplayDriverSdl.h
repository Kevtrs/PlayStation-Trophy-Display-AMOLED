#pragma once

#include <SDL.h>
#include <lvgl.h>

// Pilote d'affichage specifique au simulateur (equivalent du DisplayDriver
// ESP32 base sur Arduino_GFX/CO5300). Aucune logique d'ecran ne vit ici --
// uniquement le pont LVGL <-> SDL2, y compris le masque circulaire reel et
// l'export de captures PNG.
namespace DisplayDriverSdl {

// A appeler une fois : cree le framebuffer, la texture SDL et enregistre le
// pilote d'affichage LVGL (disp_drv.flush_cb etc.).
void init(SDL_Renderer* renderer, int widthPx, int heightPx);

// A appeler a chaque frame apres lv_timer_handler() : recopie le framebuffer
// LVGL vers la texture SDL, applique le masque circulaire, puis presente.
void present(SDL_Renderer* renderer);

// Exporte l'etat actuel du framebuffer (masque circulaire applique) en PNG.
bool saveScreenshotPng(const char* path);

}  // namespace DisplayDriverSdl
