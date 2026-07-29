# Licences des assets graphiques

## Images (trophees, medailles, halos, icones, texture de fond)

**Toutes generees par ce projet**, procéduralement, via
[`tools/asset_pipeline/generate_assets.py`](../tools/asset_pipeline/generate_assets.py)
(Python + Pillow + NumPy) -- aucune image, texture, icone ou illustration
tierce n'est utilisee. Aucune question de licence externe ne se pose : ce
sont des creations originales de ce depot, sous la meme licence que le reste
du code du projet.

Liste des assets generes (voir `src/ui/assets/*.c` pour les donnees LVGL
compilees, `tools/asset_pipeline/png_src/*.png` pour la previsualisation) :

| Asset | Usage |
|---|---|
| `trophy_hero` | Trophee central illustre -- ecran Welcome |
| `glow_hero`, `glow_ring`, `glow_small` | Halos radiaux bleu->violet (profondeur) |
| `medal_platinum`, `medal_gold`, `medal_silver`, `medal_bronze` | Medailles illustrees -- ecran Trophees, badges Dashboard |
| `icon_controller`, `icon_rank`, `icon_clock`, `icon_target` | Icones -- ecran Statistiques |
| `icon_trophy_small`, `icon_platinum_small` | Petites icones -- badges Dashboard |
| `bg_pattern` | Texture de fond hexagonale tres attenuee |

Pour regenerer ou modifier ces assets (palette, formes, tailles) :

```bash
python tools/asset_pipeline/generate_assets.py
```

Le script ecrit directement dans `src/ui/assets/*.c` (format LVGL
`lv_img_dsc_t`, `LV_IMG_CF_TRUE_COLOR_ALPHA`, coherent avec
`LV_COLOR_DEPTH=16` de `include/lv_conf.h`) -- compiles a la fois par le
firmware ESP32-S3 et le simulateur PC.

## Polices

**Montserrat** (tailles 14/16/20/24/32/48), fournie et compilee directement
par LVGL (`third_party/lvgl/src/font/lv_font_montserrat_*.c` pour le
simulateur ; equivalent via la dependance `lvgl` du firmware). Licence
**SIL Open Font License 1.1** (voir
`simulator/third_party/lvgl/src/font/` et le depot officiel
[lvgl/lvgl](https://github.com/lvgl/lvgl), lui-meme sous licence MIT pour le
code, OFL pour les polices Montserrat embarquees). Aucune police
supplementaire n'a ete ajoutee -- **ARIAL.TTF** (present dans l'ancien
firmware d'origine, police Monotype non libre) n'est repris nulle part dans
ce projet, conformement a `AUDIT.md`.

## Symboles d'interface (icones fleche, wifi, reglages, ok, warning, refresh)

Glyphes integres nativement a LVGL (police d'icones FontAwesome-like fournie
par le projet LVGL, `lv_symbol_def.h`), licence MIT (meme licence que LVGL).
Aucun logo Sony/PlayStation n'est utilise ni reproduit, conformement a la
contrainte du cahier des charges.
