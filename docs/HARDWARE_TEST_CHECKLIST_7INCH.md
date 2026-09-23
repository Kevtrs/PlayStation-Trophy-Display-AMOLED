# Checklist de validation materielle -- board 7" (Waveshare ESP32-S3-Touch-LCD-7)

A remplir lors du premier flash sur la carte reelle.

`HARDWARE_TEST_CHECKLIST.md` couvre l'ecran **rond** (CO5300 466x466,
tactile CST9217) : elle ne s'applique pas ici. Ce board a un panneau RGB
800x480 et un tactile GT911, et n'avait aucune procedure de validation
jusqu'ici.

Tout ce qui suit a ete valide **au simulateur uniquement**. Cette
checklist ne couvre donc que ce que le simulateur ne peut pas prouver :
le materiel, la latence tactile reelle, la lisibilite physique de
l'ecran, et le vrai appel reseau.

Cocher au fur et a mesure. Noter tout ecart, meme mineur : c'est la
premiere fois que ce code tourne sur du silicium.

---

## 1. Demarrage

Brancher en USB-C (port **UART**) et ouvrir le moniteur serie.

- [ ] `[BOOT] Display: Waveshare:ESP32-S3-Touch-LCD-7 initialise (800x480)`
      apparait -- pas `Board::init() a echoue` ni `LCD introuvable`.
- [ ] `[BOOT] Touch: GT911 enregistre dans LVGL` apparait.
      - Si `[BOOT] Touch: GT911 polling initialise sans IRQ (adresse 0x..)`
        apparait avant, c'est normal : le repli sans interruption a pris
        la main. Noter l'adresse.
      - Si `[BOOT] Touch: GT911 indisponible, l'ecran reste en defilement
        passif` apparait, **tout le chapitre 3 est a sauter** : l'appareil
        fonctionne, mais sans tactile.
- [ ] `[BOOT] LVGL demarre (tache dediee, 800x480)` apparait.
- [ ] `[BOOT] Pret (mode ...)` apparait, sans redemarrage en boucle.
- [ ] L'ecran affiche une image nette, pas de bandes ni de scintillement
      (le panneau RGB est sensible au timing : tout artefact se voit ici).

## 2. Premiere configuration : le QR

C'est l'ecran que verra un inconnu qui deballe l'objet. Le tester avec
un telephone qui n'a jamais vu cet appareil.

- [ ] Sans configuration existante, l'ecran affiche le QR dans son cadre
      d'anneaux, avec `TrophyDisplay-Setup` dessous.
- [ ] **Scanner le QR avec l'appareil photo du telephone, a distance de
      bras (~30 cm).** Il doit proposer de rejoindre le reseau.
      - Le code fait environ 27 mm sur la dalle. S'il ne se lit pas a
        cette distance, essayer de plus pres et **noter la distance
        limite** : c'est la mesure qui manque, le simulateur ne peut pas
        la donner.
- [ ] Accepter : le telephone rejoint `TrophyDisplay-Setup`.
- [ ] La page de configuration s'ouvre **toute seule** (portail captif).
      Si elle ne s'ouvre pas, ouvrir `192.168.4.1` a la main et le noter :
      la promesse "rien a recopier" de la fiche MakerWorld en depend.
- [ ] Renseigner Wi-Fi + pseudo PSN, enregistrer : l'appareil redemarre et
      se connecte.

## 3. Navigation tactile

Le coeur de ce qui n'a jamais tourne sur materiel. Le GT911 est en
polling **sans interruption** : c'est precisement la latence de ce mode
qui est inconnue.

- [ ] **Au repos**, l'appareil defile seul, un ecran toutes les 7 s,
      **sans aucun controle affiche** (ni fleches, ni points, ni
      engrenage). C'est le mode par defaut et il doit le rester.
- [ ] **Toucher l'ecran n'importe ou** : les controles apparaissent et le
      defilement se met en pause.
      - Le moniteur serie affiche `[UI 7"] Mode interactif (t=..., slide=N)`.
      - [ ] La reaction est-elle immediate, ou y a-t-il un retard
        perceptible ? **Noter l'impression**, c'est le point qui decidera
        s'il faut revoir la cadence de scrutation.
- [ ] **Swiper vers la gauche** : passage a l'ecran suivant.
- [ ] **Swiper vers la droite** : retour a l'ecran precedent.
      - [ ] Un swipe franc saute-t-il parfois deux ecrans d'un coup ?
        L'anti-rebond est fixe a 250 ms, valeur choisie au jugement et
        jamais mesuree au doigt.
- [ ] **Taper un point de pagination** en bas : saut direct a cet ecran,
      le point vise devient cyan.
      - [ ] Les points sont-ils faciles a viser ? La cible fait 44 px
        alors que le point dessine en fait 8. Si viser demande de
        l'attention, la cible est a agrandir.
- [ ] **Taper l'engrenage** en bas a gauche : l'ecran Reglages s'ouvre.
- [ ] **Taper Revenir** (en haut a gauche) : retour au defilement.
- [ ] **Ne plus toucher pendant 20 s** : les controles disparaissent et le
      defilement reprend seul. Le serie affiche
      `[UI 7"] Mode ambiant (t=..., slide=N)`.
- [ ] Aucun appui ne provoque de gel ni de redemarrage, meme en tapant
      vite et en enchainant les gestes.

## 4. Synchronisation Pocket PSN reelle

- [ ] Apres configuration, le log indique `mode Pocket PSN` (pas
      `mode demo`).
- [ ] Les vraies valeurs du compte s'affichent (niveau, trophees, rang) --
      pas les donnees de demonstration.
- [ ] **Verifier que le firmware flashe embarque bien une cle API.** S'il a
      ete construit sur une machine ou `include/secrets.h` porte une cle
      vide, aucune donnee ne remontera : c'est le piege documente dans le
      revert de la PR #1.
- [ ] Couper le Wi-Fi : le dernier profil connu reste affiche (cache hors
      ligne), l'appareil ne se vide pas.

## 5. Celebration

Le seul point qui demande de la patience : il faut un trophee reellement
decroche entre deux synchronisations.

- [ ] Decrocher un trophee, puis attendre la synchro suivante (30 min par
      defaut, ou forcer une resynchronisation).
- [ ] Le serie affiche
      `[UI 7"] Nouveau(x) trophee(s) : +N (P.. O.. A.. B..)`.
- [ ] L'ecran de celebration s'affiche et interrompt le defilement.
- [ ] L'animation d'entree se joue **sans a-coups** : anneaux en fondu,
      trophee qui arrive avec un rebond, textes qui montent en cascade.
      - [ ] Y a-t-il du saccade ? Sur PC c'est fluide, mais l'ESP32-S3
        n'a pas la meme puissance. C'est la mesure qui manque.
- [ ] L'ecran reste ~10 s puis rend la main au defilement.
- [ ] Le palier mis en avant est bien le plus prestigieux gagne.

## 6. Duree et stabilite

- [ ] Laisser tourner **plusieurs heures** sans y toucher : pas de fuite
      memoire visible (le defilement reconstruit un ecran entier toutes
      les 7 s), pas de redemarrage, pas de ralentissement progressif.
- [ ] Laisser tourner une nuit : l'ecran est toujours allume et a jour le
      matin.

---

## Ce qu'il faut remonter

Trois mesures interessent directement le code, et aucune n'est
observable au simulateur :

1. **Latence tactile** du GT911 en polling sans IRQ.
2. **Distance de lecture** du QR sur la dalle.
3. **Fluidite de l'animation** de celebration sur l'ESP32-S3.

Les valeurs correspondantes (`kNavDebounceMs` 250 ms, taille de cible
44 px, taille du QR, duree des animations) ont ete choisies au jugement
et attendent d'etre confrontees au reel.
