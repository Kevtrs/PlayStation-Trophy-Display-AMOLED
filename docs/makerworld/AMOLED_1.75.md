# PlayStation Trophy Display -- afficheur de trophées (écran rond AMOLED)

> Texte prêt à copier-coller dans la description de la page MakerWorld.
> Les passages entre crochets `[...]` sont à compléter avec les infos
> d'impression réelles (matériau, réglages, temps) -- tout le reste est
> vérifié et exact.

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
- [MATÉRIAU D'IMPRESSION -- ex : PLA, X g]
- [VISSERIE / INSERTS À CHAUD si nécessaire, quantité et taille]

## Impression 3D

- Matériau recommandé : [À COMPLÉTER]
- Couleur utilisée sur les photos : [À COMPLÉTER]
- Hauteur de couche : [À COMPLÉTER]
- Remplissage : [À COMPLÉTER]
- Supports : [À COMPLÉTER -- oui/non, quelles pièces]
- Temps d'impression total estimé : [À COMPLÉTER]
- Orientation recommandée : [À COMPLÉTER]

## Montage

1. [Étape 1 -- ex : insérer la carte Waveshare dans le boîtier avant]
2. [Étape 2 -- ex : fixer avec les vis fournies]
3. [Étape 3 -- ex : faire passer le câble USB-C par l'ouverture arrière]
4. [Étape 4 -- ex : refermer le boîtier]

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
