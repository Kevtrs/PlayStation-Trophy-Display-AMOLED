#include "DisplayDriverSdl.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace DisplayDriverSdl {

namespace {
int width_ = 0;
int height_ = 0;
int centerX_ = 0;
int centerY_ = 0;
int radius_ = 0;

// Pixels RGB565 bruts (2 octets/pixel) : depuis LVGL 9, lv_color_t est un
// type de couleur abstrait (3 octets RGB888, voir lv_color.h), independant
// du format reel des buffers d'affichage -- celui-ci reste RGB565 tel que
// configure par LV_COLOR_DEPTH (include/lv_conf.h), manipule ici directement
// en uint16_t plutot que via un type LVGL versionne.
std::vector<uint16_t> framebuffer_;
SDL_Texture* texture_ = nullptr;

std::vector<uint16_t> lvglBuf1_;
std::vector<uint16_t> lvglBuf2_;

bool isOutsideCircle(int x, int y) {
  int dx = x - centerX_;
  int dy = y - centerY_;
  return (dx * dx + dy * dy) > (radius_ * radius_);
}

void flushCb(lv_display_t* display, const lv_area_t* area, uint8_t* pxMap) {
  auto* colorP = reinterpret_cast<uint16_t*>(pxMap);
  for (int y = area->y1; y <= area->y2 && y < height_; ++y) {
    for (int x = area->x1; x <= area->x2 && x < width_; ++x) {
      framebuffer_[static_cast<size_t>(y) * width_ + x] = *colorP;
      ++colorP;
    }
  }
  lv_display_flush_ready(display);
}

void rgb565ToRgb888(uint16_t raw, uint8_t& r, uint8_t& g, uint8_t& b) {
  // LV_COLOR_16_SWAP=0 (voir include/lv_conf.h) -> ordre natif de la machine.
  r = static_cast<uint8_t>(((raw >> 11) & 0x1F) * 255 / 31);
  g = static_cast<uint8_t>(((raw >> 5) & 0x3F) * 255 / 63);
  b = static_cast<uint8_t>((raw & 0x1F) * 255 / 31);
}
}  // namespace

void init(SDL_Renderer* renderer, int widthPx, int heightPx) {
  width_ = widthPx;
  height_ = heightPx;
  centerX_ = widthPx / 2;
  centerY_ = heightPx / 2;
  radius_ = widthPx / 2;

  framebuffer_.assign(static_cast<size_t>(width_) * height_, 0);
  texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width_, height_);

  // Un buffer partiel de 40 lignes (l'ancienne valeur, encore utilisee cote
  // firmware ou la DMA impose des buffers modestes) exposait un vrai bug de
  // rendu LVGL 9.4 : sur un ecran a plusieurs groupes de widgets eloignes
  // verticalement (ex. ecran Statistiques, cartes a y=128 ET y=242), les
  // multiples appels flush_cb necessaires pour couvrir les 466 px de haut
  // finissaient par placer du contenu au mauvais y dans le framebuffer
  // -- constate le 2026-07-22 : positions LVGL internes verifiees correctes
  // (lv_obj_get_x/y apres lv_obj_update_layout), mais rendu final incorrect
  // (voir HANDOFF_PROGRESS.md). Un buffer couvrant la hauteur complete (un
  // seul flush par rafraichissement) elimine le probleme ; le simulateur a
  // largement assez de RAM desktop pour se le permettre.
  size_t bufLines = static_cast<size_t>(height_);
  lvglBuf1_.resize(static_cast<size_t>(width_) * bufLines);
  lvglBuf2_.resize(static_cast<size_t>(width_) * bufLines);

  lv_display_t* display = lv_display_create(width_, height_);
  lv_display_set_flush_cb(display, flushCb);
  lv_display_set_buffers(display, lvglBuf1_.data(), lvglBuf2_.data(),
                         static_cast<uint32_t>(lvglBuf1_.size() * sizeof(uint16_t)), LV_DISPLAY_RENDER_MODE_PARTIAL);
}

void present(SDL_Renderer* renderer) {
  void* pixels = nullptr;
  int pitch = 0;
  SDL_LockTexture(texture_, nullptr, &pixels, &pitch);
  auto* dst = static_cast<uint8_t*>(pixels);
  for (int y = 0; y < height_; ++y) {
    auto* row = reinterpret_cast<uint16_t*>(dst + y * pitch);
    for (int x = 0; x < width_; ++x) {
      row[x] = isOutsideCircle(x, y) ? 0x0000 : framebuffer_[static_cast<size_t>(y) * width_ + x];
    }
  }
  SDL_UnlockTexture(texture_);

  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  SDL_RenderCopy(renderer, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer);
}

bool saveScreenshotPng(const char* path) {
  std::vector<uint8_t> rgb(static_cast<size_t>(width_) * height_ * 3, 0);
  for (int y = 0; y < height_; ++y) {
    for (int x = 0; x < width_; ++x) {
      size_t idx = (static_cast<size_t>(y) * width_ + x) * 3;
      if (isOutsideCircle(x, y)) {
        rgb[idx] = rgb[idx + 1] = rgb[idx + 2] = 0;
        continue;
      }
      rgb565ToRgb888(framebuffer_[static_cast<size_t>(y) * width_ + x], rgb[idx], rgb[idx + 1], rgb[idx + 2]);
    }
  }
  return stbi_write_png(path, width_, height_, 3, rgb.data(), width_ * 3) != 0;
}

}  // namespace DisplayDriverSdl
