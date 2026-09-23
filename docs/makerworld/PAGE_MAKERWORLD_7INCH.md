# PlayStation Trophy Display -- afficheur de trophées (écran 7" tactile)

*(English version below / version anglaise plus bas)*

---

## 🇫🇷 Version française

### Accroche

Vous décrochez un Platine. Trente minutes plus tard, sans que vous ayez
rien fait, le cadre posé sur votre bureau s'illumine et l'affiche.

C'est un écran tactile 7 pouces qui suit votre profil PlayStation en
direct : niveau, trophées, statistiques. Il tourne tout seul, comme un
cadre photo -- et il vous félicite quand vous progressez.

### Ce que c'est

Un boîtier à imprimer en 3D et une carte ESP32-S3 à écran tactile
800×480, qui se connecte à votre compte PlayStation via
[Pocket PSN](https://pocketpsn.com) et fait défiler automatiquement :

- **Tableau de bord** : avatar, niveau PSN, progression, rang mondial
- **Trophées** : répartition Platine / Or / Argent / Bronze
- **Une fiche par palier** : le détail de chaque type de trophée
- **Statistiques** : jeux terminés, complétion, temps de jeu, classement
- **Célébration** : quand une synchronisation détecte un nouveau
  trophée, l'écran s'interrompt pour l'annoncer

Les données restent affichées même sans réseau (cache hors ligne). Aucune
application à installer sur votre PC ou votre téléphone.

### Il se regarde seul, il se pilote au doigt

L'appareil est fait pour être posé et oublié : il défile tout seul,
aucune information n'exige qu'on le touche.

Mais si vous le touchez, il vous donne la main : le défilement se met en
pause, des flèches et des points de navigation apparaissent, vous allez
directement à l'écran voulu ou dans les réglages. Vingt secondes sans
contact, et il repart tout seul en mode vitrine.

### Galerie

> Captures générées depuis le simulateur avec des données de
> démonstration -- le rendu sur l'écran réel est identique.
> À téléverser comme images sur la page MakerWorld.

| Écran | Fichier |
|---|---|
| Célébration animée | `screenshots_wide/celebration.gif` |
| Tableau de bord | `screenshots_wide/dashboard.png` |
| Trophées | `screenshots_wide/trophies.png` |
| Platine / Or / Argent / Bronze | `platinum.png`, `gold.png`, `silver.png`, `bronze.png` |
| Statistiques | `screenshots_wide/statistics.png` |
| Navigation tactile | `screenshots_wide/touch-navigation.png` |
| Première configuration | `screenshots_wide/wifi.png` |
| Réglages | `screenshots_wide/settings.png` |
| Synchro / hors ligne / erreur | `sync.png`, `offline.png`, `error.png` |
| À propos | `screenshots_wide/about.png` |

Le GIF de célébration est la meilleure image d'accroche pour la fiche :
c'est le seul écran qui bouge.

### Ce qu'il vous faut

- Une carte [Waveshare ESP32-S3-Touch-LCD-7](https://www.waveshare.com/esp32-s3-touch-lcd-7.htm)
  (écran tactile 800×480, GT911) -- non fournie, à acheter séparément
- Un câble USB-C (données, pas seulement charge)
- Un compte [Pocket PSN](https://pocketpsn.com) actif (gratuit, service
  tiers qui fournit les données de trophées PlayStation)
- Filament PLA (une seule couleur suffit pour ce boîtier)
- 2 vis (taille à confirmer selon vos inserts/taraudage -- 2 plots de
  fixation sont prévus à l'intérieur du support)

### Impression 3D

Le support tient en **une seule pièce** : un plateau ouvert dans lequel
la carte Waveshare vient se poser et se visser (l'écran, déjà encadré
par le module, sert lui-même de façade -- pas de capot supplémentaire).

- Imprimante utilisée pour la conception : Bambu Lab X1 Carbon (buse 0,4 mm)
- Matériau : PLA, une seule couleur
- Profil : "0,28 mm Extra Draft" (impression rapide)
- Hauteur de couche : 0,28 mm (première couche 0,2 mm)
- Parois : 2
- Remplissage : 5 %
- Supports : oui (arborescents automatiques)
- Poids de filament : ~123 g
- Temps d'impression : ~3 h 25

*(Estimations Bambu Studio, imprimante et profil ci-dessus -- peut
varier avec un autre matériel.)*

### Montage

1. Imprimez le support (voir réglages ci-dessus).
2. Posez la carte Waveshare dans le support, plots de fixation alignés.
3. Vissez avec les 2 vis.
4. Faites passer le câble USB-C par l'ouverture prévue.

### Installer le firmware (aucune compétence technique requise)

1. Ouvrez **Google Chrome** ou **Microsoft Edge** (obligatoire, les
   autres navigateurs ne fonctionnent pas pour cette étape).
2. Branchez la carte à votre ordinateur en USB-C -- **utilisez le port
   marqué "UART"**, pas le port "USB" (les deux se ressemblent, seul
   UART fonctionne pour l'installation).
3. Allez sur la page d'installation :
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall_7inch/**
4. Cliquez sur **Installer** et suivez les instructions à l'écran.
5. Une fois l'installation terminée, débranchez et rebranchez la carte.

Aucun logiciel à télécharger, aucune ligne de commande.

### Première configuration : scannez, c'est tout

Au premier démarrage, l'écran affiche un **QR code**.

1. Scannez-le avec l'appareil photo de votre téléphone. Il vous propose
   de rejoindre le réseau de l'appareil -- acceptez.
2. La page de configuration s'ouvre toute seule.
3. Choisissez votre réseau Wi-Fi, entrez son mot de passe, renseignez
   votre pseudo PSN, puis **Enregistrer**.
4. L'appareil redémarre et se connecte. Vos trophées s'affichent en
   quelques secondes.

Rien à recopier, aucune adresse à taper. Le nom du réseau et l'adresse
restent affichés sous le QR pour ceux qui préfèrent le faire à la main.

### Au quotidien

Il n'y a rien à faire. L'appareil se synchronise tout seul et défile.
Touchez l'écran uniquement si vous voulez aller quelque part
directement, ou changer la luminosité dans les réglages.

### Code source

Le firmware complet est open source :
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

### Crédits

- Données PlayStation fournies par [Pocket PSN](https://pocketpsn.com).
- Conception et développement : Kevin Torres.

### Licence

Firmware sous licence MIT. [LICENCE DU MODÈLE 3D -- à préciser sur la
page MakerWorld elle-même, ex : CC BY-NC-SA 4.0]

---

## 🇬🇧 English version

### Hook

You earn a Platinum. Thirty minutes later, without you doing anything,
the frame on your desk lights up and announces it.

It's a 7-inch touchscreen that tracks your PlayStation profile live:
level, trophies, stats. It runs on its own, like a photo frame -- and it
congratulates you when you make progress.

### What it is

A 3D-printed stand and an ESP32-S3 board with a built-in 800x480
touchscreen, connected to your PlayStation account through
[Pocket PSN](https://pocketpsn.com), automatically cycling through:

- **Dashboard**: avatar, PSN level, progress, world rank
- **Trophies**: Platinum / Gold / Silver / Bronze breakdown
- **One screen per tier**: the detail of each trophy type
- **Statistics**: games completed, completion rate, playtime, ranking
- **Celebration**: when a sync detects a new trophy, the display
  interrupts itself to announce it

Data stays on screen even without a network (offline cache). No app to
install on your PC or phone.

### It watches itself, you steer it with a finger

The device is meant to be set down and forgotten: it cycles on its own,
and no information ever requires touching it.

But touch it and it hands you control: the cycling pauses, arrows and
navigation dots appear, you jump straight to a screen or into settings.
Twenty seconds without contact and it goes back to showcase mode by
itself.

### Gallery

> Screenshots generated from the simulator with demo data -- the real
> screen looks identical. Upload these as images on the MakerWorld page.

| Screen | File |
|---|---|
| Animated celebration | `screenshots_wide/celebration.gif` |
| Dashboard | `screenshots_wide/dashboard.png` |
| Trophies | `screenshots_wide/trophies.png` |
| Platinum / Gold / Silver / Bronze | `platinum.png`, `gold.png`, `silver.png`, `bronze.png` |
| Statistics | `screenshots_wide/statistics.png` |
| Touch navigation | `screenshots_wide/touch-navigation.png` |
| First-time setup | `screenshots_wide/wifi.png` |
| Settings | `screenshots_wide/settings.png` |
| Sync / offline / error | `sync.png`, `offline.png`, `error.png` |
| About | `screenshots_wide/about.png` |

The celebration GIF is the best cover image for the listing: it's the
only screen that moves.

### What you need

- A [Waveshare ESP32-S3-Touch-LCD-7](https://www.waveshare.com/esp32-s3-touch-lcd-7.htm)
  board (800x480 touchscreen, GT911) -- not included, buy separately
- A USB-C cable (data-capable, not charge-only)
- An active [Pocket PSN](https://pocketpsn.com) account (free
  third-party service that provides PlayStation trophy data)
- PLA filament (a single color is enough for this stand)
- 2 screws (exact size depends on your inserts/tapping -- 2 mounting
  posts are built into the stand)

### 3D printing

The stand is **a single part**: an open tray the Waveshare board sits in
and screws down to (the screen module already has its own bezel, so
there's no separate front cover).

- Printer used for the design: Bambu Lab X1 Carbon (0.4 mm nozzle)
- Material: PLA, single color
- Profile: "0.28mm Extra Draft" (fast print)
- Layer height: 0.28 mm (first layer 0.2 mm)
- Wall loops: 2
- Infill: 5%
- Supports: yes (tree, automatic)
- Filament weight: ~123 g
- Print time: ~3h25

*(Bambu Studio estimates, printer and profile above -- may vary with
other hardware.)*

### Assembly

1. Print the stand (see settings above).
2. Place the Waveshare board into the stand, aligned with the mounting
   posts.
3. Secure with the 2 screws.
4. Route the USB-C cable through the provided opening.

### Installing the firmware (no technical skills required)

1. Open **Google Chrome** or **Microsoft Edge** (required -- other
   browsers won't work for this step).
2. Plug the board into your computer via USB-C -- **use the port labeled
   "UART"**, not the "USB" port (they look similar, only UART works for
   installation).
3. Go to the install page:
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall_7inch/**
4. Click **Install** and follow the on-screen steps.
5. When it's done, unplug and replug the board.

No software to download, no command line.

### First-time setup: just scan

On first boot, the screen shows a **QR code**.

1. Scan it with your phone's camera. It offers to join the device's
   network -- accept.
2. The setup page opens by itself.
3. Pick your Wi-Fi network, enter its password, fill in your PSN
   username, then **Save**.
4. The device reboots and connects. Your trophies appear within seconds.

Nothing to copy out, no address to type. The network name and address
stay visible under the QR code for anyone who prefers doing it manually.

### Day to day

There's nothing to do. The device syncs and cycles on its own. Touch the
screen only if you want to jump somewhere directly, or change the
brightness in settings.

### Source code

The full firmware is open source:
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

### Credits

- PlayStation data provided by [Pocket PSN](https://pocketpsn.com).
- Design and development: Kevin Torres.

### License

Firmware under MIT license. [3D MODEL LICENSE -- to be set on the
MakerWorld page itself, e.g. CC BY-NC-SA 4.0]
