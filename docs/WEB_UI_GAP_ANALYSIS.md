# Analyse d'ecart -- module Web UI vs implementation C++

Ecrit le 2026-07-15 lors de l'integration du module
`PlayStation-Trophy-Display-Web-UI-Module.zip` (commit
`71527ac...` -> commit suivant). **Le fichier `WEB_UI_CONTRACT.md` fourni
par l'utilisateur contenait en realite le code de `app.js`** (nom de
fichier incorrect, contenu verifie octet pour octet identique a
`data/app.js`) -- aucune prose de contrat n'a donc ete lue. Le "contrat"
ci-dessous a ete **reconstruit par lecture exhaustive de `data/app.js` et
`data/index.html`** (chaque appel `fetch()`, chaque champ lu/ecrit), ce qui
est sans ambiguite mais n'a pas pu etre recoupe avec une intention ecrite.
A revalider si le vrai `WEB_UI_CONTRACT.md` est fourni plus tard.

## Routes utilisees par `data/app.js` (exhaustif, verifie ligne par ligne)

| Route | Utilisee par | Avant cette passe | Apres cette passe |
|---|---|---|---|
| `GET /api/status` | `loadStatus()` (boot) | Implementee, **forme JSON differente** (voir ci-dessous) | Reecrite pour matcher le contrat exact |
| `GET /api/config` | `loadConfig()` (boot) | **Absente** | Implementee |
| `POST /api/config` | `saveConfig()` (formulaire) | **Absente** | Implementee (traduction de champs, voir ci-dessous) |
| `GET /api/wifi/scan` | `scanWifi()` | Implementee, champ `rssiDbm` au lieu de `rssi` | Corrige (`rssi`) |
| `POST /api/wifi/connect` | `connectWifi()` | Implementee, deja compatible | Inchangee |
| `POST /api/wifi/forget` | **Non appelee par le front** (pas de bouton dans `index.html`) | Implementee | Conservee (ne genera aucune requete du front actuel, mais reste utilisable) |
| `POST /api/profile/test` | `testProfile()` | Absente | Stub `not_implemented` (501) -- depend de Pocket PSN, laisse volontairement ainsi jusqu'a ce que Pocket PSN Probe fournisse une vraie reponse et un schema confirme |
| `POST /api/sync` | `syncNow()` | Stub `not_implemented` (501) | **Fonctionnelle** (2026-07-15, passe suivante) -- `AppController::requestManualSync()`, refuse (409) si deja en cours |
| `POST /api/reboot` | `reboot()` | Stub `not_implemented` (501) | **Fonctionnelle** -- repond en JSON puis redemarre apres 800 ms (`PendingRestart`) |
| `POST /api/reset` | `resetDevice()` | Stub `not_implemented` (501) | **Fonctionnelle** -- exige `{"confirm":true}`, efface config+cache, redemarre apres 800 ms |
| `GET /api/diagnostics` | `refreshDiagnostics()` (boot) | Stub `not_implemented` (501) | **Fonctionnelle** -- voir "Ecart de noms de champs diagnostics" ci-dessous : `refreshDiagnostics()` avale sa propre erreur (try/catch interne), donc un stub 501 ne cassait deja pas le boot ; la vraie reponse ne casse rien non plus (voir plus bas). |

## Mise a jour 2026-07-15 (passe suivante) : sync/reboot/reset/diagnostics reels

`/api/sync`, `/api/reboot`, `/api/reset`, `/api/diagnostics` sont
desormais fonctionnels (voir tableau ci-dessus). `/api/profile/test`
reste en `not_implemented` (501), comme demande, jusqu'a ce que l'outil
`tools/pocketpsn_probe/` obtienne une vraie reponse et un schema confirme.

### Ecart de noms de champs diagnostics (reel, decouvert cette passe)

L'utilisateur a explicitement dicte la liste de champs attendus pour
`GET /api/diagnostics` (`firmwareVersion`, `uptimeSeconds`,
`freeHeapBytes`, ..., voir le corps du message de la tache) -- implementee
telle quelle dans `DiagnosticsSnapshot`/`WebApiHandlers::buildDiagnosticsJson`.

**Mais `data/app.js` (`renderDiagnostics()`) lit en realite des noms de
champs differents** : `firmware`, `uptime`, `heapFree`, `psramFree`,
`rssi`, `lvgl`, `lastError`, `cacheState` -- aucun ne correspond aux noms
dictes. Consequence concrete verifiee en lisant le code (pas de crash,
juste un affichage partiel) :
- Les 8 lignes "amicales" du panneau Diagnostics (`diagnosticsGrid`)
  afficheront `-` pour la plupart des champs (`String(value ?? "-")`
  gere gracieusement `undefined`, ne leve jamais d'exception).
- Le bloc JSON brut (`diagnosticsRaw`, `JSON.stringify(diagnostics, null, 2)`)
  affichera en revanche **toutes les vraies valeurs correctement**, y
  compris `firmwareVersion`/`uptimeSeconds`/etc.
- `refreshDiagnostics()` n'est jamais rejetee (pas de `throw`), donc le
  `Promise.all` du `boot()` n'est jamais casse par ce mismatch.

**RESOLU le 2026-07-16** (a la demande explicite de l'utilisateur) :
`data/app.js` (`renderDiagnostics()`) a ete adapte pour lire les 22 noms
de champs canoniques (`firmwareVersion`, `uptimeSeconds`, `freeHeapBytes`,
..., voir liste complete dans `src/web/DiagnosticsSnapshot.h`) --
**aucun alias/duplication cote C++**, `DiagnosticsSnapshot` inchange.
Libelles du panneau diagnostics mis a jour en consequence (22 lignes au
lieu de 8, formatage lisible des octets/durees/booleens), sans toucher au
CSS/HTML du panneau (`.diagnostic-grid` accepte n'importe quel nombre de
lignes sans modification).

Une verification statique a ete ajoutee au `--selftest` du simulateur
(`simulator/src/main.cpp`) : elle lit **le vrai fichier**
`data/app.js`, extrait par expression reguliere tous les champs de la
forme `diagnostics.xxx`/`diagnostics?.xxx` reellement references, puis
verifie que chacun existe comme cle dans la reponse JSON reelle de
`WebApiHandlers::buildDiagnosticsJson()`. Volontairement **pas de liste
codee en dur** (qui se desynchroniserait silencieusement du fichier reel
si `app.js` change plus tard sans que le test soit mis a jour).

Aucune route appelee par `app.js` n'est manquante : les 10 lignes
ci-dessus couvrent l'integralite des `fetch()` du fichier.

## Ecarts de forme JSON identifies et corriges

### `GET /api/status`

| Avant (implementation Phase B initiale) | Attendu par `app.js` (releve exact) |
|---|---|
| `{"sync":{"state":...,"isOffline":...,"isDemo":...,"lastSyncEpoch":...},"network":{"state":...,"ssid":...,"ip":...,"rssiDbm":...},"settings":{"demoMode":...,"psnUsername":...}}` | `{"configured":bool,"offline":bool,"error":string\|null,"network":{"connected":bool,"ssid":string,"ip":string,"message":string},"sync":{"state":string,"lastSync":string\|"syncing"\|null,"source":string}}` |

Champs recalcules (pas de correspondance directe en interne, voir
`WebApiHandlers::buildStatusJson`) :
- `configured` = `!wifiSsid.empty() && !psnUsername.empty()` (deduit du
  texte `index.html` : "Renseignez le Wi-Fi et le pseudo PSN pour activer
  la synchronisation").
- `network.connected` = `network.state == WifiState::kConnected`.
- `network.message` = message humain derive de `WifiState` (ex:
  "Connexion en cours...", "Point d'acces de configuration actif").
- `sync.lastSync` = `TimeService::formatClock(lastSyncEpoch)` (`"HH:MM"`)
  si une synchronisation a deja eu lieu, `"syncing"` si active, sinon
  `null` (`app.js` affiche alors "Jamais").
- `sync.source` = `"cache"` si hors-ligne avec des donnees deja
  affichees, sinon `"live"`.

### `GET /api/wifi/scan`

Seul changement : `rssiDbm` -> `rssi` (nom de champ exact attendu par
`renderNetworks()`). Le champ `status` ("idle"/"scanning"/"done") est
**une extension non prevue par le contrat mais inoffensive** : `app.js`
lit uniquement `result.networks`, un champ inconnu est simplement ignore.
Conserve car utile pour les tests (`--selftest`) et un futur usage web.

### `GET /api/config` / `POST /api/config` (nouvelle route)

| Champ module Web UI (`readConfigForm()`/`renderConfig()`) | Champ interne (`AppSettings`) | Traduction |
|---|---|---|
| `ssid` | `wifiSsid` | directe |
| `psnUsername` | `psnUsername` | directe (meme nom) |
| `brightness` (10-100) | `brightnessPercent` | directe |
| `sleepEnabled` (bool) + `sleepDelay` (minutes) | `sleepTimeoutSeconds` (secondes, 0=desactive) | `sleepEnabled=false` -> `0` ; sinon `sleepDelay * 60` |
| `autoRotation` | `autoRotateEnabled` | directe |
| `rotationDelay` (secondes) | `rotationIntervalSeconds` | directe (memes unites) |
| `animations` | `animationsEnabled` | directe |
| `language` (`"fr"/"en"/"es"/"de"`) | `AppLanguage` (`kFrench`/`kEnglish` **seulement**) | voir incompatibilite ci-dessous |
| `syncInterval` (minutes) | `syncIntervalMinutes` | directe (memes unites) |

**Incompatibilite serieuse documentee** : le module Web UI propose 4
langues (`fr`/`en`/`es`/`de`), mais `AppSettings::AppLanguage` (et les
ecrans LVGL qui le lisent, ex. `SettingsScreen.cpp`) ne supportent que
`fr`/`en`. Etendre l'enum toucherait l'ecran LVGL `Settings`, ce qui sort
du perimetre de cette passe ("ne fusionne pas encore le nouveau design
LVGL"). **Decision** : `POST /api/config` avec `language:"es"` ou
`"de"` est accepte (HTTP 200, ne bloque pas le reste de la sauvegarde)
mais la langue n'est **pas modifiee** ; un champ `"message"` non bloquant
signale l'incompatibilite dans la reponse. A revoir quand la nouvelle UI
LVGL sera fusionnee (tache future, hors perimetre ici).

Traduction implementee dans `WebApiHandlers::translateConfigPatch()`
(logique de *forme* uniquement) ; la validation/persistance reste
entierement dans `ConfigManager`/`AppController::applyConfigPatch()` --
aucune logique metier dupliquee dans la couche web, conformement a la
consigne.

## Etat des 10 routes du contrat complet (liste fournie par l'utilisateur)

| Route | Statut |
|---|---|
| `GET /api/status` | Fonctionnelle |
| `GET /api/config` | Fonctionnelle |
| `POST /api/config` | Fonctionnelle |
| `GET /api/wifi/scan` | Fonctionnelle |
| `POST /api/wifi/connect` | Fonctionnelle |
| `POST /api/wifi/forget` | Fonctionnelle (non appelee par le front actuel) |
| `POST /api/profile/test` | `not_implemented` (501) -- depend de Pocket PSN |
| `POST /api/sync` | `not_implemented` (501) -- reporte, hors "parcours de base" demande |
| `POST /api/reboot` | `not_implemented` (501) -- reporte |
| `POST /api/reset` | `not_implemented` (501) -- reporte |
| `GET /api/diagnostics` | `not_implemented` (501) -- reporte, pas encore centralise |

## Page de secours (fallback technique conserve)

L'ancienne page minimale ecrite avant l'arrivee du module Web UI a ete
**remplacee** par les vrais fichiers du module Web UI (`data/index.html`,
`data/app.js`, `data/styles.css`, copies telles quelles, design non
modifie). Une version **reduite et auto-suffisante** de l'ancienne page
(HTML/CSS/JS inline, sans dependance a `app.js`/`styles.css`) est
**conservee dans le binaire firmware** (`CaptivePortalServer.cpp`,
`kFallbackHtml`) et servie uniquement si `LittleFS` ne contient pas encore
`/index.html` (avant le tout premier `pio run -t uploadfs`) : sans ce
repli, un appareil fraichement flashe afficherait une page vide/404 dans
le portail captif tant que le systeme de fichiers n'a pas ete televerse.
C'est un repli technique reel, pas une simple copie laissee par
prudence.
