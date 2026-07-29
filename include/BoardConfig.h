#pragma once

// Definitions materielles pour la Waveshare ESP32-S3-Touch-AMOLED-1.75.
//
// Toutes les valeurs ci-dessous sont recopiees telles quelles depuis le
// fichier officiel du fabricant :
//   waveshareteam/ESP32-S3-Touch-AMOLED-1.75
//   examples/arduino/libraries/Mylibrary/pin_config.h  (Apache-2.0)
// Aucune broche n'a ete devinee : voir docs/HARDWARE.md pour les sources.

#define XPOWERS_CHIP_AXP2101

// --- Ecran AMOLED CO5300, bus QSPI ---
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
// Rotation du panneau (0-3, increments de 90 degres, semantique Arduino_GFX).
// Jamais teste sur ecran reel avant le 2026-07-23. Une premiere hypothese de
// rotation a 90 degres a ete emise puis invalidee (photo de diagnostic qui
// s'etait en fait reorientee toute seule, pas l'ecran) -- valeur laissee a 0
// en attendant une observation fiable. Un seul endroit a modifier des qu'un
// vrai probleme de rotation est confirme.
#define LCD_ROTATION 0

// --- Tactile CST9217, bus I2C partage ---
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40
// Calibration tactile (voir TouchDrvInterface::setMirrorXY/setSwapXY).
// Confirmee sur materiel reel le 2026-07-23 (DIAGNOSTIC_MATERIEL, teste
// point par point sur les 4 bords + centre) : X et Y inverses tous les deux
// par rapport aux coordonnees brutes du capteur. Un seul endroit a corriger
// si le tactile est un jour change.
#define TOUCH_MIRROR_X true
#define TOUCH_MIRROR_Y true
#define TOUCH_SWAP_XY false

// --- Codec audio ES8311 (non utilise en Phase 2, reserve) ---
#define I2S_MCK_IO 16
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA_PIN 46

// --- microSD (non utilise en Phase 2, reserve) ---
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41

static_assert(LCD_WIDTH == 466 && LCD_HEIGHT == 466,
              "CircleLayout (src/ui/Layout.h) suppose 466x466 -- a mettre a jour ensemble si la resolution change");

// La geometrie circulaire (CircleLayout::*) est partagee avec le simulateur
// PC : voir src/ui/Layout.h (aucune dependance materielle acceptee la-bas).
#include "ui/Layout.h"
