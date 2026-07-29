# Premier demarrage -- guide express

Deux facons d'installer le firmware :

- **Sans rien installer** (recommande) : ouvrez la
  [page d'installation web](https://kevtrs.github.io/PlayStation-Trophy-Display-AMOLED/webinstall/)
  depuis Chrome ou Edge, branchez la carte en USB, cliquez sur "Installer".
  Passez directement a la section "Configurer le Wi-Fi et Pocket PSN"
  ci-dessous une fois le flash termine.
- **Avec PlatformIO** (pour compiler/modifier le code) : suivez les etapes
  ci-dessous.

## Ce qu'il faut faire (installation PlatformIO)

1. Branchez l'ESP32-S3 au PC avec un cable USB-C **de donnees** (pas un
   cable de charge seul).
2. Double-cliquez sur `INSTALLER_ET_LANCER.bat`.
3. Si plusieurs ports serie plausibles sont detectes, choisissez le bon
   dans la petite liste numerotee affichee (sinon le script choisit seul).
4. Attendez : compilation, ecriture du systeme de fichiers, flash, puis
   ouverture automatique du moniteur serie.
5. A la fin, l'ecran doit s'allumer et afficher le Dashboard (donnees de
   demonstration si le Wi-Fi/Pocket PSN n'est pas encore configure --
   voir plus bas, c'est normal et attendu).

Le tout premier demarrage peut prendre quelques secondes de plus qu'un
redemarrage normal (initialisation ecran/tactile/Wi-Fi). Dans le moniteur
serie, la progression reelle s'affiche ainsi, dans cet ordre :

```
[BOOT] System
[BOOT] Display: CO5300 initialise (466x466)
[BOOT] Touch: ... detecte, ... point(s) supporte(s)
[BOOT] Config
[BOOT] Cache: absent (premier demarrage)
[BOOT] Network: initialisation demarree
[BOOT] PocketPSN: reseau absent, cache/mode demo utilise
[BOOT] UI ready
```

Si la sequence s'arrete avant `UI ready`, notez la derniere ligne affichee
-- c'est exactement l'endroit ou chercher (voir `DEPANNAGE_MATERIEL.md`).

Pour quitter le moniteur serie a tout moment : `Ctrl+C`.

## Mode demonstration (normal au premier flash)

Une carte neuve n'a ni Wi-Fi ni compte Pocket PSN configures : l'application
affiche automatiquement des **donnees de demonstration** (`DemoDataProvider`)
plutot que de bloquer ou d'afficher une erreur -- c'est le comportement
voulu, pas un bug. Vous pouvez des maintenant valider a l'ecran : navigation
tactile, animations, Dashboard, Trophees, Statistiques, Parametres, About --
tout fonctionne deja avec ces donnees factices.

Rien dans le firmware ne bascule automatiquement en mode reel : c'est
`ProviderFactory::shouldUsePocketPsn()` qui decide, uniquement a partir de
la configuration (voir plus bas).

## Configurer le Wi-Fi et Pocket PSN (aucun fichier a modifier)

Aucune modification de code n'est necessaire. Sans Wi-Fi enregistre,
l'appareil ouvre lui-meme un point d'acces de secours :

1. Depuis un telephone/PC, connectez-vous au reseau Wi-Fi
   **`TrophyDisplay-Setup`** (ouvert, sans mot de passe).
2. Ouvrez `http://192.168.4.1/` dans un navigateur.
3. Selectionnez votre reseau Wi-Fi (bouton "Scanner"), entrez le mot de
   passe, puis (optionnel, pour les vraies donnees) votre pseudo PSN --
   aucune cle API a saisir, elle est deja integree au firmware.
4. Cliquez une seule fois sur "Enregistrer" : reseau, mot de passe et
   pseudo sont sauvegardes ensemble, puis l'appareil redemarre
   automatiquement quelques secondes apres (message affiche a l'ecran/dans
   la page).
5. Apres redemarrage, le log `[BOOT] PocketPSN: synchronisation reussie`
   (ou `echec`) confirme le nouveau mode.

Voir `docs/BUILD_FLASH_FIRSTBOOT.md` (section 6) pour le detail complet.

### Avertissement TLS connu (Pocket PSN)

`src/network/PocketPsnHttpClient.cpp` utilise actuellement
`WiFiClientSecure::setInsecure()` : la connexion Pocket PSN est chiffree
mais **le certificat du serveur n'est pas verifie**. C'est un compromis
assume pour ce premier essai (voir le TODO dans le fichier et
`docs/BUILD_FLASH_FIRSTBOOT.md` section 7) -- pas une negligence oubliee,
et pas non plus quelque chose de "deja securise". Ne pas bloquer le premier
essai materiel pour cette raison ; a corriger dans une passe dediee une
fois qu'une vraie connexion aura permis d'observer le certificat reel du
serveur.

## En cas de probleme

| Symptome | Aller voir |
|---|---|
| Aucun port detecte | `DEPANNAGE_MATERIEL.md` > Flash > aucun port |
| Le flash echoue | `DEPANNAGE_MATERIEL.md` > Flash |
| Ecran noir | `DEPANNAGE_MATERIEL.md` > Ecran noir |
| Tactile ne repond pas | `DEPANNAGE_MATERIEL.md` > Tactile |
| Wi-Fi ne fonctionne pas | Verifiez d'abord le point d'acces `TrophyDisplay-Setup` (section ci-dessus) ; sinon `docs/HARDWARE_TEST_CHECKLIST.md` section 2 |

## Lancer le diagnostic materiel

Si l'ecran/le tactile semblent en cause independamment de PocketPSN/Wi-Fi,
double-cliquez sur `DIAGNOSTIC_MATERIEL.bat`. Il flashe un firmware de test
**separe et independant** (mires de couleur, cercle, reperes de coin, test
tactile en direct) qui ne touche jamais a votre configuration.

## Revenir au firmware normal

Apres un diagnostic, relancez simplement `INSTALLER_ET_LANCER.bat` (ou
`DIAGNOSTIC_MATERIEL.bat -RestoreNormal` en ligne de commande) pour
reflasher le firmware normal.

## Recuperer les logs

Chaque execution des scripts ecrit un journal horodate dans `logs/`
(`build_*.log`, `flash_*.log`, `serial_*.log`, `diag_*.log`). Ces fichiers
ne sont jamais envoyes/partages automatiquement -- joignez-les manuellement
si vous demandez de l'aide.

## Informations a transmettre en cas de bug

- Le fichier `logs/serial_*.log` (ou `diag_serial_*.log`) de la session
  concernee.
- La derniere ligne `[BOOT] ...` affichee avant le blocage eventuel.
- Le modele exact de cable/adaptateur USB utilise si le flash echoue.
- Une photo de l'ecran si le probleme est visuel (couleurs, rotation,
  decoupage).
- **Ne partagez jamais** votre cle Pocket PSN ni votre mot de passe Wi-Fi --
  ils ne devraient de toute facon jamais apparaitre dans les logs (voir
  `src/utils/Logger.h`).
