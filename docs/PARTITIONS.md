# Partitions Flash (ESP32-S3, 16 Mo)

## Etat avant cette revision (2026-07-15)

`partitions.csv` declarait deja une table pour 16 Mo (2 slots OTA de 3 Mo
+ LittleFS ~9.94 Mo), mais l'application seule (firmware + LVGL + Wi-Fi,
**sans serveur web/portail captif**) occupait deja **85.2 % de son slot de
3 Mo** (2 678 701 / 3 145 728 octets, voir `HANDOFF_PROGRESS.md`). Ajouter
un serveur DNS + HTTP + les gestionnaires de routes + une future UI web
n'aurait laisse quasiment aucune marge.

## Verification : PlatformIO detecte-t-il reellement 16 Mo ?

Trois preuves independantes, toutes obtenues par compilation reelle (pas
de simple lecture de config) :

1. **Configuration effective** (`pio run -v`, ligne d'environnement) :
   `board_upload.flash_size: 16MB` est bien repris tel quel dans
   l'environnement de build resolu (pas seulement dans `platformio.ini`,
   confirme par PlatformIO apres fusion de tous les overrides).
2. **Validation de la table de partitions** : `gen_esp32part.py` (appele en
   interne par PlatformIO au moment de generer `partitions.bin`) rejette
   avec une erreur explicite toute table dont la taille totale depasse la
   taille de Flash configuree. La table actuelle totalise exactement
   `0x1000000` (16 777 216 octets = 16 Mo) et la compilation reussit sans
   avertissement -- si PlatformIO avait cru la Flash limitee a 8 Mo (taille
   par defaut de la carte `esp32-s3-devkitc-1` telle que listee dans son
   manifest), la generation de la table aurait echoue.
3. **Taille de l'image finale** : `esptool.py --chip esp32s3 elf2image`
   (etape "Creating esp32s3 image...") prend en parametre la taille de
   Flash resolue par PlatformIO (`board_upload.flash_size`) pour ecrire
   l'entete du binaire ; aucune erreur de depassement n'est levee pour une
   image dont les partitions vont jusqu'a 16 Mo.

Conclusion : **PlatformIO utilise bien 16 Mo**, malgre la ligne
`CONFIGURATION:` de `pio run` qui affiche encore "Espressif
ESP32-S3-DevKitC-1-N8 (8 MB QD, No PSRAM)" -- ce texte est uniquement le
**nom marketing fige du manifest de carte** (le board de reference N8 a 8
Mo de Flash), pas la configuration Flash reellement utilisee pour cette
compilation (ecrasee par `board_upload.flash_size = 16MB` et
`board_build.arduino.memory_type = qio_opi` dans `platformio.ini`).

## Nouvelle table (retenue)

```
# Name,     Type, SubType,  Offset,   Size
nvs,        data, nvs,      0x9000,   0x5000,     # 20 Ko -- reserve interne Arduino/ESP-IDF (Preferences, calibration Wi-Fi...), jamais utilisee directement par notre code (NvsPersistentStore utilise LittleFS malgre son nom, voir docs/ARCHITECTURE.md)
otadata,    data, ota,      0xe000,   0x2000,     # 8 Ko -- suivi du slot actif, requis des que 2 slots OTA existent
app0,       app,  ota_0,    0x10000,  0x400000,   # 4 Mo -- code applicatif (firmware + LVGL + Wi-Fi + portail captif)
app1,       app,  ota_1,    0x410000, 0x400000,   # 4 Mo -- second slot OTA (image de secours pendant une mise a jour)
littlefs,   data, spiffs,   0x810000, 0x7F0000,   # ~7.94 Mo -- config, cache de trophees, assets web (jamais compiles dans le binaire applicatif)
```

Total : `0x9000 + 0x5000 + 0x2000 + 0x400000 + 0x400000 + 0x7F0000 =
0x1000000` = exactement 16 Mo.

Avec cette table, l'usage actuel (firmware + LVGL + Wi-Fi, avant serveur
web) tombe a **2 678 701 / 4 194 304 octets = 63.9 %** (contre 85.2 %
avant), une marge bien plus confortable pour le portail captif et le
serveur HTTP a venir dans cette meme passe, plus la future UI web (dont
le code HTML/CSS/JS ira dans LittleFS, pas dans le binaire).

## Compromis avec/sans OTA

| | **Avec OTA (retenu)** | **Sans OTA (facteur unique)** |
|---|---|---|
| Partitions app | 2 slots de 4 Mo (`ota_0`/`ota_1`) + `otadata` (8 Ko) | 1 slot unique de type `app`/`factory`, pas d'`otadata` |
| Espace pour LittleFS | ~7.94 Mo | ~11.96 Mo (recupere le 2e slot + `otadata`) |
| Mise a jour a distance | Possible plus tard (tache OTA dediee, voir mandat original) : flash via le portail web, image verifiee avant redemarrage, roll-back possible si le nouveau firmware ne demarre pas | **Impossible sans USB** : toute mise a jour necessite de reconnecter l'appareil a un PC |
| Risque en cas d'echec de flash | Faible : l'ancien slot reste intact et bootable tant que le nouveau n'est pas valide | N/A (pas de flash a distance) |
| Cout Flash | Code applicatif duplique deux fois (structurellement, meme si un seul slot est actif a la fois) | Aucune duplication |
| Complexite | Nécessite `otadata` + logique de bascule de slot (deja standard sur ESP32-Arduino, aucun code custom requis pour le simple fait d'avoir la table) | Plus simple, mais ferme la porte a l'OTA sans reflasher la table de partitions (donc sans repasser par USB) |

**Decision retenue : garder les 2 slots OTA.** Avec 16 Mo de Flash
disponibles, meme le scenario "avec OTA" laisse ~7.94 Mo a LittleFS -- très
largement suffisant pour la configuration, le cache de trophees et une UI
web complete (HTML/CSS/JS + polices + icones representent au plus
quelques Mo). Le cout reel de conserver l'option OTA est donc negligeable
ici, alors que la fermer definitivement (sans reflash USB complet plus
tard) serait couteux si le besoin de mise a jour a distance apparait apres
que l'appareil soit deja installe chez un utilisateur final. L'OTA
elle-meme (flux de mise a jour, verification de fichier, ecran de
progression) reste **non implementee** dans cette passe -- seule la table
de partitions la rend possible plus tard sans nouveau repartitionnement.

## Note pour la suite

`littlefs` doit etre monte via `LittleFS.begin()` (deja fait par
`NvsPersistentStore`) et peuple via `pio run -t uploadfs` pour toute
future UI web servie en fichiers statiques (`data/index.html`,
`data/app.js`, `data/styles.css`, voir mandat original) -- ces fichiers ne
doivent jamais etre compiles dans le binaire applicatif (`app0`/`app1`),
qui doit rester leger pour que les mises a jour OTA restent rapides.
