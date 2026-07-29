#ifndef LV_CONF_H
#define LV_CONF_H

/* Migration LVGL 8.3.11 -> 9.4.0 (voir HANDOFF_PROGRESS.md, branche
 * integration/final-lvgl-ui). Structure calquee sur le lv_conf.h du design
 * final retenu (deja valide contre un vrai LVGL 9.4.0), valeurs de ce projet
 * conservees a l'identique la ou elles existaient deja en v8 (LV_MEM_SIZE,
 * police par defaut, periode de rafraichissement) pour ne changer que la
 * version, pas le comportement. Etape "socle" uniquement : les fontes/theme
 * personnalises du design final seront ajoutes a l'etape d'integration des
 * ecrans, pas ici. Mode LV_CONF_INCLUDE_SIMPLE (voir platformio.ini et
 * simulator/CMakeLists.txt) : tout ce qui n'est pas defini ici reprend le
 * defaut de lv_conf_internal.h.
 *
 * PARTAGE entre le firmware ESP32-S3 (PlatformIO) et le simulateur PC.
 */

#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0

/* Un pool statique fixe (LV_STDLIB_BUILTIN) de 160 Ko (heritage de l'ancienne
 * config LVGL 8) provoque un blocage reel de lv_timer_handler() en LVGL 9.4
 * des qu'un ecran anime plusieurs styles transform_* simultanement (ecran
 * Sync : rotation + pulsation) -- le rendu transforme de LVGL 9 alloue des
 * buffers de calque supplementaires que le moteur de rotation/mise a
 * l'echelle de LVGL 8 n'avait jamais besoin de demander. Bisecte le
 * 2026-07-22 (voir HANDOFF_PROGRESS.md) : le blocage disparait entierement a
 * partir de 512 Ko, valeur qui correspond exactement au lv_conf.h du design
 * final source (deja valide en conditions reelles). Sur le simulateur
 * (RAM desktop abondante) ce pool statique de 512 Ko passe sans probleme.
 * Sur le firmware ESP32-S3, en revanche, un bloc statique unique de cette
 * taille deborde la DRAM interne (testee : depassement de 287 488 octets,
 * voir HANDOFF_PROGRESS.md) -- ce microcontroleur dispose de 8 Mo de PSRAM
 * (voir platformio.ini : BOARD_HAS_PSRAM, qio_opi) que l'allocateur malloc()
 * standard d'Arduino-ESP32 sait deja utiliser automatiquement des que la RAM
 * interne est insuffisante. On bascule donc uniquement le firmware sur
 * LV_STDLIB_CLIB (malloc reel, pas de reservation statique fixe) et on
 * garde LV_STDLIB_BUILTIN + 512 Ko pour le simulateur (deja valide,
 * comportement inchange). */
#ifdef ARDUINO
#define LV_USE_STDLIB_MALLOC LV_STDLIB_CLIB
#else
#define LV_USE_STDLIB_MALLOC LV_STDLIB_BUILTIN
#endif
#define LV_USE_STDLIB_STRING LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF LV_STDLIB_BUILTIN

#if LV_USE_STDLIB_MALLOC == LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (512U * 1024U)
#endif

#define LV_USE_OS LV_OS_NONE

#define LV_DEF_REFR_PERIOD 30
#define LV_DPI_DEF 130

#define LV_USE_LOG 0

#define LV_USE_ASSERT_NULL 1
#define LV_USE_ASSERT_MALLOC 1
#define LV_USE_ASSERT_STYLE 0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ 0

#define LV_USE_DRAW_SW 1
#define LV_DRAW_SW_SUPPORT_RGB565 1
#define LV_DRAW_SW_SUPPORT_RGB888 0
#define LV_DRAW_SW_SUPPORT_ARGB8888 1

/* Widgets requis par l'ecran de validation minimal de cette etape --
 * complete a l'etape d'integration ecrans avec la liste exacte du design
 * retenu (voir HANDOFF_DESIGN.md : arc, bar, button, image, label, line,
 * slider, + roller/switch/dropdown deja utilises par simulator/DebugPanel). */
#define LV_USE_ARC 1
#define LV_USE_BAR 1
#define LV_USE_BUTTON 1
#define LV_USE_BUTTONMATRIX 1
#define LV_USE_CANVAS 1
#define LV_USE_CHECKBOX 1
#define LV_USE_DROPDOWN 1
#define LV_USE_IMAGE 1
#define LV_USE_LABEL 1
#define LV_USE_LINE 1
#define LV_USE_ROLLER 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1
#define LV_USE_SCALE 1

#define LV_USE_SNAPSHOT 0

/* Montserrat : uniquement pour simulator/src/DebugPanel.cpp (fenetre de
 * debug separee, jamais compilee dans le firmware -- voir
 * simulator/CMakeLists.txt). Non reference par le design produit, qui
 * utilise exclusivement les fontes TD custom ci-dessous ; le linker ne
 * garde que ce qui est reellement appele. */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_48 1

/* Fontes TD (design final) : Montserrat Medium + accents francais,
 * voir src/assets/fonts/. Requises par src/screens/screens.cpp et
 * src/widgets/widgets.cpp. */
#define LV_FONT_CUSTOM_DECLARE \
    LV_FONT_DECLARE(td_font_10) \
    LV_FONT_DECLARE(td_font_12) \
    LV_FONT_DECLARE(td_font_14) \
    LV_FONT_DECLARE(td_font_16) \
    LV_FONT_DECLARE(td_font_18) \
    LV_FONT_DECLARE(td_font_20) \
    LV_FONT_DECLARE(td_font_22) \
    LV_FONT_DECLARE(td_font_24) \
    LV_FONT_DECLARE(td_font_28) \
    LV_FONT_DECLARE(td_font_48)
#define LV_FONT_DEFAULT &td_font_16

#define LV_USE_THEME_DEFAULT 1
#define LV_USE_THEME_SIMPLE 0
#define LV_USE_THEME_MONO 0

#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_MUSIC 0

#endif
