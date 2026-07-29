# HANDOFF_FUNCTIONAL_BASE.md

Document de passation de la base fonctionnelle du projet **PlayStation Trophy
Display AMOLED**. Genere le 2026-07-15, a partir de l'etat reel du depot au
commit `52ca64f` (branche `master`). Toutes les affirmations ci-dessous ont
ete verifiees par lecture directe du code et/ou execution reelle des builds
dans cette session -- rien n'est suppose.

---

## 1. Resume fonctionnel

Ce projet est un **portage + refonte** du firmware ESP32-C3/SSD1306
`tomtechie/Playstation-Trophies-ESP-Display` vers la carte
**Waveshare ESP32-S3-Touch-AMOLED-1.75** (466x466, tactile, PSRAM), avec une
interface graphique LVGL premium et un simulateur PC partageant le meme code
d'interface.

**Ce qui existe et fonctionne reellement (verifie) :**
- Bring-up materiel ESP32-S3 (ecran CO5300/QSPI, tactile CST9217/I2C),
  compile reellement.
- Interface LVGL complete (6 ecrans) avec navigation tactile, rotation
  automatique, animations -- **fonctionnelle dans le simulateur PC**,
  compilee (non testee) sur firmware.
- Simulateur PC (SDL2 + LVGL, code partage a 100% avec le firmware pour
  `src/ui/` et `src/data/DemoDataProvider`), avec panneau de debug et export
  automatique de captures PNG.
- Fournisseur de donnees de demonstration (`DemoDataProvider`), fonctionnel.

**Ce qui est simule uniquement (aucune preuve materielle) :**
- Ecran, tactile, PSRAM : le code compile mais **aucun test sur carte
  physique** n'a jamais ete effectue (aucune carte disponible dans
  l'environnement de developpement).

**Ce qui n'est pas developpe (absent, pas juste "a finir") :**
- Wi-Fi (station et point d'acces), portail captif, serveur web, toute
  interface de configuration reseau.
- Configuration persistante (LittleFS/NVS), cache hors ligne.
- RTC/NTP, gestion reelle de la luminosite/veille (le champ existe dans le
  modele d'etat mais n'est jamais applique au materiel).
- OTA.
- Pocket PSN fonctionnel (l'interface et l'endpoint sont identifies, mais le
  fournisseur n'a jamais recu de vraie reponse -- voir section 8).
- Tests automatises (`test/` existe comme dossier vide, aucun fichier).

---

## 2. Arborescence complete et role des dossiers

```
PlayStation-Trophy-Display-AMOLED/
├── AUDIT.md                     Audit du depot d'origine (Pocket PSN, historique)
├── PROJECT_STATUS.md            Etat d'avancement phase par phase
├── HANDOFF_FUNCTIONAL_BASE.md   Ce document
├── platformio.ini               Configuration firmware (env, libs, memoire)
├── partitions.csv               Table de partitions flash 16 Mo (OTA x2 + LittleFS -- LittleFS jamais utilisee dans le code actuel)
│
├── include/
│   ├── BoardConfig.h             Broches materielles ESP32-S3 (ecran/tactile/audio/SD), copiees du BSP officiel Waveshare
│   └── lv_conf.h                 Configuration LVGL, PARTAGEE par le firmware et le simulateur
│
├── lib/
│   ├── Arduino_GFX_Library/       Bibliotheque d'affichage vendorisee (v1.6.1, modifiee -- voir NOTICE.md), pilote ecran CO5300/QSPI
│   └── NOTICE.md                  Justification de la modification (retrait du databus RGBPanel incompatible)
│
├── src/                            === POINT D'ENTREE FIRMWARE ===
│   ├── main.cpp                   Point d'entree Arduino (setup()/loop()) : bring-up materiel + init LVGL + boucle
│   ├── utils/
│   │   └── Logger.{h,cpp}        Journalisation serie a niveaux (ERROR/WARN/INFO/DEBUG)
│   ├── data/                      === MODELE DE DONNEES ET FOURNISSEURS ===
│   │   ├── TrophyStats.h          Structure de donnees (voir section 6)
│   │   ├── TrophyDataProvider.h   Interface abstraite commune
│   │   ├── DemoDataProvider.{h,cpp}     Fournisseur de demonstration -- FONCTIONNEL
│   │   └── PocketPsnProvider.{h,cpp}    Fournisseur Pocket PSN -- COMPILE, NON VERIFIE
│   └── ui/                         === CODE LVGL PARTAGE (firmware + simulateur) ===
│       ├── UiManager.{h,cpp}      Navigation, rotation auto, toast, transitions
│       ├── Theme.{h,cpp}          Palette, cartes, halos, animations (helpers reutilisables)
│       ├── Layout.h               Constantes de geometrie circulaire (centre, rayons de securite)
│       ├── AppState.h             Etat applicatif (Wi-Fi simule, sync, reglages) -- voir section 6
│       ├── assets/                19 images generees (trophee, medailles, halos, icones) en tableaux C LVGL
│       └── screens/               Les 6 ecrans : Welcome, Dashboard, Trophies, Statistics, Sync, Settings
│
├── simulator/                       === SIMULATEUR PC ===
│   ├── CMakeLists.txt             Assemble src/ui/*, src/data/DemoDataProvider.cpp + code SDL2 local
│   ├── build.ps1 / run.ps1 / run.sh   Scripts de compilation/lancement (auto-installent ziglang/cmake/ninja via pip)
│   ├── src/
│   │   ├── main.cpp              Boucle SDL2, fenetres, raccourcis clavier, export captures/GIF
│   │   ├── DisplayDriverSdl.*     Pilote d'affichage SDL2 (equivalent du pilote ecran ESP32)
│   │   ├── TouchDriverSdl.*       Pilote tactile SDL2 (souris -> LVGL indev, equivalent du pilote tactile ESP32)
│   │   └── DebugPanel.*           Panneau de reglages en direct (fenetre separee)
│   ├── toolchain/                 Fichiers de chaine de compilation zig (cc/c++/ar) + toolchain CMake
│   ├── third_party/               LVGL 8.3.11 + SDL2 (mingw) + stb_image_write vendorises (voir NOTICE.md)
│   ├── previews/                  Captures officielles de la refonte graphique (livrees a l'utilisateur)
│   └── screenshots/                (ignore par git) captures generees a chaque lancement
│
├── tools/
│   ├── asset_pipeline/
│   │   ├── generate_assets.py    Genere les 19 images de src/ui/assets/ (Pillow+NumPy)
│   │   └── png_src/               Previsualisations PNG humaines des assets
│   └── pocketpsn_probe/
│       ├── probe.py               Outil CLI reel : appelle l'endpoint Pocket PSN identifie
│       └── README.md
│
├── docs/
│   ├── DEVELOPMENT.md             Justification du choix de framework, incidents de compatibilite reels
│   ├── POCKETPSN_PROTOCOL.md       Detail de l'inspection passive du binaire Pocket PSN
│   ├── ASSET_LICENSES.md           Licences des assets (tous generes, aucune ressource tierce)
│   └── UI_REDESIGN_AUDIT.md        Audit visuel avant/apres + decisions de variantes
│
├── data/                           VIDE (dossier cree en Phase 1, jamais rempli -- pas de web/config/cache)
└── test/, test/fixtures/          VIDES (dossiers crees en Phase 1, aucun test ecrit)
```

**Elements explicitement demandes par l'utilisateur et absents du code :**
Wi-Fi (`src/network/`), portail captif, serveur web (`src/web/`, `data/index.html`),
configuration persistante (`src/config/`), cache (`src/data/TrophyCache.*`).
Aucun de ces fichiers n'existe -- ce ne sont pas des fichiers "incomplets", ils
n'ont jamais ete crees.

---

## 3. Tableau des fonctionnalites

| Fonctionnalite | Statut | Preuve / remarque |
|---|---|---|
| Demarrage firmware (`setup()`) | Compile, non teste materiel | `src/main.cpp` ; `pio run` reussit, jamais flashe |
| Diagnostics serie | Compile, non teste materiel | `Logger::info` dans `logDiagnostics()`, jamais observe sur port serie reel |
| Detection PSRAM | Compile, non teste materiel | `psramFound()`/`ESP.getPsramSize()` appeles, jamais executes sur silicium |
| Ecran CO5300 (QSPI) | Compile, non teste materiel | Pilote Arduino_GFX vendorise, sequence d'init copiee du BSP officiel, **jamais allume reellement** |
| Tactile CST9217 (I2C) | Compile, non teste materiel | Lecture par interruption, **jamais teste sur vrai silicium** ; voir risque section 11 (relachement tactile non confirme) |
| LVGL (init, buffers, tick) | Fonctionnel (simulateur) / Compile (firmware) | `lv_conf.h` partage, LVGL 8.4.0 resolu |
| Navigation tactile (swipe) | Fonctionnel (simulateur, souris) / Compile (firmware) | `UiManager::attachGestureHandling`, teste via glisser-souris dans le simulateur |
| Appui long (reglages) | Fonctionnel (simulateur) / Compile (firmware) | `LV_EVENT_LONG_PRESSED` -> `openSettings()` |
| Rotation automatique | Fonctionnel (simulateur) / Compile (firmware) | `onAutoRotateTimer`, teste visuellement |
| Mode demo | Fonctionnel | `DemoDataProvider`, donnees fictives, utilise par defaut partout |
| Modele de statistiques (`TrophyStats`) | Fonctionnel | Voir section 6 |
| Cache hors ligne | **Absent** | Aucun fichier, aucune persistance de `TrophyStats` entre redemarrages |
| Configuration persistante | **Absent** | Aucun LittleFS/NVS ecrit ou lu nulle part dans le code |
| Wi-Fi station | **Absent** | Aucun `WiFi.begin()` dans le code (seul `WiFiClientSecure` est *inclus* par `PocketPsnProvider.cpp`, jamais connecte) |
| Mode point d'acces | **Absent** | Aucun code |
| Portail captif | **Absent** | Aucun code |
| Serveur web | **Absent** | Aucun code (pas de `WebServer`/`ESPAsyncWebServer`) |
| Interface de configuration (web) | **Absent** | Aucune page, aucun fichier `data/*.html` |
| Synchronisation (bouton/logique) | Fonctionnel (simulateur, mode demo) / Partiel (Pocket PSN) | `UiManager::startManualSync()` fonctionne avec `DemoDataProvider` ; avec `PocketPsnProvider` le code s'execute mais n'a jamais ete exerce avec de vraies donnees |
| Gestion des erreurs (ecran) | Fonctionnel (simulateur, simule via touche E) | Ecran Sync affiche l'etat d'erreur ; pas de distinction fine des types d'erreur reseau (timeout vs HTTP vs JSON invalide) au niveau UI |
| Reglage luminosite | Simule (UI uniquement) | `AppState.brightnessPercent` modifiable via panneau debug/ecran reglages, **jamais applique a `gfx->setBrightness()`** sur le firmware (valeur fixe 200 au demarrage) |
| Mise en veille | **Absent** | Aucun timer d'inactivite, aucun `esp_sleep_*`, aucun assombrissement automatique |
| RTC / NTP | **Absent** | `PCF85063` (RTC) jamais instancie ; pas de `configTime()`/NTP ; `lastUpdateEpoch` utilise `std::time()` (horloge systeme, non calee) |
| Detection de nouveaux trophees | Simule | `DemoDataProvider::simulateNewTrophy()` + toast UI ; aucune vraie comparaison delta sur donnees reelles (puisqu'aucune donnee reelle n'existe encore) |
| Pocket PSN | Partiel -- voir section 8 | Endpoint confirme reseau, JSON jamais observe, aucune cle |
| OTA | **Absent** | Partitions OTA prevues dans `partitions.csv` mais aucun code `ArduinoOTA`/`Update.h` |
| Simulateur PC | Fonctionnel | Verifie par execution reelle dans cette session |
| Panneau debug (simulateur) | Fonctionnel | Fenetre separee, modifie stats/etat en direct |
| Captures PNG (simulateur) | Fonctionnel | 6 ecrans + toast exportes automatiquement, verifie |
| Tests automatises | **Absent** | `test/` et `test/fixtures/` existent mais sont vides (aucun fichier) |

---

## 4. Etat des builds (verifie dans cette session, 2026-07-15)

### Firmware

```powershell
pio run
```

**Resultat : SUCCESS**

| | |
|---|---|
| Environnement PlatformIO | `waveshare-amoled-175` |
| Board | `esp32-s3-devkitc-1` (memoire redefinie pour N16R8 : Flash 16 Mo QIO, PSRAM 8 Mo Octal) |
| Framework | `arduino` (`espressif32` platform, `framework-arduinoespressif32` par defaut de la registry PlatformIO) |
| LVGL | 8.4.0 (resolu depuis `lvgl/lvgl @ ^8.3.11`) |
| Bibliotheques principales | `GFX Library for Arduino` 1.6.1 (vendorisee, modifiee), `SensorLib` 0.4.1 (tactile CST92xx), `XPowersLib` 0.2.6 (non utilisee dans le code actuel), `ArduinoJson` 7.4.3, `HTTPClient`/`WiFiClientSecure` 2.0.0 (arduino-esp32, non connectes) |
| RAM | 56.8% -- 186 240 / 327 680 octets |
| Flash | **69.0%** -- 2 170 301 / 3 145 728 octets (a surveiller : la refonte graphique a fait passer ce chiffre de 21.7% a 69.0%, voir `PROJECT_STATUS.md`) |

Aucun flash sur materiel reel -- ces chiffres proviennent uniquement de la
compilation croisee PlatformIO.

### Simulateur PC

```powershell
.\simulator\build.ps1
.\simulator\run.ps1
```

| | |
|---|---|
| Executable | `simulator\build\trophy-display-simulator.exe` (+ `SDL2.dll` copiee a cote) |
| Dependances de build | Python 3.10+, puis `pip install ziglang cmake ninja` (installe automatiquement par les scripts) |
| Dernier build | Reussi (`ninja: no work to do` -> deja a jour ; rebuild complet verifie plus tot dans le projet) |
| Execution | Lancee et arretee proprement dans cette session (`--no-screenshots`), aucun crash |
| Fonctionnalites testees | Les 6 ecrans, navigation clavier/souris, panneau debug, export de captures et de sequence GIF (voir `simulator/previews/`) |

---

## 5. Architecture technique

```
                     ┌─────────────────────────┐
                     │   src/ui/ (partage)     │  <- LVGL uniquement, aucun appel materiel
                     │  UiManager, Theme,       │
                     │  screens/*, assets/*     │
                     └───────────┬─────────────┘
                                 │
                 ┌───────────────┴────────────────┐
                 │                                │
     ┌───────────▼───────────┐        ┌───────────▼─────────────┐
     │   src/main.cpp         │        │  simulator/src/main.cpp │
     │   (firmware ESP32-S3)  │        │  (simulateur PC)         │
     │  Arduino_GFX + CST9217 │        │  SDL2 (DisplayDriverSdl, │
     │  = pilotes materiels   │        │  TouchDriverSdl)         │
     └───────────┬─────────────┘        └───────────┬─────────────┘
                 │                                  │
     ┌───────────▼───────────┐        ┌─────────────▼─────────────┐
     │ src/data/               │        │ src/data/DemoDataProvider │
     │ DemoDataProvider (les    │◄──────┤ (seul fournisseur compile  │
     │ deux utilisent le meme) │        │ par le simulateur)         │
     │ PocketPsnProvider        │        │ PocketPsnProvider EXCLU du │
     │ (firmware uniquement,    │        │ simulateur (depend de      │
     │ HTTPClient Arduino)      │        │ HTTPClient/Arduino)        │
     └──────────────────────────┘        └────────────────────────────┘
```

Reseau, stockage et configuration **n'existent dans aucune des deux cibles**
(voir section 1) -- il n'y a donc rien a schematiser a ce niveau pour
l'instant.

### Reponse a la question "remplacer uniquement `src/ui/` et `src/assets/`"

- Le dossier reellement utilise est **`src/ui/`** (le chemin `src/assets/`
  n'existe pas ; les images generees sont dans **`src/ui/assets/`**, a
  l'interieur de `src/ui/`).
- **Remplacer uniquement `src/ui/` (en gardant sa structure de fichiers
  publique : `UiManager.h`, `screens/*.h`, `AppState.h`) est suffisant** a
  condition que le nouveau design :
  1. respecte la meme API `UiManager` (`begin()`, `tick()`,
     `goToNextMainScreen()`, etc.) puisque **`src/main.cpp` (firmware) et
     `simulator/src/main.cpp` appellent directement ces methodes** ;
  2. respecte la meme structure `TrophyStats`/`AppState` (`src/data/`,
     `src/ui/AppState.h`) puisque c'est le seul contrat de donnees entre
     l'UI et le reste du projet ;
  3. fournit ses propres fonctions `create()`/`update()` par ecran si la
     liste des ecrans (`UiManager::ScreenId`) change -- sinon il faut aussi
     adapter `UiManager.cpp` (pas seulement `src/ui/`).
- Si le nouveau design change la signature de `UiManager` ou le modele
  `AppState`/`TrophyStats`, il faudra aussi toucher `src/main.cpp` et
  `simulator/src/main.cpp` (tres peu de lignes : l'appel a `uiManager.begin(&provider)`
  et `uiManager.tick()`), et `simulator/CMakeLists.txt` (glob de fichiers,
  deja generique sur `src/ui/*.cpp`/`src/ui/screens/*.cpp`/`src/ui/assets/*.c`
  -- fonctionnera tel quel si la nouvelle arborescence reste sous ces
  memes sous-dossiers).
- **Ne pas toucher** : `src/data/TrophyDataProvider.h` (contrat stable),
  `include/lv_conf.h` (partage, deja configure pour ce materiel).

---

## 6. Modele de donnees exact

`src/data/TrophyStats.h` :

```cpp
struct TrophyStats {
  std::string username;
  std::string country;

  int psnLevel = 0;
  int levelProgressPercent = 0;   // "PSN Level Progress"
  int levelRemainingPoints = 0;   // "PSN Level Remaining"

  int platinum = 0;
  int gold = 0;
  int silver = 0;
  int bronze = 0;
  int totalTrophies = 0;

  int trophyPoints = 0;  // "Trophy Points" (score PSN)
  int pocketPoints = 0;  // "Pocket Points" (score propre a Pocket PSN)

  int totalGames = 0;
  int worldRank = 0;
  int countryRank = 0;

  int gamesCompleted = 0;
  float completionAveragePercent = 0.0f;
  float averageRarityPercent = 0.0f;
  int unearnedTrophies = 0;
  float hoursPlayed = 0.0f;

  bool hasPsPlus = false;  // nom de champ JSON exact non confirme

  bool valid = false;             // false tant qu'aucune donnee reelle n'a ete chargee
  uint32_t lastUpdateEpoch = 0;   // horodatage Unix (horloge systeme, PAS de RTC/NTP)
};
```

`src/ui/AppState.h` (etat applicatif, distinct des stats de trophees) :

```cpp
struct AppState {
  enum class WifiStatus { kDisconnected, kConnecting, kConnected };
  enum class SyncStatus { kIdle, kSyncing, kSuccess, kError };
  enum class Language { kFrench, kEnglish };

  TrophyStats stats;

  WifiStatus wifiStatus = WifiStatus::kDisconnected;   // jamais mis a jour par un vrai Wi-Fi (absent)
  bool pocketPsnVerified = false;                       // toujours false hors simulateur/debug
  SyncStatus syncStatus = SyncStatus::kIdle;
  std::string lastErrorMessage;

  int brightnessPercent = 70;      // jamais applique au firmware reel
  Language language = Language::kFrench;   // aucune i18n branchee sur les textes des ecrans
  bool animationsEnabled = true;   // present mais jamais lu par le code d'animation actuel
  bool autoRotateEnabled = true;   // reellement utilise par UiManager
  int rotationIntervalSeconds = 10;        // reellement utilise
};
```

Champs demandes non presents tels quels : il n'y a pas de champ distinct
`pseudo`/`niveau` en dehors de `TrophyStats` (deja couverts) ; il n'y a pas
de champ de "statut reseau" au niveau de `TrophyStats` (c'est `AppState.wifiStatus`,
separe) ni de "statut synchronisation" dans `TrophyStats` (c'est
`AppState.syncStatus`).

---

## 7. Fournisseurs de donnees

### `TrophyDataProvider` (interface, `src/data/TrophyDataProvider.h`)

Existe reellement, compile, ne fait rien par elle-meme (classe abstraite
pure). Methodes : `requestRefresh()`, `poll()`, `stats()`,
`lastErrorMessage()`, `isVerified()`.

### `DemoDataProvider` (`src/data/DemoDataProvider.{h,cpp}`)

- Existe : oui. Compile : oui (firmware + simulateur). Branche : **oui**,
  c'est le fournisseur utilise par defaut dans `src/main.cpp` (firmware) et
  `simulator/src/main.cpp`.
- Renvoie de vraies donnees : non, donnees fictives fixes (pseudo
  `Kevin_Trophies`, niveau 327, etc. -- alignees sur la maquette de
  reference).
- Mock : c'est lui-meme le mock (pas de mock separe).
- Methodes propres : `simulateNewTrophy()`, `simulateNextRefreshError(bool)`,
  `mutableStats()` (acces direct pour le panneau debug).

### `PocketPsnProvider` (`src/data/PocketPsnProvider.{h,cpp}`)

- Existe : oui. Compile : **oui**, verifie par `pio run` reussi.
- Branche : **non** -- aucun code dans `src/main.cpp` ne l'instancie ni ne
  l'utilise. Il est seulement compile car present dans `src/data/` (le
  firmware compile tous les `.cpp` du dossier).
- Renvoie de vraies donnees : **non, jamais teste avec de vraies donnees.**
- `isVerified()` renvoie **`false` en dur** dans le code.
- Utilise `HTTPClient`/`WiFiClientSecure` (Arduino) pour un vrai appel
  reseau -- mais ce chemin n'a jamais ete exerce (pas de cle API).

---

## 8. Pocket PSN -- etat exact (rien de nouveau tente)

- **Identifie** : l'ancien firmware `PlaystationTrophy.bin` (release GitHub
  `Playstation_Trophy_API_1.0`) utilise l'API Pocket PSN.
- **Confirme** (inspection passive de chaines ASCII dans le binaire, plus un
  test reseau minimal reel) :
  - Endpoint : `POST https://api.pocketpsn.com/PSTrophyDisplay/`
  - Corps : `application/x-www-form-urlencoded`, parametres `psn_name` et
    `key`.
  - L'endpoint existe et repond reellement (`HTTP 200`, corps vide observe
    avec des identifiants factices -- test effectue une seule fois, empreinte
    minimale).
- **Reste inconnu** :
  - Comment obtenir une cle `key` legitime (aucun programme developpeur
    public trouve).
  - Le schema JSON exact de la reponse (noms de champs **inferes** depuis le
    README du depot d'origine et les chaines du binaire, jamais confirmes
    par une reponse reelle capturee).
  - Le comportement avec des identifiants valides (jamais observe).
- Un parser existe (`PocketPsnProvider::parseResponse`, utilise
  `ArduinoJson`), **mais il n'a jamais ete execute avec une vraie reponse**.
- Le provider compile (`pio run` = SUCCESS) mais `isVerified()` renvoie
  `false` en dur -- **rien n'est presente comme fonctionnel.**

---

## 9. Web et configuration -- etat exact

Tout ce qui suit est **absent** (aucun fichier, aucune ligne de code) :
portail captif, page web, formulaire Wi-Fi, formulaire pseudo PSN,
sauvegarde de configuration (LittleFS/NVS), reinitialisation, endpoints API
locaux (`/api/*`, `/getConfig`, `/saveConfig`, etc. -- ces routes existaient
dans l'ANCIEN firmware de reference cite dans `AUDIT.md`, pas dans ce
projet).

Les ecrans "Reglages" et "Synchronisation" de l'UI LVGL sont des **vitrines
visuelles** : ils affichent/modifient uniquement `AppState` en memoire (RAM),
rien n'est sauvegarde ni envoye sur le reseau.

---

## 10. Fichiers a ne pas ecraser / fichiers remplacables

**A ne pas ecraser sans compatibilite explicite :**
- `src/data/TrophyStats.h`, `src/data/TrophyDataProvider.h`,
  `src/data/DemoDataProvider.*` (contrat de donnees utilise par tout le
  reste).
- `src/ui/UiManager.h` (signature appelee par les deux `main.cpp`).
- `include/lv_conf.h` (configuration LVGL calee pour ce materiel, partagee).
- `include/BoardConfig.h` (broches reelles, jamais devinees).
- `platformio.ini`, `partitions.csv` (config de build validee reellement).

**Remplacables sans risque pour le reste du projet :**
- `src/ui/screens/*`, `src/ui/Theme.*`, `src/ui/assets/*` (design visuel).
- `simulator/src/DebugPanel.*` (panneau de debug, outil de dev).
- Tout le contenu de `simulator/previews/`, `tools/asset_pipeline/png_src/`.

---

## 11. Risques de fusion avec un nouveau design

1. Si le nouveau design modifie `AppState`/`TrophyStats`, il faut repercuter les 2
   `main.cpp` (firmware + simulateur) et `UiManager.cpp`.
2. Si le nouveau design introduit ses propres assets/police, verifier la licence (voir
   modele dans `docs/ASSET_LICENSES.md`) et l'empreinte Flash (deja a 69%
   avec les assets actuels sur une partition de 3 Mo -- **peu de marge**).
3. Le tactile firmware ne gere qu'un seul point et **n'a jamais ete valide
   sur silicium reel** : le comportement de relachement (pas de deuxieme
   interruption garantie) est une inconnue reelle qui peut casser le
   swipe/l'appui long une fois flashe.
4. `PocketPsnProvider` est compile mais mort (jamais instancie) : toute
   fusion doit garder ce fournisseur **explicitement marque non verifie**,
   ne pas le presenter comme actif.
5. Aucune configuration/portail/Wi-Fi n'existe : si le nouveau design suppose
   des ecrans "connecte au Wi-Fi" fonctionnels, ce sont des maquettes tant
   que ce code n'existe pas.

## 12. A tester des reception du materiel

- Allumage ecran (CO5300/QSPI), tactile (CST9217/I2C) -- jamais teste.
- Comportement reel du tactile en glisse continue (swipe) et en appui long.
- Luminosite reelle, alimentation via `AXP2101` (jamais pilote par code).
- Temps reel d'affichage/latence de l'UI LVGL sur le vrai processeur.

## 13. Prochaines etapes possibles (au choix de l'utilisateur)

- Fusionner un nouveau design `src/ui/` (voir section 5 pour le perimetre
  exact).
- Implementer Wi-Fi + configuration + cache (rien n'existe, tout est a
  ecrire).
- Obtenir une cle Pocket PSN legitime pour lever le blocage de la section 8.
- Recevoir et flasher le materiel physique pour la premiere validation reelle.
