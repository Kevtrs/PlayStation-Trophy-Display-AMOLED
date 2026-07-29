# Plan d'implementation -- squelette fonctionnel complet

Redige le 2026-07-15 a partir de l'etat reel du depot (voir
`HANDOFF_FUNCTIONAL_BASE.md`). Objectif : terminer toute la logique
(reseau, stockage, Pocket PSN, synchronisation, veille, temps, diagnostics,
tests) derriere une API stable, **sans toucher au design LVGL actuel**
(couche temporaire, sera remplacee par le design final).

## Principe d'architecture retenu

```
AppController (src/app/)
  ├─ ConfigManager        (src/config/)   -- lecture/ecriture AppSettings
  ├─ TrophyCache          (src/storage/)  -- dernier snapshot valide
  ├─ TrophyRepository     (src/data/)     -- provider + cache + delta
  │    ├─ DemoDataProvider
  │    └─ PocketPsnProvider (portable -- voir IPocketPsnHttpClient ci-dessous)
  ├─ SyncService          (src/services/) -- machine a etats de synchro
  ├─ PowerManager         (src/services/) -- luminosite + veille
  ├─ TimeService          (src/services/) -- horodatage, formatage relatif
  └─ WiFiManager          (src/network/, firmware) / stub simule (simulateur)

ProviderFactory (src/data/) : decision pure (au demarrage uniquement, voir
AUDIT.md section 0ter) entre DemoDataProvider et PocketPsnProvider, selon
AppSettings.psnUsername/pocketPsnApiKey. AppController reste agnostique
(ne prend qu'un TrophyDataProvider&) ; un changement de cle via l'UI web
necessite un redemarrage pour etre pris en compte (pas de bascule a chaud).

IPocketPsnHttpClient (src/network/) : meme principe que IWiFiManager --
PocketPsnProvider ne depend jamais de HTTPClient/WiFiClientSecure
directement, seulement de cette interface. Deux implementations :
PocketPsnHttpClient (firmware, requete HTTPS reelle) et
PocketPsnHttpClientStub (simulateur, reponses en file d'attente) -- ce qui
rend PocketPsnProvider portable et testable via --selftest sans materiel.

AppState (src/ui/AppState.h) = ProfileData + TrophyStats + SyncStatus +
NetworkStatus + AppSettings -- seule donnee lue par l'UI.

UiManager (src/ui/) ne detient plus de TrophyDataProvider*, seulement un
AppController* : il lit `controller->state()` et appelle
`controller->requestManualSync()` etc. Aucun ecran LVGL n'accede jamais a
HTTP/NVS/WiFi/LittleFS.
```

Web (`src/web/` + `data/`) et portail captif s'appuient sur `AppController`
de la meme maniere (memes routes que l'UI, memes services).

## Contraintes de portabilite

`src/data/`, `src/config/`, `src/storage/`, `src/services/`, `src/app/`
doivent compiler **a la fois** sur le firmware (Arduino/ESP32) et le
simulateur (C++ desktop). La seule difference materielle passe par une
petite interface de stockage (`IPersistentStore`) et par `WiFiManager`
(reel sur firmware, simule sur PC).

## Phases (voir PROJECT_STATUS.md pour l'etat reel apres execution)

- **Phase A** -- structures de donnees, ConfigManager, TrophyCache, tests de base.
- **Phase B** -- WiFiManager (firmware), portail captif, serveur web, diagnostics.
- **Phase C** -- PocketPsnProvider finalise (erreurs structurees, timeouts, Content-Type, portable via IPocketPsnHttpClient), synchronisation reelle si cle disponible (une cle privee legitime a ete obtenue le 2026-07-18 -- voir AUDIT.md section 0ter -- mais pas encore testee reellement dans cet environnement ; sinon bloque et documente).
- **Phase D** -- SyncService, TimeService, PowerManager, TrophyDelta.
- **Phase E** -- simulateur : scenarios fonctionnels (pas seulement visuels) pilotables depuis le panneau debug.
- **Phase F** -- partitions, documentation, statut final.

Chaque phase se termine par une compilation reelle firmware (`pio run`) et
simulateur (`cmake --build`), jamais une simple relecture.
