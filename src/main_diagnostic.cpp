// Firmware de DIAGNOSTIC MATERIEL -- environnement PlatformIO separe
// (waveshare-amoled-175-diagnostic), jamais utilise par le firmware normal.
//
// Objectif : valider l'ecran, le tactile, la memoire et les infos puce
// INDEPENDAMMENT de LVGL/AppController/du design de l'ecran, en dessinant
// directement avec Arduino_GFX (memes bus/broches que src/main.cpp, voir
// BoardConfig.h -- source unique des broches). Si ce firmware fonctionne
// mais pas le firmware normal, le probleme est cote LVGL/logiciel plutot
// que materiel/QSPI/I2C ; si celui-ci echoue aussi, le probleme est plus
// fondamental (cablage, alimentation, broches).
//
// Portee volontairement limitee : pas de Wi-Fi/PocketPSN ici (deja
// largement couvert par le firmware normal -- voir ses logs [BOOT] et
// docs/HARDWARE_TEST_CHECKLIST.md section 2/3). Dupliquer WiFiClientSecure
// ici n'apporterait rien de plus et ajouterait de la complexite pour un
// firmware cense rester simple et fiable.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TouchDrvCST.hpp>
#include <WiFi.h>
#include <Wire.h>

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "BoardConfig.h"

namespace {

Arduino_DataBus *bus = new Arduino_ESP32QSPI(LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);
Arduino_CO5300 *gfx = new Arduino_CO5300(bus, LCD_RESET, LCD_ROTATION, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);
TouchDrvCST92xx touch;
volatile bool touchInterruptPending = false;

void IRAM_ATTR onTouchInterrupt() { touchInterruptPending = true; }

// Journalisation minimale, independante de src/utils/Logger -- ce firmware
// de diagnostic doit rester autonome (voir portee en tete de fichier),
// aucune dependance au reste de src/ hormis BoardConfig.h.
void logInfo(const char *fmt, ...) {
  char buf[192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.printf("[%8u][INFO ] %s\n", static_cast<unsigned>(millis()), buf);
}

void logErr(const char *fmt, ...) {
  char buf[192];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  Serial.printf("[%8u][ERROR] %s\n", static_cast<unsigned>(millis()), buf);
}

void logChipInfo() {
  logInfo("=== DIAGNOSTIC MATERIEL -- PlayStation Trophy Display AMOLED ===");
  logInfo("Chip: %s rev%d, %d coeur(s), %d MHz", ESP.getChipModel(), ESP.getChipRevision(),
               ESP.getChipCores(), ESP.getCpuFreqMHz());
  logInfo("Flash: %u octets", ESP.getFlashChipSize());
  if (psramFound()) {
    logInfo("PSRAM: %u octets detectes, %u octets libres", ESP.getPsramSize(), ESP.getFreePsram());
  } else {
    logErr("PSRAM introuvable -- verifier board_build.arduino.memory_type=qio_opi");
  }
  logInfo("Heap libre: %u octets (minimum observe depuis boot: %u)", ESP.getFreeHeap(),
               ESP.getMinFreeHeap());
  logInfo("Broches ecran: CS=%d SCLK=%d SDIO0-3=%d/%d/%d/%d RESET=%d", LCD_CS, LCD_SCLK, LCD_SDIO0,
               LCD_SDIO1, LCD_SDIO2, LCD_SDIO3, LCD_RESET);
  logInfo("Broches tactile: SDA=%d SCL=%d INT=%d RESET=%d", IIC_SDA, IIC_SCL, TP_INT, TP_RESET);
}

bool initDisplay() {
  if (!gfx->begin()) {
    logErr("[DIAG] Ecran: gfx->begin() a echoue -- verifier le cablage QSPI (CS=%d SCLK=%d)", LCD_CS,
                  LCD_SCLK);
    return false;
  }
  gfx->setBrightness(200);
  logInfo("[DIAG] Ecran: CO5300 initialise (%dx%d)", gfx->width(), gfx->height());
  return true;
}

bool initTouch() {
  touch.setPins(TP_RESET, TP_INT);
  if (!touch.begin(Wire, CST92XX_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    logErr("[DIAG] Tactile: CST9217 introuvable sur I2C (SDA=%d SCL=%d)", IIC_SDA, IIC_SCL);
    return false;
  }
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setSwapXY(TOUCH_SWAP_XY);
  touch.setMirrorXY(TOUCH_MIRROR_X, TOUCH_MIRROR_Y);
  logInfo("[DIAG] Tactile: %s detecte, %d point(s) supporte(s)", touch.getModelName(),
               touch.getSupportTouchPoint());
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), onTouchInterrupt, FALLING);
  return true;
}

// Wi-Fi : uniquement un scan (presence radio), jamais de connexion/HTTPS ici
// (voir portee ci-dessus). Fait une seule fois, au demarrage.
void logWifiScan() {
  WiFi.mode(WIFI_STA);
  int count = WiFi.scanNetworks();
  if (count < 0) {
    logErr("[DIAG] Wi-Fi: scan echoue (code %d) -- radio Wi-Fi absente ou defaillante", count);
  } else {
    logInfo("[DIAG] Wi-Fi: radio OK, %d reseau(x) visible(s)", count);
  }
  WiFi.scanDelete();
}

void drawCornerMarkers(int16_t w, int16_t h) {
  const int16_t s = 40;
  gfx->fillRect(0, 0, s, s, RED);           // haut-gauche
  gfx->fillRect(w - s, 0, s, s, GREEN);      // haut-droite
  gfx->fillRect(0, h - s, s, s, BLUE);       // bas-gauche
  gfx->fillRect(w - s, h - s, s, s, YELLOW);  // bas-droite
}

void drawLabel(const char *text, int16_t y) {
  gfx->setTextColor(WHITE, BLACK);
  gfx->setTextSize(2);
  int16_t textWidthPx = static_cast<int16_t>(strlen(text)) * 12;
  gfx->setCursor((gfx->width() - textWidthPx) / 2, y);
  gfx->print(text);
}

// Sequence de mires, ~2.5 s chacune, dans l'ordre indique par l'utilisateur.
// Un appui tactile pendant la sequence saute directement au test tactile
// (dernier ecran, qui reste affiche indefiniment).
enum class Pattern {
  kRed,
  kGreen,
  kBlue,
  kWhite,
  kBlack,
  kGradient,
  kCircle,
  kCornersAndText,
  kRotationInfo,
  kTouchTest,
};

void drawPattern(Pattern pattern) {
  const int16_t w = gfx->width();
  const int16_t h = gfx->height();
  switch (pattern) {
    case Pattern::kRed:
      gfx->fillScreen(RED);
      logInfo("[DIAG] Mire: fond rouge");
      break;
    case Pattern::kGreen:
      gfx->fillScreen(GREEN);
      logInfo("[DIAG] Mire: fond vert");
      break;
    case Pattern::kBlue:
      gfx->fillScreen(BLUE);
      logInfo("[DIAG] Mire: fond bleu");
      break;
    case Pattern::kWhite:
      gfx->fillScreen(WHITE);
      logInfo("[DIAG] Mire: fond blanc");
      break;
    case Pattern::kBlack:
      gfx->fillScreen(BLACK);
      logInfo("[DIAG] Mire: fond noir");
      break;
    case Pattern::kGradient:
      for (int16_t x = 0; x < w; ++x) {
        uint8_t level = static_cast<uint8_t>((x * 255) / w);
        gfx->drawFastVLine(x, 0, h, RGB565(level, level, 255 - level));
      }
      logInfo("[DIAG] Mire: degrade horizontal");
      break;
    case Pattern::kCircle:
      gfx->fillScreen(BLACK);
      gfx->fillCircle(w / 2, h / 2, 200, ORANGE);
      gfx->drawCircle(w / 2, h / 2, 200, WHITE);
      logInfo("[DIAG] Mire: cercle centre (rayon 200)");
      break;
    case Pattern::kCornersAndText:
      gfx->fillScreen(BLACK);
      drawCornerMarkers(w, h);
      drawLabel("466 x 466 OK", h / 2 - 10);
      gfx->drawFastHLine(0, h / 2, w, DARKGREY);
      gfx->drawFastVLine(w / 2, 0, h, DARKGREY);
      logInfo("[DIAG] Mire: reperes de coin + texte centre + croix");
      break;
    case Pattern::kRotationInfo: {
      gfx->fillScreen(NAVY);
      char buf[48];
      std::snprintf(buf, sizeof(buf), "Rotation actuelle: %d", gfx->getRotation());
      drawLabel(buf, h / 2 - 30);
      drawLabel("Comparer avec l'orientation physique", h / 2);
      logInfo("[DIAG] Mire: %s", buf);
      break;
    }
    case Pattern::kTouchTest:
      gfx->fillScreen(BLACK);
      drawLabel("Test tactile -- touchez l'ecran", 40);
      logInfo("[DIAG] Ecran final: test tactile en direct (reset pour recommencer la sequence)");
      break;
  }
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);

  logChipInfo();

  Wire.begin(IIC_SDA, IIC_SCL);
  bool displayOk = initDisplay();
  bool touchOk = initTouch();

  // Affichage immediat de la premiere mire des que l'ecran est pret, AVANT
  // le scan Wi-Fi (potentiellement bloquant plusieurs secondes sur du
  // materiel reel jamais teste) : sinon, si le scan bloque, rien ne
  // s'affiche jamais et l'ecran reste bloque sur la derniere image du
  // firmware precedent (le controleur AMOLED garde l'affichage sans
  // rafraichissement actif) -- bug reel trouve lors du premier essai
  // materiel le 2026-07-23.
  if (displayOk) {
    drawPattern(Pattern::kRed);
  }

  logWifiScan();

  logInfo("[DIAG] Resume bring-up: ecran=%s tactile=%s", displayOk ? "OK" : "ECHEC",
               touchOk ? "OK" : "ECHEC");

  if (!displayOk) {
    logErr("[DIAG] Arret: aucun affichage possible sans ecran initialise.");
    return;
  }
}

void loop() {
  static const Pattern kSequence[] = {
      Pattern::kRed,       Pattern::kGreen,          Pattern::kBlue,        Pattern::kWhite,
      Pattern::kBlack,     Pattern::kGradient,       Pattern::kCircle,      Pattern::kCornersAndText,
      Pattern::kRotationInfo,
  };
  constexpr size_t kSequenceLen = sizeof(kSequence) / sizeof(kSequence[0]);
  constexpr uint32_t kPatternDurationMs = 2500;

  static size_t patternIndex = 0;
  static bool inTouchTest = false;
  static uint32_t patternStartedMs = 0;
  static bool firstLoop = true;

  const uint32_t now = millis();

  if (firstLoop) {
    // La premiere mire (kRed == kSequence[0]) est deja dessinee dans
    // setup(), avant le scan Wi-Fi -- ici on initialise juste le minuteur.
    firstLoop = false;
    patternStartedMs = now;
  }

  // Un appui pendant la sequence saute directement au test tactile.
  if (!inTouchTest && touchInterruptPending) {
    inTouchTest = true;
    drawPattern(Pattern::kTouchTest);
  }

  if (!inTouchTest && now - patternStartedMs >= kPatternDurationMs) {
    patternIndex = (patternIndex + 1) % kSequenceLen;
    patternStartedMs = now;
    if (patternIndex == 0) {
      inTouchTest = true;  // sequence complete sans appui : on termine quand meme sur le test tactile
      drawPattern(Pattern::kTouchTest);
    } else {
      drawPattern(kSequence[patternIndex]);
    }
  }

  if (inTouchTest && touchInterruptPending) {
    touchInterruptPending = false;
    const TouchPoints &touchData = touch.getTouchPoints();
    static int16_t lastX = -1, lastY = -1;
    if (touchData.getPointCount() > 0) {
      const TouchPoint &pt = touchData.getPoint(0);
      if (pt.x != lastX || pt.y != lastY) {
        lastX = pt.x;
        lastY = pt.y;
        logInfo("[DIAG] Tactile: x=%d y=%d points=%d", pt.x, pt.y, touchData.getPointCount());
        gfx->fillScreen(BLACK);
        drawLabel("Test tactile -- touchez l'ecran", 40);
        gfx->fillCircle(pt.x, pt.y, 10, GREEN);
        char coords[32];
        std::snprintf(coords, sizeof(coords), "x=%d y=%d", pt.x, pt.y);
        drawLabel(coords, gfx->height() - 60);
      }
    }
  }

  delay(5);
}
