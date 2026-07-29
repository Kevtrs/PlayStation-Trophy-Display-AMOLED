#pragma once

// Definitions materielles pour la Waveshare ESP32-S3-Touch-LCD-7 (800x480).
//
// PAS ENCORE TESTE SUR MATERIEL REEL -- ce fichier prepare le bring-up pour
// quand la carte sera recue (voir tache "board-7inch-rgb-touch"). Toutes
// les valeurs ci-dessous sont recopiees telles quelles depuis le fichier
// officiel du fabricant (aucune broche devinee, meme discipline que
// BoardConfig.h pour l'AMOLED 1.75) :
//   waveshareteam/ESP32-S3-Touch-LCD-7
//   examples/Arduino/examples/10_lvgl_v9_demo/esp_panel_board_custom_conf.h
//   (Apache-2.0)
//
// Difference architecturale majeure avec l'AMOLED 1.75 (voir BoardConfig.h) :
// ce board pilote l'ecran en bus RGB parallele (controleur ST7262), pas en
// QSPI -- Arduino_GFX_Library seule ne suffit pas, le fabricant recommande
// sa propre bibliotheque ESP32_Display_Panel (voir examples/Arduino/
// libraries/ESP32_Display_Panel/ dans le depot ci-dessus). De plus, le
// reset ecran, le reset tactile et le retroeclairage passent tous par un
// expandeur IO I2C (CH422G) -- ce ne sont PAS des broches GPIO directes.

#define ESP_PANEL_BOARD_NAME "Waveshare:ESP32-S3-Touch-LCD-7"

// --- Resolution ---
#define LCD7_WIDTH 800
#define LCD7_HEIGHT 480

// --- Ecran ST7262, bus RGB parallele (pas QSPI) ---
#define LCD7_CONTROLLER_ST7262
#define LCD7_RGB_CLK_HZ (16 * 1000 * 1000)
#define LCD7_RGB_HPW 4
#define LCD7_RGB_HBP 8
#define LCD7_RGB_HFP 8
#define LCD7_RGB_VPW 4
#define LCD7_RGB_VBP 8
#define LCD7_RGB_VFP 8
#define LCD7_RGB_PCLK_ACTIVE_NEG 1  // 0 = front montant, 1 = front descendant
#define LCD7_RGB_DATA_WIDTH 16      // RGB565, 16 lignes de donnees

#define LCD7_RGB_IO_HSYNC 46
#define LCD7_RGB_IO_VSYNC 3
#define LCD7_RGB_IO_DE 5
#define LCD7_RGB_IO_PCLK 7
#define LCD7_RGB_IO_DISP -1  // non utilise

// Mapping des 16 lignes de donnees RGB565 (D0-D15 -> B0-4/G0-5/R0-4, voir
// tableau du fichier source officiel pour la correspondance exacte).
#define LCD7_RGB_IO_DATA0 14
#define LCD7_RGB_IO_DATA1 38
#define LCD7_RGB_IO_DATA2 18
#define LCD7_RGB_IO_DATA3 17
#define LCD7_RGB_IO_DATA4 10
#define LCD7_RGB_IO_DATA5 39
#define LCD7_RGB_IO_DATA6 0
#define LCD7_RGB_IO_DATA7 45
#define LCD7_RGB_IO_DATA8 48
#define LCD7_RGB_IO_DATA9 47
#define LCD7_RGB_IO_DATA10 21
#define LCD7_RGB_IO_DATA11 1
#define LCD7_RGB_IO_DATA12 2
#define LCD7_RGB_IO_DATA13 42
#define LCD7_RGB_IO_DATA14 41
#define LCD7_RGB_IO_DATA15 40

// Reset ecran : PAS une broche GPIO directe -- bit 3 de l'expandeur CH422G
// (voir ESP_PANEL_BOARD_LCD_PRE_BEGIN_FUNCTION dans le fichier source
// officiel : digitalWrite(3, 0) puis attente puis digitalWrite(3, 1)).
#define LCD7_RST_VIA_EXPANDER_BIT 3

// --- Tactile GT911, bus I2C partage avec l'expandeur ---
#define LCD7_TOUCH_CONTROLLER_GT911
#define LCD7_TOUCH_I2C_SCL 9
#define LCD7_TOUCH_I2C_SDA 8
#define LCD7_TOUCH_I2C_CLK_HZ (400 * 1000)
#define LCD7_TOUCH_I2C_ADDRESS 0  // 0 = adresse par defaut (GT911 : 0x5D ou 0x14)
#define LCD7_TOUCH_INT_IO 4       // broche GPIO native (pas via expandeur)
#define LCD7_TOUCH_INT_LEVEL 0    // actif a l'etat bas
// Reset tactile : PAS une broche GPIO directe -- bit 1 de l'expandeur
// CH422G (meme mecanisme que le reset ecran ci-dessus).
#define LCD7_TOUCH_RST_VIA_EXPANDER_BIT 1

// Calibration tactile (SwapXY/MirrorX/MirrorY) : jamais testee, valeurs par
// defaut du fabricant (aucune rotation/miroir) en attendant une
// verification sur materiel reel, comme pour TOUCH_MIRROR_X/Y sur
// BoardConfig.h -- un seul endroit a corriger le jour ou c'est confirme.
#define LCD7_TOUCH_SWAP_XY false
#define LCD7_TOUCH_MIRROR_X false
#define LCD7_TOUCH_MIRROR_Y false

// --- Expandeur IO CH422G (I2C, meme bus que le tactile) ---
// Pilote le reset ecran, le reset tactile et le retroeclairage -- aucun de
// ces trois n'est un GPIO ESP32 direct sur ce board.
#define LCD7_EXPANDER_CHIP_CH422G
#define LCD7_EXPANDER_I2C_ADDRESS 0x20
#define LCD7_EXPANDER_I2C_CLK_HZ (400 * 1000)

// --- Retroeclairage : marche/arret uniquement via l'expandeur (pas de PWM,
// donc pas de gradation continue comme sur l'AMOLED 1.75 -- a confirmer
// une fois le materiel en main si une gradation logicielle est possible
// autrement, sinon IBrightnessBackend restera "tout ou rien" sur ce board).
#define LCD7_BACKLIGHT_VIA_EXPANDER
#define LCD7_BACKLIGHT_ON_LEVEL 1

static_assert(LCD7_WIDTH == 800 && LCD7_HEIGHT == 480,
              "WideLayout (src/ui/layout_wide.hpp) suppose 800x480 -- a mettre a jour ensemble si la resolution change");

// La bibliotheque recommandee par le fabricant est ESP32_Display_Panel
// (esp-arduino-libs/ESP32_Display_Panel sur GitHub, Apache-2.0), qui gere
// elle-meme le bus RGB, l'expandeur CH422G et le tactile GT911 -- pas
// Arduino_GFX_Library seule (insuffisante pour un bus RGB + expandeur sur
// ce board). A integrer via platformio.ini (nouvel environnement dedie,
// jamais mele a [env:waveshare-amoled-175]) une fois le materiel recu et
// ce driver reellement teste -- rien de plus n'est ecrit dans ce fichier
// tant que ce n'est pas verifie sur l'appareil physique.
