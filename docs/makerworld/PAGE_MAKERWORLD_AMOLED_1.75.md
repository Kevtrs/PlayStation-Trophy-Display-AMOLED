# PlayStation Trophy Display -- afficheur de trophées (écran rond AMOLED)

*(English version below / version anglaise plus bas)*

---

## 🇫🇷 Version française

### Accroche

Un petit afficheur connecté qui montre en direct votre niveau, vos
trophées et vos statistiques PlayStation -- posé sur votre bureau, à
côté de votre PC ou de votre setup gaming. Écran rond AMOLED, données
mises à jour automatiquement, zéro maintenance une fois installé.

### Ce que c'est

Ce projet combine une pièce à imprimer en 3D et un petit ordinateur
(carte ESP32-S3 avec écran tactile intégré) qui se connecte à votre
compte PlayStation via [Pocket PSN](https://pocketpsn.com) pour
afficher en permanence :

- Votre niveau PSN et votre progression
- Le nombre total de trophées, dont vos Platines
- Vos statistiques : jeux terminés, taux de complétion, temps de jeu,
  classement mondial
- Les données restent affichées même sans connexion (cache hors ligne)

Aucune application à installer sur votre PC ou votre téléphone : tout
se configure directement sur l'écran, en Wi-Fi.

### Captures d'écran

> Générées depuis le simulateur (données de démonstration) -- le rendu
> sur l'écran réel est identique. À téléverser comme images sur la page
> MakerWorld.

- Accueil : `docs/screenshots/welcome.png`
- Tableau de bord : `docs/screenshots/dashboard.png`
- Trophées : `docs/screenshots/trophies.png`
- Statistiques : `docs/screenshots/statistics.png`

### Ce qu'il vous faut

- Une carte [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
  (écran rond 466×466, tactile capacitif) -- non fournie, à acheter
  séparément
- Un câble USB-C (données, pas seulement charge)
- Un compte [Pocket PSN](https://pocketpsn.com) actif (gratuit, service
  tiers qui fournit les données de trophées PlayStation)
- Filament PLA -- blanc pour le corps, bleu pour le logo PlayStation et
  le motif gravés (optionnel : imprimable en une seule couleur si vous
  n'avez pas d'AMS/changeur de filament)
- 3 petits aimants (fermeture du boîtier -- pas de vis nécessaires pour
  le boîtier lui-même)

### Impression 3D

Le boîtier tient en **2 pièces** : une façade et un dos, assemblées par
aimants (aucune vis). Le dos porte le logo PlayStation et un motif
décoratif, gravés en 2ᵉ couleur.

- Imprimante utilisée pour la conception : Bambu Lab X1 Carbon (buse 0,4 mm)
- Matériau : PLA -- blanc (corps), bleu (logo/motif gravés)
- Profil : "0,28 mm Extra Draft" (impression rapide)
- Hauteur de couche : 0,28 mm (première couche 0,2 mm)
- Parois : 2
- Remplissage : 5 %
- Supports : oui (arborescents automatiques)
- Poids de filament : ~31 g (blanc + bleu)
- Temps d'impression : ~55 min

*(Estimations Bambu Studio, imprimante et profil ci-dessus -- peut
varier avec un autre matériel.)*

### Montage

1. Imprimez la façade et le dos (voir réglages ci-dessus).
2. Insérez les 3 aimants dans leurs logements sur le dos.
3. Placez la carte Waveshare dans la façade.
4. Faites passer le câble USB-C par l'ouverture prévue.
5. Refermez avec le dos -- les aimants maintiennent l'ensemble, aucune
   vis nécessaire.

### Installer le firmware (aucune compétence technique requise)

1. Ouvrez **Google Chrome** ou **Microsoft Edge** (obligatoire, les
   autres navigateurs ne fonctionnent pas pour cette étape).
2. Branchez la carte à votre ordinateur en USB-C.
3. Allez sur la page d'installation :
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/**
4. Cliquez sur **Installer** et suivez les instructions à l'écran.
5. Une fois l'installation terminée, débranchez et rebranchez la carte.

Aucun logiciel à télécharger, aucune ligne de commande.

### Première configuration

1. La carte crée son propre réseau Wi-Fi : `TrophyDisplay-Setup`.
2. Connectez-vous à ce réseau depuis votre téléphone ou votre PC, puis
   ouvrez `http://192.168.4.1/` dans un navigateur.
3. Sélectionnez votre réseau Wi-Fi habituel, entrez son mot de passe,
   renseignez votre pseudo PSN, puis cliquez une seule fois sur
   **Enregistrer**.
4. La carte redémarre et se connecte automatiquement. Vos trophées
   s'affichent en quelques secondes.

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

A small connected display that shows your PlayStation level, trophies
and stats in real time -- sitting on your desk, next to your PC or your
gaming setup. Round AMOLED screen, data updates automatically, zero
maintenance once set up.

### What it is

This project combines a 3D-printed part with a small computer (an
ESP32-S3 board with a built-in touchscreen) that connects to your
PlayStation account through [Pocket PSN](https://pocketpsn.com) to
continuously display:

- Your PSN level and progress
- Your total trophy count, including Platinums
- Your stats: games completed, completion rate, playtime, world rank
- Data stays on screen even without a connection (offline cache)

No app to install on your PC or phone: everything is set up directly
on the screen, over Wi-Fi.

### Screenshots

> Generated from the simulator (demo data) -- the real screen looks
> identical. Upload these as images on the MakerWorld page.

- Welcome: `docs/screenshots/welcome.png`
- Dashboard: `docs/screenshots/dashboard.png`
- Trophies: `docs/screenshots/trophies.png`
- Statistics: `docs/screenshots/statistics.png`

### What you need

- A [Waveshare ESP32-S3-Touch-AMOLED-1.75](https://www.waveshare.com/esp32-s3-touch-amoled-1.75.htm)
  board (round 466x466 screen, capacitive touch) -- not included, buy
  separately
- A USB-C cable (data-capable, not charge-only)
- An active [Pocket PSN](https://pocketpsn.com) account (free
  third-party service that provides PlayStation trophy data)
- PLA filament -- white for the body, blue for the embossed PlayStation
  logo and accent shape (optional: printable in a single color if you
  don't have an AMS/filament changer)
- 3 small magnets (case closure -- no screws needed for the case itself)

### 3D printing

The case is **2 parts**: a front cover and a back shell, held together
with magnets (no screws). The back shell carries the PlayStation logo
and a decorative accent, embossed in a second color.

- Printer used for the design: Bambu Lab X1 Carbon (0.4 mm nozzle)
- Material: PLA -- white (body), blue (embossed logo/accent)
- Profile: "0.28mm Extra Draft" (fast print)
- Layer height: 0.28 mm (first layer 0.2 mm)
- Wall loops: 2
- Infill: 5%
- Supports: yes (tree, automatic)
- Filament weight: ~31 g (white + blue)
- Print time: ~55 min

*(Bambu Studio estimates, printer and profile above -- may vary with
other hardware.)*

### Assembly

1. Print the front cover and back shell (see settings above).
2. Insert the 3 magnets into their sockets on the back shell.
3. Place the Waveshare board into the front cover.
4. Route the USB-C cable through the provided opening.
5. Close with the back shell -- the magnets hold everything together,
   no screws needed.

### Installing the firmware (no technical skills required)

1. Open **Google Chrome** or **Microsoft Edge** (required -- other
   browsers won't work for this step).
2. Plug the board into your computer via USB-C.
3. Go to the install page:
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/**
4. Click **Install** and follow the on-screen instructions.
5. Once installation is complete, unplug and replug the board.

No software to download, no command line.

### First-time setup

1. The board creates its own Wi-Fi network: `TrophyDisplay-Setup`.
2. Connect to that network from your phone or computer, then open
   `http://192.168.4.1/` in a browser.
3. Select your home Wi-Fi network, enter its password, enter your PSN
   username, then click **Save** once.
4. The board restarts and connects automatically. Your trophies show
   up within seconds.

### Source code

The full firmware is open source:
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

### Credits

- PlayStation data provided by [Pocket PSN](https://pocketpsn.com).
- Design and development: Kevin Torres.

### License

Firmware under the MIT license. [3D MODEL LICENSE -- specify on the
MakerWorld page itself, e.g. CC BY-NC-SA 4.0]
