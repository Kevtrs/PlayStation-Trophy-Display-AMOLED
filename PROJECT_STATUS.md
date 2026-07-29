# PROJECT_STATUS

Derniere mise a jour : 2026-07-18 (PocketPsnProvider rendu portable +
selection au demarrage, voir [`HANDOFF_PROGRESS.md`](HANDOFF_PROGRESS.md)
pour le detail complet a reprendre).

## Recapitulatif rapide (3 niveaux de preuve, a ne pas confondre)

| | Statut |
|---|---|
| **Rendu visuel validé sur simulateur PC** | OUI (design considere temporaire, voir ci-dessous) -- refonte graphique complete, captures reelles des 6 ecrans + toast "nouveau trophee" **re-verifiees le 2026-07-15 apres le refactor AppController** (donnees demo Kevin_Trophies affichees correctement partout) |
| **Compilation firmware ESP32-S3 validée** | OUI -- `pio run` reussit reellement (RAM 65.4%, **Flash 70.3%** -- +3.4 points suite au 2026-07-18, `PocketPsnProvider` desormais reellement lie/appele via `ProviderFactory`, plus seulement compile sans etre reference) |
| **Compilation simulateur PC validée** | OUI -- `cmake --build` reussit reellement avec toute la nouvelle logique portable, y compris `PocketPsnProvider` (desormais portable via `IPocketPsnHttpClient`, voir `docs/IMPLEMENTATION_PLAN.md`) |
| **Materiel physique reel** | **NON TESTE.** Aucune carte disponible dans cet environnement d'agent -- WiFiManager et portail captif reels n'ont donc jamais tourne sur un ESP32 physique. |
| **Pocket PSN reellement fonctionnel** | **NON, mais l'architecture est prete.** Une cle API privee et legitime a ete obtenue le 2026-07-18 directement aupres du proprietaire de Pocket PSN (voir `AUDIT.md` section 0ter). `PocketPsnProvider` est portable, `isVerified()` a une semantique reelle (jamais hardcode), la selection du provider se fait au demarrage selon la config. La cle n'est pas encore presente dans cet environnement (ajoutee localement par l'utilisateur uniquement, jamais committee) -- validation reelle toujours en attente. La piste Sony NPSSO exploree entre-temps est abandonnee et retiree du depot. |

## Checkpoint squelette fonctionnel (2026-07-15) : EN COURS, PAS TERMINE

A la demande explicite de l'utilisateur, le travail a bascule du design vers
le squelette fonctionnel complet (architecture en couches, config/cache
persistants, services, synchronisation, etc.), le rendu LVGL actuel etant
desormais traite comme une **couche temporaire de test**, destinee a etre
remplacee par un nouveau design sans reecrire la logique.

**Ce chantier est un travail en cours, interrompu par une limite d'usage.**
Voir [`HANDOFF_PROGRESS.md`](HANDOFF_PROGRESS.md) pour l'etat precis
fichier par fichier, ce qui compile, ce qui est teste, et la prochaine tache
exacte a reprendre. Resume tres court :

- Nouveau modele de donnees (`ProfileData`, `TrophyStats`, `AppSettings`,
  `SyncStatus`, `NetworkStatus`, `TrophyDelta`, `AppState`) : fait, compile
  firmware + simulateur.
- Couche stockage portable (`IPersistentStore`, `FilePersistentStore` pour
  desktop, `NvsPersistentStore` pour firmware/LittleFS) : fait, compile des
  deux cotes (le firmware exclut `FilePersistentStore.cpp` via
  `build_src_filter`, voir `platformio.ini`).
- `ConfigManager` (config versionnee, validee, sauvegarde atomique JSON) :
  fait, compile des deux cotes. Pas encore expose via une API web (Phase B,
  pas commencee).
- `TrophyCache` (cache hors-ligne avec checksum FNV-1a, ne remplace jamais
  une donnee valide par une invalide) : fait, compile des deux cotes.
- `TrophyRepository` + `TrophyDelta` (validation des nouvelles donnees,
  detection de nouveau trophee, rejet des baisses anormales) : fait, compile
  des deux cotes.
- `PocketPsnParser` (parsing JSON pur, extrait de `PocketPsnProvider` pour
  etre testable sans materiel) : fait, compile, **pas encore teste contre
  une vraie reponse** (aucune cle API disponible, voir Phase 2.5).
- `TimeService` : **PARTIAL**. NTP/RTC ESP32 fonctionnel en theorie
  (`configTzTime`, table de conversion IANA -> POSIX TZ limitee a quelques
  fuseaux), mais **jamais teste sur materiel reel** (pas de carte
  disponible) ni par un test automatise dedie. `formatRelative`/
  `formatClock` sont des fonctions pures testables mais aucun test n'a
  encore ete ecrit (Phase F, tache #28).
- `PowerManager`, `SyncService` (avec backoff exponentiel + tache FreeRTOS
  dediee pour ne jamais bloquer la boucle principale sur l'appel HTTPS
  bloquant de `PocketPsnProvider`) : fait, compile des deux cotes (chemin
  FreeRTOS non exerce sur materiel reel faute de carte).
- `AppController` + `UiBridge`/`UiActionListener` (decouplage complet de
  LVGL vis-a-vis de la logique/reseau) : fait, compile des deux cotes ;
  branche dans les deux `main.cpp` (firmware et simulateur).
- **Non commence** : portail captif, serveur web/API, tests automatises +
  fixtures, partitions dediees, OTA, documentation complete
  (`docs/NETWORK.md`, `docs/POCKET_PSN.md`, etc.).

Details complets : [`HANDOFF_PROGRESS.md`](HANDOFF_PROGRESS.md).

## Phase B suite -- ecart de noms diagnostics resolu (2026-07-16) : TERMINE

`data/app.js` (`renderDiagnostics()`) adapte pour lire les 22 noms de
champs canoniques renvoyes par `DiagnosticsSnapshot`/`WebApiHandlers` --
**aucun alias/duplication cote C++**, comme demande. Libelles du panneau
diagnostics mis a jour (22 lignes, formatage lisible octets/durees/
booleens), sans toucher au CSS/HTML existant (`.diagnostic-grid` absorbe
n'importe quel nombre de lignes). `node --check data/app.js` : succes.

Verification statique ajoutee au `--selftest` (`simulator/src/main.cpp`) :
lit le vrai fichier `data/app.js`, extrait par expression reguliere tous
les champs `diagnostics.xxx` reellement references, verifie que chacun
existe comme cle dans la reponse JSON reelle de `/api/diagnostics` --
volontairement pas de liste codee en dur, pour ne pas se desynchroniser
silencieusement du fichier reel.

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.6%, inchange -- aucune
logique C++ modifiee cette passe), simulateur SUCCESS, `--selftest`
32/32 PASS (2 nouvelles assertions), 6 ecrans re-inspectes sans
regression, `pio run -t buildfs` SUCCESS.

## Phase B suite -- sync/reboot/reset/diagnostics reels (2026-07-15) : TERMINE

- **`POST /api/sync`** : appelle uniquement `AppController::requestManualSync()`
  (jamais le provider directement). Decision d'acceptation
  (`WebApiHandlers::shouldAcceptSyncRequest`) extraite en fonction pure
  testable : refuse (HTTP 409) si une synchronisation est deja active
  (`isSyncActive()`), sinon accepte (HTTP 202).
- **`POST /api/reboot`** : repond en JSON (200) **avant** de programmer le
  redemarrage (`PendingRestart`, nouvelle classe portable dans
  `src/services/`) 800 ms plus tard -- laisse le temps a la reponse HTTP
  de partir. `PendingRestart` ne connait pas `ESP.restart()` (appele par
  `CaptivePortalServer::poll()` uniquement), donc testable sans materiel.
- **`POST /api/reset`** : exige `{"confirm":true}` dans le corps (sinon
  400) ; `AppController::factoryReset()` (nouvelle methode) efface la
  configuration (`ConfigManager::resetToDefaults`), le cache de trophees
  (`TrophyCache::clear()`, nouvelle methode) et l'etat en memoire du
  repository (`TrophyRepository::resetInMemoryState()`, nouvelle methode),
  puis programme un redemarrage (meme mecanisme que `/api/reboot`).
- **`GET /api/diagnostics`** : `DiagnosticsSnapshot` (struct portable,
  `src/web/`) avec exactement les champs dictes par l'utilisateur
  (`firmwareVersion`, `uptimeSeconds`, `freeHeapBytes`, ..., 20 champs).
  Champs materiel (heap/PSRAM/Flash/LittleFS/uptime) toujours `null` dans
  le simulateur (non mesurables), remplis par `CaptivePortalServer` sur
  firmware (`ESP.getFreeHeap()`, `LittleFS.usedBytes()`, etc.). **Jamais
  de mot de passe Wi-Fi** (le champ n'existe structurellement pas dans
  `DiagnosticsSnapshot`) ni de `pocketPsnVerified` fabrique.
- **Ecart reel decouvert et documente** (voir
  `docs/WEB_UI_GAP_ANALYSIS.md`) : les noms de champs diagnostics dictes
  par l'utilisateur (`firmwareVersion`, `uptimeSeconds`, ...) **ne
  correspondent pas** a ceux lus par `data/app.js` (`firmware`, `uptime`,
  `heapFree`, ...). Verifie que cela ne casse rien (`renderDiagnostics()`
  affiche `-` pour les champs non reconnus au lieu de planter, le JSON
  brut affiche tout correctement) mais les 8 lignes "amicales" du panneau
  diagnostics resteront vides tant que ce choix de noms n'est pas
  tranche par l'utilisateur.
- `/api/profile/test` **volontairement laisse en `501 not_implemented`**,
  comme demande, jusqu'a ce que `tools/pocketpsn_probe/` fournisse une
  vraie reponse et un schema confirme.

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.6%), simulateur SUCCESS,
`--selftest` 30/30 PASS (14 nouvelles assertions : sync accepte/refuse,
reboot programme/declenche/non-redeclenche, reset refuse (absent/false)/
accepte, effets reels du reset sur `AppController`, diagnostics sans mot
de passe/avec champs materiel absents), 6 ecrans re-inspectes sans
regression, `pio run -t buildfs` SUCCESS.

**Non teste faute de materiel** : `ESP.restart()` reel, `ESP.getFreeHeap()`/
`LittleFS.usedBytes()` reels, le parcours complet reboot/reset via un
vrai navigateur.

## Phase B suite -- Integration du module Web UI (2026-07-15) : TERMINE (parcours de base)

- **Fichier `WEB_UI_CONTRACT.md` fourni par l'utilisateur invalide** : son
  contenu reel est celui de `app.js` (verifie -- 0 titre Markdown dedans),
  pas une prose de contrat. Le vrai module (`data/index.html`,
  `data/app.js`, `data/styles.css`) a ete retrouve dans l'archive extraite
  fournie et integre tel quel (design non touche). Aucun `WEB_UI_CONTRACT.md`
  ni `simulator/web_mock/` reels n'ont ete retrouves malgre la description
  de l'utilisateur -- **le contrat API a ete reconstruit par lecture
  exhaustive de `app.js`** (chaque `fetch()`), voir
  [`docs/WEB_UI_GAP_ANALYSIS.md`](docs/WEB_UI_GAP_ANALYSIS.md) pour le
  detail complet des ecarts et decisions.
- **Page de secours precedente remplacee** par les vrais fichiers du module Web UI.
  Une version reduite auto-suffisante (HTML/CSS/JS inline) est **conservee
  dans le binaire** (`CaptivePortalServer::kFallbackHtml`), servie
  uniquement si LittleFS n'a pas encore recu `pio run -t uploadfs` --
  repli technique reel (sans lui : page vide/404 au tout premier
  demarrage), pas une copie laissee par prudence.
- **`GET /api/status` entierement reecrite** pour matcher le contrat exact
  (`configured`/`offline`/`error`/`network.{connected,ssid,ip,message}`/
  `sync.{state,lastSync,source}`) -- forme precedente incompatible avec ce
  que `app.js` lit reellement.
- **`GET /api/config` + `POST /api/config` (nouvelles routes)** : traduction
  de champs (ssid/brightness/sleepEnabled+sleepDelay/autoRotation/
  rotationDelay/animations/language/syncInterval <-> wifiSsid/
  brightnessPercent/sleepTimeoutSeconds/autoRotateEnabled/
  rotationIntervalSeconds/animationsEnabled/language/syncIntervalMinutes)
  dans `WebApiHandlers` (forme uniquement) ; validation/persistance
  restent entierement dans `ConfigManager`/`AppController` (aucune
  logique metier dupliquee dans la couche web, comme demande).
- **Incompatibilite documentee** : le module Web UI propose 4 langues
  (fr/en/es/de), `AppSettings::AppLanguage` n'en supporte que 2 (fr/en) --
  etendre l'enum toucherait l'ecran LVGL Settings, hors perimetre ("ne pas
  fusionner le nouveau design LVGL pour l'instant"). Une langue non supportee est
  acceptee sans planter mais ignoree, avec un message d'avertissement
  non bloquant dans la reponse.
- **5 routes du contrat complet volontairement en `not_implemented` (501)**,
  comme demande pour les fonctionnalites non terminees : `/api/profile/test`
  (Pocket PSN), `/api/sync`, `/api/reboot`, `/api/reset`, `/api/diagnostics`
  (hors du "parcours de configuration de base" demande cette passe).
  Verifie que `refreshDiagnostics()` (appelee au chargement de la page)
  avale sa propre erreur sans faire echouer le reste du boot -- un stub
  501 ne casse donc pas la page.
- **LittleFS genere reellement** (`pio run -t buildfs`, sans materiel) :
  image de 8 323 072 octets (= taille exacte de la partition littlefs,
  normal -- une image LittleFS occupe toujours la taille totale de la
  partition qu'elle formate) contenant les 3 fichiers reels (32 878 octets
  au total : index.html 8798, app.js 15204, styles.css 8876) -- usage reel
  <1% de l'espace disponible.
- **Toutes les routes appelees par `app.js` existent** (verifie ligne par
  ligne dans le fichier, 10 routes distinctes -- voir tableau dans
  `docs/WEB_UI_GAP_ANALYSIS.md`).

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.5%), simulateur SUCCESS,
`--selftest` 15/15 PASS (4 nouvelles assertions sur `/api/config` et le
nouveau contrat `/api/status`), 6 ecrans re-inspectes sans regression,
`pio run -t buildfs` SUCCESS.

**Non teste faute de materiel** : le vrai module Web UI servi par un
navigateur reel via le portail captif, `pio run -t uploadfs` (necessite un
port serie), le repli embarque (`kFallbackHtml`) jamais reellement
declenche sur un appareil physique.

## Phase B suite -- Partitions + portail captif (2026-07-15) : TERMINE (routes de base), UI Web NON integree

- **Partitions redimensionnees** (voir `docs/PARTITIONS.md` pour le detail
  complet et le compromis avec/sans OTA) : `app0`/`app1` portes de 3 Mo a
  4 Mo chacun (usage retombe de 85.2% a 63.9% avant le portail captif, puis
  65.2% apres), `littlefs` reduit de ~9.94 Mo a ~7.94 Mo en echange (toujours
  tres largement suffisant). Deux slots OTA conserves (decision documentee,
  pas encore de flux OTA implemente). Verifie par compilation reelle avant
  tout ajout de code web, comme demande.
- **Verification 16 Mo reelle** (pas seulement lue dans `platformio.ini`) :
  `pio run -v` confirme `--flash_size 16MB` passe explicitement a
  `esptool.py elf2image` pour le bootloader et le firmware ; `gen_esp32part.py`
  accepte une table totalisant exactement 16 777 216 octets sans erreur de
  depassement. Le texte "8 MB QD" affiche par `pio run` est uniquement le nom
  fige du manifest de la carte de reference, pas la configuration reelle.
- `IWiFiManager`/`WiFiManager`/`WiFiManagerStub` etendus avec un scan Wi-Fi
  non bloquant (`requestScan()`/`scanState()`/`scanResults()`) :
  `WiFi.scanNetworks(true)` + `WiFi.scanComplete()` cote firmware (necessite
  le mode `WIFI_AP_STA`, pas `WIFI_AP` seul, pour scanner pendant que le
  point d'acces de secours reste actif), reseaux fictifs stables cote
  simulateur.
- `WebApiHandlers` (`src/web/`, **portable**, ArduinoJson uniquement) :
  construction/analyse JSON pure pour `/api/status`, `/api/wifi/scan`,
  `/api/wifi/connect` -- testee par 5 assertions `--selftest` supplementaires
  (12 au total, toutes passantes) sans avoir besoin d'un vrai socket.
- `CaptivePortalServer` (firmware uniquement, `WebServer.h`+`DNSServer.h`,
  aucune dependance externe -- resolues automatiquement comme `WiFi.h`) :
  DNS captif actif uniquement en mode point d'acces (jamais en mode
  station, pour ne pas detourner le DNS d'un reseau qui n'appartient pas a
  l'appareil), redirection HTTP 302 vers `192.168.4.1` pour tout chemin
  inconnu en mode AP, routes `GET /api/status`, `GET /api/wifi/scan`,
  `POST /api/wifi/connect`, `POST /api/wifi/forget`. `poll()` ne fait que
  relayer vers `handleClient()`/`processNextRequest()` (jamais bloquant).
- **Bug reel trouve et corrige en revue** : `handleNotFound()` redirige tout
  chemin inconnu vers `/` en mode AP, mais aucune route n'etait enregistree
  pour `/` -- boucle de redirection infinie des l'ouverture de la page
  captive. Corrige en servant `data/index.html`/`data/app.js` depuis
  LittleFS via `server_.serveStatic()`.
- **Page de secours minimale** (`data/index.html`, `data/app.js`) : exerce
  reellement le parcours scan -> selection -> mot de passe -> connexion ->
  sauvegarde -> sortie du mode AP via les routes ci-dessus. Volontairement
  sans design (voir avertissement en tete des deux fichiers) -- **a
  remplacer par le vrai module Web UI des que ses fichiers seront fournis** (voir
  "Bloque" ci-dessous).
- **BLOQUE** : `PlayStation-Trophy-Display-Web-UI-Module.zip` et
  `WEB_UI_CONTRACT.md`, mentionnes par l'utilisateur, sont introuvables
  dans cet environnement (recherche effectuee sur tout le disque). L'UI
  Web n'a donc pas pu etre integree dans cette passe -- uniquement la
  page de secours ci-dessus. A fournir pour la suite.
- **Non teste faute de materiel** : tout le parcours captif reel (ouverture
  automatique de la page sur un telephone/PC, DNS hijacking effectif,
  `serveStatic` servant reellement les fichiers depuis LittleFS -- necessite
  aussi `pio run -t uploadfs`, jamais execute ici faute de carte connectee).

## Phase B -- Wi-Fi (2026-07-15) : TERMINE (station/AP/reconnexion), portail captif NON commence

- `IWiFiManager` (interface portable, `src/network/`) : `begin/poll/
  forgetNetwork/requestReconnect/status`, jamais bloquant, seule
  abstraction connue d'`AppController`.
- `WiFiManager` (firmware, `src/network/WiFiManager.cpp`, `WiFi.h` ESP32
  reel) : machine a etats non bloquante -- connexion station, detection de
  deconnexion, reconnexion avec backoff exponentiel (2 s -> 60 s max),
  bascule automatique en point d'acces de secours (`TrophyDisplay-Setup`)
  apres 5 echecs consecutifs ou en l'absence de configuration valide.
  Toutes les transitions passent par `poll(nowMillis)` (jamais par
  begin()/requestReconnect() directement) pour eviter de calculer un delai
  ecoule depuis un horodatage factice -- bug de conception evite des la
  premiere version grace a la lecon tiree de `SyncService::everAttempted_`
  (voir Checkpoint precedent). **Jamais teste sur materiel reel** (pas de
  carte disponible dans cet environnement).
- `WiFiManagerStub` (simulateur, `simulator/src/`) : simule une connexion
  aboutissant apres 600 ms non bloquants, pilotable depuis `DebugPanel`
  (4 boutons : Deconnecte/Connexion/Connecte/Point d'acces) et par le
  clavier (touche D). Utilise par `--selftest` (7 assertions, toutes
  passantes) qui verifie que les etats Wi-Fi (connecte, deconnecte, point
  d'acces, transition asynchrone via begin()) survivent a plusieurs
  `tick()` -- exactement la classe de bug corrigee au checkpoint precedent
  pour les reglages/donnees de debug.
- `AppController` interroge desormais `IWiFiManager::status()` a chaque
  `tick()` (source de verite unique pour `state_.network`) ; l'ancienne
  methode `setNetworkStatus()` (poussee de l'exterieur) a ete supprimee.
  Nouvelles methodes `forgetWifiNetwork()`/`requestWifiReconnect()`,
  prealables a la Phase B suite (portail captif).
- **Bug reel trouve et corrige pendant la verification** : la connexion
  Wi-Fi par defaut du simulateur pokait directement le stub
  (`wifiStub.begin(...)`) sans passer par `ConfigManager` ; le premier
  appel a `debugApplySettings()` (ex: pour figer la rotation auto pendant
  l'export de captures) relisait alors les identifiants Wi-Fi **vides**
  de la configuration et reinitialisait silencieusement la connexion.
  Corrige en faisant passer la config Wi-Fi de demonstration par
  `AppController::debugApplySettings()` (donc par `ConfigManager`),
  jamais par une ecriture directe dans le stub.
- **Cout Flash reel** : la pile Wi-Fi ESP32 (`WiFi.h`) ajoute ~336 Ko
  (Flash 74.5% -> 85.2%, RAM 58.6% -> 65.1%) -- a surveiller de pres pour
  la Phase F (redimensionnement de partitions, tache #29) ; la portail
  captif + serveur web (Phase B suite) ajouteront probablement encore.
- **Non commence** (a la demande explicite de l'utilisateur, apres
  validation de ce chantier) : portail captif, serveur web, routes API
  (`GET /api/wifi/scan`, `POST /api/wifi/connect`, etc.), fichiers
  `data/index.html` etc.

## Refonte graphique premium (2026-07-14/15) : TERMINEE

A la demande explicite de l'utilisateur, la version "wireframe fonctionnel"
(Phase 3 initiale) a ete entierement remplacee visuellement -- interactions
et architecture LVGL conservees, uniquement le rendu a change. Sauvegarde de
l'ancienne version : branche git `backup/wireframe-v1`.

- **Audit visuel** de la version wireframe : [`docs/UI_REDESIGN_AUDIT.md`](docs/UI_REDESIGN_AUDIT.md).
- **19 assets graphiques generes proceduralement** (trophee illustre,
  medailles Platine/Or/Argent/Bronze en 2 tailles, 3 halos radiaux, 4 icones
  de statistiques, texture de fond) via
  `tools/asset_pipeline/generate_assets.py` (Python + Pillow + NumPy, aucune
  ressource tierce) -- convertis en tableaux C LVGL (`src/ui/assets/*.c`),
  compiles a la fois par le firmware et le simulateur. Licences :
  [`docs/ASSET_LICENSES.md`](docs/ASSET_LICENSES.md).
- **Deux variantes comparees puis tranchees** pour Dashboard et Trophees
  (captures dans `simulator/previews/*-variant-{a,b}.png`, decision
  documentee dans `docs/UI_REDESIGN_AUDIT.md`).
- **Animations reelles** : entrees decalees (fondu+zoom), pulsation des
  halos, anneau de progression anime, compteur progressif, toast "nouveau
  trophee" sur la couche superieure LVGL.
- **Preview complete livree** : `simulator/previews/` contient les 6 ecrans
  finaux + toast nouveau trophee, les 4 captures de variantes, une planche
  avant/apres (`before-after-board.png`), et un GIF anime
  (`preview.gif`, sequence demarrage -> navigation -> synchronisation ->
  nouveau trophee).
- **Flash firmware a surveiller** : passe de 21.7% a **69.0%**
  (2.17 Mo / 3.15 Mo) avec les nouveaux assets embarques (la texture de fond
  466x466 a elle seule pese ~650 Ko en RGB565+alpha). Reste dans la
  partition (31% libre) mais a optimiser si d'autres assets sont ajoutes --
  piste : format `LV_IMG_CF_ALPHA_8BIT` (1 octet/pixel au lieu de 3) pour la
  texture de fond, qui n'a besoin que d'un canal d'opacite.

## Phase 1 -- Analyse : TERMINEE

## Phase 2 -- Bring-up materiel : CODE ECRIT + COMPILATION REELLE OK, NON FLASHE

Voir commit precedent. Broches/pilotes copies du depot officiel Waveshare.

## Phase 2.5 -- Pocket PSN Proof of Concept : TERMINEE (avec limites documentees)

Corrige suite a un signalement utilisateur : le ZIP fourni initialement
correspondait a un ancien commit (2025-10-22) du depot d'origine ; le README
actuel (HEAD, commit du 2025-12-06) decrit une version reecrite basee sur
l'API Pocket PSN, distribuee uniquement sous forme du firmware
`PlaystationTrophy.bin` (release `Playstation_Trophy_API_1.0`), sans code
source publie. Voir [`AUDIT.md`](AUDIT.md) (sections 0bis a 6) pour le detail
complet de la correction.

Inspection passive du binaire (chaines ASCII uniquement, aucun
desassemblage) -- voir [`docs/POCKETPSN_PROTOCOL.md`](docs/POCKETPSN_PROTOCOL.md) :
- **Endpoint confirme** : `POST https://api.pocketpsn.com/PSTrophyDisplay/`,
  corps `application/x-www-form-urlencoded` avec `psn_name` et `key`.
- **Verification reseau reelle effectuee** (identifiants factices, empreinte
  minimale) : l'endpoint existe et repond (`HTTP 200`, corps vide pour des
  identifiants invalides).
- **Bloquant restant** : aucune methode d'obtention publique d'une cle
  Pocket PSN legitime n'a ete confirmee. Une chaine ressemblant a une cle a
  ete trouvee dans le binaire mais **n'est pas reutilisee** (appartiendrait a
  l'auteur du firmware d'origine).
- Schema JSON de reponse **infere** (noms de champs observes dans le binaire
  + README), **jamais confirme par une vraie reponse capturee**.

Livrables :
- `tools/pocketpsn_probe/probe.py` : outil CLI reel, teste, qui appelle
  l'endpoint confirme sans deviner de champ et sans cle codee en dur.
- `src/data/TrophyStats.h`, `TrophyDataProvider.h` (interface abstraite),
  `PocketPsnProvider.{h,cpp}` : **compile avec le firmware** mais **non
  fonctionnel/non verifie** (`isVerified() == false`) tant qu'une cle
  legitime n'aura pas ete testee reellement.
- `src/data/DemoDataProvider.{h,cpp}` : fonctionnel, donnees fictives
  alignees sur la maquette de reference (Kevin_Trophies, niveau 327, etc.).

Pocket PSN reste le fournisseur **obligatoire** de la version finale (decision
utilisateur) ; le mode demo n'est qu'un outil de developpement temporaire.

## Phase 3 -- Interface LVGL en mode demo : TERMINEE (simulateur + firmware)

- `src/ui/` : theme, geometrie circulaire partagee (`Layout.h`), `UiManager`
  (navigation swipe/fleches, indicateurs de page, rotation auto, appui long
  -> reglages, synchronisation manuelle avec ecran dedie), 6 ecrans
  (Welcome, Dashboard, Trophies, Statistics, Sync, Settings).
- **Code strictement partage** entre firmware ESP32-S3
  (`platformio.ini`/`src/main.cpp`) et simulateur PC (`simulator/CMakeLists.txt`).
  Seules les couches materielles differencient (Arduino_GFX+CST9217 vs
  SDL2).
- Plusieurs bugs reels trouves et corriges en verifiant visuellement sur le
  simulateur (non detectables par la seule compilation) :
  - `lv_tick_inc()` jamais appele -> l'affichage restait fige sur le premier
    ecran rendu (tous les changements d'ecran semblaient ignores).
  - Padding par defaut du theme LVGL sur des conteneurs imbriques -> libelles
    et valeurs superposes dans les badges du tableau de bord.
  - Alignement centre sur texte de largeur variable -> point colore
    chevauchant la premiere lettre des libellés (ecran Trophees).
  - Texte XP masque par un badge dessine par-dessus -> largeur contrainte +
    abreviation en "k" au-dela de 9999.
  - `lv_scr_load()` appele deux fois sans laisser une transition (meme a
    duree nulle) se terminer -> plantage reel (corrige).
- **RAM firmware a surveiller** : le pool memoire interne LVGL
  (`LV_MEM_SIZE`, `include/lv_conf.h`) a du etre porte de 64 Ko a 160 Ko pour
  eviter un crash reel (`realloc` echouant silencieusement). Avec les
  buffers d'affichage, l'usage RAM du firmware est passe de 6.2% a 56.8%
  (186 Ko/327 Ko rapportes par PlatformIO pour ce profil de carte). A revoir
  si d'autres ecrans/widgets sont ajoutes -- envisager de deplacer les
  buffers LVGL en PSRAM si necessaire.

## Simulateur PC (`simulator/`) : FONCTIONNEL ET VERIFIE REELLEMENT

- Chaine de compilation 100% reproductible sans Visual Studio/MSYS2 :
  `pip install ziglang cmake ninja` (installe automatiquement par
  `simulator/run.ps1`/`run.sh` au premier lancement).
- SDL2 (sous-ensemble mingw) et LVGL 8.3.11 vendorises dans
  `simulator/third_party/` (memes versions que le firmware, voir NOTICE.md).
- `simulator/run.ps1` **teste de bout en bout dans cette session** (build
  propre depuis zero + execution + export des 6 captures) : fonctionne.
- Masque circulaire reel applique (rayon 233px), zone de securite partagee
  avec le firmware (`src/ui/Layout.h`).
- Raccourcis clavier (fleches, R/E/N/D/F), swipe/glisse souris, appui long,
  panneau de debug (fenetre separee, jamais dans le rendu circulaire)
  permettant de modifier en direct pseudo/niveau/trophees/stats/Wi-Fi/
  Pocket PSN simule/luminosite/langue/animations/rotation -- tout verifie
  fonctionnel par test reel.
- Limite connue : le panneau de debug n'a pas de saisie de texte libre pour
  le pseudo (3 boutons preremplis) -- simplification volontaire de cette
  passe.

## Decisions utilisateur actees

1. Strategie de donnees reelles : mode demo d'abord (2026-07-13), **Pocket
   PSN devient le fournisseur obligatoire de la version finale (2026-07-14)**.
2. Portee : MVP puis iterations.

## Prochaine etape proposee

Selon priorite utilisateur :
- (a) Obtenir une cle Pocket PSN legitime (contact avec l'editeur ou compte
  personnel) pour lever le blocage de la Phase 2.5 et rendre
  `PocketPsnProvider` reellement testable ;
- (b) Continuer a affiner l'UI dans le simulateur (i18n, luminosite/veille
  reelle, portail Wi-Fi/captif, tests) avant de recevoir le materiel ;
- (c) Flasher reellement la carte des reception pour valider Phase 2/3 sur
  materiel physique.
