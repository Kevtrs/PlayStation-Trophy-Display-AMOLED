// Phase 3 -- Interface LVGL en mode demo, partagee avec le simulateur PC
// (simulator/). Seule cette couche (init ecran/tactile/LVGL) est specifique
// a l'ESP32 -- toute la logique d'ecrans vit dans src/ui/.
//
// Sequence d'init LVGL/QSPI/tactile reprise de
// waveshareteam/ESP32-S3-Touch-AMOLED-1.75/examples/arduino/06_LVGL_Widgets.

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TouchDrvCST.hpp>
#include <Wire.h>
#include <lvgl.h>

#include "BoardConfig.h"
#include "app/AppController.h"
#include "board/Co5300BrightnessBackend.h"
#include "config/ConfigManager.h"
#include "data/DemoDataProvider.h"
#include "data/PocketPsnProvider.h"
#include "data/ProviderFactory.h"
#include "network/PocketPsnHttpClient.h"
#include "network/WiFiManager.h"
#include "storage/NvsPersistentStore.h"
#include "ui/RoundUiBridge.h"
#include "ui/GestureRecognizer.h"
#include "utils/Logger.h"
#include "web/CaptivePortalServer.h"

namespace {

constexpr uint32_t kLvglTickPeriodMs = 2;

Arduino_DataBus *bus = new Arduino_ESP32QSPI(
    LCD_CS, LCD_SCLK, LCD_SDIO0, LCD_SDIO1, LCD_SDIO2, LCD_SDIO3);

Arduino_CO5300 *gfx = new Arduino_CO5300(
    bus, LCD_RESET, LCD_ROTATION, LCD_WIDTH, LCD_HEIGHT, 6, 0, 0, 0);

TouchDrvCST92xx touch;
volatile bool touchInterruptPending = false;

DemoDataProvider demoProvider;
// Design final, branche sur les vraies donnees via RoundUiBridge (voir
// src/ui/RoundUiBridge.h) -- remplace l'ecran minimal de validation de
// l'etape socle de la migration LVGL 9 (voir HANDOFF_PROGRESS.md).
RoundUiBridge roundUi;
GestureRecognizer gestureRecognizer;
Co5300BrightnessBackend brightnessBackend(gfx);
NvsPersistentStore persistentStore;
WiFiManager wifiManager;
PocketPsnHttpClient pocketPsnHttpClient;

// appController/captivePortalServer/pocketPsnProvider ne peuvent pas etre
// des objets globaux construits directement : le choix du provider actif
// (ProviderFactory::shouldUsePocketPsn(), voir AUDIT.md section 0ter)
// depend de la configuration persistante, qui n'est chargeable qu'au debut
// de setup() (pas garanti plus tot dans l'init Arduino). Alloues donc dans
// setup(), jamais liberes -- duree de vie = programme entier, meme
// semantique qu'un objet global.
PocketPsnProvider* pocketPsnProvider = nullptr;
AppController* appController = nullptr;
CaptivePortalServer* captivePortalServer = nullptr;

void IRAM_ATTR onTouchInterrupt() { touchInterruptPending = true; }

void logDiagnostics() {
  Logger::info("[BOOT] System");
  Logger::info("=== PlayStation Trophy Display AMOLED -- diagnostics de demarrage ===");
  Logger::info("Chip: %s rev%d, %d coeur(s), %d MHz", ESP.getChipModel(),
               ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Logger::info("Flash: %u octets (mode detecte: %d)", ESP.getFlashChipSize(),
               ESP.getFlashChipMode());
  if (psramFound()) {
    Logger::info("PSRAM: %u octets detectes, %u octets libres", ESP.getPsramSize(),
                 ESP.getFreePsram());
  } else {
    Logger::error("PSRAM introuvable -- verifier board_build.arduino.memory_type=qio_opi");
  }
  Logger::info("Heap libre: %u octets", ESP.getFreeHeap());
  esp_reset_reason_t reason = esp_reset_reason();
  Logger::info("Raison du dernier redemarrage: %d", static_cast<int>(reason));
}

bool initDisplay() {
  if (!gfx->begin()) {
    Logger::error("gfx->begin() a echoue -- verifier le cablage QSPI (CS=%d SCLK=%d)", LCD_CS,
                  LCD_SCLK);
    return false;
  }
  gfx->setBrightness(200);
  Logger::info("[BOOT] Display: CO5300 initialise (%dx%d)", gfx->width(), gfx->height());
  return true;
}

bool initTouch() {
  touch.setPins(TP_RESET, TP_INT);
  if (!touch.begin(Wire, CST92XX_SLAVE_ADDRESS, IIC_SDA, IIC_SCL)) {
    Logger::error("[BOOT] Touch: CST9217 introuvable sur I2C (SDA=%d SCL=%d) -- non bloquant, l'app demarre sans tactile",
                  IIC_SDA, IIC_SCL);
    return false;
  }
  touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT);
  touch.setSwapXY(TOUCH_SWAP_XY);
  touch.setMirrorXY(TOUCH_MIRROR_X, TOUCH_MIRROR_Y);
  Logger::info("[BOOT] Touch: %s detecte, %d point(s) supporte(s)", touch.getModelName(),
               touch.getSupportTouchPoint());
  pinMode(TP_INT, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(TP_INT), onTouchInterrupt, FALLING);
  return true;
}

void lvglFlushCb(lv_display_t *display, const lv_area_t *area, uint8_t *px_map) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;
  gfx->draw16bitRGBBitmap(area->x1, area->y1, reinterpret_cast<uint16_t *>(px_map), w, h);
  lv_display_flush_ready(display);
}

// Hisse au niveau fichier (pas static-local) : loop() en a aussi besoin pour
// alimenter gestureRecognizer, en plus de LVGL via ce callback.
int16_t lastTouchX = 0, lastTouchY = 0;
bool lastTouchPressed = false;

void lvglTouchpadReadCb(lv_indev_t * /*indev*/, lv_indev_data_t *data) {
  if (touchInterruptPending) {
    touchInterruptPending = false;
    const TouchPoints &touchData = touch.getTouchPoints();
    if (touchData.getPointCount() > 0) {
      const TouchPoint &pt = touchData.getPoint(0);
      lastTouchX = pt.x;
      lastTouchY = pt.y;
      lastTouchPressed = true;
    } else {
      lastTouchPressed = false;
    }
  }

  data->point.x = lastTouchX;
  data->point.y = lastTouchY;
  data->state = lastTouchPressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

void increaseLvglTick(void * /*arg*/) { lv_tick_inc(kLvglTickPeriodMs); }

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);  // laisse le temps au moniteur serie de se connecter

  logDiagnostics();

  Wire.begin(IIC_SDA, IIC_SCL);

  bool displayOk = initDisplay();
  bool touchOk = initTouch();

  Logger::info("Bring-up materiel. Ecran: %s. Tactile: %s.", displayOk ? "OK" : "ECHEC",
               touchOk ? "OK" : "ECHEC");

  if (!displayOk) {
    return;  // rien d'autre a faire sans ecran
  }

  lv_init();

  static lv_color_t *buf1 = static_cast<lv_color_t *>(
      heap_caps_malloc(CircleLayout::kScreenWidth * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA));
  static lv_color_t *buf2 = static_cast<lv_color_t *>(
      heap_caps_malloc(CircleLayout::kScreenWidth * 40 * sizeof(lv_color_t), MALLOC_CAP_DMA));

  lv_display_t *display = lv_display_create(CircleLayout::kScreenWidth, CircleLayout::kScreenHeight);
  lv_display_set_flush_cb(display, lvglFlushCb);
  lv_display_set_buffers(display, buf1, buf2, CircleLayout::kScreenWidth * 40 * sizeof(lv_color_t),
                         LV_DISPLAY_RENDER_MODE_PARTIAL);

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lvglTouchpadReadCb);

  const esp_timer_create_args_t lvglTickTimerArgs = {.callback = &increaseLvglTick, .name = "lvgl_tick"};
  esp_timer_handle_t lvglTickTimer = nullptr;
  esp_timer_create(&lvglTickTimerArgs, &lvglTickTimer);
  esp_timer_start_periodic(lvglTickTimer, kLvglTickPeriodMs * 1000);

  roundUi.begin();

  // Selection du provider actif au demarrage uniquement (voir
  // ProviderFactory.h et AUDIT.md section 0ter) : lecture anticipee de la
  // config, avant la construction d'AppController (qui rechargera la
  // config une seconde fois en interne via begin() -- leger cout accepte,
  // evite de restructurer la propriete de ConfigManager par AppController).
  ConfigManager earlyConfig(persistentStore);
  earlyConfig.load();
  TrophyDataProvider* activeProvider = &demoProvider;
  if (ProviderFactory::shouldUsePocketPsn(earlyConfig.settings())) {
    pocketPsnProvider = new PocketPsnProvider(
        earlyConfig.settings().psnUsername,
        ProviderFactory::effectiveApiKey(earlyConfig.settings()), pocketPsnHttpClient);
    activeProvider = pocketPsnProvider;
  }

  appController = new AppController(persistentStore, persistentStore, *activeProvider, brightnessBackend,
                                     wifiManager, roundUi);
  captivePortalServer = new CaptivePortalServer(*appController);

  appController->begin();

  // Veille desactivee par defaut : ecran d'affichage permanent (retour
  // utilisateur du 2026-07-29 -- "il faut juste enlever la veille sur le
  // amoled"). Applique a chaque demarrage (idempotent : reste a 0 en
  // fonctionnement normal), meme mecanisme que main_7inch.cpp -- si un
  // reglage de veille redevient souhaitable, le reactiver ici plutot que
  // via le portail captif (qui serait ecrase au prochain redemarrage).
  std::string patchError;
  appController->applyConfigPatch(R"({"sleepTimeoutSeconds":0})", patchError);

  captivePortalServer->begin();
  Logger::info("Squelette fonctionnel demarre (mode %s, Wi-Fi + portail captif reels branches).",
               activeProvider == &demoProvider ? "demo" : "Pocket PSN");
}

void loop() {
  uint32_t nowMillis = millis();
  appController->tick(nowMillis);
  roundUi.tick(nowMillis);
  captivePortalServer->poll();

  // Capture AVANT notifyTouchActivity() (qui reveille l'ecran plus bas) :
  // un geste survenant pendant que l'ecran est attenue/en veille ne doit
  // jamais AUSSI declencher une action (sync, navigation...) -- seulement
  // reveiller l'ecran. Sans cette garde, le tout premier tap apres une
  // veille relancait systematiquement une synchronisation (ou naviguait),
  // que l'utilisateur ne voit meme pas venir puisque l'ecran vient tout
  // juste de se rallumer -- bug reel signale le 2026-07-28.
  const bool wasFullyAwake = appController->isDisplayAwake();

  GestureRecognizer::Gesture gesture =
      gestureRecognizer.update(lastTouchX, lastTouchY, lastTouchPressed, nowMillis);

  if (touchInterruptPending) {
    appController->notifyTouchActivity(nowMillis);
  }

  if (wasFullyAwake) {
    switch (gesture) {
      case GestureRecognizer::Gesture::kSwipeLeft:
        roundUi.swipeLeft();
        break;
      case GestureRecognizer::Gesture::kSwipeRight:
        roundUi.swipeRight();
        break;
      case GestureRecognizer::Gesture::kTap:
        // Sur Dashboard, un tap declenche une vraie synchronisation manuelle
        // (voir RoundUiBridge::onDashboard()) plutot que la simulation interne
        // au design (App::activate() -> simulate_sync(), qui ne touche jamais
        // au reseau reel).
        if (roundUi.onDashboard()) {
          appController->requestManualSync();
        } else {
          roundUi.activate();
        }
        break;
      case GestureRecognizer::Gesture::kLongPress:
        roundUi.longPress();
        break;
      case GestureRecognizer::Gesture::kNone:
        break;
    }
  }
  lv_timer_handler();
  delay(5);
}
