// Point d'entree firmware pour le board Waveshare ESP32-S3-Touch-LCD-7
// (800x480, RGB/GT911/CH422G -- voir include/BoardConfig7inch.h pour les
// broches documentees et include/esp_panel_board_custom_conf.h, vendorise
// tel quel depuis l'exemple officiel du fabricant, pour la config reelle
// utilisee par ESP32_Display_Panel). Sequence d'init RGB/expander/LVGL
// reprise de waveshareteam/ESP32-S3-Touch-LCD-7,
// examples/Arduino/examples/10_lvgl_v9_demo (vendorise dans
// src/board7inch/esp_lv_adapter_arduino.{h,cpp}).
//
// Design majoritairement passif (voir screens_wide.cpp / WideUiBridge.h) :
// le carrousel defile seul, mais le GT911 est aussi enregistre aupres de
// LVGL pour permettre un acces direct aux reglages depuis la roue crantee.
//
// MATERIEL JAMAIS TESTE avant ce soir (2026-07-28). Si l'ecran reste noir
// au premier flash, verifier au moniteur serie (115200 bauds) dans l'ordre :
//   1. "Board::init() a echoue" -> config invalide (esp_panel_board_custom_conf.h)
//   2. "Board::begin() a echoue" -> cablage I2C expander CH422G (SDA=8 SCL=9)
//      ou alimentation insuffisante
//   3. Init OK mais ecran noir -> retroeclairage (expander, IO2) ou reset
//      LCD (egalement pilote par l'expander, voir
//      ESP_PANEL_BOARD_LCD_PRE_BEGIN_FUNCTION dans la config vendorisee)
//   4. Ecran allume mais rien ne s'affiche -> cablage RGB HSYNC=46/VSYNC=3/
//      DE=5/PCLK=7 ou brochage donnees D0-D15 (voir BoardConfig7inch.h)

#include <Arduino.h>
#include <esp_display_panel.hpp>
#include <esp_err.h>
#include <lvgl.h>

#include <string>

#include "app/AppController.h"
#include "board7inch/PanelBrightnessBackend.h"
#include "board7inch/WideUiBridge.h"
#include "board7inch/esp_lv_adapter_arduino.h"
#include "config/ConfigManager.h"
#include "data/DemoDataProvider.h"
#include "drivers/bus/esp_panel_bus_i2c.hpp"
#include "drivers/touch/esp_panel_touch_gt911.hpp"
#include "data/PocketPsnProvider.h"
#include "data/ProviderFactory.h"
#include "network/PocketPsnHttpClient.h"
#include "network/WiFiManager.h"
#include "storage/NvsPersistentStore.h"
#include "theme/theme.hpp"
#include "utils/Logger.h"
#include "web/CaptivePortalServer.h"

using namespace esp_panel::drivers;
using namespace esp_panel::board;

namespace {

DemoDataProvider demoProvider;
WideUiBridge wideUi;
NvsPersistentStore persistentStore;
WiFiManager wifiManager;
PocketPsnHttpClient pocketPsnHttpClient;

// Meme raison qu'en main.cpp (round board) : le choix du provider actif
// depend de la config persistante, chargeable seulement en debut de
// setup() -- allocation dynamique, jamais liberee (duree de vie = programme
// entier).
PocketPsnProvider* pocketPsnProvider = nullptr;
AppController* appController = nullptr;
CaptivePortalServer* captivePortalServer = nullptr;
PanelBrightnessBackend* brightnessBackend = nullptr;
BusI2C* manualTouchBus = nullptr;
TouchGT911* manualTouch = nullptr;

void logDiagnostics() {
  Logger::info("=== PlayStation Trophy Display -- board 7\" (800x480 RGB) -- diagnostics de demarrage ===");
  Logger::info("Chip: %s rev%d, %d coeur(s), %d MHz", ESP.getChipModel(), ESP.getChipRevision(),
               ESP.getChipCores(), ESP.getCpuFreqMHz());
  Logger::info("Flash: %u octets (mode detecte: %d)", ESP.getFlashChipSize(), ESP.getFlashChipMode());
  if (psramFound()) {
    Logger::info("PSRAM: %u octets detectes, %u octets libres", ESP.getPsramSize(), ESP.getFreePsram());
  } else {
    Logger::error("PSRAM introuvable -- verifier board_build.arduino.memory_type=qio_opi");
  }
  Logger::info("Heap libre: %u octets", ESP.getFreeHeap());
}

Touch* createPollingGt911Touch(LCD* lcd) {
  if (lcd == nullptr) return nullptr;

  const uint8_t addresses[] = {
      ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS,
      ESP_LCD_TOUCH_IO_I2C_GT911_ADDRESS_BACKUP,
  };

  for (uint8_t address : addresses) {
    BusI2C::ControlPanelFullConfig ioConfig =
        ESP_LCD_TOUCH_IO_I2C_GT911_CONFIG_WITH_ADDR(address);
    auto* bus = new BusI2C(I2C_NUM_0, ioConfig);

    Touch::DeviceFullConfig deviceConfig = {};
    deviceConfig.x_max = static_cast<uint16_t>(lcd->getFrameWidth());
    deviceConfig.y_max = static_cast<uint16_t>(lcd->getFrameHeight());
    deviceConfig.rst_gpio_num = GPIO_NUM_NC;
    deviceConfig.int_gpio_num = GPIO_NUM_NC;
    deviceConfig.levels.reset = 0;
    deviceConfig.levels.interrupt = 0;
    deviceConfig.flags.swap_xy = 0;
    deviceConfig.flags.mirror_x = 0;
    deviceConfig.flags.mirror_y = 0;
    deviceConfig.process_coordinates = nullptr;
    deviceConfig.interrupt_callback = nullptr;
    deviceConfig.user_data = nullptr;
    deviceConfig.driver_data = nullptr;

    Touch::Config touchConfig = {};
    touchConfig.device = deviceConfig;

    auto* touch = new TouchGT911(bus, touchConfig);
    if (touch->begin()) {
      manualTouchBus = bus;
      manualTouch = touch;
      Logger::info("[BOOT] Touch: GT911 polling initialise sans IRQ (adresse 0x%02X)", address);
      return manualTouch;
    }

    Logger::warn("[BOOT] Touch: GT911 polling non detecte a l'adresse 0x%02X", address);
    delete touch;
    delete bus;
  }

  return nullptr;
}

}  // namespace

namespace {
// Trampoline entre l'ecran Reglages et la configuration persistante :
// meme chemin que POST /api/config, donc sauvegarde sur disque et
// services reconfigures sans redemarrage.
bool applyConfigFromUi(const char* json, void* owner) {
  auto* controller = static_cast<AppController*>(owner);
  if (controller == nullptr || json == nullptr) return false;
  std::string error;
  if (controller->applyConfigPatch(json, error)) return true;
  Logger::warn("[UI 7\"] Patch de configuration refuse : %s", error.c_str());
  return false;
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);  // laisse le temps au moniteur serie de se connecter

  logDiagnostics();

  Board* board = new Board();
  if ((board == nullptr) || !board->init()) {
    Logger::error("[BOOT] Board::init() a echoue -- verifier esp_panel_board_custom_conf.h");
    while (true) delay(1000);
  }

  const esp_lv_adapter_rotation_t rotation = ESP_LV_ADAPTER_ROTATE_0;
  const esp_lv_adapter_tear_avoid_mode_t tearMode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_DEFAULT_RGB;
  const uint8_t frameBufferCount = esp_lv_adapter_get_required_frame_buffer_count(tearMode, rotation);

  LCD* lcd = board->getLCD();
  if (lcd == nullptr) {
    Logger::error("[BOOT] LCD introuvable sur le board -- verifier ESP_PANEL_BOARD_USE_LCD");
    while (true) delay(1000);
  }
  auto* lcdBus = lcd->getBus();
  if (lcdBus->getBasicAttributes().type == ESP_PANEL_BUS_TYPE_RGB) {
    lcd->configFrameBufferNumber(frameBufferCount);
    static_cast<BusRGB*>(lcdBus)->configRGB_BounceBufferSize(lcd->getFrameWidth() * 10);
  }

  if (!board->begin()) {
    Logger::error(
        "[BOOT] Board::begin() a echoue -- verifier alimentation/cablage I2C (expander CH422G, SDA=8 SCL=9)");
    while (true) delay(1000);
  }
  // "Waveshare:ESP32-S3-Touch-LCD-7" en dur (pas ESP_PANEL_BOARD_NAME) :
  // cette macro n'est visible que dans les .cpp internes a la librairie,
  // pas depuis un .cpp consommateur -- erreur de compilation reelle trouvee
  // le 2026-07-28.
  Logger::info("[BOOT] Display: Waveshare:ESP32-S3-Touch-LCD-7 initialise (%dx%d)", lcd->getFrameWidth(),
               lcd->getFrameHeight());

  brightnessBackend = new PanelBrightnessBackend(board->getBacklight());

  esp_lv_adapter_config_t adapterConfig = ESP_LV_ADAPTER_DEFAULT_CONFIG();
  adapterConfig.task_stack_size = 12 * 1024;
  adapterConfig.task_priority = 2;
  adapterConfig.task_core_id = ARDUINO_RUNNING_CORE;
  ESP_ERROR_CHECK(esp_lv_adapter_init(&adapterConfig));

  esp_lv_adapter_display_config_t dispConfig = ESP_LV_ADAPTER_DISPLAY_RGB_DEFAULT_CONFIG(
      lcd, static_cast<uint16_t>(lcd->getFrameWidth()), static_cast<uint16_t>(lcd->getFrameHeight()), rotation);
  dispConfig.profile.use_psram = true;
  lv_display_t* disp = esp_lv_adapter_register_display(&dispConfig);
  if (disp == nullptr) {
    Logger::error("[BOOT] esp_lv_adapter_register_display() a echoue");
    while (true) delay(1000);
  }

  Touch* touch = board->getTouch();
  if (touch == nullptr) {
    touch = createPollingGt911Touch(lcd);
  }
  if (touch != nullptr) {
    esp_lv_adapter_touch_config_t touchConfig = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch);
    if (esp_lv_adapter_register_touch(&touchConfig) == nullptr) {
      Logger::error("[BOOT] Touch: GT911 present mais enregistrement LVGL impossible");
    } else {
      Logger::info("[BOOT] Touch: GT911 enregistre dans LVGL");
    }
  } else {
    Logger::warn("[BOOT] Touch: GT911 indisponible, l'ecran reste en defilement passif");
  }

  ESP_ERROR_CHECK(esp_lv_adapter_start());
  Logger::info("[BOOT] LVGL demarre (tache dediee, %dx%d)", lcd->getFrameWidth(), lcd->getFrameHeight());

  // --- Logique applicative (AppController/TrophyRepository/SyncService),
  // partagee telle quelle avec le board rond (voir src/app/, src/data/,
  // src/services/) -- seule la couche ecran change (WideUiBridge). ---
  ConfigManager earlyConfig(persistentStore);
  earlyConfig.load();
  TrophyDataProvider* activeProvider = &demoProvider;
  if (ProviderFactory::shouldUsePocketPsn(earlyConfig.settings())) {
    pocketPsnProvider = new PocketPsnProvider(earlyConfig.settings().psnUsername,
                                               ProviderFactory::effectiveApiKey(earlyConfig.settings()),
                                               pocketPsnHttpClient);
    activeProvider = pocketPsnProvider;
  }

  appController = new AppController(persistentStore, persistentStore, *activeProvider, *brightnessBackend,
                                     wifiManager, wideUi);
  captivePortalServer = new CaptivePortalServer(*appController);
  // Chemin retour de l'ecran Reglages vers la configuration persistante,
  // le meme que celui du portail web. Sans lui, les lignes de reglages
  // gardent leur retour visuel a l'appui mais ne changent rien.
  wideUi.setConfigPatchHandler(&applyConfigFromUi, appController);

  esp_lv_adapter_lock(-1);
  // MANQUAIT ICI (bug reel trouve au tout premier flash materiel, retour
  // utilisateur du 2026-07-29 -- "le fond est blanc") : sans cet appel,
  // style_screen/style_label/etc. (src/theme/theme.cpp) restent des
  // lv_style_t vides (jamais passes a lv_style_init()), donc wide_screen_
  // root() n'applique en pratique aucun style -- le fond retombe sur le
  // blanc par defaut de LVGL en l'absence de tout theme (LV_USE_THEME_
  // DEFAULT ne s'auto-applique pas, voir lv_conf.h). Les deux simulateurs
  // (main_wide.cpp/main_wide_live.cpp) l'appellent deja -- c'est pour ça
  // que le bug n'etait jamais apparu avant le premier flash reel : le
  // simulateur ne pouvait pas le reveler puisqu'il fait cet appel.
  trophy::init_theme();
  appController->begin();

  // Ecran d'ambiance toujours allume : aucune interaction tactile ne peut
  // jamais reveiller ce board (voir commentaire d'en-tete), donc la veille
  // par defaut (PowerManager, 180s d'inactivite) l'assombrirait
  // definitivement peu apres le demarrage. Applique a chaque demarrage
  // (idempotent : reste a 0 en fonctionnement normal) -- si un jour un
  // reglage de veille redevient souhaitable pour ce board specifique, le
  // reactiver ici plutot que via le portail captif (qui serait ecrase au
  // prochain redemarrage).
  std::string patchError;
  appController->applyConfigPatch(R"({"sleepTimeoutSeconds":0})", patchError);

  wideUi.begin();
  esp_lv_adapter_unlock();

  captivePortalServer->begin();
  Logger::info("[BOOT] Pret (mode %s, Wi-Fi + portail captif reels branches).",
               activeProvider == &demoProvider ? "demo" : "Pocket PSN");
}

void loop() {
  uint32_t nowMillis = millis();

  esp_lv_adapter_lock(-1);
  appController->tick(nowMillis);
  wideUi.tick(nowMillis);
  esp_lv_adapter_unlock();

  captivePortalServer->poll();
  delay(5);
}
