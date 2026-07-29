# PlayStation Trophy Display -- afficheur de trophées (écran rond AMOLED)

> Texte prêt à copier-coller dans la description de la page MakerWorld.
> Réglages et estimations extraits du projet Bambu Studio fourni
> (`PocketPSN Trophy.3mf`, tranché sur Bambu Lab X1 Carbon). Peut varier
> légèrement avec une autre imprimante.

## Accroche

Un petit afficheur connecté qui montre en direct votre niveau, vos
trophées et vos statistiques PlayStation -- posé sur votre bureau, à
côté de votre PC ou de votre setup gaming. Écran rond AMOLED, données
mises à jour automatiquement, zéro maintenance une fois installé.

## Ce que c'est

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

## Captures d'écran

> Générées depuis le simulateur (données de démonstration) -- le rendu
> sur l'écran réel est identique. À téléverser comme images sur la page
> MakerWorld (le lien Markdown ci-dessous ne s'affichera pas tel quel
> sur MakerWorld, c'est juste une référence).

| Accueil | Tableau de bord | Trophées | Statistiques |
|---|---|---|---|
| ![Accueil](../screenshots/welcome.png) | ![Tableau de bord](../screenshots/dashboard.png) | ![Trophées](../screenshots/trophies.png) | ![Statistiques](../screenshots/statistics.png) |

## Ce qu'il vous faut

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

## Impression 3D

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

## Montage

1. Imprimez la façade et le dos (voir réglages ci-dessus).
2. Insérez les 3 aimants dans leurs logements sur le dos.
3. Placez la carte Waveshare dans la façade.
4. Faites passer le câble USB-C par l'ouverture prévue.
5. Refermez avec le dos -- les aimants maintiennent l'ensemble, aucune
   vis nécessaire.

## Installer le firmware (aucune compétence technique requise)

1. Ouvrez **Google Chrome** ou **Microsoft Edge** (obligatoire, les
   autres navigateurs ne fonctionnent pas pour cette étape).
2. Branchez la carte à votre ordinateur en USB-C.
3. Allez sur la page d'installation :
   **https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/**
4. Cliquez sur **Installer** et suivez les instructions à l'écran.
5. Une fois l'installation terminée, débranchez et rebranchez la carte.

Aucun logiciel à télécharger, aucune ligne de commande.

## Première configuration

1. La carte crée son propre réseau Wi-Fi : `TrophyDisplay-Setup`.
2. Connectez-vous à ce réseau depuis votre téléphone ou votre PC, puis
   ouvrez `http://192.168.4.1/` dans un navigateur.
3. Sélectionnez votre réseau Wi-Fi habituel, entrez son mot de passe,
   renseignez votre pseudo PSN, puis cliquez une seule fois sur
   **Enregistrer**.
4. La carte redémarre et se connecte automatiquement. Vos trophées
   s'affichent en quelques secondes.

## Code source

Le firmware complet est open source :
**https://github.com/Kevtrs/PlayStation-Trophy-Display-AMOLED**

## Crédits

- Données PlayStation fournies par [Pocket PSN](https://pocketpsn.com).
- Conception et développement : Kevin Torres.

## Licence

Firmware sous licence MIT. [LICENCE DU MODÈLE 3D -- à préciser sur la
page MakerWorld elle-même, ex : CC BY-NC-SA 4.0]
