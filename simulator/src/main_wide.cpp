// Simulateur design 7 pouces pour le board Waveshare ESP32-S3-Touch-LCD-7.
// Cette entree est volontairement limitee a la preview UI 800x480 : aucune API,
// aucun reseau, aucun driver materiel reel.

#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <lvgl.h>

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>
#include <system_error>

#include "DisplayDriverSdlWide.h"
#include "assets/board7_icons.hpp"
#include "assets/fonts/td_fonts.hpp"
#include "assets/lucide_icons.hpp"
#include "assets/premium_trophies.hpp"
#include "assets/pocketpsn_logo.hpp"
#include "assets/profile_avatar.hpp"
#include "assets/profile_avatar_demo.hpp"
#include "assets/ps_plus_badge.hpp"
#include "screens/screens_wide.hpp"
#include "theme/theme.hpp"
#include "ui/layout_wide.hpp"
#include "ui/ui_model.hpp"
#include "widgets/widgets.hpp"

namespace {

constexpr Uint32 kSlideMs = 3500;
constexpr int kW = trophy::WIDE_WIDTH;
constexpr int kH = trophy::WIDE_HEIGHT;
constexpr int kCx = kW / 2;
constexpr int kCy = kH / 2;

constexpr lv_opa_t opa_pct(int pct) {
  return static_cast<lv_opa_t>((pct * 255) / 100);
}

struct DemoMetrics {
  int level_next = 315;
  int remaining_points = 555;
  const char* country = "FR";
  bool ps_plus = true;
  int games = 376;
  int completed = 1;
  int completion = 22;
  const char* rarity = "38.76 %";
  const char* play_time = "2 582 h";
  int unearned = 10743;
  const char* world_rank = "291 264";
  const char* national_rank = "13 717";
  const char* trophy_points = "72 885";
};

struct Rect {
  int x;
  int y;
  int w;
  int h;
};

enum class WideSlide {
  Dashboard,
  Trophies,
  TrophyPlatinum,
  TrophyGold,
  TrophySilver,
  TrophyBronze,
  Statistics,
  Sync,
  Offline,
  Error,
  Settings,
  About,
  IconGallery,
  Showroom,
  SyncSummary,
  Ambient,
  Timeline,
  NextGoal,
  TransitionLab,
  DebugOverlay,
  LayoutOverlay,
};

trophy::ProfileData demoProfile() {
  trophy::ProfileData p;
  p.username = "gaz91610";
  p.avatar_file_name = "A2117_l.png";
  p.level = 314;
  p.progress = 38;
  p.total = 3376;
  p.platinum = 1;
  p.gold = 185;
  p.silver = 539;
  p.bronze = 2651;
  p.games_completed = 1;
  p.completion = 22;
  p.world_rank = "291 264";
  p.play_time = "2 582 h";
  p.updated = "à l'instant";
  p.sync = trophy::SyncState::Done;
  return p;
}

lv_color_t color_hex(uint32_t hex) {
  return lv_color_hex(hex);
}

void clear_interaction(lv_obj_t* obj) {
  lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

lv_obj_t* root_screen() {
  lv_obj_t* root = lv_obj_create(nullptr);
  lv_obj_remove_style_all(root);
  lv_obj_set_size(root, kW, kH);
  lv_obj_set_style_bg_color(root, trophy::colors.background_primary, 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);
  clear_interaction(root);
  return root;
}

lv_obj_t* rect(lv_obj_t* parent, Rect r, lv_color_t fill, lv_opa_t fill_opa, lv_color_t border, lv_opa_t border_opa,
               int border_w = 1, int radius = 16) {
  lv_obj_t* obj = lv_obj_create(parent);
  clear_interaction(obj);
  lv_obj_set_size(obj, r.w, r.h);
  lv_obj_set_pos(obj, r.x, r.y);
  lv_obj_set_style_radius(obj, radius, 0);
  lv_obj_set_style_bg_color(obj, fill, 0);
  lv_obj_set_style_bg_opa(obj, fill_opa, 0);
  lv_obj_set_style_border_width(obj, border_w, 0);
  lv_obj_set_style_border_color(obj, border, 0);
  lv_obj_set_style_border_opa(obj, border_opa, 0);
  lv_obj_set_style_pad_all(obj, 0, 0);
  return obj;
}

lv_obj_t* label(lv_obj_t* parent, const char* text, const lv_font_t* font, lv_color_t color, Rect r,
                lv_text_align_t align = LV_TEXT_ALIGN_LEFT, lv_label_long_mode_t mode = LV_LABEL_LONG_DOT) {
  lv_obj_t* obj = lv_label_create(parent);
  lv_label_set_text(obj, text);
  lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, color, 0);
  lv_obj_set_style_text_align(obj, align, 0);
  lv_obj_set_style_text_letter_space(obj, 0, 0);
  lv_obj_set_size(obj, r.w, r.h);
  lv_obj_set_pos(obj, r.x, r.y);
  lv_label_set_long_mode(obj, mode);
  return obj;
}

void line(lv_obj_t* parent, Rect r, lv_color_t color, lv_opa_t opa = LV_OPA_COVER, int radius = 2) {
  rect(parent, r, color, opa, color, LV_OPA_TRANSP, 0, radius);
}

void glow_pill(lv_obj_t* parent, Rect r, lv_color_t color, int opacity) {
  rect(parent, r, color, opa_pct(opacity), color, LV_OPA_TRANSP, 0, r.h / 2);
}

void card_accent(lv_obj_t* parent, Rect r, lv_color_t accent, int top_inset = 22) {
  line(parent, {r.x + top_inset, r.y + 16, r.w - top_inset * 2, 2}, accent, opa_pct(82));
  line(parent, {r.x + top_inset, r.y + r.h - 13, r.w - top_inset * 2, 2}, accent, opa_pct(16));
}

void card_top_accent(lv_obj_t* parent, Rect r, lv_color_t accent, int top_inset = 22) {
  line(parent, {r.x + top_inset, r.y + 16, r.w - top_inset * 2, 2}, accent, opa_pct(82));
}

void tiny_dot(lv_obj_t* parent, int x, int y, lv_color_t color, int size = 5) {
  rect(parent, {x, y, size, size}, color, LV_OPA_COVER, color, LV_OPA_TRANSP, 0, LV_RADIUS_CIRCLE);
}

std::string number_with_spaces(int value) {
  std::string s = std::to_string(value);
  for (int i = static_cast<int>(s.size()) - 3; i > 0; i -= 3) s.insert(static_cast<size_t>(i), " ");
  return s;
}

const char* trophy_name(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return "Platine";
    case trophy::TrophyKind::Gold:
      return "Or";
    case trophy::TrophyKind::Silver:
      return "Argent";
    case trophy::TrophyKind::Bronze:
      return "Bronze";
    default:
      return "Trophées";
  }
}

lv_color_t trophy_color(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return trophy::colors.platinum;
    case trophy::TrophyKind::Gold:
      return trophy::colors.gold;
    case trophy::TrophyKind::Silver:
      return trophy::colors.silver;
    case trophy::TrophyKind::Bronze:
      return trophy::colors.bronze;
    default:
      return trophy::colors.accent_cyan;
  }
}

int trophy_value(const trophy::ProfileData& p, trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return p.platinum;
    case trophy::TrophyKind::Gold:
      return p.gold;
    case trophy::TrophyKind::Silver:
      return p.silver;
    case trophy::TrophyKind::Bronze:
      return p.bronze;
    default:
      return p.total;
  }
}

const char* trophy_focus_label(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return "Chef-d'oeuvre";
    case trophy::TrophyKind::Gold:
      return "Performance";
    case trophy::TrophyKind::Silver:
      return "Progression";
    case trophy::TrophyKind::Bronze:
      return "Collection";
    default:
      return "Trophee";
  }
}

const char* trophy_rarity_label(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return "Ultra rare";
    case trophy::TrophyKind::Gold:
      return "Rare";
    case trophy::TrophyKind::Silver:
      return "Avance";
    case trophy::TrophyKind::Bronze:
      return "Base";
    default:
      return "Standard";
  }
}

int trophy_share_percent(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return 1;
    case trophy::TrophyKind::Gold:
      return 5;
    case trophy::TrophyKind::Silver:
      return 16;
    case trophy::TrophyKind::Bronze:
      return 79;
    default:
      return 0;
  }
}

const char* trophy_share_text(trophy::TrophyKind kind) {
  switch (kind) {
    case trophy::TrophyKind::Platinum:
      return "0.0 %";
    case trophy::TrophyKind::Gold:
      return "5.5 %";
    case trophy::TrophyKind::Silver:
      return "16.0 %";
    case trophy::TrophyKind::Bronze:
      return "78.5 %";
    default:
      return "0 %";
  }
}

enum class BackgroundStyle {
  TrophyShowcase,
  CollectorStage,
  ConsoleDashboard,
  TrophyVault,
  LeaderboardWall,
  ButtonScatter,
};

BackgroundStyle g_background_style = BackgroundStyle::ButtonScatter;

const char* background_style_name(BackgroundStyle style) {
  switch (style) {
    case BackgroundStyle::TrophyShowcase:
      return "01-vitrine-trophees";
    case BackgroundStyle::CollectorStage:
      return "02-scene-collector";
    case BackgroundStyle::ConsoleDashboard:
      return "03-interface-console";
    case BackgroundStyle::TrophyVault:
      return "04-coffre-collection";
    case BackgroundStyle::LeaderboardWall:
      return "05-mur-classement";
    case BackgroundStyle::ButtonScatter:
      return "06-pele-mele-touches";
  }
  return "00-fond";
}

void background_arc(lv_obj_t* root, int size, int x, int y, lv_color_t color, lv_opa_t opa, int width, int start, int end,
                    bool rounded = false) {
  lv_obj_t* arc = lv_arc_create(root);
  lv_obj_remove_style_all(arc);
  clear_interaction(arc);
  lv_obj_set_size(arc, size, size);
  lv_obj_set_pos(arc, x, y);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, 100);
  lv_arc_set_bg_angles(arc, start, end);
  lv_obj_set_style_arc_color(arc, color, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(arc, opa, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, width, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc, rounded, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc, 0, LV_PART_INDICATOR);
  lv_obj_set_style_opa(arc, LV_OPA_COVER, 0);
}

void background_dot(lv_obj_t* root, int x, int y, int size, lv_color_t color, int opacity) {
  rect(root, {x, y, size, size}, color, opa_pct(opacity), color, LV_OPA_TRANSP, 0, LV_RADIUS_CIRCLE);
}

void background_square_mark(lv_obj_t* root, int x, int y, int size, lv_color_t color, int opacity) {
  rect(root, {x, y, size, size}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, color, opa_pct(opacity), 1, 3);
}

void background_cross_mark(lv_obj_t* root, int x, int y, int size, lv_color_t color, int opacity) {
  const int center = size / 2;
  line(root, {x, y + center, size, 1}, color, opa_pct(opacity), 1);
  line(root, {x + center, y, 1, size}, color, opa_pct(opacity), 1);
}

void background_polyline(lv_obj_t* root, const lv_point_precise_t* points, uint32_t point_count, lv_color_t color, int opacity,
                         int width = 2) {
  lv_point_precise_t* stored_points = new lv_point_precise_t[point_count];
  for (uint32_t i = 0; i < point_count; ++i) stored_points[i] = points[i];
  lv_obj_t* obj = lv_line_create(root);
  clear_interaction(obj);
  lv_line_set_points(obj, stored_points, point_count);
  lv_obj_set_style_line_color(obj, color, 0);
  lv_obj_set_style_line_opa(obj, opa_pct(opacity), 0);
  lv_obj_set_style_line_width(obj, width, 0);
  lv_obj_set_style_line_rounded(obj, true, 0);
  lv_obj_add_event_cb(
      obj,
      [](lv_event_t* e) {
        delete[] static_cast<lv_point_precise_t*>(lv_event_get_user_data(e));
      },
      LV_EVENT_DELETE, stored_points);
}

template <size_t N>
void background_polyline(lv_obj_t* root, const lv_point_precise_t (&points)[N], lv_color_t color, int opacity, int width = 2) {
  background_polyline(root, points, static_cast<uint32_t>(N), color, opacity, width);
}

void background_triangle_mark(lv_obj_t* root, int x, int y, int size, lv_color_t color, int opacity, int width = 2) {
  const int half = size / 2;
  const lv_point_precise_t points[] = {
      {static_cast<lv_value_precise_t>(x + half), static_cast<lv_value_precise_t>(y)},
      {static_cast<lv_value_precise_t>(x + size), static_cast<lv_value_precise_t>(y + size)},
      {static_cast<lv_value_precise_t>(x), static_cast<lv_value_precise_t>(y + size)},
      {static_cast<lv_value_precise_t>(x + half), static_cast<lv_value_precise_t>(y)},
  };
  background_polyline(root, points, color, opacity, width);
}

void background_button_mark(lv_obj_t* root, char kind, int x, int y, int size, lv_color_t color, int opacity, int width = 2) {
  switch (kind) {
    case 's':
      rect(root, {x, y, size, size}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, color, opa_pct(opacity), width,
           std::max(4, size / 10));
      break;
    case 'o':
      background_arc(root, size, x, y, color, opa_pct(opacity), width, 0, 360, true);
      break;
    case 'x': {
      const int inset = std::max(1, size / 8);
      const lv_point_precise_t slash_a[] = {
          {static_cast<lv_value_precise_t>(x + inset), static_cast<lv_value_precise_t>(y + inset)},
          {static_cast<lv_value_precise_t>(x + size - inset), static_cast<lv_value_precise_t>(y + size - inset)},
      };
      const lv_point_precise_t slash_b[] = {
          {static_cast<lv_value_precise_t>(x + size - inset), static_cast<lv_value_precise_t>(y + inset)},
          {static_cast<lv_value_precise_t>(x + inset), static_cast<lv_value_precise_t>(y + size - inset)},
      };
      background_polyline(root, slash_a, color, opacity, width);
      background_polyline(root, slash_b, color, opacity, width);
      break;
    }
    case 't':
      background_triangle_mark(root, x, y, size, color, opacity, width);
      break;
  }
}

void background_neon_button(lv_obj_t* root, char kind, int x, int y, int size, lv_color_t color, int opacity, int width = 2) {
  background_button_mark(root, kind, x - 4, y - 4, size + 8, color, std::max(3, opacity / 4), width + 4);
  background_button_mark(root, kind, x - 2, y - 2, size + 4, color, std::max(5, opacity / 2), width + 2);
  background_button_mark(root, kind, x, y, size, color, opacity, width);
}

void background_corner_bracket(lv_obj_t* root, Rect r, lv_color_t color, int opacity) {
  const int len = 28;
  line(root, {r.x, r.y, len, 1}, color, opa_pct(opacity), 1);
  line(root, {r.x, r.y, 1, len}, color, opa_pct(opacity), 1);
  line(root, {r.x + r.w - len, r.y, len, 1}, color, opa_pct(opacity), 1);
  line(root, {r.x + r.w - 1, r.y, 1, len}, color, opa_pct(opacity), 1);
  line(root, {r.x, r.y + r.h - 1, len, 1}, color, opa_pct(opacity), 1);
  line(root, {r.x, r.y + r.h - len, 1, len}, color, opa_pct(opacity), 1);
  line(root, {r.x + r.w - len, r.y + r.h - 1, len, 1}, color, opa_pct(opacity), 1);
  line(root, {r.x + r.w - 1, r.y + r.h - len, 1, len}, color, opa_pct(opacity), 1);
}

lv_obj_t* background_trophy(lv_obj_t* root, trophy::TrophyKind kind, int x, int y, int size, int opacity) {
  lv_obj_t* img = lv_image_create(root);
  lv_image_set_src(img, trophy::premium_trophy_descriptor(kind, size));
  lv_obj_set_size(img, size, size);
  lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_set_pos(img, x, y);
  lv_obj_set_style_opa(img, opa_pct(opacity), 0);
  clear_interaction(img);
  return img;
}

void background_ps_symbols(lv_obj_t* root, int x, int y, lv_color_t color, int opacity) {
  rect(root, {x, y, 34, 34}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, color, opa_pct(opacity), 2, 4);
  background_arc(root, 40, x + 58, y - 3, color, opa_pct(opacity), 2, 0, 360, true);
  background_cross_mark(root, x + 128, y + 4, 28, color, opacity);
  line(root, {x + 204, y + 30, 42, 2}, color, opa_pct(opacity), 1);
  line(root, {x + 225, y + 4, 2, 28}, color, opa_pct(opacity), 1);
}

void background_constellation(lv_obj_t* root, lv_color_t accent, lv_color_t secondary) {
  struct Node {
    int x;
    int y;
    int size;
    int opacity;
    lv_color_t color;
  };
  const Node nodes[] = {
      {76, 86, 5, 48, accent},   {138, 126, 3, 28, secondary}, {226, 62, 4, 35, accent},
      {318, 170, 3, 26, secondary}, {410, 92, 5, 42, accent},  {536, 154, 3, 30, secondary},
      {688, 74, 4, 36, accent},  {720, 210, 3, 28, secondary}, {612, 386, 4, 34, accent},
      {488, 424, 3, 24, secondary}, {334, 404, 5, 30, accent}, {178, 370, 3, 26, secondary},
      {70, 286, 4, 36, accent},
  };
  for (const Node& n : nodes) background_dot(root, n.x, n.y, n.size, n.color, n.opacity);
  line(root, {80, 88, 58, 1}, accent, opa_pct(16), 1);
  line(root, {140, 126, 84, 1}, secondary, opa_pct(12), 1);
  line(root, {412, 94, 122, 1}, accent, opa_pct(13), 1);
  line(root, {616, 388, 74, 1}, accent, opa_pct(14), 1);
  line(root, {180, 372, 150, 1}, secondary, opa_pct(10), 1);
}

void background_rings(lv_obj_t* root, lv_color_t accent = LV_COLOR_MAKE(0, 220, 240)) {
  struct Ring {
    int size;
    int x;
    int y;
    lv_color_t color;
    lv_opa_t opa;
    int width;
    int start;
    int end;
  };

  auto draw_rings = [&](const Ring* rings, size_t count) {
    for (size_t i = 0; i < count; ++i) {
      const Ring& ring = rings[i];
      background_arc(root, ring.size, ring.x, ring.y, ring.color, ring.opa, ring.width, ring.start, ring.end);
    }
  };

  switch (g_background_style) {
    case BackgroundStyle::TrophyShowcase: {
      const Ring rings[] = {
          {760, 20, -330, accent, opa_pct(20), 1, 114, 360},
          {520, 232, -126, trophy::colors.accent_cyan, opa_pct(18), 1, 120, 346},
          {420, -120, 90, trophy::colors.accent_blue, opa_pct(16), 1, 300, 68},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));
      background_corner_bracket(root, {52, 50, 696, 378}, trophy::colors.accent_cyan, 20);
      rect(root, {82, 84, 636, 322}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, trophy::colors.accent_blue, opa_pct(12), 1, 18);
      line(root, {106, 160, 588, 2}, trophy::colors.accent_cyan, opa_pct(25), 1);
      line(root, {106, 284, 588, 2}, trophy::colors.accent_cyan, opa_pct(20), 1);
      line(root, {106, 400, 588, 2}, trophy::colors.accent_cyan, opa_pct(28), 1);
      line(root, {148, 108, 1, 286}, trophy::colors.accent_blue, opa_pct(12), 1);
      line(root, {652, 108, 1, 286}, trophy::colors.accent_blue, opa_pct(12), 1);
      glow_pill(root, {326, 54, 148, 148}, trophy::colors.platinum, 7);
      glow_pill(root, {128, 188, 112, 112}, trophy::colors.gold, 5);
      glow_pill(root, {552, 188, 112, 112}, trophy::colors.silver, 5);
      background_trophy(root, trophy::TrophyKind::Platinum, 336, 72, 128, 22);
      background_trophy(root, trophy::TrophyKind::Gold, 150, 196, 96, 18);
      background_trophy(root, trophy::TrophyKind::Silver, 554, 196, 96, 18);
      background_trophy(root, trophy::TrophyKind::Bronze, 352, 302, 96, 16);
      for (int x = 160; x <= 640; x += 120) background_dot(root, x, 404, 4, accent, 35);
      break;
    }
    case BackgroundStyle::CollectorStage: {
      const Ring rings[] = {
          {660, 70, -230, accent, opa_pct(26), 1, 116, 358},
          {360, 220, 48, trophy::colors.accent_cyan, opa_pct(22), 1, 190, 350},
          {260, 270, 170, trophy::colors.accent_violet, opa_pct(20), 2, 198, 342},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));
      background_corner_bracket(root, {58, 44, 684, 392}, accent, 18);
      glow_pill(root, {270, 92, 260, 260}, trophy::colors.platinum, 6);
      glow_pill(root, {318, 176, 164, 164}, accent, 7);
      background_trophy(root, trophy::TrophyKind::Platinum, 336, 132, 128, 28);
      rect(root, {310, 306, 180, 26}, color_hex(0x041018), opa_pct(65), accent, opa_pct(25), 1, 10);
      rect(root, {340, 332, 120, 38}, color_hex(0x041018), opa_pct(72), accent, opa_pct(36), 1, 10);
      rect(root, {260, 370, 280, 30}, color_hex(0x041018), opa_pct(76), accent, opa_pct(44), 1, 12);
      line(root, {86, 402, 628, 2}, accent, opa_pct(44), 1);
      line(root, {116, 432, 568, 1}, trophy::colors.accent_blue, opa_pct(30), 1);
      for (int x = 132; x <= 668; x += 134) {
        rect(root, {x, 354, 70, 18}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, trophy::colors.accent_cyan, opa_pct(18), 1, 9);
      }
      background_ps_symbols(root, 276, 66, trophy::colors.accent_cyan, 24);
      break;
    }
    case BackgroundStyle::ConsoleDashboard: {
      const Ring rings[] = {
          {620, 370, -204, trophy::colors.accent_cyan, opa_pct(22), 1, 140, 318},
          {420, 492, 26, trophy::colors.accent_violet, opa_pct(20), 1, 166, 300},
          {360, -116, 132, trophy::colors.accent_blue, opa_pct(18), 1, 296, 58},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));
      rect(root, {54, 54, 156, 372}, color_hex(0x020B12), opa_pct(56), trophy::colors.accent_cyan, opa_pct(20), 1, 18);
      line(root, {92, 84, 80, 2}, accent, opa_pct(46), 1);
      for (int y = 116; y <= 356; y += 60) {
        rect(root, {82, y, 100, 38}, color_hex(0x04131D), opa_pct(62), trophy::colors.accent_blue, opa_pct(20), 1, 10);
        background_dot(root, 100, y + 17, 5, trophy::colors.accent_cyan, 44);
        line(root, {118, y + 18, 42, 2}, trophy::colors.accent_cyan, opa_pct(26), 1);
      }
      rect(root, {258, 72, 428, 80}, color_hex(0x020B12), opa_pct(42), trophy::colors.accent_violet, opa_pct(18), 1, 18);
      rect(root, {258, 192, 428, 80}, color_hex(0x020B12), opa_pct(42), trophy::colors.accent_cyan, opa_pct(18), 1, 18);
      rect(root, {258, 312, 428, 80}, color_hex(0x020B12), opa_pct(42), trophy::colors.gold, opa_pct(14), 1, 18);
      background_ps_symbols(root, 320, 92, trophy::colors.accent_cyan, 42);
      background_ps_symbols(root, 360, 212, trophy::colors.accent_violet, 34);
      background_ps_symbols(root, 300, 332, trophy::colors.gold, 30);
      line(root, {238, 424, 500, 2}, accent, opa_pct(30), 1);
      break;
    }
    case BackgroundStyle::TrophyVault: {
      const Ring rings[] = {
          {640, 80, -196, accent, opa_pct(22), 1, 116, 360},
          {420, 420, 8, trophy::colors.accent_cyan, opa_pct(18), 1, 160, 318},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));
      rect(root, {68, 54, 664, 372}, color_hex(0x020B12), opa_pct(42), trophy::colors.accent_blue, opa_pct(18), 1, 24);
      rect(root, {112, 92, 576, 296}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, accent, opa_pct(22), 1, 18);
      for (int x = 146; x <= 650; x += 84) {
        line(root, {x, 100, 1, 280}, trophy::colors.accent_cyan, opa_pct(11), 1);
      }
      for (int y = 130; y <= 350; y += 70) {
        line(root, {122, y, 556, 1}, trophy::colors.accent_blue, opa_pct(12), 1);
      }
      background_trophy(root, trophy::TrophyKind::Platinum, 336, 168, 128, 18);
      background_trophy(root, trophy::TrophyKind::Gold, 172, 132, 96, 14);
      background_trophy(root, trophy::TrophyKind::Silver, 532, 132, 96, 14);
      background_trophy(root, trophy::TrophyKind::Bronze, 172, 276, 96, 14);
      background_trophy(root, trophy::TrophyKind::Bronze, 532, 276, 96, 12);
      background_arc(root, 112, 344, 176, accent, opa_pct(34), 3, 0, 360, true);
      background_dot(root, 392, 224, 16, accent, 35);
      background_corner_bracket(root, {80, 66, 640, 348}, accent, 18);
      break;
    }
    case BackgroundStyle::LeaderboardWall: {
      const Ring rings[] = {
          {760, 52, -290, trophy::colors.accent_cyan, opa_pct(20), 1, 118, 352},
          {520, 484, -72, accent, opa_pct(18), 1, 150, 306},
          {360, 108, 118, trophy::colors.accent_violet, opa_pct(16), 1, 214, 42},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));
      rect(root, {68, 68, 664, 118}, color_hex(0x020B12), opa_pct(36), trophy::colors.accent_cyan, opa_pct(17), 1, 18);
      for (int i = 0; i < 5; ++i) {
        const int y = 92 + i * 18;
        background_dot(root, 98, y, 5, accent, 34);
        line(root, {120, y + 2, 242 + i * 46, 2}, trophy::colors.accent_cyan, opa_pct(18 + i * 3), 1);
        line(root, {542, y + 2, 104 - i * 12, 2}, trophy::colors.gold, opa_pct(16 + i * 2), 1);
      }
      rect(root, {266, 300, 76, 94}, color_hex(0x030E14), opa_pct(70), trophy::colors.silver, opa_pct(30), 1, 10);
      rect(root, {362, 250, 76, 144}, color_hex(0x030E14), opa_pct(78), trophy::colors.gold, opa_pct(42), 1, 10);
      rect(root, {458, 330, 76, 64}, color_hex(0x030E14), opa_pct(66), trophy::colors.bronze, opa_pct(34), 1, 10);
      line(root, {230, 396, 340, 2}, accent, opa_pct(34), 1);
      background_trophy(root, trophy::TrophyKind::Gold, 338, 144, 128, 20);
      background_dot(root, 400, 224, 84, trophy::colors.gold, 4);
      background_square_mark(root, 110, 302, 18, trophy::colors.accent_cyan, 30);
      background_cross_mark(root, 640, 316, 24, trophy::colors.accent_violet, 28);
      break;
    }
    case BackgroundStyle::ButtonScatter: {
      const lv_color_t ps_blue = color_hex(0x0A55C8);
      rect(root, {0, 0, kW, kH}, color_hex(0x000612), LV_OPA_COVER, color_hex(0x000612), LV_OPA_TRANSP, 0, 0);
      glow_pill(root, {-120, 22, 430, 430}, ps_blue, 6);
      glow_pill(root, {360, -136, 520, 520}, ps_blue, 5);
      glow_pill(root, {120, 230, 560, 320}, ps_blue, 4);

      const Ring rings[] = {
          {820, -72, -342, ps_blue, opa_pct(7), 1, 112, 352},
          {690, 242, -246, ps_blue, opa_pct(6), 1, 118, 334},
          {520, -156, 126, ps_blue, opa_pct(6), 1, 296, 78},
          {420, 554, 116, ps_blue, opa_pct(7), 1, 186, 330},
      };
      draw_rings(rings, sizeof(rings) / sizeof(rings[0]));

      const char kinds[] = {'s', 'o', 'x', 't'};

      for (int row = 0; row < 7; ++row) {
        for (int col = 0; col < 10; ++col) {
          const int x = -24 + col * 88 + (row % 2) * 42;
          const int y = 22 + row * 66;
          const int variant = (row * 3 + col * 5) % 4;
          const int size = 22 + ((row + col) % 3) * 8;
          const int opacity = 22 - ((row + col) % 3) * 4;
          background_neon_button(root, kinds[variant], x, y, size, ps_blue, opacity, 2);
        }
      }

      for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 9; ++col) {
          const int x = 26 + col * 92 + (row % 2) * 18;
          const int y = 54 + row * 74;
          const int variant = (row + col + 2) % 4;
          const int size = 16 + ((row * 2 + col) % 2) * 8;
          background_neon_button(root, kinds[variant], x, y, size, ps_blue, 16, 2);
        }
      }

      background_neon_button(root, 'o', -28, 288, 50, ps_blue, 24, 3);
      background_neon_button(root, 's', 742, 72, 42, ps_blue, 22, 3);
      background_neon_button(root, 't', 648, 396, 46, ps_blue, 22, 3);
      background_neon_button(root, 'x', 332, 418, 34, ps_blue, 20, 2);

      rect(root, {0, 0, kW, kH}, color_hex(0x000000), opa_pct(34), color_hex(0x000000), LV_OPA_TRANSP, 0, 0);
      break;
    }
  }
}

void title_block(lv_obj_t* root, const char* title, const char* subtitle) {
  label(root, title, &td_font_28, trophy::colors.text_primary, {48, 64, 260, 34});
  label(root, subtitle, &td_font_16, trophy::colors.text_secondary, {50, 100, 260, 22});
  line(root, {50, 125, 198, 2}, trophy::colors.accent_cyan, LV_OPA_COVER);
}

lv_obj_t* status_badge(lv_obj_t* root, const char* text, int x = 650, int y = 67, lv_color_t accent = LV_COLOR_MAKE(0, 255, 210),
                       const char* icon = "circle-dot") {
  lv_obj_t* badge = rect(root, {x, y, 104, 28}, color_hex(0x03120F), opa_pct(92), accent, LV_OPA_80, 1, 14);
  lv_obj_t* glyph = trophy::make_icon_box(badge, icon, 18, 10, accent);
  lv_obj_set_pos(glyph, 9, 5);
  label(badge, text, &td_font_12, trophy::colors.text_primary, {30, 5, 66, 16}, LV_TEXT_ALIGN_LEFT);
  return badge;
}

void country_chip(lv_obj_t* root, Rect r, const char* country) {
  lv_obj_t* chip = rect(root, r, color_hex(0x061A2F), opa_pct(90), trophy::colors.accent_blue, LV_OPA_90, 1, r.h / 2);
  const int flag_w = 18;
  const int flag_h = 12;
  const int label_w = 18;
  const int gap = 5;
  const int group_w = flag_w + gap + label_w;
  const int flag_x = (r.w - group_w) / 2;
  const int flag_y = (r.h - flag_h) / 2;
  rect(chip, {flag_x, flag_y, 18, 12}, trophy::colors.text_primary, LV_OPA_COVER, trophy::colors.text_primary, LV_OPA_TRANSP, 0,
       2);
  rect(chip, {flag_x, flag_y, 6, 12}, color_hex(0x2446A8), LV_OPA_COVER, color_hex(0x2446A8), LV_OPA_TRANSP, 0, 0);
  rect(chip, {flag_x + 12, flag_y, 6, 12}, color_hex(0xE63B45), LV_OPA_COVER, color_hex(0xE63B45), LV_OPA_TRANSP, 0, 0);
  label(chip, country, &td_font_12, trophy::colors.text_primary, {flag_x + flag_w + gap, 7, label_w, 14},
        LV_TEXT_ALIGN_CENTER);
}

void ps_plus_chip(lv_obj_t* root, int x, int y) {
  glow_pill(root, {x - 2, y - 2, 44, 44}, trophy::colors.gold, 16);
  lv_obj_t* img = lv_image_create(root);
  lv_image_set_src(img, assets::ps_plus_badge_descriptor());
  lv_obj_set_size(img, 40, 40);
  lv_obj_set_pos(img, x, y);
  clear_interaction(img);
}

lv_obj_t* icon_circle(lv_obj_t* parent, const char* icon, int cx, int cy, int box, int glyph, lv_color_t color) {
  lv_obj_t* holder = rect(parent, {cx - box / 2, cy - box / 2, box, box}, trophy::colors.surface_primary, opa_pct(55), color,
                         opa_pct(85), 2, LV_RADIUS_CIRCLE);
  if (const lv_image_dsc_t* dsc = trophy::board7_icon_descriptor(icon, glyph)) {
    const int asset_size = glyph <= 24 ? 24 : (glyph <= 32 ? 32 : 48);
    lv_obj_t* inner = lv_image_create(holder);
    lv_image_set_src(inner, dsc);
    lv_obj_set_size(inner, asset_size, asset_size);
    lv_image_set_inner_align(inner, LV_IMAGE_ALIGN_CONTAIN);
    lv_obj_set_style_image_recolor(inner, color, 0);
    lv_obj_set_style_image_recolor_opa(inner, LV_OPA_COVER, 0);
    lv_obj_center(inner);
  } else {
    lv_obj_t* inner = trophy::make_icon_box(holder, icon, glyph + 10, glyph, color);
    lv_obj_center(inner);
  }
  return holder;
}

void progress_bar(lv_obj_t* parent, Rect r, int percent, lv_color_t color) {
  rect(parent, r, color_hex(0x083746), LV_OPA_70, color, LV_OPA_TRANSP, 0, r.h / 2);
  const int fill = std::max(4, r.w * std::clamp(percent, 0, 100) / 100);
  line(parent, {r.x, r.y, fill, r.h}, color, LV_OPA_COVER, r.h / 2);
}

void trophy_img(lv_obj_t* parent, trophy::TrophyKind kind, int x, int y, int size) {
  lv_obj_t* img = lv_image_create(parent);
  lv_image_set_src(img, trophy::premium_trophy_descriptor(kind, size));
  lv_obj_set_size(img, size, size);
  lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_set_pos(img, x, y);
}

void avatar_img(lv_obj_t* parent, const trophy::ProfileData& p, Rect r) {
  lv_obj_t* holder = rect(parent, r, trophy::colors.surface_secondary, LV_OPA_90, trophy::colors.accent_cyan, LV_OPA_COVER, 2,
                         LV_RADIUS_CIRCLE);
  const lv_image_dsc_t* dsc = assets::profile_avatar_descriptor(p.avatar_file_name);
  if (dsc) {
    lv_obj_t* img = lv_image_create(holder);
    lv_image_set_src(img, dsc);
    lv_obj_set_size(img, r.w - 8, r.h - 8);
    lv_image_set_inner_align(img, LV_IMAGE_ALIGN_COVER);
    lv_obj_center(img);
  } else {
    icon_circle(holder, "user", r.w / 2, r.h / 2, r.w - 14, r.w - 36, trophy::colors.accent_cyan);
  }
}

void xp_ring(lv_obj_t* parent, int cx, int cy, int size, int percent) {
  lv_obj_t* bg = lv_arc_create(parent);
  lv_obj_remove_style_all(bg);
  clear_interaction(bg);
  lv_obj_set_size(bg, size, size);
  lv_obj_set_pos(bg, cx - size / 2, cy - size / 2);
  lv_arc_set_range(bg, 0, 100);
  lv_arc_set_value(bg, 100);
  lv_arc_set_bg_angles(bg, 130, 50);
  lv_obj_set_style_arc_color(bg, color_hex(0x113052), LV_PART_MAIN);
  lv_obj_set_style_arc_width(bg, 16, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(bg, false, LV_PART_MAIN);
  lv_obj_set_style_arc_width(bg, 0, LV_PART_INDICATOR);

  lv_obj_t* arc = lv_arc_create(parent);
  lv_obj_remove_style_all(arc);
  clear_interaction(arc);
  lv_obj_set_size(arc, size, size);
  lv_obj_set_pos(arc, cx - size / 2, cy - size / 2);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, percent);
  lv_arc_set_bg_angles(arc, 130, 50);
  lv_obj_set_style_arc_width(arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, trophy::colors.accent_cyan, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc, 16, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);

  lv_obj_t* tick = lv_arc_create(parent);
  lv_obj_remove_style_all(tick);
  clear_interaction(tick);
  lv_obj_set_size(tick, size, size);
  lv_obj_set_pos(tick, cx - size / 2, cy - size / 2);
  lv_arc_set_range(tick, 0, 100);
  lv_arc_set_value(tick, 100);
  lv_arc_set_bg_angles(tick, 137, 142);
  lv_obj_set_style_arc_width(tick, 8, LV_PART_MAIN);
  lv_obj_set_style_arc_color(tick, trophy::colors.text_primary, LV_PART_MAIN);
  lv_obj_set_style_arc_width(tick, 0, LV_PART_INDICATOR);
}

void add_analysis_overlay(lv_obj_t* root, const std::array<Rect, 12>& boxes, int count, const char* label_text) {
  line(root, {0, kCy, kW, 1}, trophy::colors.accent_cyan, LV_OPA_50);
  line(root, {kCx, 0, 1, kH}, trophy::colors.accent_cyan, LV_OPA_50);
  rect(root, {36, 36, 728, 408}, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, trophy::colors.warning, LV_OPA_70, 1, 0);
  for (int i = 0; i < count; ++i) {
    const Rect& b = boxes[static_cast<size_t>(i)];
    rect(root, b, LV_COLOR_MAKE(0, 0, 0), LV_OPA_TRANSP, trophy::colors.success, LV_OPA_70, 1, 0);
  }
  label(root, label_text, &td_font_12, trophy::colors.warning, {18, 446, 760, 20});
}

lv_obj_t* build_dashboard(const trophy::ProfileData& p, const DemoMetrics& m, bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);

  const Rect profile_card{36, 54, 264, 334};
  const Rect xp_card{332, 54, 432, 264};
  rect(root, profile_card, trophy::colors.surface_primary, opa_pct(88), trophy::colors.accent_cyan, opa_pct(55), 1, 16);
  line(root, {58, 70, 220, 2}, trophy::colors.accent_cyan);
  avatar_img(root, p, {60, 89, 86, 86});
  label(root, p.username.c_str(), &td_font_24, trophy::colors.text_primary, {164, 106, 118, 30});
  label(root, "Profil actif", &td_font_12, trophy::colors.text_secondary, {166, 138, 100, 17});
  country_chip(root, {164, 154, 56, 28}, m.country);
  if (m.ps_plus) ps_plus_chip(root, 236, 150);

  const Rect level_card{57, 207, 222, 160};
  rect(root, level_card, color_hex(0x07111A), opa_pct(94), trophy::colors.accent_cyan, opa_pct(32), 1, 14);
  glow_pill(root, {72, 254, 172, 76}, trophy::colors.accent_cyan, 8);
  rect(root, {72, 220, 96, 26}, color_hex(0x001A20), LV_OPA_COVER, trophy::colors.accent_cyan, LV_OPA_COVER, 1, 13);
  label(root, "NIVEAU PSN", &td_font_12, trophy::colors.accent_cyan, {72, 226, 96, 14}, LV_TEXT_ALIGN_CENTER);
  label(root, std::to_string(p.level).c_str(), &td_font_96, trophy::colors.text_primary, {72, 253, 172, 86},
        LV_TEXT_ALIGN_CENTER);
  label(root, std::to_string(m.remaining_points).c_str(), &td_font_12, trophy::colors.text_secondary, {72, 341, 40, 17});
  label(root, "pts restants", &td_font_12, trophy::colors.text_secondary, {112, 341, 86, 17});
  label(root, std::to_string(p.progress).append("%").c_str(), &td_font_12, trophy::colors.accent_cyan, {230, 341, 36, 17},
        LV_TEXT_ALIGN_RIGHT);
  progress_bar(root, {72, 358, 188, 8}, p.progress, trophy::colors.accent_cyan);
  label(root, std::to_string(m.level_next).c_str(), &td_font_12, trophy::colors.text_secondary, {224, 228, 34, 16},
        LV_TEXT_ALIGN_RIGHT);
  label(root, "suivant", &td_font_10, trophy::colors.text_muted, {218, 244, 44, 12}, LV_TEXT_ALIGN_RIGHT);

  rect(root, xp_card, trophy::colors.surface_primary, opa_pct(78), trophy::colors.accent_blue, LV_OPA_40, 1, 18);
  status_badge(root, p.updated.c_str());
  xp_ring(root, 548, 186, 216, p.progress);
  label(root, "38%", &td_font_24, trophy::colors.text_primary, {506, 148, 84, 30}, LV_TEXT_ALIGN_CENTER);
  label(root, "vers niveau 315", &td_font_16, trophy::colors.text_secondary, {484, 181, 132, 20}, LV_TEXT_ALIGN_CENTER);
  label(root, "progression PSN", &td_font_12, trophy::colors.accent_cyan, {491, 214, 116, 18}, LV_TEXT_ALIGN_CENTER);

  struct SmallCard {
    Rect r;
    const char* value;
    const char* label;
    lv_color_t accent;
  };
  const SmallCard small[] = {
      {{332, 346, 132, 80}, m.world_rank, "Monde", trophy::colors.accent_violet},
      {{476, 346, 132, 80}, m.national_rank, "France", LV_COLOR_MAKE(82, 198, 255)},
      {{620, 346, 144, 80}, m.trophy_points, "Points", trophy::colors.gold},
  };
  for (const SmallCard& c : small) {
    rect(root, c.r, trophy::colors.surface_primary, opa_pct(88), c.accent, opa_pct(48), 1, 12);
    label(root, c.value, &td_font_22, trophy::colors.text_primary, {c.r.x + 8, c.r.y + 21, c.r.w - 16, 24},
          LV_TEXT_ALIGN_CENTER);
    label(root, c.label, &td_font_12, trophy::colors.text_secondary, {c.r.x + 8, c.r.y + 50, c.r.w - 16, 16},
          LV_TEXT_ALIGN_CENTER);
    progress_bar(root, {c.r.x + 22, c.r.y + 70, c.r.w - 44, 4}, 100, c.accent);
  }

  if (analysis) {
    add_analysis_overlay(root, {profile_card, xp_card, level_card, small[0].r, small[1].r, small[2].r, {164, 154, 56, 28},
                                {236, 150, 36, 36}},
                         8, "Dashboard: profil, pays FR 56x28, badge PS+ 36x36, niveau centre dans 172px");
  }
  return root;
}

void trophy_row(lv_obj_t* root, trophy::TrophyKind kind, int value, const char* pct, int y, int bar_pct) {
  const Rect r{72, y, 656, 58};
  lv_color_t accent = trophy_color(kind);
  rect(root, r, trophy::colors.surface_primary, opa_pct(90), accent,
       kind == trophy::TrophyKind::Platinum ? LV_OPA_80 : opa_pct(62), 1, 14);
  line(root, {r.x + 1, r.y + 1, r.w - 2, 1}, accent, opa_pct(18));
  trophy_img(root, kind, 94, y + 7, 44);
  label(root, trophy_name(kind), &td_font_20, trophy::colors.text_primary, {158, y + 13, 120, 24});
  label(root, pct, &td_font_10, trophy::colors.text_secondary, {158, y + 36, 64, 12});
  progress_bar(root, {334, y + 42, 286, 4}, bar_pct, accent);
  label(root, number_with_spaces(value).c_str(), &td_font_20, kind == trophy::TrophyKind::Bronze ? accent : trophy::colors.text_primary,
        {642, y + 15, 62, 24}, LV_TEXT_ALIGN_RIGHT);
}

void trophy_focus_arc(lv_obj_t* parent, int cx, int cy, int size, int percent, lv_color_t accent) {
  lv_obj_t* bg = lv_arc_create(parent);
  lv_obj_remove_style_all(bg);
  clear_interaction(bg);
  lv_obj_set_size(bg, size, size);
  lv_obj_set_pos(bg, cx - size / 2, cy - size / 2);
  lv_arc_set_range(bg, 0, 100);
  lv_arc_set_value(bg, 100);
  lv_arc_set_bg_angles(bg, 132, 48);
  lv_obj_set_style_arc_color(bg, color_hex(0x102B38), LV_PART_MAIN);
  lv_obj_set_style_arc_opa(bg, opa_pct(70), LV_PART_MAIN);
  lv_obj_set_style_arc_width(bg, 14, LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(bg, false, LV_PART_MAIN);
  lv_obj_set_style_arc_width(bg, 0, LV_PART_INDICATOR);

  lv_obj_t* arc = lv_arc_create(parent);
  lv_obj_remove_style_all(arc);
  clear_interaction(arc);
  lv_obj_set_size(arc, size, size);
  lv_obj_set_pos(arc, cx - size / 2, cy - size / 2);
  lv_arc_set_range(arc, 0, 100);
  lv_arc_set_value(arc, std::clamp(percent, 0, 100));
  lv_arc_set_bg_angles(arc, 132, 48);
  lv_obj_set_style_arc_width(arc, 0, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc, accent, LV_PART_INDICATOR);
  lv_obj_set_style_arc_opa(arc, LV_OPA_COVER, LV_PART_INDICATOR);
  lv_obj_set_style_arc_width(arc, 14, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc, false, LV_PART_INDICATOR);
}

void trophy_detail_chip(lv_obj_t* root, Rect r, const char* label_text, const char* value, lv_color_t accent, int bar_pct) {
  rect(root, r, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(50), 1, 12);
  const int value_y = r.h >= 74 ? 12 : 10;
  const int label_y = r.h >= 74 ? 42 : 38;
  const int bar_y = r.h >= 74 ? r.h - 12 : r.h - 8;
  label(root, value, std::strlen(value) > 7 ? &td_font_16 : &td_font_20, trophy::colors.text_primary,
        {r.x + 10, r.y + value_y, r.w - 20, 24}, LV_TEXT_ALIGN_CENTER);
  label(root, label_text, &td_font_12, trophy::colors.text_secondary, {r.x + 16, r.y + label_y, r.w - 32, 16},
        LV_TEXT_ALIGN_CENTER);
  progress_bar(root, {r.x + 16, r.y + bar_y, r.w - 32, 4}, bar_pct, accent);
}

lv_obj_t* build_trophy_detail(const trophy::ProfileData& p, trophy::TrophyKind kind, bool analysis = false) {
  lv_color_t accent = trophy_color(kind);
  const int value = trophy_value(p, kind);
  const int share = trophy_share_percent(kind);
  const std::string value_text = number_with_spaces(value);
  const std::string share_total = std::string(trophy_share_text(kind)) + " de " + number_with_spaces(p.total);
  const Rect card{112, 116, 576, 270};
  const Rect trophy_zone{150, 144, 228, 204};
  const Rect trophy_box{200, 188, 128, 128};
  const Rect text_zone{416, 154, 232, 174};
  const Rect value_box{416, 204, 190, 58};
  const Rect bar_box{416, 314, 208, 8};
  const Rect share_box{416, 338, 190, 22};

  lv_obj_t* root = root_screen();
  background_rings(root, accent);

  label(root, trophy_name(kind), &td_font_28, trophy::colors.text_primary, {48, 56, 220, 34});
  label(root, trophy_focus_label(kind), &td_font_16, trophy::colors.text_secondary, {50, 90, 220, 22});
  line(root, {50, 116, 54, 2}, accent, LV_OPA_COVER);

  rect(root, card, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 18);
  card_accent(root, card, accent, 46);
  glow_pill(root, {174, 160, 184, 184}, accent, kind == trophy::TrophyKind::Platinum ? 18 : 11);
  trophy_focus_arc(root, 266, 252, 210, share, accent);
  trophy_img(root, kind, trophy_box.x, trophy_box.y, 128);
  label(root, trophy_name(kind), &td_font_18, trophy::colors.text_secondary, {416, 164, 140, 24});
  label(root, value_text.c_str(), &td_font_48, accent, value_box);
  label(root, "trophées", &td_font_16, trophy::colors.text_secondary, {416, 270, 120, 20});
  progress_bar(root, bar_box, share, accent);
  label(root, share_total.c_str(), &td_font_16, trophy::colors.text_secondary, share_box);

  if (analysis) {
    add_analysis_overlay(root,
                         {{{48, 56, 220, 62}, card, trophy_zone, trophy_box, text_zone, value_box, bar_box, share_box,
                           {50, 116, 54, 2}}},
                         9, "Trophee detail N: carte 576x270 centree, trophy 128, gap 38px entre zones trophy/texte");
  }
  return root;
}

lv_obj_t* build_bronze_variant(const trophy::ProfileData& p, int variant) {
  const lv_color_t accent = trophy::colors.bronze;
  const int share = trophy_share_percent(trophy::TrophyKind::Bronze);
  lv_obj_t* root = root_screen();
  background_rings(root, accent);

  auto bronze_header = [&](const char* variant_label, const char* subtitle) {
    label(root, variant_label, &td_font_28, trophy::colors.text_primary, {48, 52, 280, 34});
    label(root, subtitle, &td_font_16, trophy::colors.text_secondary, {50, 88, 280, 22});
    line(root, {50, 116, 54, 2}, accent, LV_OPA_COVER);
    status_badge(root, "variante", 650, 67, accent, "trophy-pro");
  };

  switch (variant) {
    case 1: {
      bronze_header("Bronze A", "Fiche respirante");
      const Rect hero{48, 134, 300, 294};
      const Rect info{386, 126, 362, 238};
      rect(root, hero, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(56), 1, 18);
      card_accent(root, hero, accent, 34);
      glow_pill(root, {110, 160, 172, 172}, accent, 13);
      trophy_focus_arc(root, 196, 252, 232, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 124, 178, 144);
      label(root, "BRONZE", &td_font_12, trophy::colors.text_muted, {118, 340, 156, 16}, LV_TEXT_ALIGN_CENTER);
      label(root, "Collection", &td_font_20, accent, {96, 362, 200, 26}, LV_TEXT_ALIGN_CENTER);
      progress_bar(root, {104, 404, 184, 6}, share, accent);

      rect(root, info, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(44), 1, 16);
      card_accent(root, info, accent, 34);
      label(root, "Collection Bronze", &td_font_16, trophy::colors.text_secondary, {420, 154, 190, 22});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {418, 190, 210, 58});
      label(root, "trophees obtenus", &td_font_16, trophy::colors.text_primary, {422, 252, 210, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_22, trophy::colors.text_primary, {640, 202, 76, 28},
            LV_TEXT_ALIGN_RIGHT);
      label(root, "du total", &td_font_12, trophy::colors.text_secondary, {652, 232, 64, 16}, LV_TEXT_ALIGN_RIGHT);
      progress_bar(root, {422, 302, 286, 8}, share, accent);
      label(root, "Lecture simple, sans cartes secondaires", &td_font_14, trophy::colors.text_secondary, {422, 382, 292, 20},
            LV_TEXT_ALIGN_CENTER);
      break;
    }
    case 2: {
      bronze_header("Bronze B", "Focus valeur");
      const Rect hero{56, 132, 250, 296};
      const Rect info{340, 116, 408, 312};
      rect(root, hero, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(50), 1, 18);
      glow_pill(root, {90, 166, 182, 182}, accent, 12);
      trophy_focus_arc(root, 181, 257, 228, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 113, 185, 136);
      label(root, "TROPHEE", &td_font_12, trophy::colors.text_muted, {111, 358, 140, 16}, LV_TEXT_ALIGN_CENTER);

      rect(root, info, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 18);
      line(root, {380, 144, 284, 2}, accent, LV_OPA_COVER);
      label(root, "Bronze", &td_font_28, trophy::colors.text_primary, {388, 170, 180, 34});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_96, accent, {384, 210, 238, 92});
      label(root, "trophees dans la collection", &td_font_16, trophy::colors.text_secondary, {392, 310, 268, 22});
      progress_bar(root, {392, 350, 238, 8}, share, accent);
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_20, trophy::colors.text_primary, {636, 340, 84, 28},
            LV_TEXT_ALIGN_RIGHT);
      break;
    }
    case 3: {
      bronze_header("Bronze C", "Synthese propre");
      const Rect hero{48, 138, 286, 286};
      const Rect info{370, 120, 374, 174};
      const Rect strip{370, 314, 374, 82};
      rect(root, hero, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(52), 1, 18);
      trophy_focus_arc(root, 191, 256, 222, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 127, 188, 128);
      label(root, "Bronze", &td_font_24, trophy::colors.text_primary, {108, 350, 166, 30}, LV_TEXT_ALIGN_CENTER);

      rect(root, info, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(42), 1, 16);
      card_accent(root, info, accent, 32);
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {410, 154, 168, 58});
      label(root, "trophees bronze", &td_font_16, trophy::colors.text_secondary, {414, 214, 170, 22});
      label(root, number_with_spaces(p.total).c_str(), &td_font_24, trophy::colors.text_primary, {636, 164, 84, 30},
            LV_TEXT_ALIGN_CENTER);
      label(root, "total", &td_font_12, trophy::colors.text_secondary, {650, 198, 56, 16}, LV_TEXT_ALIGN_CENTER);

      rect(root, strip, color_hex(0x07111A), opa_pct(92), accent, opa_pct(42), 1, 14);
      label(root, "Part dans la collection", &td_font_16, trophy::colors.text_primary, {400, 334, 210, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_24, accent, {638, 326, 76, 30}, LV_TEXT_ALIGN_RIGHT);
      progress_bar(root, {400, 368, 286, 8}, share, accent);
      break;
    }
    case 4: {
      bronze_header("Bronze D", "Vitrine centrale");
      const Rect stage{90, 128, 620, 298};
      rect(root, stage, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(45), 1, 20);
      card_accent(root, stage, accent, 42);
      glow_pill(root, {288, 154, 224, 224}, accent, 13);
      trophy_focus_arc(root, 400, 266, 260, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 316, 180, 168);
      label(root, "BRONZE", &td_font_12, trophy::colors.text_muted, {326, 348, 148, 16}, LV_TEXT_ALIGN_CENTER);
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {126, 204, 146, 58}, LV_TEXT_ALIGN_CENTER);
      label(root, "obtenus", &td_font_16, trophy::colors.text_secondary, {138, 264, 122, 22}, LV_TEXT_ALIGN_CENTER);
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_28, trophy::colors.text_primary, {552, 210, 100, 36},
            LV_TEXT_ALIGN_CENTER);
      label(root, "du total", &td_font_16, trophy::colors.text_secondary, {562, 252, 80, 22}, LV_TEXT_ALIGN_CENTER);
      progress_bar(root, {148, 390, 504, 8}, share, accent);
      break;
    }
    default: {
      bronze_header("Bronze E", "Minimal premium");
      const Rect card{126, 124, 548, 292};
      rect(root, card, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(46), 1, 20);
      card_accent(root, card, accent, 48);
      glow_pill(root, {180, 162, 188, 188}, accent, 12);
      trophy_focus_arc(root, 274, 256, 240, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 202, 184, 144);
      line(root, {404, 166, 1, 178}, accent, opa_pct(25));
      label(root, "Bronze", &td_font_28, trophy::colors.text_primary, {438, 166, 172, 36});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {438, 210, 178, 58});
      label(root, "dans la collection", &td_font_16, trophy::colors.text_secondary, {442, 274, 176, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_22, trophy::colors.text_primary, {442, 320, 90, 28});
      progress_bar(root, {438, 360, 178, 7}, share, accent);
      break;
    }
  }

  return root;
}

lv_obj_t* build_bronze_variant_extra(const trophy::ProfileData& p, int variant, bool analysis = false) {
  const lv_color_t accent = trophy::colors.bronze;
  const int share = trophy_share_percent(trophy::TrophyKind::Bronze);
  lv_obj_t* root = root_screen();
  background_rings(root, accent);

  std::array<Rect, 12> audit_boxes{};
  int audit_count = 0;
  const char* audit_text = "";

  auto set_audit = [&](int index, Rect r) {
    if (index >= 0 && index < 12) audit_boxes[static_cast<size_t>(index)] = r;
  };

  auto header = [&](const char* title, const char* subtitle) {
    label(root, title, &td_font_28, trophy::colors.text_primary, {48, 52, 300, 34});
    label(root, subtitle, &td_font_16, trophy::colors.text_secondary, {50, 88, 300, 22});
    line(root, {50, 116, 54, 2}, accent, LV_OPA_COVER);
    status_badge(root, analysis ? "audit" : "detail", 650, 67, accent, "trophy-pro");
  };

  switch (variant) {
    case 6: {
      header("Bronze F", "Carte collector");
      const Rect stage{72, 130, 656, 284};
      const Rect trophy_box{114, 162, 224, 224};
      const Rect value_box{390, 196, 260, 58};
      const Rect share_box{392, 302, 252, 34};
      const Rect bar_box{392, 356, 260, 8};
      rect(root, stage, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(48), 1, 22);
      card_accent(root, stage, accent, 48);
      glow_pill(root, {trophy_box.x + 18, trophy_box.y + 18, 188, 188}, accent, 12);
      trophy_focus_arc(root, 226, 274, 244, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 154, 196, 144);
      line(root, {360, 166, 1, 192}, accent, opa_pct(25));
      label(root, "Bronze", &td_font_28, trophy::colors.text_primary, {392, 156, 160, 34});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, value_box);
      label(root, "trophees obtenus", &td_font_16, trophy::colors.text_secondary, {396, 260, 180, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_24, trophy::colors.text_primary, share_box,
            LV_TEXT_ALIGN_LEFT);
      label(root, "du total", &td_font_14, trophy::colors.text_secondary, {500, 310, 84, 18}, LV_TEXT_ALIGN_LEFT);
      progress_bar(root, bar_box, share, accent);
      audit_count = 7;
      set_audit(0, {48, 52, 300, 66});
      set_audit(1, stage);
      set_audit(2, trophy_box);
      set_audit(3, value_box);
      set_audit(4, {396, 260, 180, 22});
      set_audit(5, share_box);
      set_audit(6, bar_box);
      audit_text = "Bronze F: stage 656x284, titre/nombre/sous-texte separes, 8px mini entre zones texte";
      break;
    }
    case 7: {
      header("Bronze G", "Double panneau");
      const Rect trophy_card{72, 132, 304, 296};
      const Rect value_card{420, 132, 308, 126};
      const Rect share_card{420, 284, 308, 110};
      rect(root, trophy_card, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(52), 1, 20);
      card_accent(root, trophy_card, accent, 34);
      glow_pill(root, {126, 166, 196, 196}, accent, 12);
      trophy_focus_arc(root, 224, 264, 244, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 152, 190, 144);
      label(root, "BRONZE", &td_font_12, trophy::colors.text_muted, {150, 360, 148, 16}, LV_TEXT_ALIGN_CENTER);
      rect(root, value_card, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 16);
      label(root, "Collection", &td_font_14, trophy::colors.text_secondary, {448, 154, 130, 18});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {446, 184, 178, 58});
      label(root, "bronze", &td_font_16, trophy::colors.text_secondary, {634, 206, 70, 20}, LV_TEXT_ALIGN_RIGHT);
      rect(root, share_card, trophy::colors.surface_primary, opa_pct(86), accent, opa_pct(36), 1, 16);
      label(root, "Part du total", &td_font_16, trophy::colors.text_primary, {448, 306, 150, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_28, accent, {584, 300, 118, 34}, LV_TEXT_ALIGN_RIGHT);
      progress_bar(root, {448, 358, 226, 8}, share, accent);
      audit_count = 7;
      set_audit(0, {48, 52, 300, 66});
      set_audit(1, trophy_card);
      set_audit(2, {126, 166, 196, 196});
      set_audit(3, value_card);
      set_audit(4, {446, 184, 178, 58});
      set_audit(5, share_card);
      set_audit(6, {448, 358, 226, 8});
      audit_text = "Bronze G: 24px entre panneaux droits, part totale 118px alignee a droite, texte non tronque";
      break;
    }
    case 8: {
      header("Bronze H", "Bande technique");
      const Rect top_card{98, 132, 604, 186};
      const Rect bottom_band{98, 342, 604, 70};
      const Rect trophy_box{128, 154, 164, 164};
      rect(root, top_card, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(46), 1, 18);
      card_accent(root, top_card, accent, 40);
      glow_pill(root, {trophy_box.x, trophy_box.y, trophy_box.w, trophy_box.h}, accent, 11);
      trophy_img(root, trophy::TrophyKind::Bronze, 150, 174, 120);
      label(root, "Bronze", &td_font_28, trophy::colors.text_primary, {336, 166, 172, 36});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {334, 210, 178, 58});
      label(root, "trophees obtenus", &td_font_16, trophy::colors.text_secondary, {338, 270, 190, 22});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_28, trophy::colors.text_primary, {580, 214, 98, 34},
            LV_TEXT_ALIGN_RIGHT);
      label(root, "du total", &td_font_14, trophy::colors.text_secondary, {600, 252, 76, 18}, LV_TEXT_ALIGN_RIGHT);
      rect(root, bottom_band, color_hex(0x07111A), opa_pct(92), accent, opa_pct(45), 1, 18);
      label(root, "Poids dominant de la collection", &td_font_16, trophy::colors.text_secondary, {136, 358, 300, 20});
      progress_bar(root, {136, 390, 526, 9}, share, accent);
      audit_count = 8;
      set_audit(0, {48, 52, 300, 66});
      set_audit(1, top_card);
      set_audit(2, trophy_box);
      set_audit(3, {334, 210, 178, 58});
      set_audit(4, {580, 214, 98, 34});
      set_audit(5, bottom_band);
      set_audit(6, {136, 390, 526, 9});
      set_audit(7, {136, 358, 300, 20});
      audit_text = "Bronze H: top 604x186 + bande 604x70, label 32px au-dessus de la barre, bas respire";
      break;
    }
    case 9: {
      header("Bronze I", "Vitrine asymetrique");
      const Rect canvas{74, 124, 652, 304};
      const Rect trophy_box{88, 152, 330, 246};
      const Rect metric_box{452, 156, 238, 214};
      rect(root, canvas, trophy::colors.surface_primary, opa_pct(86), accent, opa_pct(42), 1, 22);
      line(root, {124, 148, 552, 2}, accent, LV_OPA_COVER);
      glow_pill(root, {154, 160, 208, 208}, accent, 12);
      trophy_focus_arc(root, 258, 264, 252, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 178, 184, 160);
      label(root, "BRONZE", &td_font_12, trophy::colors.text_muted, {184, 362, 148, 16}, LV_TEXT_ALIGN_CENTER);
      rect(root, metric_box, color_hex(0x07111A), opa_pct(72), accent, opa_pct(28), 1, 18);
      label(root, "Collection", &td_font_14, trophy::colors.text_secondary, {482, 184, 140, 18});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {480, 214, 166, 58});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_24, trophy::colors.text_primary, {480, 300, 100, 30});
      progress_bar(root, {480, 340, 170, 7}, share, accent);
      audit_count = 7;
      set_audit(0, {48, 52, 300, 66});
      set_audit(1, canvas);
      set_audit(2, trophy_box);
      set_audit(3, {178, 184, 160, 160});
      set_audit(4, metric_box);
      set_audit(5, {480, 214, 166, 58});
      set_audit(6, {480, 340, 170, 7});
      audit_text = "Bronze I: asymetrie controlee, metric box 238x214, valeur et pourcentage separes de 28px";
      break;
    }
    default: {
      header("Bronze J", "Console card");
      const Rect card{118, 122, 564, 304};
      const Rect trophy_box{152, 158, 204, 204};
      const Rect text_box{392, 168, 228, 190};
      rect(root, card, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 20);
      card_accent(root, card, accent, 46);
      trophy_focus_arc(root, 254, 260, 246, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, 182, 184, 144);
      line(root, {370, 166, 1, 190}, accent, opa_pct(28));
      label(root, "Bronze", &td_font_28, trophy::colors.text_primary, {394, 170, 180, 34});
      label(root, number_with_spaces(p.bronze).c_str(), &td_font_48, accent, {392, 214, 176, 58});
      label(root, "trophees", &td_font_16, trophy::colors.text_secondary, {396, 276, 120, 20});
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_24, trophy::colors.text_primary, {396, 320, 92, 28});
      progress_bar(root, {506, 336, 86, 7}, share, accent);
      audit_count = 8;
      set_audit(0, {48, 52, 300, 66});
      set_audit(1, card);
      set_audit(2, trophy_box);
      set_audit(3, {182, 184, 144, 144});
      set_audit(4, {370, 166, 1, 190});
      set_audit(5, text_box);
      set_audit(6, {392, 214, 176, 58});
      set_audit(7, {506, 336, 86, 7});
      audit_text = "Bronze J: carte 564x304 centree, separateur 34px apres trophee, barre courte volontaire";
      break;
    }
  }

  if (analysis) add_analysis_overlay(root, audit_boxes, audit_count, audit_text);
  return root;
}

lv_obj_t* build_bronze_variant_lvgl(const trophy::ProfileData& p, int variant, bool analysis = false) {
  const lv_color_t accent = trophy::colors.bronze;
  const int share = trophy_share_percent(trophy::TrophyKind::Bronze);
  const std::string count = number_with_spaces(p.bronze);
  const std::string total = number_with_spaces(p.total);
  lv_obj_t* root = root_screen();
  background_rings(root, accent);

  std::array<Rect, 12> audit_boxes{};
  int audit_count = 0;
  const char* audit_text = "";

  auto set_audit = [&](int index, Rect r) {
    if (index >= 0 && index < 12) audit_boxes[static_cast<size_t>(index)] = r;
  };

  auto header = [&](const char* title, const char* subtitle) {
    label(root, title, &td_font_28, trophy::colors.text_primary, {22, 20, 280, 34});
    label(root, subtitle, &td_font_16, trophy::colors.text_secondary, {22, 54, 320, 22});
    line(root, {22, 80, 132, 2}, accent, LV_OPA_COVER);
    status_badge(root, analysis ? "audit" : "detail", 646, 20, accent, "trophy-pro");
  };

  switch (variant) {
    case 11: {
      header("Bronze K", "Anneau signature");
      const Rect stage{46, 90, 708, 350};
      const Rect ring_box{244, 112, 312, 312};
      const Rect trophy_box{322, 180, 128, 128};
      const Rect value_box{248, 322, 304, 58};
      rect(root, stage, trophy::colors.surface_primary, opa_pct(38), accent, opa_pct(24), 1, 18);
      glow_pill(root, {298, 152, 196, 196}, accent, 11);
      trophy_focus_arc(root, 400, 266, 252, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, trophy_box.x, trophy_box.y, 128);
      label(root, count.c_str(), &td_font_48, accent, value_box, LV_TEXT_ALIGN_CENTER);
      label(root, "trophees bronze", &td_font_16, trophy::colors.text_secondary, {284, 382, 160, 22}, LV_TEXT_ALIGN_RIGHT);
      label(root, trophy_share_text(trophy::TrophyKind::Bronze), &td_font_16, trophy::colors.text_primary,
            {454, 382, 90, 22});
      label(root, ("sur " + total + " au compte").c_str(), &td_font_14, trophy::colors.text_muted, {292, 414, 216, 18},
            LV_TEXT_ALIGN_CENTER);
      audit_count = 6;
      set_audit(0, {22, 20, 320, 62});
      set_audit(1, stage);
      set_audit(2, ring_box);
      set_audit(3, trophy_box);
      set_audit(4, value_box);
      set_audit(5, {284, 382, 260, 22});
      audit_text = "Bronze K: anneau centre x=400, trophy 128 reel, valeur separee de 14px, aucun chevauchement";
      break;
    }
    case 12: {
      header("Bronze L", "Duo horizontal epure");
      const Rect stage{50, 86, 704, 354};
      const Rect trophy_card{78, 128, 252, 252};
      const Rect trophy_box{140, 190, 128, 128};
      const Rect text_col{360, 132, 344, 238};
      rect(root, stage, trophy::colors.surface_primary, opa_pct(36), trophy::colors.accent_blue, opa_pct(18), 1, 18);
      rect(root, trophy_card, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(38), 1, 16);
      glow_pill(root, {114, 162, 180, 180}, accent, 11);
      trophy_focus_arc(root, 204, 254, 204, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, trophy_box.x, trophy_box.y, 128);
      label(root, "Collection", &td_font_16, trophy::colors.text_secondary, {360, 138, 200, 22});
      label(root, count.c_str(), &td_font_48, accent, {360, 178, 240, 58});
      label(root, "trophees obtenus", &td_font_18, trophy::colors.text_secondary, {360, 242, 240, 24});
      progress_bar(root, {360, 296, 300, 8}, share, accent);
      label(root, "78.5 % du total de la collection", &td_font_16, trophy::colors.text_secondary, {360, 326, 300, 22});
      audit_count = 7;
      set_audit(0, {22, 20, 320, 62});
      set_audit(1, stage);
      set_audit(2, trophy_card);
      set_audit(3, trophy_box);
      set_audit(4, text_col);
      set_audit(5, {360, 178, 240, 58});
      set_audit(6, {360, 296, 300, 8});
      audit_text = "Bronze L: colonne texte x=360, carte trophy 252x252, gap horizontal 30px, barre 300px";
      break;
    }
    case 13: {
      header("Bronze M", "Contexte collection");
      const Rect trophy_card{20, 104, 224, 224};
      const Rect trophy_box{68, 152, 128, 128};
      const Rect metric_col{278, 120, 330, 170};
      const Rect dist_band{20, 352, 560, 68};
      rect(root, trophy_card, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(42), 1, 16);
      glow_pill(root, {44, 128, 176, 176}, accent, 11);
      trophy_focus_arc(root, 132, 216, 188, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, trophy_box.x, trophy_box.y, 128);
      label(root, count.c_str(), &td_font_48, accent, {278, 124, 220, 58});
      label(root, "78.5 % du total", &td_font_18, trophy::colors.text_secondary, {280, 188, 230, 24});
      progress_bar(root, {280, 226, 300, 8}, share, accent);
      label(root, ("total " + total).c_str(), &td_font_16, trophy::colors.text_muted, {280, 258, 170, 22});
      rect(root, dist_band, trophy::colors.surface_primary, opa_pct(72), accent, opa_pct(32), 1, 12);
      label(root, "Pt 1", &td_font_14, trophy::colors.text_secondary, {36, 376, 54, 18});
      label(root, "Or 185", &td_font_14, trophy::colors.gold, {112, 376, 70, 18});
      label(root, "Ag 539", &td_font_14, trophy::colors.silver, {204, 376, 74, 18});
      label(root, ("Br " + count).c_str(), &td_font_14, accent, {304, 376, 96, 18});
      label(root, ("total " + total).c_str(), &td_font_14, trophy::colors.text_muted, {440, 376, 120, 18},
            LV_TEXT_ALIGN_RIGHT);
      audit_count = 8;
      set_audit(0, {22, 20, 320, 62});
      set_audit(1, trophy_card);
      set_audit(2, trophy_box);
      set_audit(3, metric_col);
      set_audit(4, {278, 124, 220, 58});
      set_audit(5, {280, 226, 300, 8});
      set_audit(6, dist_band);
      set_audit(7, {36, 376, 524, 18});
      audit_text = "Bronze M: bande recap 560x68, 28px entre carte et colonne, valeurs sur axe y=376";
      break;
    }
    case 14: {
      header("Bronze N", "Carte console centree");
      const Rect card{112, 116, 576, 270};
      const Rect trophy_zone{150, 144, 228, 204};
      const Rect trophy_box{200, 188, 128, 128};
      const Rect text_zone{416, 154, 232, 174};
      rect(root, card, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 18);
      card_accent(root, card, accent, 46);
      glow_pill(root, {174, 160, 184, 184}, accent, 11);
      trophy_focus_arc(root, 266, 252, 210, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, trophy_box.x, trophy_box.y, 128);
      label(root, "Bronze", &td_font_18, trophy::colors.text_secondary, {416, 164, 140, 24});
      label(root, count.c_str(), &td_font_48, accent, {416, 204, 190, 58});
      label(root, "trophees", &td_font_16, trophy::colors.text_secondary, {416, 270, 120, 20});
      progress_bar(root, {416, 314, 208, 8}, share, accent);
      label(root, ("78.5 % de " + total).c_str(), &td_font_16, trophy::colors.text_secondary, {416, 338, 190, 22});
      audit_count = 8;
      set_audit(0, {22, 20, 320, 62});
      set_audit(1, card);
      set_audit(2, trophy_zone);
      set_audit(3, trophy_box);
      set_audit(4, text_zone);
      set_audit(5, {416, 204, 190, 58});
      set_audit(6, {416, 314, 208, 8});
      set_audit(7, {416, 338, 190, 22});
      audit_text = "Bronze N: carte 576x270 centree, trophy reel 128, gap 38px entre zones trophy/texte";
      break;
    }
    default: {
      header("Bronze O", "Vitrine compacte");
      const Rect card{96, 92, 608, 330};
      const Rect trophy_box{136, 152, 192, 192};
      const Rect real_trophy{168, 184, 128, 128};
      const Rect metric_box{382, 136, 242, 204};
      rect(root, card, trophy::colors.surface_primary, opa_pct(86), accent, opa_pct(38), 1, 22);
      line(root, {140, 124, 520, 2}, accent, opa_pct(88));
      glow_pill(root, trophy_box, accent, 12);
      trophy_focus_arc(root, 232, 250, 220, share, accent);
      trophy_img(root, trophy::TrophyKind::Bronze, real_trophy.x, real_trophy.y, 128);
      rect(root, metric_box, color_hex(0x07111A), opa_pct(74), accent, opa_pct(28), 1, 18);
      label(root, "Bronze", &td_font_16, trophy::colors.text_secondary, {412, 164, 160, 22});
      label(root, count.c_str(), &td_font_48, accent, {410, 202, 180, 58});
      label(root, "trophees collection", &td_font_14, trophy::colors.text_secondary, {412, 270, 190, 18});
      progress_bar(root, {412, 316, 170, 7}, share, accent);
      audit_count = 7;
      set_audit(0, {22, 20, 320, 62});
      set_audit(1, card);
      set_audit(2, trophy_box);
      set_audit(3, real_trophy);
      set_audit(4, metric_box);
      set_audit(5, {410, 202, 180, 58});
      set_audit(6, {412, 316, 170, 7});
      audit_text = "Bronze O: vitrine 608x330, deux masses equilibrees, trophy 128 visible au-dessus du halo";
      break;
    }
  }

  if (analysis) add_analysis_overlay(root, audit_boxes, audit_count, audit_text);
  return root;
}

lv_obj_t* build_trophies(const trophy::ProfileData& p, bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "Trophées", "Répartition globale");
  status_badge(root, p.updated.c_str());
  rect(root, {580, 105, 148, 32}, color_hex(0x07111A), opa_pct(78), trophy::colors.accent_cyan, opa_pct(28), 1, 16);
  label(root, "Total", &td_font_10, trophy::colors.text_secondary, {600, 113, 42, 12});
  label(root, number_with_spaces(p.total).c_str(), &td_font_14, trophy::colors.text_primary, {640, 109, 66, 18},
        LV_TEXT_ALIGN_RIGHT);
  trophy_row(root, trophy::TrophyKind::Platinum, p.platinum, "0.0 %", 148, 1);
  trophy_row(root, trophy::TrophyKind::Gold, p.gold, "5.5 %", 222, 5);
  trophy_row(root, trophy::TrophyKind::Silver, p.silver, "16.0 %", 296, 16);
  trophy_row(root, trophy::TrophyKind::Bronze, p.bronze, "78.5 %", 370, 79);
  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 220, 64}, {72, 148, 656, 58}, {72, 222, 656, 58}, {72, 296, 656, 58},
                                {72, 370, 656, 58}, {650, 67, 104, 28}, {580, 105, 148, 32}}},
                         6, "Trophées: lignes 684x61, pas vertical 76px, valeurs alignées à droite x=710");
  }
  return root;
}

void stat_tile(lv_obj_t* root, Rect r, const char* icon, lv_color_t accent, const char* value, const char* caption,
               int bar_pct) {
  LV_UNUSED(bar_pct);
  rect(root, r, trophy::colors.surface_primary, opa_pct(84), accent, opa_pct(68), 1, 14);
  line(root, {r.x + 24, r.y + 16, r.w - 48, 1}, accent, opa_pct(28));
  glow_pill(root, {r.x + 18, r.y + 24, 68, 68}, accent, 7);
  icon_circle(root, icon, r.x + 54, r.y + 58, 52, 34, accent);

  const int text_x = r.x + 100;
  const int text_w = r.w - 112;
  label(root, value, std::strlen(value) >= 8 ? &td_font_20 : &td_font_24, trophy::colors.text_primary,
        {text_x, r.y + 30, text_w, 30}, LV_TEXT_ALIGN_LEFT);
  label(root, caption, &td_font_14, trophy::colors.text_secondary, {text_x, r.y + 68, text_w, 20},
        LV_TEXT_ALIGN_LEFT);
  line(root, {text_x, r.y + 94, 46, 3}, accent, opa_pct(86), 2);
}

lv_obj_t* build_statistics(const trophy::ProfileData&, const DemoMetrics& m, bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "Statistiques", "Vue rapide du profil");
  status_badge(root, "à l'instant");
  const Rect tiles[] = {
      {48, 146, 216, 108}, {292, 146, 216, 108}, {536, 146, 216, 108},
      {48, 288, 216, 108}, {292, 288, 216, 108}, {536, 288, 216, 108},
  };
  stat_tile(root, tiles[0], "gamepad-pro", trophy::colors.accent_cyan, number_with_spaces(m.games).c_str(), "jeux", 100);
  stat_tile(root, tiles[1], "seal-check-pro", trophy::colors.success, number_with_spaces(m.completed).c_str(), "terminé", 100);
  stat_tile(root, tiles[2], "percent-pro", trophy::colors.accent_violet, "22 %", "complétion", 100);
  stat_tile(root, tiles[3], "rarity-star-pro", LV_COLOR_MAKE(82, 198, 255), m.rarity, "rareté moy.", 100);
  stat_tile(root, tiles[4], "timer-pro", trophy::colors.gold, m.play_time, "temps joué", 100);
  stat_tile(root, tiles[5], "trophy-pro", trophy::colors.bronze, number_with_spaces(m.unearned).c_str(), "non obtenus", 100);
  if (analysis) {
    add_analysis_overlay(root,
                         {tiles[0], tiles[1], tiles[2], tiles[3], tiles[4], tiles[5], {76, 178, 52, 52},
                          {320, 178, 52, 52}, {564, 178, 52, 52}, {76, 320, 52, 52}, {320, 320, 52, 52},
                          {564, 320, 52, 52}},
                         12, "Statistiques: grille 3x2, colonnes x=48/292/536, tuiles 216x108, sans fausses barres");
  }
  return root;
}

lv_obj_t* build_sync(const trophy::ProfileData&, bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  status_badge(root, "en cours", 650, 67, trophy::colors.success, "circle-dot");
  xp_ring(root, kCx, 198, 220, 64);
  icon_circle(root, "refresh-cw", kCx, 198, 74, 42, trophy::colors.accent_cyan);
  label(root, "Synchronisation", &td_font_28, trophy::colors.text_primary, {272, 320, 256, 36}, LV_TEXT_ALIGN_CENTER);
  label(root, "Récupération du profil", &td_font_16, trophy::colors.text_secondary, {292, 356, 216, 22}, LV_TEXT_ALIGN_CENTER);
  progress_bar(root, {326, 392, 148, 5}, 64, trophy::colors.accent_cyan);
  label(root, "64 %", &td_font_12, trophy::colors.accent_cyan, {378, 405, 44, 18}, LV_TEXT_ALIGN_CENTER);
  const char* steps[] = {"Connexion", "Profil", "Données"};
  for (int i = 0; i < 3; ++i) {
    const int x = 285 + i * 82;
    const lv_color_t c = i == 1 ? trophy::colors.accent_cyan : trophy::colors.text_muted;
    tiny_dot(root, x, 438, c, 5);
    label(root, steps[i], &td_font_10, c, {x + 10, 434, 64, 14});
  }
  if (analysis) {
    add_analysis_overlay(root, {{{290, 88, 220, 220}, {363, 161, 74, 74}, {272, 320, 256, 36}, {292, 356, 216, 22},
                                {326, 392, 148, 5}, {650, 67, 104, 28}}},
                         6, "Sync: anneau, icone et barre de progression centres sur x=400");
  }
  return root;
}

lv_obj_t* build_state_screen(const char* title, const char* subtitle, const char* icon, lv_color_t accent, const char* card_title,
                             const char* message, const char* detail, bool error, bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, title, subtitle);
  const Rect card_r{102, 158, 596, 184};
  rect(root, card_r, trophy::colors.surface_primary, opa_pct(92), accent, error ? opa_pct(45) : opa_pct(55), 1, 14);
  card_top_accent(root, card_r, accent, 34);
  icon_circle(root, icon, 196, 246, 84, 42, accent);
  label(root, card_title, &td_font_24, trophy::colors.text_primary, {276, 196, 306, 30});
  label(root, message, &td_font_16, trophy::colors.text_secondary, {278, 236, 396, 22});
  label(root, detail, &td_font_12, trophy::colors.text_muted, {278, 264, 390, 18});
  rect(root, {278, 288, 168, 40}, error ? color_hex(0x321106) : color_hex(0x042937), opa_pct(94), accent, LV_OPA_COVER, 1,
       21);
  label(root, "Réessayer", &td_font_14, trophy::colors.text_primary, {318, 300, 88, 18}, LV_TEXT_ALIGN_CENTER);
  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 230, 64}, card_r, {154, 202, 84, 84}, {276, 198, 306, 30},
                                {278, 238, 420, 22}, {278, 288, 168, 42}}},
                         6, "Etat: bouton 168x40, marge basse 14px, accent haut sans trait derriere le bouton");
  }
  return root;
}

void settings_row(lv_obj_t* root, int y, const char* icon, const char* name, const char* value, lv_color_t accent, int bar_pct = -1,
                  bool toggle = false) {
  const Rect row{77, y, 646, 44};
  rect(root, row, trophy::colors.surface_primary, opa_pct(84), accent, opa_pct(78), 1, 22);
  icon_circle(root, icon, 108, y + 22, 30, 18, accent);
  label(root, name, &td_font_16, trophy::colors.text_primary, {144, y + 13, 260, 20});
  if (bar_pct < 0 && std::strcmp(value, "82 %") == 0) bar_pct = 82;
  if (!toggle && std::strcmp(value, "ON") == 0) toggle = true;
  if (bar_pct >= 0) {
    progress_bar(root, {506, y + 20, 86, 5}, bar_pct, accent);
  } else if (toggle) {
    rect(root, {640, y + 12, 48, 20}, color_hex(0x03120F), opa_pct(95), accent, LV_OPA_80, 1, 10);
    tiny_dot(root, 672, y + 17, accent, 10);
  }
  const Rect value_r = (bar_pct >= 0 || toggle) ? Rect{600, y + 14, 32, 16} : Rect{604, y + 14, 86, 16};
  label(root, value, &td_font_12, trophy::colors.text_secondary, value_r, LV_TEXT_ALIGN_RIGHT);
}

lv_obj_t* build_settings(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "Réglages", "Contrôles rapides");
  status_badge(root, "à l'instant");
  settings_row(root, 133, "sun", "Luminosité", "82 %", trophy::colors.accent_cyan);
  settings_row(root, 186, "rotate-ccw", "Rotation auto", "ON", LV_COLOR_MAKE(82, 198, 255));
  settings_row(root, 239, "sparkles", "Animations", "ON", trophy::colors.accent_violet);
  settings_row(root, 292, "refresh-cw", "Actualisation", "30 min", trophy::colors.gold);
  settings_row(root, 345, "gamepad-pro", "Profil", "gaz91610", trophy::colors.success);
  settings_row(root, 398, "info", "À propos", "Version 1.0", trophy::colors.text_secondary);
  if (analysis) {
    add_analysis_overlay(root,
                         {{{77, 133, 646, 44}, {77, 186, 646, 44}, {77, 239, 646, 44}, {77, 292, 646, 44},
                           {77, 345, 646, 44}, {77, 398, 646, 44}, {93, 148, 30, 30}, {144, 133, 260, 44},
                           {604, 133, 86, 44}}},
                         9, "Réglages: lignes 44px min, axe icônes x=108, libellés x=144, valeurs droite x=690");
  }
  return root;
}

lv_obj_t* build_about(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "À propos", "");
  const Rect card_r{126, 112, 548, 300};
  const Rect pocket_r{226, 240, 348, 90};
  rect(root, card_r, trophy::colors.surface_primary, opa_pct(92), trophy::colors.accent_blue, opa_pct(45), 1, 16);
  card_accent(root, card_r, trophy::colors.accent_cyan, 56);

  label(root, "Trophy Display", &td_font_28, trophy::colors.text_primary, {248, 144, 304, 34}, LV_TEXT_ALIGN_CENTER);
  label(root, "Version 1.0", &td_font_16, trophy::colors.text_secondary, {326, 182, 148, 20}, LV_TEXT_ALIGN_CENTER);
  line(root, {256, 212, 288, 1}, trophy::colors.accent_cyan, opa_pct(65));

  label(root, "Données PlayStation fournies par", &td_font_14, trophy::colors.text_secondary, {242, 222, 316, 20},
        LV_TEXT_ALIGN_CENTER);
  rect(root, pocket_r, color_hex(0x07111A), opa_pct(86), trophy::colors.accent_cyan, opa_pct(48), 1, 14);

  constexpr int kLogoW = assets::kPocketPsnLogoWidth * 2;
  constexpr int kLogoH = assets::kPocketPsnLogoHeight * 2;
  lv_obj_t* logo = lv_image_create(root);
  lv_image_set_src(logo, assets::pocketpsn_logo_descriptor());
  lv_image_set_scale(logo, 512);
  lv_obj_set_size(logo, kLogoW, kLogoH);
  lv_image_set_inner_align(logo, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_set_pos(logo, pocket_r.x + 30, pocket_r.y + 22);
  clear_interaction(logo);

  label(root, "Pocket PSN", &td_font_24, trophy::colors.text_primary, {pocket_r.x + 134, pocket_r.y + 20, 176, 30},
        LV_TEXT_ALIGN_LEFT);
  label(root, "pocketpsn.com", &td_font_16, trophy::colors.accent_cyan, {pocket_r.x + 134, pocket_r.y + 52, 176, 20},
        LV_TEXT_ALIGN_LEFT);

  label(root, "Conception et développement", &td_font_12, trophy::colors.text_secondary, {252, 354, 296, 16},
        LV_TEXT_ALIGN_CENTER);
  label(root, "Kevin Torres", &td_font_16, trophy::colors.text_primary, {324, 374, 152, 20}, LV_TEXT_ALIGN_CENTER);
  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 230, 64}, card_r, pocket_r, {248, 144, 304, 34}, {326, 182, 148, 20},
                                {242, 222, 316, 20}, {pocket_r.x + 30, pocket_r.y + 22, kLogoW, kLogoH},
                                {pocket_r.x + 134, pocket_r.y + 20, 176, 30}, {252, 354, 296, 40}}},
                         9, "À propos: carte 548x300 centrée, logo Pocket PSN x2 net, attribution lisible");
  }
  return root;
}

lv_obj_t* build_icon_gallery(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "IconGallery", "Assets réels utilisés");
  const struct Item {
    const char* icon;
    const char* name;
    lv_color_t color;
  } items[] = {
      {"gamepad-pro", "Jeux", trophy::colors.accent_cyan},       {"seal-check-pro", "Terminé", trophy::colors.success},
      {"percent-pro", "Complétion", trophy::colors.accent_violet}, {"rarity-star-pro", "Rareté", LV_COLOR_MAKE(82, 198, 255)},
      {"timer-pro", "Temps", trophy::colors.gold},                 {"trophy-pro", "Non obtenus", trophy::colors.bronze},
      {"refresh-cw", "Sync", trophy::colors.accent_cyan},      {"wifi-off", "Hors ligne", LV_COLOR_MAKE(82, 198, 255)},
      {"triangle-alert", "Erreur", trophy::colors.warning},    {"settings", "Réglages", trophy::colors.text_secondary},
  };
  for (int i = 0; i < 10; ++i) {
    const int col = i % 5;
    const int row = i / 5;
    const int x = 78 + col * 138;
    const int y = 168 + row * 112;
    icon_circle(root, items[i].icon, x + 38, y, 56, 28, items[i].color);
    label(root, items[i].name, &td_font_12, trophy::colors.text_secondary, {x - 20, y + 38, 116, 18}, LV_TEXT_ALIGN_CENTER);
  }
  if (analysis) {
    add_analysis_overlay(root, {{{60, 128, 680, 250}}}, 1, "IconGallery: grille 5 colonnes, boîtes 56x56, libellés centrés");
  }
  return root;
}

lv_obj_t* build_showroom(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root);
  title_block(root, "Showroom", "Séquence démo");
  const char* names[] = {"Dashboard", "Trophées", "Stats", "Sync", "Hors ligne", "Erreur", "Réglages", "À propos"};
  const char* icons[] = {"gauge", "trophy-pro", "chart-no-axes-combined", "refresh-cw", "wifi-off", "triangle-alert",
                         "settings", "info"};
  const lv_color_t accents[] = {trophy::colors.accent_cyan, trophy::colors.gold, trophy::colors.accent_violet,
                                trophy::colors.accent_cyan, LV_COLOR_MAKE(82, 198, 255), trophy::colors.warning,
                                trophy::colors.success, trophy::colors.text_secondary};
  for (int i = 0; i < 8; ++i) {
    const int col = i % 4;
    const int row = i / 4;
    const Rect r{68 + col * 170, 155 + row * 108, 134, 78};
    rect(root, r, trophy::colors.surface_primary, opa_pct(88), accents[i], opa_pct(55), 1, 12);
    icon_circle(root, icons[i], r.x + 67, r.y + 27, 34, 18, accents[i]);
    label(root, names[i], &td_font_12, trophy::colors.text_primary, {r.x + 10, r.y + 53, r.w - 20, 16}, LV_TEXT_ALIGN_CENTER);
  }
  if (analysis) {
    add_analysis_overlay(root, {{{68, 155, 644, 186}}}, 1, "Showroom: 8 etapes, cartes 134x78, grille 4 colonnes");
  }
  return root;
}

void sync_delta_row(lv_obj_t* root, int y, trophy::TrophyKind kind, const char* name, const char* delta, const char* total,
                    int pct) {
  const lv_color_t accent = trophy_color(kind);
  trophy_img(root, kind, 402, y + 2, 34);
  label(root, name, &td_font_18, trophy::colors.text_primary, {448, y + 4, 102, 22});
  label(root, delta, &td_font_20, accent, {558, y + 2, 62, 24}, LV_TEXT_ALIGN_RIGHT);
  label(root, total, &td_font_16, trophy::colors.text_secondary, {642, y + 5, 70, 20}, LV_TEXT_ALIGN_RIGHT);
  progress_bar(root, {448, y + 32, 264, 4}, pct, accent);
}

lv_obj_t* build_sync_summary(bool analysis = false) {
  lv_obj_t* root = root_screen();
  const lv_color_t accent = trophy::colors.success;
  background_rings(root, accent);
  title_block(root, "Bilan sync", "Résumé global");
  status_badge(root, "a l'instant", 646, 67, accent, "refresh-cw");

  const Rect hero{52, 132, 292, 286};
  const Rect detail{376, 118, 372, 222};
  const Rect meta_a{376, 350, 116, 70};
  const Rect meta_b{504, 350, 116, 70};
  const Rect meta_c{632, 350, 116, 70};

  rect(root, hero, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(60), 1, 18);
  card_accent(root, hero, accent, 32);
  glow_pill(root, {106, 164, 184, 184}, accent, 12);
  trophy_focus_arc(root, 198, 250, 222, 62, accent);
  label(root, "+14", &td_font_96, trophy::colors.text_primary, {80, 176, 236, 100}, LV_TEXT_ALIGN_CENTER);
  label(root, "trophées ajoutés", &td_font_20, accent, {82, 284, 232, 26}, LV_TEXT_ALIGN_CENTER);
  label(root, "depuis la derniere synchro", &td_font_14, trophy::colors.text_secondary, {70, 320, 258, 20},
        LV_TEXT_ALIGN_CENTER);
  trophy_img(root, trophy::TrophyKind::Gold, 86, 360, 40);
  trophy_img(root, trophy::TrophyKind::Silver, 138, 360, 40);
  trophy_img(root, trophy::TrophyKind::Bronze, 190, 360, 40);
  trophy_img(root, trophy::TrophyKind::Platinum, 242, 360, 40);
  progress_bar(root, {106, 406, 184, 6}, 62, accent);

  rect(root, detail, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(48), 1, 16);
  card_accent(root, detail, accent, 34);
  label(root, "Compteurs mis a jour", &td_font_24, trophy::colors.text_primary, {412, 144, 260, 30});
  label(root, "Données globales uniquement.", &td_font_14, trophy::colors.text_secondary, {414, 176, 286, 20});
  sync_delta_row(root, 210, trophy::TrophyKind::Platinum, "Platine", "+0", "1", 0);
  sync_delta_row(root, 252, trophy::TrophyKind::Gold, "Or", "+1", "185", 8);
  sync_delta_row(root, 294, trophy::TrophyKind::Silver, "Argent", "+3", "539", 26);

  trophy_detail_chip(root, meta_a, "Bronze", "+10", trophy::colors.bronze, 78);
  trophy_detail_chip(root, meta_b, "Total", "3 376", trophy::colors.accent_cyan, 100);
  trophy_detail_chip(root, meta_c, "Statut", "OK", trophy::colors.success, 100);

  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 220, 64}, hero, detail, meta_a, meta_b, meta_c, {87, 139, 222, 222},
                                {80, 176, 236, 100}, {86, 360, 196, 40}, {402, 212, 34, 34}, {646, 67, 108, 28}}},
                         10, "Bilan sync: aucun dernier trophee detaille, seulement deltas de compteurs globaux");
  }
  return root;
}

lv_obj_t* build_ambient(bool analysis = false) {
  lv_obj_t* root = root_screen();
  const lv_color_t accent = trophy::colors.platinum;
  background_rings(root, accent);
  line(root, {126, 82, 548, 1}, accent, opa_pct(42));
  line(root, {126, 398, 548, 1}, accent, opa_pct(24));

  trophy_focus_arc(root, 526, 228, 256, 38, accent);
  glow_pill(root, {434, 136, 184, 184}, accent, 13);
  trophy_img(root, trophy::TrophyKind::Platinum, 462, 154, 128);
  label(root, "22:41", &td_font_96, trophy::colors.text_primary, {74, 156, 250, 100}, LV_TEXT_ALIGN_CENTER);
  label(root, "gaz91610", &td_font_24, trophy::colors.text_primary, {104, 276, 190, 30}, LV_TEXT_ALIGN_CENTER);
  label(root, "Niveau 314  |  38 % vers 315", &td_font_16, trophy::colors.text_secondary, {76, 314, 282, 22},
        LV_TEXT_ALIGN_CENTER);
  progress_bar(root, {112, 352, 180, 6}, 38, trophy::colors.accent_cyan);
  label(root, "mode veille", &td_font_14, trophy::colors.text_muted, {560, 352, 112, 20}, LV_TEXT_ALIGN_RIGHT);

  if (analysis) {
    add_analysis_overlay(root, {{{74, 156, 250, 100}, {462, 154, 128, 128}, {104, 276, 190, 30}, {76, 314, 282, 22},
                                {112, 352, 180, 6}, {398, 100, 256, 256}}},
                         6, "Veille: horloge bloc gauche, trophee/anneau centre x=526, aucun chevauchement");
  }
  return root;
}

void timeline_row(lv_obj_t* root, int y, trophy::TrophyKind kind, const char* title, const char* game, const char* meta) {
  const lv_color_t accent = trophy_color(kind);
  const Rect r{78, y, 644, 72};
  rect(root, r, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(50), 1, 14);
  trophy_img(root, kind, 100, y + 10, 52);
  label(root, title, &td_font_20, trophy::colors.text_primary, {174, y + 13, 250, 24});
  label(root, game, &td_font_12, trophy::colors.text_secondary, {176, y + 42, 238, 16});
  progress_bar(root, {438, y + 36, 164, 5}, kind == trophy::TrophyKind::Bronze ? 78 : 18, accent);
  label(root, meta, &td_font_14, accent, {622, y + 26, 74, 20}, LV_TEXT_ALIGN_RIGHT);
}

lv_obj_t* build_timeline(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root, trophy::colors.accent_cyan);
  title_block(root, "Timeline", "Derniers trophées");
  status_badge(root, "3 recents", 650, 67, trophy::colors.success, "history");
  timeline_row(root, 148, trophy::TrophyKind::Gold, "Un autre chemin", "Marvel's Spider-Man 2", "12.4 %");
  timeline_row(root, 242, trophy::TrophyKind::Silver, "Equipement complet", "Gran Turismo 7", "31.8 %");
  timeline_row(root, 336, trophy::TrophyKind::Bronze, "Premier depart", "Astro Bot", "78.5 %");
  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 220, 64}, {78, 148, 644, 72}, {78, 242, 644, 72}, {78, 336, 644, 72},
                                {100, 158, 52, 52}, {174, 161, 250, 24}, {438, 184, 164, 5}, {622, 174, 74, 20}}},
                         8, "Timeline: 3 lignes 644x72, pas 94px, icone 52px, texte/barre/meta separes");
  }
  return root;
}

lv_obj_t* build_next_goal(const trophy::ProfileData& p, const DemoMetrics& m, bool analysis = false) {
  lv_obj_t* root = root_screen();
  const lv_color_t accent = trophy::colors.accent_cyan;
  background_rings(root, accent);
  title_block(root, "Objectif suivant", "Niveau PSN");
  status_badge(root, "actif", 650, 67, trophy::colors.success, "circle-dot");

  const Rect ring_card{52, 136, 300, 284};
  const Rect detail{384, 128, 364, 170};
  const Rect step_a{384, 324, 108, 74};
  const Rect step_b{512, 324, 108, 74};
  const Rect step_c{640, 324, 108, 74};

  rect(root, ring_card, trophy::colors.surface_primary, opa_pct(88), accent, opa_pct(54), 1, 18);
  card_accent(root, ring_card, accent, 32);
  xp_ring(root, 202, 254, 210, p.progress);
  label(root, std::to_string(p.progress).append("%").c_str(), &td_font_28, trophy::colors.text_primary, {156, 218, 92, 36},
        LV_TEXT_ALIGN_CENTER);
  label(root, "vers niveau 315", &td_font_16, trophy::colors.text_secondary, {132, 256, 140, 22}, LV_TEXT_ALIGN_CENTER);
  label(root, "progression PSN", &td_font_12, accent, {138, 288, 128, 18}, LV_TEXT_ALIGN_CENTER);

  rect(root, detail, trophy::colors.surface_primary, opa_pct(90), accent, opa_pct(45), 1, 16);
  card_accent(root, detail, accent, 34);
  label(root, "Encore", &td_font_14, trophy::colors.text_secondary, {418, 158, 84, 18});
  label(root, std::to_string(m.remaining_points).c_str(), &td_font_48, trophy::colors.text_primary, {418, 184, 130, 56});
  label(root, "points avant le niveau 315", &td_font_16, trophy::colors.text_secondary, {418, 244, 220, 22});
  progress_bar(root, {418, 274, 272, 6}, p.progress, accent);
  trophy_img(root, trophy::TrophyKind::Platinum, 626, 160, 72);

  trophy_detail_chip(root, step_a, "Niveau", "314", accent, p.progress);
  trophy_detail_chip(root, step_b, "Suivant", "315", trophy::colors.accent_violet, 62);
  trophy_detail_chip(root, step_c, "Reste", "555", trophy::colors.gold, 38);

  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 220, 64}, ring_card, detail, step_a, step_b, step_c, {97, 149, 210, 210},
                                {418, 184, 130, 56}, {626, 160, 72, 72}, {418, 274, 272, 6}}},
                         10, "Objectif: ring 210 centre x=202, detail 364x170, chips 108x74, marges >=20px");
  }
  return root;
}

lv_obj_t* build_transition_lab(bool analysis = false) {
  lv_obj_t* root = root_screen();
  background_rings(root, trophy::colors.accent_violet);
  title_block(root, "Transition test", "Trophées");
  status_badge(root, "prototype", 626, 67, trophy::colors.accent_violet, "sparkles");

  const Rect stage{74, 132, 652, 244};
  rect(root, stage, trophy::colors.surface_primary, opa_pct(88), trophy::colors.accent_violet, opa_pct(52), 1, 18);
  card_accent(root, stage, trophy::colors.accent_violet, 38);
  trophy_img(root, trophy::TrophyKind::Gold, 154, 198, 88);
  trophy_img(root, trophy::TrophyKind::Platinum, 356, 174, 128);
  trophy_img(root, trophy::TrophyKind::Silver, 590, 204, 72);
  trophy_focus_arc(root, 420, 238, 196, 72, trophy::colors.accent_violet);
  label(root, "Or", &td_font_16, trophy::colors.gold, {154, 300, 88, 20}, LV_TEXT_ALIGN_CENTER);
  label(root, "Platine", &td_font_24, trophy::colors.text_primary, {334, 312, 172, 30}, LV_TEXT_ALIGN_CENTER);
  label(root, "Argent", &td_font_16, trophy::colors.silver, {582, 300, 90, 20}, LV_TEXT_ALIGN_CENTER);
  line(root, {268, 238, 54, 2}, trophy::colors.text_muted, opa_pct(55));
  line(root, {508, 238, 54, 2}, trophy::colors.text_muted, opa_pct(55));

  trophy_detail_chip(root, {120, 386, 150, 68}, "Fade", "220 ms", trophy::colors.accent_cyan, 100);
  trophy_detail_chip(root, {325, 386, 150, 68}, "Scale", "96-100", trophy::colors.accent_violet, 86);
  trophy_detail_chip(root, {530, 386, 150, 68}, "Anneau", "0-100", trophy::colors.gold, 72);

  if (analysis) {
    add_analysis_overlay(root, {{{48, 64, 220, 64}, stage, {154, 198, 88, 88}, {356, 174, 128, 128}, {590, 204, 72, 72},
                                {120, 386, 150, 68}, {325, 386, 150, 68}, {530, 386, 150, 68}}},
                         8, "Transition lab: stage 652x244, centre hero x=420, cartes specs 150x68");
  }
  return root;
}

lv_obj_t* build_background_only(lv_color_t accent = LV_COLOR_MAKE(0, 220, 240)) {
  lv_obj_t* root = root_screen();
  background_rings(root, accent);
  return root;
}

lv_obj_t* build_slide(WideSlide slide, const trophy::ProfileData& p, const DemoMetrics& m) {
  switch (slide) {
    case WideSlide::Dashboard:
      return build_dashboard(p, m);
    case WideSlide::Trophies:
      return build_trophies(p);
    case WideSlide::TrophyPlatinum:
      return build_trophy_detail(p, trophy::TrophyKind::Platinum);
    case WideSlide::TrophyGold:
      return build_trophy_detail(p, trophy::TrophyKind::Gold);
    case WideSlide::TrophySilver:
      return build_trophy_detail(p, trophy::TrophyKind::Silver);
    case WideSlide::TrophyBronze:
      return build_trophy_detail(p, trophy::TrophyKind::Bronze);
    case WideSlide::Statistics:
      return build_statistics(p, m);
    case WideSlide::Sync:
      return build_sync(p);
    case WideSlide::Offline:
      return build_state_screen("Hors ligne", "Dernières données disponibles", "wifi-off", LV_COLOR_MAKE(82, 198, 255),
                                "Connexion absente", "Le profil reste lisible avec la dernière sauvegarde.",
                                "Nouvelle tentative possible depuis cet écran.", false);
    case WideSlide::Error:
      return build_state_screen("Erreur", "Synchronisation interrompue", "triangle-alert", trophy::colors.warning,
                                "Profil indisponible", "La réponse Pocket PSN n'est pas exploitable.",
                                "Les anciennes données restent affichées.", true);
    case WideSlide::Settings:
      return build_settings();
    case WideSlide::About:
      return build_about();
    case WideSlide::IconGallery:
      return build_icon_gallery();
    case WideSlide::Showroom:
      return build_showroom();
    case WideSlide::SyncSummary:
      return build_sync_summary();
    case WideSlide::Ambient:
      return build_ambient();
    case WideSlide::Timeline:
      return build_timeline();
    case WideSlide::NextGoal:
      return build_next_goal(p, m);
    case WideSlide::TransitionLab:
      return build_transition_lab();
    case WideSlide::DebugOverlay:
      return build_statistics(p, m, true);
    case WideSlide::LayoutOverlay:
      return build_dashboard(p, m, true);
  }
  return nullptr;
}

lv_obj_t* build_slide_analysis(WideSlide slide, const trophy::ProfileData& p, const DemoMetrics& m) {
  switch (slide) {
    case WideSlide::Dashboard:
      return build_dashboard(p, m, true);
    case WideSlide::Trophies:
      return build_trophies(p, true);
    case WideSlide::TrophyPlatinum:
      return build_trophy_detail(p, trophy::TrophyKind::Platinum, true);
    case WideSlide::TrophyGold:
      return build_trophy_detail(p, trophy::TrophyKind::Gold, true);
    case WideSlide::TrophySilver:
      return build_trophy_detail(p, trophy::TrophyKind::Silver, true);
    case WideSlide::TrophyBronze:
      return build_trophy_detail(p, trophy::TrophyKind::Bronze, true);
    case WideSlide::Statistics:
      return build_statistics(p, m, true);
    case WideSlide::Sync:
      return build_sync(p, true);
    case WideSlide::Offline:
      return build_state_screen("Hors ligne", "Dernières données disponibles", "wifi-off", LV_COLOR_MAKE(82, 198, 255),
                                "Connexion absente", "Le profil reste lisible avec la dernière sauvegarde.",
                                "Nouvelle tentative possible depuis cet écran.", false, true);
    case WideSlide::Error:
      return build_state_screen("Erreur", "Synchronisation interrompue", "triangle-alert", trophy::colors.warning,
                                "Profil indisponible", "La réponse Pocket PSN n'est pas exploitable.",
                                "Les anciennes données restent affichées.", true, true);
    case WideSlide::Settings:
      return build_settings(true);
    case WideSlide::About:
      return build_about(true);
    case WideSlide::IconGallery:
      return build_icon_gallery(true);
    case WideSlide::Showroom:
      return build_showroom(true);
    case WideSlide::SyncSummary:
      return build_sync_summary(true);
    case WideSlide::Ambient:
      return build_ambient(true);
    case WideSlide::Timeline:
      return build_timeline(true);
    case WideSlide::NextGoal:
      return build_next_goal(p, m, true);
    case WideSlide::TransitionLab:
      return build_transition_lab(true);
    case WideSlide::DebugOverlay:
      return build_statistics(p, m, true);
    case WideSlide::LayoutOverlay:
      return build_dashboard(p, m, true);
  }
  return nullptr;
}

void settle_frame(SDL_Renderer* renderer, int frames = 8) {
  for (int f = 0; f < frames; ++f) {
    lv_tick_inc(16);
    lv_timer_handler();
    SDL_Delay(16);
  }
  DisplayDriverSdlWide::present(renderer);
}

bool save_slide(SDL_Renderer* renderer, const char* dir, const char* name, lv_obj_t* screen, lv_obj_t*& previous) {
  lv_scr_load(screen);
  if (previous) lv_obj_delete(previous);
  previous = screen;
  settle_frame(renderer, 10);
  char path[320];
  std::snprintf(path, sizeof(path), "%s/%s.png", dir, name);
  const bool ok = DisplayDriverSdlWide::saveScreenshotPng(path);
  std::printf("%s : %s\n", ok ? "Capture enregistree" : "Capture impossible", path);
  return ok;
}

}  // namespace

int main(int argc, char** argv) {
  const trophy::ProfileData profile = demoProfile();
  const DemoMetrics metrics;
  assets::set_profile_avatar_resolver(assets::demo_profile_avatar_descriptor);

  const std::array<WideSlide, 21> slides = {
      WideSlide::Dashboard, WideSlide::Trophies, WideSlide::TrophyPlatinum, WideSlide::TrophyGold,
      WideSlide::TrophySilver, WideSlide::TrophyBronze, WideSlide::Statistics, WideSlide::Sync,
      WideSlide::Offline,   WideSlide::Error,    WideSlide::Settings,   WideSlide::About,
      WideSlide::IconGallery, WideSlide::Showroom, WideSlide::SyncSummary, WideSlide::Ambient,
      WideSlide::Timeline, WideSlide::NextGoal, WideSlide::TransitionLab, WideSlide::DebugOverlay, WideSlide::LayoutOverlay,
  };
  const std::array<const char*, 21> names = {
      "01-dashboard", "02-trophies", "03-trophy-platinum", "04-trophy-gold",
      "05-trophy-silver", "06-trophy-bronze", "07-statistics", "08-sync",
      "09-offline", "10-error", "11-settings", "12-about",
      "13-icon-gallery", "14-showroom", "15-sync-summary", "16-ambient",
      "17-timeline", "18-next-goal", "19-transition-lab", "20-debug-overlay", "21-layout-overlay",
  };

  bool exportOnly = false;
  bool exportBronzeVariants = false;
  bool exportBronzeMore = false;
  bool exportBronzeLvgl = false;
  bool exportBackgroundVariants = false;
  bool exportBackgroundOnly = false;
  const char* screenshotDir = "simulator_wide_screenshots/dashboard-family-v1";
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) == "--export-screenshots") exportOnly = true;
    if (std::string(argv[i]) == "--export-bronze-variants") exportBronzeVariants = true;
    if (std::string(argv[i]) == "--export-bronze-more") exportBronzeMore = true;
    if (std::string(argv[i]) == "--export-bronze-lvgl") exportBronzeLvgl = true;
    if (std::string(argv[i]) == "--export-background-variants") exportBackgroundVariants = true;
    if (std::string(argv[i]) == "--export-background-only") exportBackgroundOnly = true;
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    std::fprintf(stderr, "SDL_Init a echoue: %s\n", SDL_GetError());
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow("Trophy Display LVGL - 7 pouces 800x480", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, kW, kH, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (!window || !renderer) {
    std::fprintf(stderr, "Creation SDL impossible: %s\n", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  lv_init();
  DisplayDriverSdlWide::init(renderer, kW, kH);
  trophy::init_theme();

  if (exportBackgroundOnly) {
    const char* backgroundDir = "simulator_wide_screenshots/background-variants";
    std::error_code ec;
    std::filesystem::create_directories(backgroundDir, ec);
    lv_obj_t* previous = nullptr;
    const BackgroundStyle styles[] = {BackgroundStyle::TrophyShowcase, BackgroundStyle::CollectorStage,
                                      BackgroundStyle::ConsoleDashboard, BackgroundStyle::TrophyVault,
                                      BackgroundStyle::LeaderboardWall, BackgroundStyle::ButtonScatter};
    for (BackgroundStyle style : styles) {
      g_background_style = style;
      char name[96];
      std::snprintf(name, sizeof(name), "%s-background-only", background_style_name(style));
      save_slide(renderer, backgroundDir, name, build_background_only(), previous);
    }
    if (previous) lv_obj_delete(previous);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (exportBackgroundVariants) {
    const char* backgroundDir = "simulator_wide_screenshots/background-variants";
    std::error_code ec;
    std::filesystem::create_directories(backgroundDir, ec);
    lv_obj_t* previous = nullptr;
    const BackgroundStyle styles[] = {BackgroundStyle::TrophyShowcase, BackgroundStyle::CollectorStage,
                                      BackgroundStyle::ConsoleDashboard, BackgroundStyle::TrophyVault,
                                      BackgroundStyle::LeaderboardWall, BackgroundStyle::ButtonScatter};
    for (BackgroundStyle style : styles) {
      g_background_style = style;
      char name[96];
      std::snprintf(name, sizeof(name), "%s-dashboard", background_style_name(style));
      save_slide(renderer, backgroundDir, name, build_dashboard(profile, metrics), previous);
      std::snprintf(name, sizeof(name), "%s-trophies", background_style_name(style));
      save_slide(renderer, backgroundDir, name, build_trophies(profile), previous);
      std::snprintf(name, sizeof(name), "%s-statistics", background_style_name(style));
      save_slide(renderer, backgroundDir, name, build_statistics(profile, metrics), previous);
    }
    if (previous) lv_obj_delete(previous);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (exportBronzeVariants) {
    const char* bronzeDir = "simulator_wide_screenshots/bronze-variants";
    std::error_code ec;
    std::filesystem::create_directories(bronzeDir, ec);
    lv_obj_t* previous = nullptr;
    for (int i = 1; i <= 5; ++i) {
      char name[48];
      std::snprintf(name, sizeof(name), "bronze-variant-%02d", i);
      save_slide(renderer, bronzeDir, name, build_bronze_variant(profile, i), previous);
    }
    if (previous) lv_obj_delete(previous);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (exportBronzeMore) {
    const char* bronzeDir = "simulator_wide_screenshots/bronze-variants-v2";
    std::error_code ec;
    std::filesystem::create_directories(bronzeDir, ec);
    lv_obj_t* previous = nullptr;
    for (int i = 6; i <= 10; ++i) {
      char name[48];
      std::snprintf(name, sizeof(name), "bronze-variant-%02d", i);
      save_slide(renderer, bronzeDir, name, build_bronze_variant_extra(profile, i), previous);
      char audit_name[64];
      std::snprintf(audit_name, sizeof(audit_name), "bronze-variant-%02d-audit", i);
      save_slide(renderer, bronzeDir, audit_name, build_bronze_variant_extra(profile, i, true), previous);
    }
    if (previous) lv_obj_delete(previous);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (exportBronzeLvgl) {
    const char* bronzeDir = "simulator_wide_screenshots/bronze-variants-lvgl";
    std::error_code ec;
    std::filesystem::create_directories(bronzeDir, ec);
    lv_obj_t* previous = nullptr;
    for (int i = 11; i <= 15; ++i) {
      char name[48];
      std::snprintf(name, sizeof(name), "bronze-variant-%02d", i);
      save_slide(renderer, bronzeDir, name, build_bronze_variant_lvgl(profile, i), previous);
      char audit_name[64];
      std::snprintf(audit_name, sizeof(audit_name), "bronze-variant-%02d-audit", i);
      save_slide(renderer, bronzeDir, audit_name, build_bronze_variant_lvgl(profile, i, true), previous);
    }
    if (previous) lv_obj_delete(previous);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  if (exportOnly) {
    std::error_code ec;
    std::filesystem::create_directories(screenshotDir, ec);
    lv_obj_t* previous = nullptr;
    for (size_t i = 0; i < slides.size(); ++i) {
      save_slide(renderer, screenshotDir, names[i], build_slide(slides[i], profile, metrics), previous);
      char analysis_file[120];
      std::snprintf(analysis_file, sizeof(analysis_file), "%s-analysis", names[i]);
      save_slide(renderer, screenshotDir, analysis_file, build_slide_analysis(slides[i], profile, metrics), previous);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
  }

  int idx = 0;
  bool paused = false;
  lv_obj_t* current = build_slide(slides[static_cast<size_t>(idx)], profile, metrics);
  lv_scr_load(current);
  Uint32 lastSlideMs = SDL_GetTicks();
  bool running = true;

  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) running = false;
      if (event.type == SDL_KEYDOWN) {
        if (event.key.keysym.sym == SDLK_RIGHT) {
          idx = (idx + 1) % static_cast<int>(slides.size());
          lv_obj_t* next = build_slide(slides[static_cast<size_t>(idx)], profile, metrics);
          lv_obj_t* previous = current;
          lv_scr_load(next);
          current = next;
          lv_obj_delete(previous);
          lastSlideMs = SDL_GetTicks();
        } else if (event.key.keysym.sym == SDLK_LEFT) {
          idx = (idx + static_cast<int>(slides.size()) - 1) % static_cast<int>(slides.size());
          lv_obj_t* next = build_slide(slides[static_cast<size_t>(idx)], profile, metrics);
          lv_obj_t* previous = current;
          lv_scr_load(next);
          current = next;
          lv_obj_delete(previous);
          lastSlideMs = SDL_GetTicks();
        } else if (event.key.keysym.sym == SDLK_SPACE) {
          paused = !paused;
        } else if (event.key.keysym.sym >= SDLK_1 && event.key.keysym.sym <= SDLK_9) {
          idx = static_cast<int>(event.key.keysym.sym - SDLK_1);
          lv_obj_t* next = build_slide(slides[static_cast<size_t>(idx)], profile, metrics);
          lv_obj_t* previous = current;
          lv_scr_load(next);
          current = next;
          lv_obj_delete(previous);
          lastSlideMs = SDL_GetTicks();
        }
      }
    }

    const Uint32 now = SDL_GetTicks();
    if (!paused && now - lastSlideMs >= kSlideMs) {
      idx = (idx + 1) % static_cast<int>(slides.size());
      lv_obj_t* next = build_slide(slides[static_cast<size_t>(idx)], profile, metrics);
      lv_obj_t* previous = current;
      lv_scr_load(next);
      current = next;
      lv_obj_delete(previous);
      lastSlideMs = now;
    }

    lv_tick_inc(16);
    lv_timer_handler();
    DisplayDriverSdlWide::present(renderer);
    SDL_Delay(16);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
