#include "DisplayDriverSdlWide.h"

#include <cstdint>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace DisplayDriverSdlWide {

namespace {
int width_ = 0;
int height_ = 0;

std::vector<uint16_t> framebuffer_;
SDL_Texture* texture_ = nullptr;

std::vector<uint16_t> lvglBuf1_;
std::vector<uint16_t> lvglBuf2_;

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
  r = static_cast<uint8_t>(((raw >> 11) & 0x1F) * 255 / 31);
  g = static_cast<uint8_t>(((raw >> 5) & 0x3F) * 255 / 63);
  b = static_cast<uint8_t>((raw & 0x1F) * 255 / 31);
}
}  // namespace

void init(SDL_Renderer* renderer, int widthPx, int heightPx) {
  width_ = widthPx;
  height_ = heightPx;

  framebuffer_.assign(static_cast<size_t>(width_) * height_, 0);
  texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB565, SDL_TEXTUREACCESS_STREAMING, width_, height_);

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
      row[x] = framebuffer_[static_cast<size_t>(y) * width_ + x];
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
      rgb565ToRgb888(framebuffer_[static_cast<size_t>(y) * width_ + x], rgb[idx], rgb[idx + 1], rgb[idx + 2]);
    }
  }
  return stbi_write_png(path, width_, height_, 3, rgb.data(), width_ * 3) != 0;
}

}  // namespace DisplayDriverSdlWide
