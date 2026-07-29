# Simulateur PC -- PlayStation Trophy Display AMOLED

Reproduction fidele sur PC (Windows) de l'ecran rond AMOLED Waveshare
466x466 : meme resolution, meme masque circulaire, meme code d'interface
(`src/ui/`) que le firmware ESP32-S3. Permet de valider visuellement chaque
ecran **avant** de recevoir la carte physique.

## Lancer

```powershell
.\simulator\run.ps1
```

ou, si vous preferez piloter CMake vous-meme :

```powershell
.\simulator\build.ps1
.\simulator\build\trophy-display-simulator.exe
```

Depuis Git Bash :

```bash
./simulator/run.sh
```

**Aucune installation prealable requise** au-dela de Python 3.10+ : le
premier lancement installe automatiquement (via `pip`) un compilateur
(`ziglang`, base sur Clang/LLD), `cmake` et `ninja`. Rien n'est installe au
niveau systeme -- tout reste dans l'environnement Python de l'utilisateur.
Si vous avez deja Visual Studio + vcpkg et preferez votre propre chaine de
compilation, ouvrez simplement `simulator/CMakeLists.txt` avec CMake sans
passer `-DCMAKE_TOOLCHAIN_FILE=...` (le SDL2 vendorise dans
`third_party/SDL2` reste compatible MSVC pour les headers/`.lib`, mais il
faudra alors un `libSDL2.lib` MSVC au lieu de `libSDL2.dll.a` MinGW -- non
fourni ici, a recuperer depuis la release officielle SDL2 "VC").

Au premier lancement, l'application exporte automatiquement une capture PNG
de chacun des 6 ecrans dans `simulator/screenshots/` (voir plus bas), puis
reste ouverte en mode interactif normal.

## Controles (fenetre principale, 466x466)

| Entree | Action |
|---|---|
| Clic-glisse souris | Simule un swipe tactile (change d'ecran) |
| Fleche gauche/droite | Change d'ecran (Welcome/Dashboard/Trophies/Statistics) |
| `R` | Simule une synchronisation manuelle |
| `E` | Simule une erreur de synchronisation (Pocket PSN) |
| `N` | Simule l'obtention d'un nouveau trophee (Bronze +1) |
| `D` | Bascule le mode demo (reserve, voir limites ci-dessous) |
| `F` | Affiche les FPS dans la console |
| `1`-`9`, `0`, `-` | Declenche directement un etat du scenario showroom (voir ci-dessous) |
| `Espace` | Lance la sequence showroom automatique complete |
| Appui long (clic maintenu) | Ouvre l'ecran Reglages |

Une **seconde fenetre** ("Debug -- Trophy Display") s'ouvre a cote : panneau
de reglages en direct (pseudo, niveau, trophees, statistiques, etat Wi-Fi,
Pocket PSN simule, luminosite, langue, animations, rotation auto). Ce
panneau n'apparait jamais dans le rendu circulaire principal.

## Captures d'ecran automatiques

Generees dans `simulator/screenshots/` a chaque lancement :

```
01-welcome.png   02-dashboard.png   03-trophies.png
04-statistics.png   05-sync.png   06-settings.png
```

Utilisez `--no-screenshots` pour desactiver cet export, ou
`--screenshot-dir <dossier>` pour changer la destination.

Les captures "officielles" de la refonte graphique (validees et documentees)
sont dans [`simulator/previews/`](previews/) : les 6 ecrans finaux, le toast
"nouveau trophee", les variantes comparees pour Dashboard/Trophees, une
planche avant/apres, et un GIF anime (`preview.gif`).

## Mode demonstration (showroom)

Presente le produit fini sans materiel, via un scenario automatique et
reproductible qui pilote les **vrais services** applicatifs (`AppController`
-> `SyncService` -> `TrophyRepository` -> `PocketPsnProvider`) au travers de
transports simules controles (`WiFiManagerStub`, `PocketPsnHttpClientStub`)
-- voir `simulator/src/ShowroomScenario.h` pour le detail. Aucune donnee
affichee n'est fabriquee directement : tout passe par les memes chemins de
code qu'une vraie synchronisation Pocket PSN.

```powershell
.\simulator\run.ps1 -Showroom
```

(Accepte aussi `--showroom`/`-showroom`, insensible a la casse.)

La sequence traverse automatiquement, dans l'ordre, avec des delais de
maintien penses pour rester observables a l'oeil nu (~25-30 secondes au
total) :

1. Demarrage
2. Chargement (connexion Wi-Fi)
3. Synchronisation PocketPSN
4. Affichage du profil
5. Utilisation du cache
6. Perte du reseau
7. Ecran hors ligne
8. Reconnexion (fenetre de stabilisation reelle de 4 secondes, voir
   `SyncService::kReconnectStabilizationMs`)
9. Nouvelle synchronisation (declenchee automatiquement par
   `SyncService`, pas simulee au niveau UI)
10. Erreur API simulee, sans ecraser le cache
11. Retour a un etat normal

### Mode manuel (declencher un etat precis)

Pour capturer un etat individuel (captures d'ecran, demonstration ciblee)
sans attendre la sequence complete, deux options equivalentes, disponibles
a tout moment en mode interactif (pas besoin de `-Showroom`) :

- **Clavier** (fenetre principale) : touches `1` a `9`, `0` et `-` (voir le
  tableau des controles ci-dessus) declenchent directement l'action reelle
  de l'etape correspondante ; `Espace` relance la sequence automatique
  complete depuis le debut.
- **Panneau de debug** ("Debug -- Trophy Display") : section "Showroom
  (demonstration)", un bouton par etape plus un bouton "Lancer la sequence
  automatique complete".

Le mode manuel ne suppose aucun ordre : chaque declenchement effectue
l'action reelle de cet etat immediatement, quel que soit l'etat courant de
l'application.

## Enregistrer une sequence animee (GIF)

```powershell
.\simulator\build\trophy-display-simulator.exe --no-screenshots --record-gif --gif-dir gif_frames
```

Enregistre ~150 images (demarrage -> navigation -> animation dashboard ->
synchronisation -> nouveau trophee) dans `gif_frames/`. Assemblage en GIF via
Pillow (voir le script utilise pour `simulator/previews/preview.gif`,
sous-echantillonnage 1 image sur 2, redimensionnement 233x233).

## Architecture -- code partage avec le firmware

```
src/ui/                  <- logique visuelle LVGL commune (ecrans, theme,
                            navigation) -- utilisee TELLE QUELLE par le
                            firmware ESP32-S3 (platformio.ini) ET ce
                            simulateur (CMakeLists.txt). Ne jamais dupliquer.
src/data/                <- modele de donnees + fournisseurs. Seul
                            DemoDataProvider est compile ici (PocketPsnProvider
                            depend d'Arduino/HTTPClient, non pertinent sur PC).
simulator/src/           <- couche materielle SPECIFIQUE au simulateur :
  DisplayDriverSdl.*        pont LVGL <-> SDL2 (framebuffer, masque
                            circulaire, export PNG)
  TouchDriverSdl.*          pont LVGL <-> etat souris SDL2
  DebugPanel.*              panneau de debug (fenetre/ecran LVGL separe)
  ShowroomScenario.*        orchestrateur du scenario de demonstration
                            (voir "Mode demonstration (showroom)" plus haut)
  WiFiManagerStub.*         simulation Wi-Fi (voir network/IWiFiManager.h)
  PocketPsnHttpClientStub.* simulation transport HTTP Pocket PSN (voir
                            network/IPocketPsnHttpClient.h)
  main.cpp                  fenetres SDL2, boucle principale, raccourcis
```

Le firmware ESP32-S3 (`src/main.cpp` a la racine du depot) fournit
l'equivalent materiel : `Arduino_GFX`/CO5300 pour l'affichage,
`TouchDrvCST92xx` pour le tactile. C'est la SEULE difference entre les deux
cibles -- tout le reste (`src/ui/`, `src/data/`) est identique.

## Fidelite aux contraintes reelles de l'ESP32

- Meme resolution (466x466) et meme profondeur de couleur (RGB565,
  `LV_COLOR_DEPTH=16` dans `include/lv_conf.h`, partage avec le firmware).
- Meme masque circulaire (rayon 233px) et meme zone de securite
  (`src/ui/Layout.h`, rayon 200px) que le firmware.
- Polices Montserrat integrees a LVGL (aucune police lourde/externe).
- Animations limitees aux transitions d'ecran et a l'arc de progression --
  rien qui ne pourrait etre recalcule a 240 MHz sur l'ESP32-S3.

## Limites connues de cette version

- Le panneau de debug modifie directement les statistiques du
  `DemoDataProvider` (mode demo) ; il n'est pas connecte a `PocketPsnProvider`
  (non fonctionnel -- voir `AUDIT.md`). Le commutateur "Pocket PSN verifie
  (simule)" du panneau de debug ne reflete jamais un vrai etat reseau --
  c'est une simulation locale.
- La touche `D` (bascule mode demo) est un point d'extension reserve : dans
  cette phase, seul le mode demo existe, donc elle n'a pas d'effet visible.
- Pas de saisie de texte libre pour le pseudo dans le panneau de debug
  (3 boutons pseudo pre-remplis) -- simplification volontaire de cette passe.
