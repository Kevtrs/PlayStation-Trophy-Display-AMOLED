#pragma once

#include <cstdint>

namespace trophy {

constexpr int DISPLAY_WIDTH = 466;
constexpr int DISPLAY_HEIGHT = 466;
constexpr int DISPLAY_CENTER_X = 233;
constexpr int DISPLAY_CENTER_Y = 233;
constexpr int DISPLAY_RADIUS = 233;
constexpr int SAFE_RADIUS = 205;
constexpr int CONTENT_RADIUS = 190;
constexpr int EDGE_MARGIN = 22;

constexpr int TOP_SAFE_Y = 28;
constexpr int BOTTOM_SAFE_Y = 438;
constexpr int TITLE_Y = 52;
constexpr int SUBTITLE_Y = 78;
constexpr int CONTENT_TOP = 96;
constexpr int CONTENT_BOTTOM = 396;
constexpr int PAGE_INDICATOR_Y = 420;

constexpr int CARD_GAP = 12;
constexpr int TEXT_GAP_SMALL = 6;
constexpr int TEXT_GAP_MEDIUM = 10;
constexpr int TEXT_GAP_LARGE = 16;
constexpr int TOUCH_TARGET_MIN = 44;

namespace icons {
constexpr int BADGE_BOX = 20;
constexpr int BADGE_GLYPH = 16;
constexpr int SETTINGS_BOX = 32;
constexpr int SETTINGS_GLYPH = 22;
constexpr int STAT_BOX = 30;
constexpr int STAT_GLYPH = 22;
constexpr int LINK_BOX = 24;
constexpr int LINK_GLYPH = 18;
constexpr int STATUS_GLYPH = 56;
} // namespace icons

constexpr int center_x(int width) {
    return (DISPLAY_WIDTH - width) / 2;
}

constexpr int DASH_STATUS_Y = 38;
constexpr int DASH_NAME_Y = 76;
constexpr int DASH_UPDATED_Y = 108;
constexpr int DASH_RING_TOP = 136;
constexpr int DASH_RING_SIZE = 180;
constexpr int DASH_CARD_Y = 315;
constexpr int DASH_CARD_H = 60;

constexpr int BOOT_OUTER_RING_SIZE = 444;
constexpr int BOOT_INNER_RING_SIZE = 360;
constexpr int BOOT_PROGRESS_RING_SIZE = 300;
constexpr int BOOT_PROGRESS_RING_X = center_x(BOOT_PROGRESS_RING_SIZE);
constexpr int BOOT_PROGRESS_RING_Y = 48;
constexpr int BOOT_PROGRESS_RING_CENTER_Y = BOOT_PROGRESS_RING_Y + BOOT_PROGRESS_RING_SIZE / 2;
constexpr int BOOT_TROPHY_SIZE = 116;
constexpr int BOOT_TROPHY_X = center_x(BOOT_TROPHY_SIZE);
constexpr int BOOT_TROPHY_Y = 118;
constexpr int BOOT_TITLE_W = 200;
constexpr int BOOT_TITLE_X = center_x(BOOT_TITLE_W);
constexpr int BOOT_TITLE_Y = 260;
constexpr int BOOT_PERCENT_W = 96;
constexpr int BOOT_PERCENT_X = center_x(BOOT_PERCENT_W);
constexpr int BOOT_PERCENT_Y = 292;
constexpr int BOOT_ATTR_W = 230;
constexpr int BOOT_ATTR_X = center_x(BOOT_ATTR_W);
constexpr int BOOT_ATTR_Y = 366;
constexpr int BOOT_WORDMARK_W = 176;
constexpr int BOOT_WORDMARK_X = center_x(BOOT_WORDMARK_W);
constexpr int BOOT_WORDMARK_Y = 388;
constexpr int BOOT_LOGO_GAP = 6;

constexpr int STAT_CARD_W = 150;
constexpr int STAT_CARD_H = 96;
constexpr int STAT_CARD_TOP_Y = 128;
constexpr int STAT_CARD_BOTTOM_Y = 242;

constexpr int TROPHY_ROW_W = 314;
constexpr int TROPHY_ROW_X = 76;
constexpr int TROPHY_ROW_H = 78;
constexpr int TROPHY_ROW_GAP = 8;
constexpr int TROPHY_ROW_FIRST_Y = 108;

constexpr int CELEBRATION_TROPHY_Y = 88;
constexpr int CELEBRATION_TITLE_Y = 286;
constexpr int CELEBRATION_GAIN_Y = 326;
constexpr int CELEBRATION_HINT_Y = 378;

constexpr int SETTINGS_ROW_W = 280;
constexpr int SETTINGS_ROW_X = center_x(SETTINGS_ROW_W);
constexpr int SETTINGS_ROW_H = 44;
constexpr int SETTINGS_BRIGHTNESS_ROW_H = 48;
constexpr int SETTINGS_ROW_GAP = 4;
constexpr int SETTINGS_FIRST_Y = 94;
constexpr int SETTINGS_SECOND_Y = SETTINGS_FIRST_Y + SETTINGS_BRIGHTNESS_ROW_H + SETTINGS_ROW_GAP;
constexpr int SETTINGS_THIRD_Y = SETTINGS_SECOND_Y + SETTINGS_ROW_H + SETTINGS_ROW_GAP;
constexpr int SETTINGS_FOURTH_Y = SETTINGS_THIRD_Y + SETTINGS_ROW_H + SETTINGS_ROW_GAP;
constexpr int SETTINGS_FIFTH_Y = SETTINGS_FOURTH_Y + SETTINGS_ROW_H + SETTINGS_ROW_GAP;
constexpr int SETTINGS_SIXTH_Y = SETTINGS_FIFTH_Y + SETTINGS_ROW_H + SETTINGS_ROW_GAP;
constexpr int SETTINGS_ICON_X = 10;
constexpr int SETTINGS_LABEL_X = 50;
constexpr int SETTINGS_LABEL_W = 130;
constexpr int SETTINGS_VALUE_X = 190;
constexpr int SETTINGS_VALUE_W = 80;
constexpr int SETTINGS_CONTROL_X = 228;
constexpr int SETTINGS_SLIDER_X = 50;
constexpr int SETTINGS_SLIDER_Y = 37;
constexpr int SETTINGS_SLIDER_W = 200;
constexpr int SETTINGS_SLIDER_H = 6;

constexpr int STATUS_BADGE_W = 124;
constexpr int STATUS_BADGE_H = 30;
constexpr int STATUS_BADGE_X = center_x(STATUS_BADGE_W);
constexpr int STATUS_BADGE_Y = 88;
constexpr int STATUS_ICON_SIZE = 108;
constexpr int STATUS_ICON_X = center_x(STATUS_ICON_SIZE);
constexpr int STATUS_ICON_Y = 126;
constexpr int STATUS_TITLE_Y = 238;
constexpr int STATUS_TITLE_W = 310;
constexpr int STATUS_TITLE_X = center_x(STATUS_TITLE_W);
constexpr int STATUS_MESSAGE_Y = 272;
constexpr int STATUS_MESSAGE_W = 318;
constexpr int STATUS_MESSAGE_H = 46;
constexpr int STATUS_MESSAGE_X = center_x(STATUS_MESSAGE_W);
constexpr int STATUS_BUTTON_W = 190;
constexpr int STATUS_BUTTON_H = 48;
constexpr int STATUS_BUTTON_X = center_x(STATUS_BUTTON_W);
constexpr int STATUS_BUTTON_Y = 334;

constexpr int SYNC_BADGE_Y = 58;
constexpr int SYNC_RING_TOP = 82;
constexpr int SYNC_RING_SIZE = 214;
constexpr int SYNC_DISC_SIZE = 96;
constexpr int SYNC_DISC_TOP = 141;
constexpr int SYNC_TITLE_Y = 314;
constexpr int SYNC_STATUS_Y = 346;
constexpr int SYNC_MESSAGE_Y = 372;
constexpr int SYNC_MESSAGE_W = 300;

constexpr int ABOUT_PRODUCT_Y = 102;
constexpr int ABOUT_VERSION_Y = 130;
constexpr int ABOUT_ATTR_Y = 176;
constexpr int ABOUT_PROVIDER_Y = 204;
constexpr int ABOUT_LINK_W = 196;
constexpr int ABOUT_LINK_H = 44;
constexpr int ABOUT_LINK_X = center_x(ABOUT_LINK_W);
constexpr int ABOUT_LINK_Y = 232;
constexpr int ABOUT_DESIGN_Y = 296;
constexpr int ABOUT_AUTHOR_Y = 320;
constexpr int ABOUT_BUTTON_Y = 354;

constexpr int SPACE_4 = 4;
constexpr int SPACE_8 = 8;
constexpr int SPACE_12 = 12;
constexpr int SPACE_16 = 16;
constexpr int SPACE_20 = 20;
constexpr int SPACE_24 = 24;
constexpr int SPACE_32 = 32;

inline bool inside_circle(int x, int y, int radius = DISPLAY_RADIUS) {
    const int dx = x - DISPLAY_CENTER_X;
    const int dy = y - DISPLAY_CENTER_Y;
    return dx * dx + dy * dy <= radius * radius;
}

} // namespace trophy
