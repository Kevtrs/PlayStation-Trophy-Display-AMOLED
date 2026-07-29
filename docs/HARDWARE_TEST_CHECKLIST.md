# Checklist de validation materielle (premiere fois)

A remplir lors de la toute premiere validation sur l'ESP32-S3 et l'ecran
AMOLED reels. Tout le reste (logique metier, parser, cache, reconnexion,
etc.) a deja ete valide via le simulateur (voir `HANDOFF_PROGRESS.md`) --
cette checklist couvre uniquement ce que le simulateur ne peut pas tester :
le materiel lui-meme et la vraie connexion reseau. Le design/la mise en page
LVGL sont geres separement (passe de design dediee) et ne font pas partie de cette
checklist (sauf ou indique "non bloquant pour le design").

Cocher chaque case au fur et a mesure ; noter tout ecart par rapport au
comportement attendu (voir `docs/BUILD_FLASH_FIRSTBOOT.md` pour la
procedure de flash correspondante).

## 1. Bring-up materiel de base

- [ ] Le moniteur serie affiche le bloc de diagnostics au demarrage
      (`logDiagnostics()`), sans redemarrage en boucle.
- [ ] `PSRAM: <n> octets detectes` apparait (pas `PSRAM introuvable`).
- [ ] `Ecran CO5300 initialise (466x466)` apparait -- pas
      `gfx->begin() a echoue`.
- [ ] `Tactile detecte: ...` apparait -- pas `Tactile CST9217 introuvable`.
- [ ] L'ecran affiche effectivement une image (pas noir/blanc uni) des la
      fin de `setup()`.
- [ ] Toucher l'ecran produit une reaction visible (changement d'ecran,
      reveil apres veille, etc.) -- confirme le mapping tactile
      (`touch.setMaxCoordinates`) sans inversion X/Y ni decalage.

## 2. Wi-Fi et portail captif

- [ ] Sans configuration existante, un reseau Wi-Fi `TrophyDisplay-Setup`
      (ouvert, sans mot de passe) est bien visible depuis un telephone.
- [ ] La page `http://192.168.4.1/` se charge en mode point d'acces.
- [ ] Se connecter a un reseau Wi-Fi reel via cette page fonctionne
      (`POST /api/wifi/connect`) : l'appareil bascule en mode station,
      `WiFiManager::status().state == kConnected` (visible via
      `GET /api/status` ou les logs).
- [ ] Couper le Wi-Fi (eteindre la box/changer le mot de passe) : l'appareil
      detecte la perte de connexion (`kDisconnected`) et retente avec
      backoff exponentiel (logs `WiFiManager: connexion perdue` puis
      tentatives espacees, pas une boucle serree).
- [ ] Apres 3 echecs consecutifs (`kMaxFailuresBeforeAp`), l'appareil
      revient automatiquement en point d'acces de secours.
- [ ] `POST /api/wifi/forget` efface les identifiants et rebascule en point
      d'acces immediatement.
- [ ] `GET /api/wifi/scan` renvoie une liste de reseaux plausible (SSID,
      RSSI, `requiresPassword`) en un temps raisonnable (pas de blocage de
      l'UI pendant le scan asynchrone).

## 3. Configuration Pocket PSN et premiere synchronisation reelle

**Point critique : c'est le tout premier vrai appel HTTPS jamais emis par ce
firmware vers `api.pocketpsn.com`** (tout le reste a ete valide en
simulateur avec des reponses simulees). A surveiller de pres.

- [ ] Saisir pseudo PSN + cle API via le portail captif, sauvegarder : la
      page confirme un redemarrage imminent, l'appareil redemarre vraiment
      quelques secondes plus tard (`configPatchRequiresRestart`).
- [ ] Apres redemarrage, le log affiche `mode Pocket PSN` (pas `mode demo`)
      -- confirme que `ProviderFactory::shouldUsePocketPsn()` a bien
      bascule le provider actif.
- [ ] La premiere synchronisation reelle reussit : `isVerified()` devient
      vrai, un vrai profil (pseudo, trophees) s'affiche -- pas les donnees
      de demonstration.
- [ ] Aucune erreur TLS/certificat (`transportOk=false`) -- si ca arrive,
      voir le TODO connu `setInsecure()` dans
      `src/network/PocketPsnHttpClient.cpp` (verification de certificat non
      implementee, cause probable si le comportement change cote serveur).
- [ ] La taille du corps recu (visible dans les logs
      `PocketPsnProvider: HTTP 200, ..., N octet(s) recus`) est proche de la
      valeur documentee (~691 octets, voir `docs/POCKETPSN_PROTOCOL.md`) --
      un ecart important indiquerait un changement de format cote serveur
      non encore pris en compte par `PocketPsnParser`.
- [ ] Couper le Wi-Fi puis le reconnecter (vraie coupure de quelques
      secondes, pas juste un blip) : une seule resynchronisation se
      declenche automatiquement apres le delai de stabilisation (~4s), pas
      de synchronisations en rafale (voir `SyncService::kReconnectStabilizationMs`).
- [ ] Saisir volontairement un pseudo ou une cle invalide : la synchronisation
      echoue proprement (`kErrorEmptyResponse`), sans crash, sans effacer un
      cache deja valide s'il y en avait un.

## 4. Cache hors ligne

- [ ] Apres au moins une synchronisation reussie, redemarrer l'appareil
      hors Wi-Fi (ou couper le Wi-Fi avant redemarrage) : les dernieres
      donnees connues s'affichent quand meme depuis le cache LittleFS
      (pas d'ecran vide/de crash).
- [ ] `GET /api/diagnostics` confirme `hasCachedData: true` et un
      `lastCacheFetchEpoch` coherent avec la derniere synchro reelle.
- [ ] Une reponse invalide/erreur serveur pendant une synchro ulterieure ne
      remplace jamais les donnees deja affichees (comportement deja
      verifie en simulateur -- a reconfirmer une fois sur materiel reel
      avec une vraie coupure/erreur, pas simulee).

## 5. Luminosite et veille

- [ ] Le reglage de luminosite (portail captif) change effectivement la
      luminosite physique de l'ecran (`Co5300BrightnessBackend`).
- [ ] Apres la moitie du delai de veille configure (`sleepTimeoutSeconds`),
      l'ecran s'assombrit (etat `kDimmed`).
- [ ] Apres le delai complet, l'ecran s'eteint/passe en veille profonde
      (`kAsleep`).
- [ ] Toucher l'ecran en veille le reveille immediatement
      (`notifyTouchActivity` -> `PowerManager::notifyActivity`).
- [ ] Mettre `sleepTimeoutSeconds` a 0 desactive bien la veille (ecran
      toujours allume).

## 6. Endpoints de diagnostic et de maintenance

- [ ] `GET /api/diagnostics` renvoie des valeurs plausibles pour
      `freeHeapBytes`, `minFreeHeapBytes`, `psramFreeBytes`,
      `littleFsUsedBytes`/`littleFsTotalBytes` -- comparer avec les chiffres
      de compilation (`pio run`) pour reperer une fuite memoire eventuelle
      lors d'un test longue duree (section 7).
- [ ] `POST /api/reboot` redemarre effectivement l'appareil apres la reponse
      HTTP (pas avant, pas jamais).
- [ ] `POST /api/reset` (avec `{"confirm":true}`) efface bien la
      configuration ET le cache, puis redemarre ; l'appareil revient au
      point d'acces de secours au demarrage suivant.

## 7. Stabilite longue duree (sur materiel, pas seulement en simulateur)

Le simulateur a deja valide 500 cycles logiques sans fuite ni incoherence
(`runLongDurationStressSelfTest`, voir `HANDOFF_PROGRESS.md`), mais jamais
avec un vrai radio Wi-Fi/HTTPS ni un vrai ecran AMOLED qui consomme de la
memoire graphique reelle. A verifier specifiquement sur materiel :

- [ ] Laisser tourner l'appareil au moins 24h avec des cycles reguliers de
      synchronisation (intervalle normal, pas force). Surveiller
      `freeHeapBytes`/`minFreeHeapBytes` via `GET /api/diagnostics` a
      intervalles reguliers : une baisse continue et non recuperee
      indiquerait une fuite memoire (le plus probable candidat : le chemin
      FreeRTOS de `SyncService`, jamais exerce avant le materiel reel --
      voir `HANDOFF_PROGRESS.md`, section `SyncService`).
- [ ] Provoquer plusieurs coupures/reconnexions Wi-Fi reelles pendant cette
      periode (pas seulement au demarrage) : aucun redemarrage inattendu,
      aucun blocage de l'UI.
- [ ] Confirmer qu'aucun `esp_reset_reason()` inattendu (watchdog, brownout,
      panic) n'apparait dans les logs au fil du test.

## 8. Apres validation complete

Une fois toutes les cases ci-dessus cochees sans probleme bloquant :
revenir au mandat original pour planifier la suite (integration
design/LVGL finale, puis eventuelle passe OTA -- non implementee
a ce stade, voir `docs/PARTITIONS.md`).
