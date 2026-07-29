# Depannage materiel

Arborescence de symptomes pour le premier essai sur l'ESP32-S3 reel. Voir
`PREMIER_DEMARRAGE.md` pour la procedure normale, et
`docs/HARDWARE_TEST_CHECKLIST.md` pour la checklist complete de validation.

```
Ecran noir
├── aucun log serie              -> voir "Aucun log serie" ci-dessous
├── logs presents mais ecran non initialise -> "gfx->begin() a echoue"
├── ecran allume mais image absente -> "Ecran allume, image absente"
└── image deformee               -> "Image deformee / couleurs / rotation"

Tactile
├── aucune detection             -> "Tactile: aucune detection"
├── axes inverses                -> "Tactile: axes/orientation"
├── coordonnees decalees         -> "Tactile: axes/orientation"
└── faux appuis                  -> "Tactile: faux appuis"

Flash
├── aucun port                   -> "Flash: aucun port"
├── port occupe                  -> "Flash: port occupe"
├── bootloader inaccessible      -> "Flash: bootloader inaccessible"
└── mauvais cable                -> "Flash: mauvais cable"
```

---

## Ecran noir

### Aucun log serie

Le moniteur serie n'affiche rien du tout, meme pas `[BOOT] System`.

- Cause la plus probable : mauvais port choisi, ou carte pas alimentee.
- Verifiez que la LED d'alimentation de la carte est allumee.
- Rebranchez le cable USB, relancez `INSTALLER_ET_LANCER.bat`.
- Si le port choisi semble correct mais rien ne s'affiche, essayez un
  autre port USB du PC (certains hubs ne fournissent pas assez de courant).

### `gfx->begin() a echoue`

Le log affiche `[BOOT] Display: gfx->begin() a echoue` (ou l'equivalent
avant l'ajout des logs `[BOOT]`).

- Cause la plus probable : mauvais cablage QSPI, ou mauvaise revision de
  carte par rapport a `include/BoardConfig.h`.
- Verifiez que la carte est bien un Waveshare ESP32-S3-Touch-AMOLED-1.75
  (466x466, controleur CO5300) -- une autre carte AMOLED aurait des
  broches differentes.
- Lancez `DIAGNOSTIC_MATERIEL.bat` : s'il echoue aussi de la meme facon,
  le probleme est bien materiel/broches, pas logiciel (LVGL).

### Ecran allume, image absente

Le rétroeclairage semble actif (ecran gris/blanchatre uniforme) mais aucun
contenu ne s'affiche.

- Lancez `DIAGNOSTIC_MATERIEL.bat` : s'il affiche correctement les mires
  de couleur (rouge/vert/bleu...), le probleme vient du firmware normal
  (LVGL/design), pas du materiel -- notez a quelle etape `[BOOT]` le
  firmware normal s'arrete.
- Si le diagnostic AUSSI reste blanc/noir : probleme materiel plus profond
  (alimentation ecran, buffer DMA trop grand pour la PSRAM disponible --
  verifiez `PSRAM: <n> octets detectes` dans les logs).

### Image deformee / couleurs / rotation

- Utilisez `DIAGNOSTIC_MATERIEL.bat` : l'ecran "reperes de coin + texte
  centre" affiche 4 carres de couleur differente dans chaque coin
  (rouge=haut-gauche, vert=haut-droite, bleu=bas-gauche, jaune=bas-droite).
  Si les couleurs sont a la mauvaise position, c'est un probleme de
  rotation/miroir ; si les couleurs elles-memes sont fausses (ex: rouge et
  bleu inverses), c'est un probleme RGB/BGR.
- Ces reglages sont centralises dans la construction de `Arduino_CO5300`
  (`src/main.cpp` et `src_diagnostic/main.cpp`, parametre `rotation`) --
  ne pas les modifier au hasard ; documenter l'observation exacte (quel
  coin montre quelle couleur) avant de changer quoi que ce soit.

---

## Tactile

### Tactile: aucune detection

Le log affiche `[BOOT] Touch: CST9217 introuvable sur I2C`.

- Verifiez `docs/HARDWARE_TEST_CHECKLIST.md` section 1 -- un echec tactile
  n'empeche pas le reste de demarrer (non bloquant), mais aucune
  interaction ne sera possible.
- Cause la plus probable : bus I2C partage (adresse, cablage SDA/SCL) ou
  alimentation tactile absente.
- `DIAGNOSTIC_MATERIEL.bat` affiche le meme message de maniere isolee,
  sans dependre du reste du firmware -- utile pour confirmer que ce n'est
  pas une regression logicielle recente.

### Tactile: axes/orientation

Toucher un coin de l'ecran reagit comme si un autre coin avait ete touche.

- Lancez `DIAGNOSTIC_MATERIEL.bat`, allez jusqu'a l'ecran "Test tactile" :
  il affiche en direct les coordonnees X/Y exactes de chaque appui. Touchez
  les 4 coins un par un et notez les valeurs obtenues.
- Comparez avec la resolution attendue (0-466 sur chaque axe). Un axe
  invers e(X croissant vers la gauche au lieu de la droite) ou permute
  (X et Y echanges) se voit immediatement sur cet ecran de test.
- Le mapping se regle via `touch.setMaxCoordinates(LCD_WIDTH, LCD_HEIGHT)`
  (meme code dans `src/main.cpp` et `src_diagnostic/main.cpp`) -- documenter
  precisement l'anomalie observee avant de modifier quoi que ce soit.

### Tactile: faux appuis

Des appuis apparaissent sans qu'on touche l'ecran, ou un appui reste
"colle".

- Verifiez l'interruption tactile (broche `TP_INT`, voir `BoardConfig.h`)
  -- un flottement electrique sur cette broche peut generer de faux
  declenchements.
- Verifiez qu'aucun autre peripherique ne partage le meme bus I2C de
  maniere conflictuelle.

---

## Flash

### Flash: aucun port

`INSTALLER_ET_LANCER.bat` affiche "AUCUN PORT SERIE DETECTE".

1. Verifiez que le cable USB supporte les donnees (pas seulement la
   charge) -- c'est la cause la plus frequente.
2. Verifiez que la carte est bien alimentee (LED allumee).
3. Installez le pilote USB-UART si Windows ne reconnait pas le
   peripherique (CP210x pour la plupart des cartes Waveshare -- cherchez
   "CP210x Windows driver" chez Silicon Labs si besoin).
4. Rebranchez la carte, relancez le script.

### Flash: port occupe

Le port est detecte mais le flash echoue immediatement avec une erreur
d'acces/permission.

- Fermez tout autre programme qui pourrait utiliser ce port : moniteur
  serie deja ouvert, Arduino IDE, PlatformIO IDE, autre instance de ce
  script.
- Redemarrez le script.

### Flash: bootloader inaccessible

Le port est detecte, aucun autre programme ne l'utilise, mais le flash
echoue avec un timeout/absence de reponse du bootloader.

Procedure (affichee automatiquement par le script) :
1. Maintenez le bouton **BOOT**.
2. Appuyez brievement sur **RESET**.
3. Relachez **RESET**.
4. Relachez **BOOT**.
5. Appuyez sur Entree pour reessayer.

Cette combinaison force la carte a rester en mode bootloader le temps que
l'outil de flash s'y connecte. Le script reessaie automatiquement jusqu'a
3 fois.

### Flash: mauvais cable

Certains cables USB-C ne cablent que l'alimentation (pas les lignes de
donnees D+/D-) -- la carte se charge mais n'apparait jamais comme port
serie. Essayez un autre cable, idealement celui fourni avec la carte.
