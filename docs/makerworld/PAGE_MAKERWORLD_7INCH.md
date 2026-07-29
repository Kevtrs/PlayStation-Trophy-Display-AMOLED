# PlayStation Trophy Display -- afficheur de trophées (écran 7" tactile)

*(English version below / version anglaise plus bas)*

---

## 🇫🇷 Version française

### Accroche

La version grand format de l'afficheur de trophées PlayStation : un
écran tactile 7 pouces qui affiche en direct votre niveau, vos trophées
et vos statistiques -- posé sur votre bureau, une étagère, ou à côté de
votre setup gaming.

### Ce que c'est

Ce projet combine un boîtier à imprimer en 3D et un petit ordinateur
(carte ESP32-S3 avec écran tactile 800×480 intégré) qui se connecte à
votre compte PlayStation via [Pocket PSN](https://pocketpsn.com) pour
afficher en boucle, automatiquement :

- Un tableau de bord : niveau PSN, progression, trophées totaux
- Un écran Trophées : résumé Platine / Or / Argent / Bronze
- Un écran Statistiques : jeux terminés, taux de complétion, temps de
  jeu, classement mondial
- Les données restent affichées même sans connexion (cache hors ligne)

Aucune application à installer sur votre PC ou votre téléphone : tout
se configure directement sur l'écran, en Wi-Fi.

### Captures d'écran

> Générées depuis le simulateur (données de démonstration) -- le rendu
> sur l'écran réel est identique. À téléverser comme images sur la page
> MakerWorld.

- Tableau de bord : `docs/screenshots_wide/dashboard.png`
- Trophées : `docs/screenshots_wide/trophies.png`
- Statistiques : `docs/screenshots_wide/statistics.png`

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

### Première configuration

1. La carte crée son propre réseau Wi-Fi : `TrophyDisplay-Setup`.
2. Connectez-vous à ce réseau depuis votre téléphone ou votre PC, puis
   ouvrez `http://192.168.4.1/` dans un navigateur.
3. Sélectionnez votre réseau Wi-Fi habituel (ou saisissez son nom
   manuellement si le scan ne le trouve pas), entrez son mot de passe,
   renseignez votre pseudo PSN, puis cliquez une seule fois sur
   **Enregistrer**.
4. La carte redémarre et se connecte automatiquement. Vos trophées
   s'affichent en quelques secondes et défilent automatiquement entre
   les écrans.

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

The large-format version of the PlayStation trophy display: a 7-inch
touchscreen that shows your level, trophies and stats in real time --
sitting on your desk, a shelf, or next to your gaming setup.

### What it is

This project combines a 3D-printed enclosure with a small computer (an
ESP32-S3 board with a built-in 800x480 touchscreen) that connects to
your PlayStation account through [Pocket PSN](https://pocketpsn.com) to
automatically cycle through:

- A dashboard: PSN level, progress, total trophies
- A Trophies screen: Platinum / Gold / Silver / Bronze summary
- A Statistics screen: games completed, completion rate, playtime,
  world rank
- Data stays on screen even without a connection (offline cache)

No app to install on your PC or phone: everything is set up directly
on the screen, over Wi-Fi.

### Screenshots

> Generated from the simulator (demo data) -- the real screen looks
> identical. Upload these as images on the MakerWorld page.

- Dashboard: `docs/screenshots_wide/dashboard.png`
- Trophies: `docs/screenshots_wide/trophies.png`
- Statistics: `docs/screenshots_wide/statistics.png`

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

The stand is **a single part**: an open tray the Waveshare board sits
in and screws down to (the screen module already has its own bezel, so
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
2. Plug the board into your computer via USB-C -- **use the port
   labeled "UART"**, not the "USB" port (they look similar, only UART
   works for installation).
3. Go to the install page:
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall_7inch/**
4. Click **Install** and follow the on-screen instructions.
5. Once installation is complete, unplug and replug the board.

No software to download, no command line.

### First-time setup

1. The board creates its own Wi-Fi network: `TrophyDisplay-Setup`.
2. Connect to that network from your phone or computer, then open
   `http://192.168.4.1/` in a browser.
3. Select your home Wi-Fi network (or type its name manually if the
   scan doesn't find it), enter its password, enter your PSN username,
   then click **Save** once.
4. The board restarts and connects automatically. Your trophies show
   up within seconds and cycle automatically between screens.

### Source code

The full firmware is open source:
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

### Credits

- PlayStation data provided by [Pocket PSN](https://pocketpsn.com).
- Design and development: Kevin Torres.

### License

Firmware under the MIT license. [3D MODEL LICENSE -- specify on the
MakerWorld page itself, e.g. CC BY-NC-SA 4.0]
