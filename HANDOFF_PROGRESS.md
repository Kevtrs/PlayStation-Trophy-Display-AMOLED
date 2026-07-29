# HANDOFF_PROGRESS

Checkpoint cree le 2026-07-15, suite a une interruption pour limite
d'usage. Ce document est la reference precise pour reprendre le travail
exactement ou il s'est arrete. Voir aussi [`PROJECT_STATUS.md`](PROJECT_STATUS.md)
pour la vue d'ensemble et [`docs/IMPLEMENTATION_PLAN.md`](docs/IMPLEMENTATION_PLAN.md)
pour le plan complet en phases A-F.

## Mise a jour 2026-07-19 (diagnostic corps vide : cause probablement cote serveur)

Les tests reels avec la vraie cle renvoient tous `HTTP 200` + corps vide.
Toutes les causes cote client ont ete testees et eliminees (URL, slash,
Content-Type, User-Agent, encodage du corps, caracteres invisibles -- voir
`docs/POCKETPSN_PROTOCOL.md`). Recherche en ligne du projet d'origine
(`github.com/tomtechie/Playstation-Trophies-ESP-Display`) : la `config.json`
d'origine ne stocke **aucune cle** -> la cle est codee en dur dans le
binaire, **partagee par tous les utilisateurs** (celle de l'auteur). Notre
cle est donc nouvelle et distincte ; un corps vide est le symptome le plus
coherent avec une cle pas encore activee/autorisee cote backend Pocket PSN.

Seul test client-side restant (gratuit) : essayer le nom d'affichage
`N3X2R` au lieu de l'onlineId `gaz91610` comme `psn_name`. Sinon, la
resolution est cote proprietaire Pocket PSN.

**Message type a envoyer au proprietaire de Pocket PSN** (ne divulgue
jamais la cle) :

> Hi! Thanks again for the API key. I've wired it into my ESP32 build and
> tested the `POST https://api.pocketpsn.com/PSTrophyDisplay/` endpoint
> with `psn_name=<psn>&key=<the key you sent>`, Content-Type
> `application/x-www-form-urlencoded` -- exactly like the original
> firmware. The server always replies `HTTP 200`,
> `Content-Type: application/json`, but an **empty body**
> (Content-Length 0).
>
> I've narrowed it down: I tried three different `psn_name` values with the
> same key -- my own profile (publicly tracked on pocketpsn.com), a
> well-known tracked profile (PowerPyx), and a **made-up username that
> doesn't exist**. All three return the exact same empty body. Since even a
> non-existent username returns empty (instead of a "not on PocketPSN"
> style response), it looks like the request never gets past key
> validation. Could you check on your side whether this key is fully
> activated/enabled? Happy to share exact request details -- the key
> itself stays private. Thanks!

## Mise a jour 2026-07-21 (correction cosmetique : state ne reste plus a kOffline apres reconnexion)

Suite au constat annexe signale dans la mise a jour precedente : une fois
`isOffline` remis a `false` (Wi-Fi reconnecte), `SyncStatus.state` restait
parfois affiche a `kOffline` (herite du dernier `poll()` hors-ligne) tant
qu'aucune synchro n'avait effectivement redemarre -- laissant croire a
l'UI que l'app etait toujours hors-ligne.

Correction dans `SyncService::poll()` : des que `networkAvailable_` est de
nouveau vrai, si `state` valait encore `kOffline`, il repasse a `kIdle`
("reconnecte, en attente de synchronisation") -- ne declenche **aucune**
synchro, change uniquement la valeur affichee. Les etats issus d'une
vraie tentative (`kSuccess`/`kError`/etc.) ne sont jamais ecrases par
cette correction : seul `kOffline` est concerne.

3 nouvelles assertions --selftest verifient explicitement que `state`
devient `kIdle` (jamais `kOffline`) des la reconnexion, y compris pendant
toute la fenetre de stabilisation du debounce (voir mise a jour
precedente), et meme apres un echec anterieur (`consecutiveFailures`
inchange, aucune synchro forcee par ce changement d'etat).

**Verifie** : simulateur `--selftest` **180/180 PASS** (177 precedents +
3 nouveaux), firmware `pio run` SUCCESS (RAM 65.4%/214308 octets, Flash
70.5%/2958813 sur 4194304 octets -- +8 octets negligeables).

## Mise a jour 2026-07-21 (debounce de reconnexion Wi-Fi, SyncService)

Suite au constat du scenario 4 precedent : une reconnexion Wi-Fi apres une
vraie coupure declenche desormais **une synchro unique**, mais seulement
apres un **delai de stabilisation de 4 secondes** (`kReconnectStabilizationMs`,
`src/services/SyncService.h/.cpp`) de connexion continue -- demande
explicite de l'utilisateur, avec 3 contraintes :

- **Reconnexions rapides et repetees** : chaque coupure pendant la fenetre
  de stabilisation l'annule et fait repartir le delai a zero a la
  reconnexion suivante -- jamais plusieurs synchros sur des reconnexions
  en rafale (verifie : scenario "flapping" 4x en moins de 4s, aucune
  synchro tant que non stable).
- **Backoff jamais contourne** : si un echec est deja en cours
  (`consecutiveFailures > 0`), la reconnexion stabilisee ne force RIEN --
  seul le backoff existant (deja actif independamment) gere la reprise.
  Verifie explicitement : une reconnexion stabilisee pendant un backoff
  actif ne declenche aucune tentative avant l'expiration reelle du
  backoff.
- **Pas de boucle de tentatives** : si la synchro declenchee par la
  reconnexion echoue elle-meme, aucune tentative supplementaire immediate
  -- seul le backoff normal (deja existant) reprend ensuite.

Implementation : detection de la transition hors-ligne -> connecte dans
`SyncService::poll()` (nouveau, avant le traitement existant), arme un
drapeau consomme par le meme mecanisme de declenchement que
`requestManualSync()`/l'intervalle automatique -- aucune duplication de
logique de synchronisation. Toute synchro qui demarre (quelle que soit sa
cause) annule une stabilisation de reconnexion encore en attente, pour
eviter une synchro redondante quelques secondes plus tard.

Nouvelle fonction `runSyncServiceReconnectDebounceSelfTest()` (15
assertions) teste directement `SyncService` (sans AppController/WiFiManagerStub,
controle precis des millis simules) : reconnexion stable declenche une
synchro (cache affiche pendant toute l'attente), flapping rapide n'en
declenche aucune, backoff actif non contourne, pas de boucle sur un
nouvel echec.

**Constat annexe (non corrige, hors perimetre de cette demande)** : le
champ `SyncStatus.state` peut rester a `kOffline` (herite du dernier
`poll()` hors-ligne) meme une fois reconnecte, tant qu'aucune synchro n'a
effectivement redemarre -- `isOffline`, lui, est correctement remis a
`false`. Preexistant, indépendant de cette fonctionnalite.

**Verifie** : simulateur `--selftest` **177/177 PASS** (162 precedents +
15 nouveaux), firmware `pio run` SUCCESS (RAM 65.4%/214308 octets, Flash
70.5%/2958805 sur 4194304 octets -- +72 octets negligeables).

## Mise a jour 2026-07-21 (simulation longue duree du flux AppController complet)

Nouvelle fonction `runAppControllerLongRunSelfTest()` (`simulator/src/main.cpp`) :
construit de vraies instances d'`AppController` (WiFiManagerStub +
PocketPsnProvider/PocketPsnHttpClientStub + TrophyCache/TrophyRepository/
SyncService reels, `NullUiBridge` headless -- aucune dependance
LVGL/design, voir `ui/UiBridge.h`, interface pure) pour valider 7 scenarios
demandes explicitement par l'utilisateur, sans materiel :

1. Demarrage avec Internet -- synchro automatique des le premier tick une
   fois connecte (premiere synchro jamais soumise a l'intervalle complet).
2. Demarrage hors ligne avec cache -- donnees precedentes servies des
   `begin()`, avant toute tentative reseau.
3. Perte du Wi-Fi -- aucune nouvelle synchro tant que hors-ligne (le
   simulateur execute une synchro integralement en un seul `poll()`, sans
   tache de fond FreeRTOS : la coupure est donc testee entre deux
   tentatives, ce qui reste le comportement reellement observable ici).
4. Reconnexion -- **constat reel important** : `SyncService` ne relance
   PAS automatiquement de synchro sur la seule reconnexion
   (`everAttempted_` reste vrai apres la premiere synchro, et la coupure
   n'a jamais compte comme un echec donc aucun backoff ne s'applique) --
   seule une synchro manuelle ou l'intervalle complet (30 min par defaut)
   la relance. A signaler/discuter : faut-il forcer une tentative
   immediate a la reconnexion ? Non modifie pour l'instant (comportement
   existant, pas un bug introduit).
5. Reponse API invalide -- rejetee par `TrophyRepository::validate()`,
   cache existant jamais corrompu (verifie aussi apres relecture disque).
6. Plusieurs synchronisations successives -- 3 synchros manuelles
   enchainees, donnees et compteur mis a jour a chaque fois.
7. Redemarrage simule entre deux synchros -- nouvelle instance
   d'`AppController` sur les memes fichiers : donnees de la premiere
   synchro relues correctement (confirme le fix du 2026-07-21 ci-dessous),
   compteur de synchros reinitialise (non persiste, attendu), deuxieme
   synchro s'enchaine normalement.

Chaque scenario utilise un sous-dossier dedie de
`simulator/.simulator_data/` (deja ignore par Git). Aucun fichier de
`src/ui/` touche, aucune modification du design/LVGL (demande explicite de
l'utilisateur -- ce travail est reserve a une passe de design separee).

**Verifie** : simulateur `--selftest` **162/162 PASS** (135 precedents +
27 nouveaux), firmware `pio run` SUCCESS (RAM 65.4%/214308 octets, Flash
70.5%/2958733 sur 4194304 octets -- inchange, ce code est simulateur
uniquement).

## Mise a jour 2026-07-21 (pipeline complet valide sans materiel)

A la demande explicite de l'utilisateur : ne pas toucher au design/LVGL
(reserve a une passe de design separee), se concentrer sur l'appel API reel, le parser/gestion
d'erreurs, le cache local, les modes connecte/hors-ligne/reponse invalide,
la simulation Wi-Fi, et la stabilite.

**Bug reel trouve et corrige** : `TrophyCache::serializePayload()`/
`deserializePayload()` n'incluaient pas `displayName`/`avatarFileName`
(`ProfileData`) ni `trophiesPerDay` (`TrophyStats`) -- ces champs auraient
ete silencieusement perdus a chaque redemarrage (le cache les charge a
leur valeur par defaut). Corrige.

Nouvelle fonction `runPocketPsnIntegrationSelfTest()` (20 assertions,
`simulator/src/main.cpp`) validant le pipeline complet
`PocketPsnProvider -> TrophyRepository -> TrophyCache -> SyncService` sans
materiel, avec la vraie fixture (`test/fixtures/pocketpsn_response_real.json`) :
1. Succes + persistance a travers un "redemarrage" simule (nouvelle
   instance de `TrophyCache` sur le meme fichier) -- confirme que le bug
   ci-dessus est bien corrige.
2. Reponse invalide (incoherence trophees) rejetee par
   `TrophyRepository::validate()` sans corrompre un cache valide existant.
3. Panne de transport reseau : cache existant toujours servi, erreur
   distincte remontee.
4. Mode hors-ligne via `SyncService::setNetworkAvailable(false)` : aucune
   tentative reseau, dernieres donnees en cache toujours servies.
5. Stabilite : 30 cycles repetes parse+cache, resultats identiques a
   chaque fois.

Repertoire de test dedie (`simulator/.simulator_data/selftest_pocketpsn/`,
sous-dossier deja ignore par Git) pour ne jamais interferer avec le cache
demo de l'execution normale.

Aucun fichier de `src/ui/` touche. Aucune modification du design/LVGL.

**Verifie** : simulateur `--selftest` **135/135 PASS** (115 precedents +
20 nouveaux), firmware `pio run` SUCCESS (RAM 65.4%/214308 octets, Flash
70.5%/2958733 sur 4194304 octets -- +244 octets negligeables).

## Mise a jour 2026-07-18 (cle Pocket PSN officielle obtenue, abandon NPSSO)

L'utilisateur a obtenu une vraie cle API Pocket PSN, privee et legitime,
directement aupres du proprietaire du site (voir `AUDIT.md` section 0ter
pour le contexte complet). Consequence : la piste Sony NPSSO est abandonnee
et supprimee du depot -- `docs/NPSSO_VS_POCKETPSN.md`,
`tools/psn_official_probe/` (entier), et `test/fixtures/psn_official_*.json`
retires (recuperables via l'historique git si jamais necessaire).

**Refactor implemente et verifie** (meme session) : `PocketPsnProvider` est
maintenant portable via `IPocketPsnHttpClient` (meme pattern que
`IWiFiManager`) -- `src/network/IPocketPsnHttpClient.h`,
`src/network/PocketPsnHttpClient.{h,cpp}` (firmware), `simulator/src/PocketPsnHttpClientStub.{h,cpp}`
(simulateur, reponses en file d'attente + `lastUrl()`/`lastBody()` pour les
assertions). `isVerified()` n'est plus hardcode a `false` : il devient
`true` (et le reste, semantique "sticky") uniquement apres un vrai succes
de parsing avec pseudo non vide -- jamais dans `--selftest`, qui n'utilise
que des fixtures synthetiques explicitement documentees comme telles.

`src/data/ProviderFactory.{h,cpp}` (`shouldUsePocketPsn()`, decision pure)
selectionne le provider actif **au demarrage uniquement** :
`simulator/src/main.cpp` charge une config anticipee avant de construire
`AppController` (mecanique, `main()` est une vraie fonction) ; `src/main.cpp`
(firmware) a du passer `appController`/`captivePortalServer` de globaux a
des pointeurs alloues dans `setup()` (seul changement non mecanique du
plan, `AppController` avait besoin de la config, chargeable seulement au
debut de `setup()`).

Flux "sauvegarde de la cle -> redemarrage" : `WebApiHandlers::configPatchRequiresRestart()`
(nouvelle fonction pure) + `CaptivePortalServer::handleConfigPost()` (reutilise
`PendingRestart`/`restartTimer_`, meme mecanisme que `/api/reboot`/`/api/reset`) ;
`data/index.html`/`data/app.js` : nouveau champ cle masque (jamais prerempli,
statut "configuree/non configuree" uniquement), credit Pocket PSN avec lien
visible (condition du proprietaire, voir section 0ter), message de
redemarrage affiche 10s (pas un toast auto-dismiss classique).

La cle elle-meme n'a jamais ete committee/journalisee/affichee ; aucun
appel reseau reel n'a ete fait (elle n'est pas presente dans cet
environnement). `PocketPsnHtmlParser`/`tools/pocketpsn_public_probe/`
(scraping HTML public) conserves tels quels comme repli isole, non charge
par defaut.

**Verifie** : simulateur `--selftest` 88/88 PASS (68 precedents + 20
nouveaux : PocketPsnProvider succes/erreurs/isVerified sticky/construction
de requete, ProviderFactory, configPatchRequiresRestart), `pio run` SUCCESS
(RAM 65.4%/214260 octets, Flash 70.3%/2949913 sur 4194304 octets -- +3.4
points de Flash, attendu : PocketPsnProvider est maintenant reellement
lie/appele, plus seulement compile sans etre reference), `node --check
data/app.js` OK.

**Prochaine tache** : validation reelle par l'utilisateur en local (jamais
dans ce depot) avec la vraie cle -- corriger le parsing/l'endpoint si la
vraie reponse differe des hypotheses de `docs/POCKETPSN_PROTOCOL.md`.

## Mise a jour 2026-07-16 (etude NPSSO reelle, apres commit 63027e9)

Test reel effectue avec le NPSSO du compte propre de l'utilisateur (fourni
via un fichier local jamais commite, jamais affiche en clair). Nouvel
outil `tools/psn_official_probe/` : flux OAuth complet
(NPSSO -> code -> access_token/refresh_token), jamais de jeton en clair
dans les logs (`mask()`), renouvellement via `refresh_token` teste avec
succes (`--refresh-token-file`).

**Bug reel trouve et corrige pendant cette passe** : la premiere version
de l'anonymisation ne remplacait que `onlineId` dans les fixtures ; la
vraie reponse `/profile` contient aussi `personalDetail.firstName/lastName`
(prenom/nom reels du compte) et `aboutMe`, qui ont fuite dans la premiere
fixture generee avant d'etre repere et corrige (`scrub_sensitive_fields()`,
recursif, remplace desormais `onlineId`/`firstName`/`lastName`/`aboutMe`/
`accountId` a n'importe quelle profondeur). 25 tests (`test_probe.py`)
verifient qu'aucune des vraies valeurs ne survit, y compris sur les
fixtures reellement committees dans `test/fixtures/psn_official_*.json`.

**Corrections reelles apportees a `docs/NPSSO_VS_POCKETPSN.md`** (les
hypotheses initiales, jamais testees, etaient partiellement fausses) :
- `/users/me/profiles` echoue (HTTP 400) ; il faut le vrai `accountId`
  (obtenu via `trophySummary`) dans le chemin.
- Champ de niveau reel : `trophyLevel`, pas `level`.
- `refresh_token` valable ~10 jours (`refresh_token_expires_in`=863999s),
  pas ~2 mois comme suppose.
- `trophyTitles` est paginee (377 jeux pour ce compte, 100 par appel) --
  non geree dans cette premiere passe.
- `World Rank`/`Country Rank`/`Pocket Points`/`Hours Played` confirmes
  absents des 3 endpoints testes (`profile`, `trophySummary`,
  `trophyTitles`).

Aucune modification du firmware ni du design. `PocketPsnProvider` et
`/api/profile/test` non touches.

## Mise a jour 2026-07-16 (parser HTML public Pocket PSN, valide sur PC uniquement)

Sur demande explicite : aucune tentative d'extraction/reutilisation de la
cle privee du firmware original, aucun contournement de Cloudflare.
Recherche publique : `tools/pocketpsn_public_probe/` (deja commite,
`b24fc2d`) confirme que `pocketpsn.com` entier renvoie un defi Cloudflare
(403) a tout client HTTP simple -- 0 donnee recuperable par cette voie
automatisee.

L'utilisateur a ensuite fourni manuellement le HTML reel de sa propre page
de profil (obtenu dans son navigateur, apres passage normal du defi),
confirmant que les statistiques sont rendues cote serveur. Nettoye de
toute donnee de compte (email reel, session, ID forum/Stripe) avant tout
commit -- voir `test/fixtures/pocketpsn_profile_public.html` (ne garde que
les blocs necessaires a l'extraction, pseudo/displayName anonymises).

Nouveau : `src/data/PocketPsnHtmlParser.h/.cpp` (portable, pur, sans
dependance HTTP/Arduino) extrait 18 champs par libelle semantique (pas par
position ordinale de balise), avec validations croisees (total =
platine+or+argent+bronze, progression 0-100, coherence avec la balise meta
description) et detection explicite `CLOUDFLARE_CHALLENGE`. `ProfileData`
gagne `displayName`, `TrophyStats` gagne `trophiesPerDay` (champs absents
du JSON prive, presents uniquement sur cette page publique).

36 assertions ajoutees au `--selftest` (fixture reelle, HTML tronque,
profil introuvable, page Cloudflare, nombres avec separateur espace, champ
manquant, incoherence du total, variante d'espaces/retours a la ligne) --
toutes PASS. Un bug reel a ete trouve et corrige pendant cette passe :
`findLabelValue()` utilisait une capture non-gourmande `[\s\S]*?` pour le
corps du `<h4>`, ce qui lui permettait de s'etendre par erreur depuis un
`<h4>` non lie plus haut dans la page (celui du pseudo) jusqu'au libelle
cible, avalant tout le HTML intermediaire ; corrige avec `[^<]*` (ces
cellules ne contiennent jamais de balise imbriquee).

**Non fait dans cette passe (sur demande explicite)** : le
`PocketPsnProvider` existant n'est pas touche, aucun provider alternatif
n'est branche a `AppController`/`TrophyDataProvider`, le design n'est pas
touche. `/api/profile/test` reste a raison en `501 not_implemented`.

Etude separee ajoutee : `docs/NPSSO_VS_POCKETPSN.md` (comparaison des
champs disponibles via l'API PSN officielle par NPSSO vs Pocket PSN,
aucun code ni requete reelle -- juste une etude documentaire).

Verifie : simulateur SUCCESS, `--selftest` 68/68 PASS (32 precedents +
36 nouveaux), firmware `pio run` SUCCESS (RAM 65.5%/214776 octets, Flash
66.9%/2804661 sur 4194304 octets -- coherent avec le palier precedent,
aucune regression materielle malgre le nouveau fichier portable).

**Prochaine tache** : tests automatises generaux (tache #28), ou
poursuite Pocket PSN si une cle privee legitime est un jour obtenue.

## Mise a jour 2026-07-16 (ecart de noms diagnostics resolu, apres commit 58173d4)

`data/app.js` adapte pour lire les 22 champs canoniques de
`DiagnosticsSnapshot` (aucun alias C++, comme demande). Verification
statique ajoutee au `--selftest` : lit le vrai `data/app.js`, extrait les
champs `diagnostics.xxx` references, verifie leur presence dans la vraie
reponse JSON -- pas de liste codee en dur. `node --check data/app.js` OK.
Aucune logique C++ modifiee -- Flash/RAM inchanges (65.6%/65.4%).

Verifie : simulateur SUCCESS, `--selftest` 32/32 PASS, firmware
`pio run` SUCCESS, `pio run -t buildfs` SUCCESS, 6 ecrans re-inspectes.

**Prochaine tache** : Pocket PSN Probe (obtenir une vraie reponse +
schema confirme pour enfin lever `/api/profile/test` de `not_implemented`),
ou tests automatises (tache #28).

## Mise a jour 2026-07-15 (sync/reboot/reset/diagnostics reels, apres commit 14f1df5)

`/api/sync` (via `AppController::requestManualSync()`, refuse 409 si deja
en cours), `/api/reboot` (repond puis programme un redemarrage 800ms plus
tard via `PendingRestart`, nouvelle classe portable), `/api/reset` (exige
`{"confirm":true}`, `AppController::factoryReset()` efface config+cache
puis redemarre), `/api/diagnostics` (`DiagnosticsSnapshot`, champs exacts
dictes par l'utilisateur, jamais de mot de passe Wi-Fi) sont maintenant
fonctionnels. `/api/profile/test` reste en 501 `not_implemented` comme
demande, jusqu'a Pocket PSN Probe.

**Ecart reel decouvert** (voir `docs/WEB_UI_GAP_ANALYSIS.md`, section
"Ecart de noms de champs diagnostics") : les noms de champs diagnostics
dictes par l'utilisateur ne correspondent pas a ceux lus par
`data/app.js` (`renderDiagnostics()` lit `firmware`/`uptime`/`heapFree`/...,
pas `firmwareVersion`/`uptimeSeconds`/`freeHeapBytes`/...). Verifie que ca
ne casse rien (affichage `-`, pas de crash, JSON brut correct) mais les
lignes "amicales" du panneau diagnostics resteront vides. **A trancher
par l'utilisateur** : adapter `app.js` ou dupliquer les champs cote C++.

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.6%), simulateur SUCCESS,
`--selftest` 30/30 PASS (14 nouvelles assertions), 6 ecrans re-inspectes,
`pio run -t buildfs` SUCCESS.

**Prochaine tache** : selon priorite utilisateur -- trancher l'ecart de
noms diagnostics, continuer Pocket PSN Probe, ou passer aux tests
automatises (tache #28). Non teste faute de materiel : `ESP.restart()`
reel, lecture heap/PSRAM/LittleFS reelle, parcours reboot/reset via un
vrai navigateur.

## Mise a jour 2026-07-15 (module Web UI integre, apres commit 71527ac)

Le fichier `WEB_UI_CONTRACT.md` fourni ne contenait pas de contrat (voir
`docs/WEB_UI_GAP_ANALYSIS.md`) -- reconstruit par lecture de `app.js`.
Module reel (`data/index.html`/`app.js`/`styles.css`) retrouve dans
l'archive et integre sans y toucher (design intact). `GET /api/status`
reecrite pour matcher le contrat exact ; `GET`/`POST /api/config` ajoutees
(traduction de champs dans `WebApiHandlers`, logique metier intacte dans
`ConfigManager`/`AppController`) ; 5 routes (`profile/test`, `sync`,
`reboot`, `reset`, `diagnostics`) renvoient `501 not_implemented` de
maniere honnete, sans casser le boot de la page (verifie que
`refreshDiagnostics()` avale sa propre erreur). Ancienne page de secours
remplacee par les vrais fichiers du module Web UI ; une version reduite est conservee
**dans le binaire** comme repli technique reel si LittleFS n'a pas encore
ete televerse.

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.5%), simulateur SUCCESS,
`--selftest` 15/15 PASS, 6 ecrans re-inspectes, `pio run -t buildfs`
SUCCESS (image LittleFS 8 323 072 octets, contenu reel 32 878 octets).

**Prochaine tache** : selon priorite utilisateur -- soit brancher
`/api/sync`/`/api/reboot`/`/api/reset` pour de vrai (ils ne dependent pas
de Pocket PSN), soit continuer Pocket PSN, soit passer aux tests
automatises (tache #28). Non teste faute de materiel : le vrai portail
servi a un navigateur, `pio run -t uploadfs`, le repli embarque.

## Mise a jour 2026-07-15 (Phase B suite -- partitions + portail captif, apres commit f0df8a1)

**BLOQUANT A RESOUDRE EN PREMIER A LA PROCHAINE REPRISE** :
`PlayStation-Trophy-Display-Web-UI-Module.zip` et `WEB_UI_CONTRACT.md`,
demandes par l'utilisateur pour integrer l'UI Web, sont introuvables
dans cet environnement (recherche effectuee sur tout le disque, y compris
`Downloads/` et `Desktop/`). Une page de secours minimale et volontairement
non designee (`data/index.html`, `data/app.js`) a ete ecrite a la place
pour que le parcours Wi-Fi reste testable de bout en bout. **Ne pas
supposer le contenu de `WEB_UI_CONTRACT.md`** -- demander le fichier avant
de continuer l'integration de l'UI Web.

Travail effectue cette passe :
1. **Partitions** : `app0`/`app1` 3 Mo -> 4 Mo (usage retombe de 85.2% a
   63.9%, voir `docs/PARTITIONS.md` pour le detail complet et le compromis
   avec/sans OTA -- OTA conserve, 2 slots). Verification reelle (pas
   seulement lecture de config) que PlatformIO utilise bien 16 Mo : `pio
   run -v` montre `--flash_size 16MB` passe explicitement a
   `esptool.py elf2image`.
2. **Scan Wi-Fi non bloquant** ajoute a `IWiFiManager`/`WiFiManager`
   (`WiFi.scanNetworks(true)`, necessite `WIFI_AP_STA` pas `WIFI_AP` seul)
   /`WiFiManagerStub` (reseaux fictifs).
3. **`WebApiHandlers`** (`src/web/`, portable) : JSON pour
   `/api/status`, `/api/wifi/scan`, `/api/wifi/connect` -- 5 tests
   `--selftest` supplementaires (12 au total).
4. **`CaptivePortalServer`** (firmware, `WebServer.h`+`DNSServer.h`) : DNS
   captif actif seulement en mode AP, redirection 302 vers 192.168.4.1,
   4 routes (`GET /api/status`, `GET /api/wifi/scan`,
   `POST /api/wifi/connect`, `POST /api/wifi/forget`).
5. **Bug reel corrige** : pas de route pour `/` -> boucle de redirection
   infinie en mode AP. Corrige en servant `data/index.html`/`app.js`
   depuis LittleFS (`serveStatic`).

Verifie : `pio run` SUCCESS (RAM 65.4%, Flash 65.2%), simulateur SUCCESS,
6 ecrans re-inspectes (pas de regression), `--selftest` 12/12 PASS.

**Non teste faute de materiel** : portail captif reel (DNS hijacking,
`serveStatic` effectif, necessite aussi `pio run -t uploadfs` jamais
execute ici), tout le parcours "TrophyDisplay-Setup -> page -> scan ->
connexion" sur un vrai telephone/PC.

## Mise a jour 2026-07-15 (Phase B Wi-Fi, apres commit 746d102)

WiFiManager termine (voir `PROJECT_STATUS.md` section "Phase B -- Wi-Fi"
pour le detail) : `IWiFiManager` (interface), `WiFiManager` (firmware,
`WiFi.h` reel, machine a etats station/AP/reconnexion/backoff),
`WiFiManagerStub` (simulateur). `AppController` lit desormais
`IWiFiManager::status()` a chaque `tick()` ; `setNetworkStatus()` a ete
supprime (remplace par la source de verite unique du WiFiManager).

Un bug de la meme famille que le checkpoint precedent a ete trouve et
corrige : la connexion Wi-Fi de demonstration du simulateur pokait le
stub directement (bypass de `ConfigManager`), et se faisait ecraser par
le premier `debugApplySettings()` venu (qui relit toujours
`config_.settings().wifiSsid/wifiPassword`). Corrige en routant la
configuration Wi-Fi de demo par `AppController::debugApplySettings()`.

**Verifie** : `pio run` SUCCESS (RAM 65.1%, Flash 85.2% -- +11 points de
Flash du fait de la pile Wi-Fi, a surveiller), simulateur SUCCESS, les 6
ecrans + toast re-inspectes visuellement (donnees demo toujours
correctes, Wi-Fi affiche "Connecte"), `--selftest` etendu a 7 assertions
(reglages, 3 etats Wi-Fi instantanes, transition asynchrone
Connecting->Connected, donnees de debug) -- toutes passantes.

**Non fait** (a la demande explicite de l'utilisateur) : portail captif,
serveur web/API, WiFiManager jamais teste sur materiel reel (pas de
carte). Prochaine tache : portail captif + serveur web (Phase B suite,
tache #27), routes `GET/POST /api/wifi/*`.

## Mise a jour 2026-07-15 (checkpoint suivant, apres commit e25866a)

Correction du bug DebugPanel/AppController documente ci-dessous (section
"Ce qui est partiel" -- desormais resolu) : le panneau de debug du
simulateur passe maintenant exclusivement par des methodes dediees
d'`AppController` (`debugApplySettings()`, `debugSetDisplayedData()`,
`setNetworkStatus()`) au lieu d'ecrire directement dans `UiManager::state()`,
qui etait ecrase au tick suivant.

En verifiant ce correctif par un export reel de captures d'ecran, **deux
autres bugs reels distincts ont ete trouves et corriges** (non lies au
DebugPanel, mais bloquant la verification visuelle demandee) :

1. **`SyncService` ne declenchait jamais la toute premiere synchronisation** :
   la condition d'intervalle comparait `nowMillis - lastAttemptMillis_`
   (tous deux ~0 au demarrage) a `syncIntervalMinutes_ * 60000` (30 min par
   defaut) -- resultat, aucune synchronisation n'etait jamais tentee avant
   qu'un intervalle complet ne se soit ecoule depuis le boot, sur firmware
   comme sur simulateur. Corrige par un drapeau `everAttempted_` qui force
   le tout premier essai des que le reseau est disponible (voir
   `src/services/SyncService.h/.cpp`).
2. **Le panneau de debug/outil d'export ecrasait ses propres captures** :
   `AppController::tick()` effacait le drapeau `debugOverrideActive_` des
   qu'il *lisait* `SyncState::kSuccess`, pas seulement lors d'une nouvelle
   reussite -- un override de debug positionne apres une synchronisation
   deja reussie etait donc annule des le tick suivant. Corrige par une
   detection de front (comparaison a l'etat precedent) plutot qu'un test de
   niveau (voir `AppController::tick()`).
3. **`exportScreenshots()` (outil de capture du simulateur) etait corrompu
   par la rotation automatique et le retour automatique depuis l'ecran
   Sync** : forcer rapidement les 6 ecrans declenchait ces navigations
   automatiques au milieu d'une capture ulterieure (ex: "06-settings.png"
   affichait en realite le Dashboard). Corrige en desactivant la rotation
   auto pendant l'export (via `debugApplySettings`) et en ajoutant
   `UiManager::cancelPendingSyncReturn()`, appele apres chaque capture.

**Verifie reellement dans cette session** : `pio run` (SUCCESS, RAM 58.6%,
Flash 74.5%), compilation + execution simulateur, les 6 ecrans + toast
nouveau trophee captures et inspectes visuellement un par un (donnees demo
Kevin_Trophies/niveau 327/58 platine correctement affichees partout), et un
test automatise `--selftest` (nouveau, voir `simulator/src/main.cpp`) qui
verifie programmatiquement que les 3 voies d'ecriture du panneau de debug
survivent a 5 `tick()` consecutifs -- les 3 assertions passent.

**Prochaine tache exacte a reprendre** (inchangee sinon) : Phase B --
`WiFiManager` reel (firmware) + stub simule complet dans le simulateur, non
commence, voir section correspondante plus bas.

## Dernier commit

Voir `git log -1` au moment de la lecture de ce document -- ce fichier est
commite dans le meme commit que le code qu'il decrit (message de commit :
"Squelette fonctionnel (Phase A partielle) : modele de donnees, config,
cache, sync, AppController"). Executer `git log --oneline -5` pour
confirmer la position exacte dans l'historique.

## Contexte : pourquoi ce chantier existe

L'utilisateur a explicitement demande d'arreter tout travail de design
(apparence, assets, trophees, icones, animations, composition d'ecran) et de
terminer le **squelette fonctionnel** de l'application (architecture en
couches, modele de donnees central, config/cache persistants, services de
synchronisation, Pocket PSN reel, Wi-Fi, portail captif, tests) afin qu'un
futur design definitif puisse s'y brancher sans reecrire la logique.
L'interface LVGL actuelle est une **couche de test temporaire uniquement**.

Methode demandee : Phase A (fondations) -> B (reseau) -> C (Pocket PSN) ->
D (services) -> E (simulateur) -> F (finalisation/OTA/docs), avec
compilation firmware + simulateur + tests + mise a jour de
`PROJECT_STATUS.md` apres chaque phase.

**Etat reel : Phase A tres avancee mais pas terminee (WiFiManager de la
Phase B pas commence). Interruption forcee par une limite d'usage.**

## Ce qui compile (verifie reellement dans cette session)

- **Firmware ESP32-S3** : `pio run` -> `SUCCESS`. RAM 58.6% (191912/327680
  octets), Flash 74.5% (2342281/3145728 octets, partition 3 Mo actuelle --
  voir "Ce qui reste absent", partitions pas encore agrandies).
- **Simulateur PC** : `cmake --build` (toolchain zig/ninja) -> succes,
  `trophy-display-simulator.exe` lie sans erreur.

Aucune des deux commandes n'avait ete executee depuis le debut du
refactoring de structures de donnees -- c'est la premiere verification
reelle de tout ce travail.

## Ce qui est teste

- **Compilation reelle** des deux cibles (firmware + simulateur), listee
  ci-dessus.
- **Rien d'autre.** Aucun test automatise (`test/fixtures/` est vide),
  aucune execution du simulateur au-dela de la compilation/link dans cette
  session (l'ancien flux d'export de captures `simulator/run.ps1` n'a pas
  ete relance -- a faire en premiere action de la prochaine session pour
  confirmer que l'UI s'affiche toujours correctement avec le nouvel
  `AppController`).
- Aucun test materiel (pas de carte physique disponible dans cet
  environnement).

## Ce qui est partiel

- **`TimeService` : PARTIAL.** Implemente (NTP via `configTzTime` sur
  firmware, horloge systeme sur desktop, table de conversion nom IANA ->
  chaine POSIX TZ limitee a une poignee de fuseaux courants). **Jamais
  exerce sur materiel reel** ni couvert par un test automatise dedie.
  `formatRelative()`/`formatClock()` sont des fonctions statiques pures
  (donc facilement testables) mais aucun test n'existe encore.
- **`SyncService` (chemin FreeRTOS)** : la tache de fond dediee a l'appel
  HTTPS bloquant de `PocketPsnProvider` compile mais n'a jamais tourne sur
  un ESP32 reel (pas de carte disponible) -- la logique de synchronisation
  cross-thread (drapeau atomique `taskDone_`) est correcte par construction
  (analyse manuelle) mais non verifiee a l'execution.
  Voir [`src/services/SyncService.h`](src/services/SyncService.h) (commentaire
  en tete de fichier) pour le detail de la garantie de thread-safety.
- ~~**Simulateur : integration `AppController`/`DebugPanel`**~~ **RESOLU**
  (voir section "Mise a jour 2026-07-15" en tete de document) : le panneau
  de debug passe desormais par `AppController::debugApplySettings()`/
  `debugSetDisplayedData()`/`setNetworkStatus()` -- plus aucune ecriture
  directe dans `UiManager::state()`. Verifie par un test automatise
  (`--selftest`) et par inspection visuelle des 6 ecrans.
- **`PowerManager`** : logique complete (etats Awake/Dimmed/Asleep,
  luminosite reelle via `Co5300BrightnessBackend::setBrightness()` sur
  firmware, aucun effet visuel sur simulateur par choix explicite -- design
  hors perimetre) mais jamais verifie sur materiel reel.

## Ce qui reste absent (non commence, conforme a la consigne "stop apres TimeService")

- **`WiFiManager` reel** (firmware) : absent. `AppController::setNetworkStatus()`
  existe et fonctionne mais rien n'appelle Wi-Fi/`WiFi.h` reellement --
  seul le simulateur simule la connexion (touche D) et `main.cpp` firmware
  ne configure aucun reseau.
- **Portail captif / serveur web / routes API** (`GET /api/status`, etc.) :
  absent. Dossiers `src/web/` et `data/` crees mais vides.
- **Tests automatises + fixtures** : absent. `test/fixtures/` vide.
- **Partitions dediees** (au-dela des 3 Mo actuels de la partition `app`) :
  absent, `partitions.csv` inchange.
- **OTA** : absent (attendu, doit venir apres le reste).
- **Fusion du design final** : explicitement hors perimetre, non touchee.
- **Documentation** (`docs/NETWORK.md`, `docs/POCKET_PSN.md`,
  `docs/CONFIGURATION.md`, `docs/CACHE.md`, `docs/WEB_UI.md`,
  `docs/TESTING.md`, `docs/HARDWARE_TEST_PLAN.md`,
  `docs/TROUBLESHOOTING.md`) : absente, seul `docs/IMPLEMENTATION_PLAN.md`
  existe.
- **Pocket PSN reellement valide** : toujours bloque par l'absence de cle
  API legitime (inchange depuis la Phase 2.5, voir `AUDIT.md`).

## Prochaine tache exacte a reprendre

Dans cet ordre :

1. **Corriger le flux DebugPanel/AppController dans le simulateur** (voir
   "Ce qui est partiel" ci-dessus) : soit exposer un accesseur mutable sur
   `AppController` pour les reglages de test (ex: `updateSettingsForTesting()`
   qui passe par `ConfigManager` + republie l'etat), soit river le panneau
   de debug a `appController.config()` directement plutot qu'a
   `uiManager.state()`. Fichier concerne : [`simulator/src/DebugPanel.cpp`](simulator/src/DebugPanel.cpp),
   [`src/app/AppController.h/.cpp`](src/app/AppController.h).
2. **Relancer `simulator/run.ps1`** pour verifier visuellement que les 6
   ecrans + le panneau de debug fonctionnent toujours correctement avec le
   nouvel `AppController` (derniere verification visuelle date d'avant ce
   refactoring).
3. **Phase B : `WiFiManager`** (firmware) + stub simule complet dans le
   simulateur (au-dela de la simple touche D actuelle), evenements
   Connecting/Connected/Disconnected/AccessPoint/Error, jamais bloquant.
4. Puis Phase B suite (portail captif + serveur web + routes API), Phase C
   (test Pocket PSN reel des qu'une cle est disponible), Phase F (tests
   automatises + fixtures + documentation + partitions + PROJECT_STATUS
   final).

Task list interne (voir outil de suivi de taches) : #1-#24 termines, #25
("AppController + refactor UiManager") **in_progress** (essentiellement
fait, cf. point 1 ci-dessus pour le finir), #26-#29 pending.

## Erreurs connues (non bloquantes, a garder en tete)

- Voir "Ce qui est partiel" : DebugPanel vs AppController (state ecrase).
- `TrophyRepository::validate()` rejette toute reponse Pocket PSN ou le
  total de trophees diminue par rapport au cache -- cela inclut un vrai
  reset de compte PSN (indistinguable d'une erreur serveur avec les
  informations actuellement disponibles). Documente dans le code
  ([`src/data/TrophyRepository.cpp`](src/data/TrophyRepository.cpp)),
  accepte comme compromis volontaire.
- `platformio.ini` force desormais `-std=gnu++17` (`build_unflags`/
  `build_flags`) -- necessaire pour que `AppError{code, message}`
  s'agregat-initialise correctement (echouait sous le gnu++11 par defaut du
  framework arduino-esp32 : erreur de compilation reelle rencontree et
  corrigee dans cette session).
- Flash firmware a 74.5% (partition app actuelle 3 Mo) -- pas encore de
  nouvelle table de partitions ; a surveiller si Wi-Fi/portail web
  ajoutent du code (tache #29).

## Commandes de build

```bash
# Firmware (depuis la racine du depot)
python -m platformio run

# Simulateur PC (PowerShell, installe automatiquement zig/cmake/ninja au
# premier lancement via pip)
simulator/build.ps1
# ou, pour compiler + lancer :
simulator/run.ps1
```

## Dependances ajoutees dans cette session

- **`simulator/third_party/ArduinoJson/ArduinoJson.h`** : en-tete unique
  ArduinoJson v7.4.3 vendorise (meme version que celle resolue par
  PlatformIO pour le firmware, `bblanchon/ArduinoJson @ ^7.1.0`), pour que
  `ConfigManager`/`TrophyCache`/`PocketPsnParser` (code portable) compilent
  a l'identique sur le simulateur. Licence MIT, voir
  `simulator/third_party/NOTICE.md`.
- Aucune autre dependance externe ajoutee (pas de nouvelle librairie
  PlatformIO, pas de nouveau paquet Python).

## Decisions d'architecture prises dans cette session

1. **Separation stricte donnees/logique vs UI** via deux interfaces
   symetriques : [`UiBridge`](src/ui/UiBridge.h) (AppController -> UI :
   `setAppState`, `showSyncState`, `showTrophyDelta`, `showError`) et
   [`UiActionListener`](src/ui/UiActionListener.h) (UI -> AppController :
   `onRequestManualSync`). `UiManager` implemente `UiBridge` ;
   `AppController` implemente `UiActionListener`. Aucun ecran LVGL ne touche
   plus jamais HTTP/NVS/WiFi/LittleFS.
2. **Persistance portable** via `IPersistentStore` (interface a 3 methodes
   `load/save/remove`), deux implementations : `FilePersistentStore`
   (desktop, `std::filesystem`, exclu du firmware via
   `platformio.ini:build_src_filter`) et `NvsPersistentStore` (firmware,
   LittleFS, exclu du simulateur car absent du glob CMake). `ConfigManager`
   et `TrophyCache` partagent la meme instance de store via des cles
   differentes (`"config"` vs `"trophy_cache"`).
3. **Parsing Pocket PSN extrait en fonction pure** (`PocketPsnParser::parse`,
   namespace, aucune dependance Arduino/HTTPClient) specifiquement pour
   pouvoir etre teste sans materiel (Phase F, pas encore fait) tout en
   restant utilise par le vrai `PocketPsnProvider` (pas de duplication de
   logique).
4. **`Logger` rendu portable** : l'ancienne version dependait de
   `<Arduino.h>` (`Serial.printf`, `millis()`), ce qui cassait la
   compilation du simulateur des que du code partage (`ConfigManager`)
   l'incluait. Desormais `#ifdef ARDUINO` bascule entre `Serial`/`millis()`
   et `stderr`/horloge monotone `std::chrono` ; l'interface publique est
   inchangee.
5. **`SyncService` : tache FreeRTOS dediee** pour l'appel HTTPS bloquant de
   `PocketPsnProvider::requestRefresh()`, avec un seul point de
   synchronisation cross-thread (`std::atomic<bool> taskDone_`,
   release/acquire) : la tache de fond ne touche jamais
   `TrophyRepository`/`AppState` (qui restent mono-thread, lus/ecrits
   uniquement depuis la boucle principale). Sur simulateur/desktop, le
   provider demo etant synchrone, aucune tache n'est necessaire (chemin
   `#else`).
6. **`pocketPsnVerified` ne peut jamais etre force** : ni par
   `SyncService` (qui delegue a `TrophyRepository::isProviderVerified()` ->
   `TrophyDataProvider::isVerified()`), ni par le panneau de debug du
   simulateur (bouton retire volontairement) -- respecte la regle du projet
   "ne jamais presenter Pocket PSN comme fonctionnel sans test reel reussi".
7. **`-std=gnu++17` force** dans `platformio.ini` (voir "Erreurs connues").
8. **Compromis documente sur `PowerManager`** : la demande initiale
   mentionnait "reproduire visuellement l'assombrissement" sur simulateur,
   ce qui contredit l'interdiction explicite de travail de design de ce
   meme message. Choix retenu : implementer uniquement la logique
   (luminosite/etats Awake-Dimmed-Asleep) via `IBrightnessBackend`, sans
   ajouter d'effet visuel sur simulateur (`NullBrightnessBackend` ne fait
   qu'enregistrer la valeur).

## Etat precis des composants demandes

### `ProfileData` (`src/data/ProfileData.h`)
Struct simple : `username`, `country`, `level`, `levelProgressPercent`,
`levelRemainingPoints`, `hasPsPlus`. Stable, compile des deux cotes. Aucun
test dedie.

### `TrophyStats` (`src/data/TrophyStats.h`)
Struct simple, uniquement stats de trophees/jeux (platinum/gold/silver/
bronze/totalTrophies/trophyPoints/pocketPoints/totalGames/worldRank/
nationalRank/gamesCompleted/completionRatePercent/averageRarityPercent/
unearnedTrophies/playtimeHours). **Breaking change** vs l'ancienne version
(qui melangeait profil + stats) : tous les usages ont ete mis a jour dans
cette session (ecrans LVGL, DebugPanel, DemoDataProvider,
PocketPsnParser). Compile des deux cotes.

### `AppSettings` (`src/config/AppSettings.h`)
Struct versionnee (`schemaVersion`), tous les champs demandes presents
(Wi-Fi SSID/mot de passe, pseudo PSN, cle Pocket PSN, langue, luminosite,
veille, animations, rotation auto + intervalle, intervalle de sync,
fuseau horaire, mode demo). Compile des deux cotes. Pas encore expose via
API web (Phase B).

### `AppState` (`src/ui/AppState.h`)
Agregat simple `{profile, stats, sync, network, settings}`. Seule donnee
lue par l'UI (via `UiBridge::setAppState`). Compile des deux cotes, utilise
par tous les ecrans.

### `ConfigManager` (`src/config/ConfigManager.h/.cpp`)
Complet : `load()/save()/resetToDefaults()`, validation avec clamps
(luminosite 5-100, veille 0-3600s, rotation 5-300s, sync 5-360min),
`toPublicJson()` (sans secrets, pour `GET /api/config` futur),
`applyJsonPatch()` (patch partiel, pour `POST /api/config` futur),
migration de schema (point d'extension, un seul schema existant
aujourd'hui). Compile des deux cotes. **Pas de test automatise.**

### `FilePersistentStore` (`src/storage/FilePersistentStore.h/.cpp`)
Backend desktop `std::filesystem`, ecriture atomique (fichier `.tmp` puis
renommage). Compile uniquement simulateur/tests (exclu du firmware via
`build_src_filter`). Verifie par compilation reelle uniquement (pas de
test unitaire).

### `TrophyCache` (`src/storage/TrophyCache.h/.cpp`)
Complet : enveloppe JSON versionnee avec checksum FNV-1a (detection de
corruption au-dela de la seule validite JSON), ne remplace jamais une
donnee valide en memoire par une ecriture echouee, `dataAgeSeconds()`.
Compile des deux cotes. **Pas de test automatise** (fixtures attendues en
Phase F : profil normal, profil large, sans platine, reponse partielle,
erreur serveur, HTML, JSON invalide, profil introuvable -- aucune n'existe
encore).

### `TrophyRepository` (`src/data/TrophyRepository.h/.cpp`)
Orchestre provider + cache : `loadFromCache()`, `poll(nowEpoch)`,
validation stricte (rejette baisse anormale, incoherence total/detail),
`consumePendingDelta()` (evenement nouveau trophee, jamais rejoue au
redemarrage car non persiste). Compile des deux cotes. **Pas de test
automatise.**

### `DemoDataProvider` (`src/data/DemoDataProvider.h/.cpp`)
Fonctionnel, adapte au nouveau modele (`ProfileData`/`TrophyStats`
separes), `mutableProfile()/mutableStats()` pour le panneau de debug,
`simulateNewTrophy()`/`simulateNextRefreshError()` inchanges. Compile des
deux cotes, utilise reellement par le simulateur et le firmware (mode
demo).

### `PocketPsnParser` (`src/data/PocketPsnParser.h/.cpp`)
Extrait de `PocketPsnProvider` dans cette session : namespace pur
(`PocketPsnParser::parse`), aucune dependance Arduino/HTTPClient (seulement
ArduinoJson), donc testable sans materiel. Compile des deux cotes.
**Jamais teste contre une vraie reponse Pocket PSN** (aucune cle API
disponible) -- c'est le blocage documente depuis la Phase 2.5, inchange.

### `Logger` (`src/utils/Logger.h/.cpp`)
Rendu portable dans cette session (voir decisions d'architecture #4).
Interface publique inchangee (`error/warn/info/debug`). Compile des deux
cotes, utilise partout.

### `PowerManager` (`src/services/PowerManager.h/.cpp`)
Complet : etats Awake/Dimmed/Asleep, seuils bases sur `sleepTimeoutSeconds`
(dim a la moitie du delai, sommeil au delai complet, 0 = veille
desactivee), reveil immediat sur activite tactile. Materiel reel via
`Co5300BrightnessBackend` (firmware, `gfx->setBrightness()`),
`NullBrightnessBackend` (simulateur, aucun effet visuel). Compile des deux
cotes. **Jamais teste sur materiel reel.**

### `TimeService` (`src/services/TimeService.h/.cpp`) -- **PARTIAL**
Voir "Ce qui est partiel" ci-dessus pour le detail complet. Resume : code
complet et compilant des deux cotes, mais non verifie sur materiel reel et
sans aucun test automatise. Marque explicitement PARTIAL a la demande de
l'utilisateur plutot que d'etre presente comme termine.

### `SyncService` (`src/services/SyncService.h/.cpp`)
Complet : etats Idle/WaitingForNetwork/Connecting/Downloading/Parsing/
Saving/Success/Error/Offline, sync manuelle/auto, intervalle configurable,
backoff exponentiel (max 5 tentatives, plafond 5 min), annulation propre
(resultat ignore si deja termine), tache FreeRTOS dediee sur firmware (voir
decision d'architecture #5). Compile des deux cotes. **Chemin FreeRTOS
jamais exerce sur materiel reel.**

## 2026-07-21 -- Derniere passe avant materiel (v0.9.0-hardware-ready)

Demande explicite de l'utilisateur : "La partie logique est maintenant
quasiment terminee. Fais une derniere passe de preparation avant materiel."
Peri­metre strict : pas de changement d'architecture ni de comportement sauf
bug reel trouve ; design LVGL gere separement dans une autre passe, non touche.

**Test longue duree du simulateur** (`runLongDurationStressSelfTest`,
`simulator/src/main.cpp`) : 500 cycles connect/sync/disconnect/reconnect via
`AppController` reel (`NullUiBridge`, `WiFiManagerStub`,
`PocketPsnHttpClientStub`), avec des reponses invalides intercalees (jamais
au tout premier cycle, pour etablir d'abord un profil valide). Verifie :
aucun plantage/profil vide, profil final coherent (les reponses invalides
n'ecrasent jamais le dernier profil valide), au moins une synchro reussie
comptabilisee, temps d'execution raisonnable (500 cycles en ~2s), et surtout
que le fichier de cache reste borne (< 10 Ko, aucune accumulation) grace a
l'ecriture atomique de `FilePersistentStore`. Resultat : 186/186 assertions
`PASS` (187 lignes de sortie au total, la derniere etant une ligne
d'information sur le temps d'execution, sans PASS/FAIL). Bug rencontre et
corrige pendant l'ecriture du test lui-meme (pas un bug du code applicatif) :
la condition d'injection de reponse invalide etait vraie des `i=0`, empechant
tout profil valide de s'etablir avant que le test ne detecte (a tort) une
incoherence.

**Verification des cas limites de l'API Pocket PSN** (`PocketPsnParser`,
`PocketPsnProvider`, `TrophyRepository::validate()`) : relecture complete a
la recherche de reponses malformees/extremes non geree. La plupart des cas
limites etaient deja correctement couverts par les defenses existantes
(non revues en detail avant cette passe) : nom d'utilisateur vide rejete par
`TrophyRepository::validate()` (meme si le parser l'accepte techniquement
comme "present"), toute baisse de trophees rejetee (protege aussi contre un
total remis a zero), incoherence total/detail rejetee, JSON invalide/tronque
correctement rejete par `stripTrailingCommas()` + `deserializeJson()`,
reponse > 64 Ko rejetee par `PocketPsnProvider`.

Un vrai bug a ete trouve et corrige, strictement cote transport firmware
(`src/network/PocketPsnHttpClient.cpp`, jamais compile par le simulateur) :
`https.getString()` lisait integralement le corps de la reponse en memoire
**avant** que `PocketPsnProvider` ne puisse jamais verifier sa taille
(`kMaxResponseBytes`, 64 Ko) -- le garde-fou existant etait donc inutile
face a une reponse anormalement volumineuse (bug serveur, reponse
inattendue, etc.), avec un risque reel d'epuisement memoire sur l'ESP32-S3
(320 Ko de RAM interne). Correction : verification de `https.getSize()`
(Content-Length) avant l'appel a `getString()`, refus de lecture si
annoncee > 64 Ko (Content-Length inconnu/chunked laisse passer, la vraie API
ne renvoie jamais de reponse chunked). Verifie par `pio run` : compile avec
succes, impact negligeable (Flash +136 octets, RAM inchangee -- 65.4%/214308
octets, 70.5%/2958949 octets). Non testable via `--selftest` (fichier
firmware-only, hors peri­metre du simulateur) ; a re-verifier manuellement au
premier flash reel (voir checklist materiel).

## 2026-07-21 -- Mode demonstration showroom (simulateur uniquement)

Demande explicite de l'utilisateur, apres le tag `v0.9.0-hardware-ready` :
presenter le produit fini sans materiel via un scenario automatique et
reproductible (`.\simulator\run.ps1 -Showroom`), plus un mode manuel pour
declencher chaque etat individuellement (captures d'ecran utilisables pour la prochaine passe de design).
Peri­metre strict respecte : aucun fichier `src/ui/` touche, design LVGL
inchange -- uniquement de l'orchestration cote simulateur.

**`ShowroomScenario`** (nouveau, `simulator/src/ShowroomScenario.h/.cpp`) :
pilote les vrais services (`AppController` -> `SyncService` ->
`TrophyRepository` -> `PocketPsnProvider`) au travers de transports simules
controles (`WiFiManagerStub`, `PocketPsnHttpClientStub`) -- aucune donnee
affichee n'est fabriquee directement. Traverse 11 etapes (demarrage,
chargement, synchronisation, affichage du profil, utilisation du cache,
perte du reseau, ecran hors ligne, reconnexion apres le delai reel de
`SyncService::kReconnectStabilizationMs`, nouvelle synchronisation
automatique, erreur API simulee sans ecraser le cache, retour a la normale)
plus un etat terminal. Deux modes d'usage : automatique (`tick()`, delais de
maintien penses pour rester observables a l'oeil nu) et manuel
(`triggerStep()`, declenche l'action reelle d'une etape a la demande, sans
supposer d'ordre).

**Wiring** (`simulator/src/main.cpp`) : flag `-Showroom`/`--showroom`
(insensible a la casse, correspond a l'invocation demandee) lance la
sequence automatique au demarrage de la boucle interactive ; l'instance
`ShowroomScenario` est toujours construite en mode interactif (meme sans le
flag) pour que le mode manuel reste disponible a tout moment. Raccourcis
clavier `1`-`9`/`0`/`-` (un par etape) et `Espace` (relance la sequence
complete) ajoutes a la fenetre principale. `stdout` passe en line-buffered
(`setvbuf(_IOLBF)`) pour que la narration console s'affiche en temps reel.

**Mode manuel visuel** (`simulator/src/DebugPanel.cpp`) : nouvelle section
"Showroom (demonstration)" avec un bouton par etape (meme action que les
raccourcis clavier) plus un bouton pour relancer la sequence automatique.

**Tests** (`runShowroomScenarioSelfTest`, `simulator/src/main.cpp`) : pilote
la sequence automatique avec une horloge simulee (pas de vrai delai, execution
instantanee malgre les temps de maintien penses pour l'observation humaine),
verifie que les 12 etapes (11 + terminal) sont traversees exactement une
fois chacune et dans l'ordre attendu, que `isVerified()`/`pocketPsnVerified`
devient vrai, que le profil n'est jamais vide une fois la premiere synchro
passee, et surtout que le profil affiche reste strictement intact pendant
toute la duree de l'etape "erreur API simulee" (preuve que le cache n'est
jamais ecrase). Suite complete : 193/193 assertions `PASS`.

Verifie : `pio run` (firmware) inchange -- RAM 65.4%/214308 octets, Flash
70.5%/2958949 octets, identique au tag `v0.9.0-hardware-ready` (tout le
travail de cette phase est simulateur-only, `ShowroomScenario.cpp` n'est
jamais compile par PlatformIO). Verification live (`--showroom` en mode
interactif reel) tentee mais non concluante dans cet environnement
d'automatisation (fenetre SDL non observable/capturable depuis ce terminal) --
la logique sous-jacente reste neanmoins entierement prouvee par
`runShowroomScenarioSelfTest`, qui exerce exactement le meme code
(`ShowroomScenario::tick()`/`enterStep()`) avec les memes services reels. A
confirmer visuellement par l'utilisateur (ou lors de la prochaine passe de design) sur un poste avec affichage.

## 2026-07-22 -- Fusion du design final : audit + migration LVGL 9 (etape 1/N)

Demande explicite de l'utilisateur : assembler le projet fonctionnel avec le
design LVGL final (`PlayStation-Trophy-Display-LVGL-Design`, projet
separe sous OneDrive). Travail effectue sur la branche dediee
`integration/final-lvgl-ui` (jamais sur `master`), en plusieurs etapes
commitees separement pour rester revertibles individuellement.

**Audit** : comparaison complete des deux projets avant toute modification.
Constat majeur : ce design est ecrit nativement pour **LVGL 9.4.0**
(`lv_display_create`/`lv_indev_create`, format d'image
`lv_image_dsc_t`/`LV_COLOR_FORMAT_RGB565A8`) alors que le firmware/simulateur
utilisaient **LVGL 8.3.11** (`lv_disp_drv_t`/`lv_indev_drv_t`,
`lv_img_dsc_t`). Estimation de risque demandee et fournie avant toute
migration : 7 fichiers glue a reecrire (drivers ecran/tactile firmware +
simulateur, `lv_conf.h`, CMake/PlatformIO), shim officiel `lv_api_map_v8.h`
fourni par LVGL 9 lui-meme (couvre `lv_obj_del`, `lv_img_*`, `lv_scr_load`,
~150 renommages -- mais PAS `lv_disp_drv_t`/`lv_indev_drv_t`, seul point
vraiment obligatoire a la main). Strategie retenue par l'utilisateur : migrer
tout le projet en LVGL 9.4.0 plutot que de porter le design vers LVGL 8 (qui
aurait exige de reconstruire tous les descripteurs d'image ~3 Mo et de
revalider tout le rendu visuel deja fige dans le design d'origine).

**Etape 1 (ce commit, `b20f3f9`) : socle LVGL 9 uniquement, sans le design.**
Demande explicite : d'abord un projet qui compile (firmware + simulateur)
avec un ecran minimal de validation, avant de brancher quoi que ce soit du
nouveau design.

- `include/lv_conf.h` : reecrit au format v9 (structure calquee sur le
  `lv_conf.h` deja valide du nouveau design), memes valeurs qu'avant la ou elles
  existaient (LV_MEM_SIZE 160 Ko, police par defaut, 30 ms de
  rafraichissement) -- seule la version change, pas le comportement vise.
- `src/main.cpp` (firmware), `simulator/src/main.cpp`,
  `simulator/src/DisplayDriverSdl.cpp`, `simulator/src/TouchDriverSdl.cpp` :
  init ecran/tactile reecrite avec l'API v9
  (`lv_display_create`/`lv_display_set_flush_cb`/`lv_display_set_buffers`,
  `lv_indev_create`/`lv_indev_set_type`/`lv_indev_set_read_cb`) --
  aucun equivalent shimme pour cette partie, portage manuel obligatoire des
  4 fichiers (5 avec le 2e afficheur du panneau de debug, dans
  `simulator/src/main.cpp`).
- `simulator/third_party/lvgl` : revendorise en 9.4.0 (source recuperee
  localement depuis le projet design, deja telechargee par son propre
  `FetchContent` -- pas de nouvelle dependance reseau ajoutee ici). Le
  firmware resout `lvgl/lvgl @ 9.4.0` via PlatformIO.
- **Bug de portage reel trouve et corrige** (pas seulement un renommage) :
  `lv_color_t` n'est plus le type de pixel brut en LVGL 9 (desormais un type
  de couleur abstrait RGB888 3 octets, voir `lv_color.h`) -- le format de
  framebuffer RGB565 reel doit etre manipule en `uint16_t` directement.
  Touche `DisplayDriverSdl.cpp` et le framebuffer du panneau de debug dans
  `simulator/src/main.cpp` (`.full` n'existe plus). Egalement :
  `lv_event_get_target()` renvoie desormais un `void*` generique (plus un
  `lv_obj_t*`) -- `simulator/src/DebugPanel.cpp` corrige avec la nouvelle
  fonction `lv_event_get_target_obj()`.
- `src/ui/MinimalValidationScreen.h/.cpp` (nouveau, temporaire) : implemente
  `UiBridge`, un seul ecran (fond noir, libelle, anneau anime), prouve que
  tout le pipeline fonctionne de bout en bout (affichage, tactile,
  animation, AppState propage) avant tout travail de design. Remplace
  `UiManager` (dont les 6 ecrans + assets `src/ui/screens/*` et
  `src/ui/assets/*.c` sont au format d'image LVGL 8 et ne compilent plus
  contre v9) -- ces fichiers ne sont **pas supprimes**, seulement exclus de
  la compilation (`build_src_filter` / CMakeLists.txt) le temps de l'etape
  suivante, qui les remplacera definitivement par le nouveau design (jamais
  re-inclus tels quels).
- Consequences temporaires assumees, documentees en commentaires dans le
  code, a lever a l'etape suivante : pas de navigation (un seul ecran), pas
  de synchronisation declenchee depuis l'UI (raccourci clavier `R` appelle
  directement `AppController::requestManualSync()`),
  `exportScreenshots()`/`recordGifSequence()` suspendues (`#if 0`, code
  conserve), `runDebugPanelSelfTest` adapte pour utiliser
  `MinimalValidationScreen` a la place de `UiManager`.

**Verifie** : `pio run` (firmware) compile -- RAM 64.9%/212812 octets, Flash
34.5%/1446921 octets (plus leger qu'avant : assets/ecrans LVGL 8 exclus,
reviendra a l'etape suivante avec les nouveaux assets). Simulateur : build
CMake propre, **193/193 assertions `--selftest` PASS** (toute la logique
metier est intacte, `UiBridge` decouplait deja completement
`AppController` de LVGL). Lancement interactif verifie sans crash (processus
stable). Prochaine etape : integration progressive des ecrans/assets/API
Boot du nouveau design, avec sa propre serie de commits.
