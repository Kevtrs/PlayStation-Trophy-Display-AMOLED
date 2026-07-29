# Audit visuel -- version wireframe (avant refonte)

Reference : captures de `backup/wireframe-v1` (branche de sauvegarde creee le
2026-07-14 avant la refonte graphique). Sert de base de comparaison pour la
planche avant/apres (`simulator/previews/before-after-board.png`).

## Welcome
- Un simple cercle bleu (arc fin monochrome) avec un point blanc central : ne
  se lit pas comme un trophee, aucune identite visuelle.
- Fond 100% noir uni, aucune texture ni profondeur.
- Bouton "Configurer mon profil PSN" : rectangle bleu plat, aucun degrade,
  aucune ombre, aucun symbole.
- Aucune animation.

## Dashboard
- Anneau de progression monochrome (bleu uni), pas de degrade bleu->violet.
- Deux "badges" lateraux identiques (rectangles arrondis gris fonce) --
  aucune icone, aucune distinction visuelle entre "Trophees" et "Platine".
- Pseudo/niveau en simple texte empile, pas de hierarchie forte (tailles
  trop proches), pas d'avatar/symbole de profil.
- Valeur centrale (72%) correcte en taille mais sans mise en valeur
  (pas de halo, pas d'effet de profondeur autour de l'anneau).
- Aucune animation (l'anneau apparait déjà a sa valeur finale).

## Trophees
- Rendu comme une liste de parametres : point colore + libelle + valeur,
  separateurs fins. Aucune icone de trophee, aucun rendu metallique.
- Les 4 lignes sont visuellement identiques (seule la couleur du point
  change) -- aucune hierarchie, aucune impression de collection/recompense.
- Beaucoup d'espace vide a gauche et a droite de chaque ligne.

## Statistiques
- Grille 2x2 stricte avec croix de separation -- ressemble a un tableau de
  bord de tableur, pas a une interface produit.
- Aucune icone, aucune jauge/anneau par statistique, aucun centre visuel.
- Couleurs uniformes (blanc/gris) sans hierarchie secondaire.

## Decisions de variantes (2026-07-14)

### Dashboard
- **Variante A retenue** (`simulator/previews/dashboard-variant-a.png`) :
  avatar + pseudo + pastille niveau au-dessus d'un anneau degrade
  bleu->violet avec halo, badges lateraux illustres (icones trophee/platine).
- Variante B ecartee (`dashboard-variant-b.png`) : anneau surdimensionne
  (210px) avec avatar en badge sur l'anneau et pseudo relegue en petit texte
  sous des puces -- visuellement audacieux mais hierarchie identite/
  progression moins claire qu'en variante A (le pseudo, information
  identitaire, devient moins visible que le pourcentage).

### Trophees
- **Variante A retenue** (`trophies-variant-a.png`) : liste verticale de
  cartes, medaillon illustre + libelle + grand nombre colore, bordure
  accent gauche par palier. Tres lisible a taille reelle, aucun
  chevauchement.
- Variante B ecartee (`trophies-variant-b.png`) : composition orbitale (4
  medailles aux points cardinaux autour d'un total central). Visuellement
  original mais nombres plus petits et moins lisibles a taille reelle sur
  466px, et redondant avec le total deja visible sur le Dashboard.

## Assets et animations ajoutes
- 19 images procedurales generees par
  `tools/asset_pipeline/generate_assets.py` (trophee illustre, 4+4
  medailles pleine/petite taille, 3 halos, 4 icones de statistiques, 2
  icones de badge, texture de fond) -- voir `docs/ASSET_LICENSES.md`.
- Animations reelles verifiees dans le simulateur : entree decalee
  (fondu+zoom) de chaque widget, pulsation continue des halos (Welcome,
  Sync), anneau de progression anime vers sa valeur, compteur progressif du
  pourcentage, toast de celebration "nouveau trophee" sur la couche
  superieure LVGL (visible depuis n'importe quel ecran).
- Bugs reels rencontres et corriges pendant cette passe : halo carre au
  lieu de circulaire (rayon normalise sur la diagonale au lieu du plus
  petit cote), superposition de libelles due au padding par defaut du theme
  LVGL sur des conteneurs imbriques, chevauchement point colore/texte sur
  alignement centre a largeur variable, `lv_img_set_zoom` + pivot par defaut
  qui rognait/decalait le rendu (resolu en generant des medaillons a taille
  native plus petite plutot qu'en zoomant).

## Constat general
- Direction "wireframe fonctionnel" : prouve la logique et la navigation,
  mais aucune identite de marque, aucune texture, aucun halo, aucune
  animation d'entree, aucune iconographie. C'est la base de depart de la
  refonte -- pas le design final (confirme par l'utilisateur le 2026-07-14).
