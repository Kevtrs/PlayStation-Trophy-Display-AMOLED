# PlayStation Trophy Display -- afficheur de trophées (écran 7" tactile)

> Texte prêt à copier-coller dans la description de la page MakerWorld.
> Réglages et estimations extraits du projet Bambu Studio fourni
> (`PocketPSN Trophy.3mf`, tranché sur Bambu Lab X1 Carbon). Peut varier
> légèrement avec une autre imprimante.

## Accroche

La version grand format de l'afficheur de trophées PlayStation : un
écran tactile 7 pouces qui affiche en direct votre niveau, vos trophées
et vos statistiques -- posé sur votre bureau, une étagère, ou à côté de
votre setup gaming.

## Ce que c'est

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

## Captures d'écran

> Générées depuis le simulateur (données de démonstration) -- le rendu
> sur l'écran réel est identique. À téléverser comme images sur la page
> MakerWorld (le lien Markdown ci-dessous ne s'affichera pas tel quel
> sur MakerWorld, c'est juste une référence).

| Tableau de bord | Trophées | Statistiques |
|---|---|---|
| ![Tableau de bord](../screenshots_wide/dashboard.png) | ![Trophées](../screenshots_wide/trophies.png) | ![Statistiques](../screenshots_wide/statistics.png) |

## Ce qu'il vous faut

- Une carte [Waveshare ESP32-S3-Touch-LCD-7](https://www.waveshare.com/esp32-s3-touch-lcd-7.htm)
  (écran tactile 800×480, GT911) -- non fournie, à acheter séparément
- Un câble USB-C (données, pas seulement charge)
- Un compte [Pocket PSN](https://pocketpsn.com) actif (gratuit, service
  tiers qui fournit les données de trophées PlayStation)
- Filament PLA (une seule couleur suffit pour ce boîtier)
- 2 vis (taille à confirmer selon vos inserts/taraudage -- 2 plots de
  fixation sont prévus à l'intérieur du support)

## Impression 3D

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

## Montage

1. Imprimez le support (voir réglages ci-dessus).
2. Posez la carte Waveshare dans le support, plots de fixation alignés.
3. Vissez avec les 2 vis.
4. Faites passer le câble USB-C par l'ouverture prévue.

## Installer le firmware (aucune compétence technique requise)

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

## Première configuration

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

## Code source

Le firmware complet est open source :
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

## Crédits

- Données PlayStation fournies par [Pocket PSN](https://pocketpsn.com).
- Conception et développement : Kevin Torres.

## Licence

Firmware sous licence MIT. [LICENCE DU MODÈLE 3D -- à préciser sur la
page MakerWorld elle-même, ex : CC BY-NC-SA 4.0]
