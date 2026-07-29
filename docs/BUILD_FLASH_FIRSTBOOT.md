# Compilation, flash et premier demarrage

Procedure a suivre pour la toute premiere validation sur le materiel reel
(Waveshare ESP32-S3-Touch-AMOLED-1.75). Redigee avant d'avoir jamais flashe
ce firmware sur l'ecran reel -- a corriger avec les observations reelles
des la premiere tentative (voir la checklist materiel,
`docs/HARDWARE_TEST_CHECKLIST.md`, pour le suivi structure de cette
premiere validation).

## 1. Prerequis

- [PlatformIO Core](https://platformio.org/) installe (CLI `pio`, ou
  l'extension VSCode qui l'embarque).
- Cable USB-C supportant les donnees (pas seulement l'alimentation).
- Le module ESP32-S3 Waveshare connecte, pilotes USB-UART installes si
  necessaire (CP210x/CH340 selon la revision de carte).

## 2. Compiler le firmware

Depuis la racine du depot :

```
pio run
```

Verifie surtout, a la fin de la sortie :

```
RAM:   [==        ]  15.3% (used 50228 bytes from 327680 bytes)
Flash: [=====     ]  50.6% (used 2123521 bytes from 4194304 bytes)
```

Ces chiffres (derniere mesure connue, 2026-07-23, build propre depuis zero,
0 warning) doivent rester du meme ordre de grandeur. Une hausse brutale et inexpliquee de la RAM/Flash apres
une modification est un signal a ne jamais ignorer avant de flasher (voir
`docs/PARTITIONS.md` pour le detail des marges disponibles : le firmware vit
dans un slot `app0`/`app1` de 4 Mo chacun, sur un total Flash de 16 Mo).

Si la compilation echoue avec une erreur liee a `esp32-hal-periman.h` ou a
une version de `GFX Library for Arduino`, voir le commentaire de
compatibilite en tete de `platformio.ini` -- c'est un probleme connu et deja
resolu par l'epinglage de version qui s'y trouve ; ne pas essayer de forcer
une autre version de framework sans relire cette note.

## 3. Preparer et flasher le systeme de fichiers (LittleFS)

Le portail captif sert `data/index.html`, `data/app.js`, `data/styles.css`
depuis une partition LittleFS separee (`littlefs`, voir
`docs/PARTITIONS.md`) -- ces fichiers ne sont jamais compiles dans le
binaire applicatif. Cette partition doit etre flashee separement, **au
moins une fois**, et de nouveau a chaque modification du contenu de
`data/` :

```
pio run -t buildfs
pio run -t uploadfs
```

`uploadfs` efface et reecrit toute la partition LittleFS -- ce qui inclut,
sur un appareil deja configure, **la configuration persistee (Wi-Fi, pseudo
PSN, cle API, cache de trophees hors ligne)** puisque
`NvsPersistentStore`/`FilePersistentStore` s'appuient sur ce meme systeme de
fichiers (voir `docs/ARCHITECTURE.md`). Sur un flash initial (carte neuve ou
deja effacee), c'est sans consequence. Sur un appareil deja configure et en
usage, `uploadfs` forcera une reconfiguration complete au prochain
demarrage -- a faire sciemment, jamais par automatisme.

## 4. Flasher le firmware

```
pio run -t upload
```

Ouvrir ensuite le moniteur serie pour observer le demarrage :

```
pio device monitor
```

(`monitor_speed = 115200` et `monitor_filters = esp32_exception_decoder`
sont deja configures dans `platformio.ini` -- un crash affichera directement
la trace decodee avec noms de fonctions plutot que des adresses brutes.)

## 5. Sequence attendue au premier demarrage

D'apres `src/main.cpp::setup()`, dans l'ordre :

1. `Serial.begin(115200)` puis un bloc de diagnostics
   (`logDiagnostics()`) : modele de puce, taille Flash/PSRAM detectee,
   heap libre, raison du dernier redemarrage. **A verifier en premier** :
   `PSRAM: <n> octets detectes` doit apparaitre -- si `PSRAM introuvable`
   s'affiche a la place, `board_build.arduino.memory_type = qio_opi` n'a
   pas ete pris en compte (verifier le cablage/la revision de carte avant
   toute autre chose, aucun ecran ne fonctionnera correctement sans PSRAM
   ici).
2. Initialisation ecran (`gfx->begin()`) puis tactile (`touch.begin()`) --
   chacun logue OK/ECHEC separement. **Si l'ecran echoue, `setup()`
   s'arrete la** (`return` explicite) : rien d'autre ne demarre, y compris
   pas de Wi-Fi/portail captif. Un echec tactile seul n'est pas bloquant
   (l'app demarre quand meme, mais sans interaction possible).
3. Init LVGL + double buffer DMA, puis `uiManager.begin()`.
4. Selection du provider de donnees : sans configuration existante
   (pseudo PSN + cle API vides, cas d'un flash initial), c'est
   **`DemoDataProvider`** qui est utilise (voir
   `ProviderFactory::shouldUsePocketPsn`) -- l'app affichera donc des
   donnees factices de demonstration jusqu'a ce que le Wi-Fi et le compte
   Pocket PSN soient configures via le portail captif.
5. `AppController::begin()` puis `CaptivePortalServer::begin()` (serveur
   HTTP port 80).
6. Log final : `Squelette fonctionnel demarre (mode demo, ...)`.

## 6. Premiere configuration via le portail captif

Sans identifiants Wi-Fi enregistres, `WiFiManager` bascule automatiquement
en **point d'acces de secours** (`WiFiManager::startAccessPoint()`) :

- SSID : `TrophyDisplay-Setup` (sans mot de passe, ouvert -- comportement
  assume, voir le commentaire dans `src/network/WiFiManager.cpp`).
- Se connecter a ce reseau depuis un telephone/PC, puis ouvrir
  `http://192.168.4.1/` (IP AP standard ESP32 ; l'adresse exacte est aussi
  logguee au demarrage cote moniteur serie).
- Renseigner le Wi-Fi domestique (SSID + mot de passe) via la page servie.
- Renseigner le pseudo PSN et la cle API Pocket PSN (**a saisir uniquement
  ici, jamais committee ni journalisee** -- voir la contrainte de securite
  rappelee dans `src/data/PocketPsnProvider.h`).
- Enregistrer : l'appareil redemarre automatiquement des que la
  configuration touche le pseudo PSN ou la cle API (voir
  `WebApiHandlers::configPatchRequiresRestart()` /
  `CaptivePortalServer::handleConfigPost()`) -- message "en cours de
  redemarrage" attendu a l'ecran/dans la reponse HTTP, appareil brievement
  injoignable.
- Apres redemarrage, `ProviderFactory::shouldUsePocketPsn()` devient vrai
  (pseudo + cle non vides) : le provider actif passe de `DemoDataProvider`
  a `PocketPsnProvider`, log `mode Pocket PSN` attendu.

## 7. Points de vigilance specifiques au tout premier flash

- **Jamais de vraie cle API dans ce depot.** Elle est saisie uniquement via
  le portail captif, stockee en LittleFS, jamais commitee/loggee (voir
  `docs/POCKETPSN_PROTOCOL.md` et la verification d'historique Git deja
  faite, section correspondante de `HANDOFF_PROGRESS.md`).
- Le TODO connu et deliberement non resolu : `setInsecure()` dans
  `src/network/PocketPsnHttpClient.cpp` (pas de verification du certificat
  TLS racine d'`api.pocketpsn.com`, faute d'avoir pu l'observer sur une
  vraie connexion avant ce point). Accepte pour ce POC, a corriger dans une
  passe ulterieure.
- Le premier vrai appel a `api.pocketpsn.com` depuis ce firmware est
  **un evenement en soi** : tout ce qui a ete valide jusqu'ici (parser,
  gestion d'erreurs, cache, reconnexion) l'a ete via le simulateur et des
  reponses simulees/anonymisees, jamais via une requete HTTPS reelle emise
  par le microcontroleur. Surveiller en particulier, au premier essai
  reussi : la taille reelle du corps recu (le protocole documente
  ~691 octets, voir `docs/POCKETPSN_PROTOCOL.md`), et l'absence d'erreur
  TLS (`transportOk=false`) qui indiquerait un probleme de certificat/SNI
  specifique a `WiFiClientSecure` non rencontre sur le simulateur (qui ne
  fait aucune vraie requete HTTPS).
