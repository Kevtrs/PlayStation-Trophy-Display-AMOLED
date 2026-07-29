# Protocole Pocket PSN — inspection passive du firmware officiel

## Méthode utilisée (Phase 2.5)

Fichier inspecté : `PlaystationTrophy.bin` (release GitHub `Playstation_Trophy_API_1.0`
du 2025-12-06, asset unique, 4 194 304 octets — identique au fichier fourni par
l'utilisateur, taille vérifiée octet pour octet).

Méthode : extraction de **toutes les séquences de caractères ASCII imprimables
de longueur ≥ 5** contenues dans le binaire (equivalent de l'utilitaire Unix
`strings`), sans désassemblage, sans tentative de reconstruction du flot de
contrôle ou de la logique du programme. C'est une inspection strictement
passive des données déjà en clair dans le fichier tel que distribué
publiquement par l'auteur — pas une décompilation.

Ce qui n'a **pas** été fait, conformément à la consigne :
- pas de désassemblage x86/RISC-V du code ;
- pas de reconstruction du flot logique du firmware ;
- pas de tentative d'exécution du firmware (aucune carte ESP32-C3 + SSD1306
  disponible dans cet environnement, et le firmware ciblerait de toute façon
  un ESP32-C3, pas la carte AMOLED S3 visée par ce projet) ;
- pas de capture de trafic réseau réel (nécessiterait le matériel ci-dessus).

## Faits confirmés (chaînes trouvées telles quelles)

```
https://api.pocketpsn.com/PSTrophyDisplay/
psn_name=
&key=
application/x-www-form-urlencoded
---- HTTP POST ----
JSON parse error:
Bad JSON
Non-200 response from PocketPSN
Empty response from PocketPSN.
PocketPSN status:
Not on PocketPSN
PocketPSN data updated.
PSTrophyDisplayESP
```

`PSTrophyDisplayESP` (trouvée par recherche ciblée par mot-clé, jamais un
dump de la zone contenant la clé candidate identifiée en section 3 de
`AUDIT.md`) apparaît juste avant l'URL de l'endpoint dans le binaire —
valeur plausible d'un en-tête `User-Agent` envoyé par le firmware
d'origine, jamais confirmée par une capture réseau réelle. Piste testée
dans `tools/pocketpsn_probe/` (voir section "Diagnostic HTTP 200 + corps
vide" de son README) suite à deux tests réels renvoyant `HTTP 200` +
corps vide avec des identifiants qui ne semblent pas être en cause
(l'hypothèse d'une erreur de saisie de la clé a été écartée : testée par
saisie interactive puis par fichier local, même résultat).

Noms de champs JSON observés (chaînes exactes, dans cet ordre d'apparition
dans le binaire) :
```
Status
Username
Country
PSN Level
PSN Level Progress
PSN Level Remaining
Trophies Bronze
Trophies Silver
Trophies Gold
Trophies Plats
Trophies Total
Trophy Points
Pocket Points
Total Games
World Rank
Country Rank
Quick Stats
Games Completed
Completion Average
Average Rarity
Unearned Trophies
Hours Played
```

Ces noms correspondent aux métriques listées dans le README actuel du dépôt
(tableau "What It Does"), ce qui renforce la confiance qu'il s'agit bien des
clés JSON réelles et non de coïncidences.

## Requête déduite (non capturée, reconstituée à partir des chaînes ci-dessus)

```
POST https://api.pocketpsn.com/PSTrophyDisplay/
Content-Type: application/x-www-form-urlencoded

psn_name=<pseudo_psn>&key=<cle_api>
```

**Le paramètre `key` est confirmé nécessaire** (la chaîne `&key=` apparaît
juste après `psn_name=` dans le binaire). Sans cette clé, l'endpoint ne peut
pas être appelé de façon représentative de ce que fait le firmware original.

## Trouvaille sensible — NON réutilisée

Une chaîne de 30 caractères alphanumériques ressemblant à une clé d'API a été
trouvée dans le binaire, à proximité immédiate de la chaîne de l'URL de
l'endpoint. Elle n'est **pas reproduite dans ce dépôt** et **n'est utilisée
nulle part** dans `tools/pocketpsn_probe/` ni dans `PocketPsnProvider`. Si
cette chaîne est effectivement une clé Pocket PSN personnelle de l'auteur du
firmware d'origine, l'extraire et la réutiliser reviendrait à consommer un
accès tiers auquel nous n'avons pas droit — cela s'apparenterait à un vol
d'identifiants, même passif. Le projet exige que l'utilisateur obtienne sa
propre clé par un canal légitime.

## Verification reseau reelle effectuee (empreinte minimale)

Le 2026-07-13, `tools/pocketpsn_probe/probe.py` a ete execute **une seule
fois** avec un pseudo et une cle manifestement factices
(`ConnectivityTestOnly` / `invalid-test-key-not-a-real-credential`), dans le
seul but de verifier que l'outil fonctionne et que l'endpoint existe
reellement -- pas pour tenter d'obtenir de vraies donnees ni de contourner une
protection.

Resultat reel observe :
```
HTTP status  : 200
Content-Type : application/json; charset=utf-8
Corps         : vide (0 octet)
```

Ceci confirme que `https://api.pocketpsn.com/PSTrophyDisplay/` **existe
reellement et repond**. En revanche, avec des identifiants invalides, le
serveur repond **200 + corps vide** plutot qu'un code d'erreur HTTP explicite
-- l'outil ne peut donc pas, en l'etat, distinguer "cle invalide" de "compte
inconnu" par le seul code HTTP. Le comportement exact avec un pseudo/cle
valides reste non observe (aucune cle legitime disponible).

## Tests reels avec la cle privee legitime (2026-07-18) : corps vide, causes eliminees une par une

Une cle API privee et legitime a ete obtenue directement aupres du
proprietaire de Pocket PSN (voir `AUDIT.md` section 0ter, 31 caracteres --
a noter, distincte de la chaine candidate de 30 caracteres mentionnee
ci-dessus, qui reste non reutilisee). Le pseudo testé (`gaz91610`) est
confirme reellement suivi par Pocket PSN : sa page publique
`https://pocketpsn.com/gaz91610` affiche un vrai profil avec de vraies
statistiques (voir `test/fixtures/pocketpsn_profile_public.html`, obtenu
manuellement dans un navigateur plus tot dans le projet).

Malgre cela, **chaque requete reelle vers l'endpoint prive renvoie
`HTTP 200`, `Content-Type: application/json; charset=utf-8`,
`Content-Length: 0`, corps entierement vide** (serveur `Apache/2.4.63
(Ubuntu)`). Causes methodiquement testees et **eliminees** une par une via
`tools/pocketpsn_probe/probe.py` (voir son README, section "Diagnostic
HTTP 200 + corps vide") :

| Hypothese testee | Resultat |
|---|---|
| Erreur de saisie de la cle | Ecartee : longueur stable (31 caracteres) sur deux chargements independants (saisie interactive puis fichier local) |
| Slash final de l'URL manquant/en trop | Ecartee : confirme obligatoire (`404` sans le slash, `200` avec) |
| `User-Agent` manquant (chaine `PSTrophyDisplayESP` trouvee dans le binaire) | Ecartee : meme corps vide avec ou sans cet en-tete |
| Percent-encodage du corps (le firmware d'origine concatene brut, notre outil utilisait `urlencode()`) | Ecartee : la cle ne contient aucun caractere modifie par l'encodage (longueur identique brute/encodee) |
| Caractere invisible dans la cle (espace insecable, BOM, saut de ligne residuel) | Ecartee : aucun caractere invisible detecte par le diagnostic dedie |

**Conclusion honnete a ce stade** : le format de la requete (URL, methode,
Content-Type, noms de parametres, encodage) correspond exactement a ce que
montre le firmware d'origine et a ce que documente `PocketPsnProvider`. Le
corps vide n'est donc plus, avec un niveau de confiance raisonnable,
imputable a un probleme de format cote client.

### Recherche en ligne (2026-07-19) : modele de cle du projet d'origine

Le depot source du projet d'origine est public
(`github.com/tomtechie/Playstation-Trophies-ESP-Display`) mais **ne
contient que le firmware `.bin` et un README** -- pas le code source. Le
README apporte deux elements decisifs :

1. **La `config.json` du firmware d'origine ne stocke PAS de cle API**,
   seulement le pseudo PSN (+ Wi-Fi + reglages d'affichage). La cle est
   donc **codee en dur dans le binaire au moment du build** -- ce qui
   correspond exactement a la chaine ~30 caracteres trouvee dans le `.bin`
   pres de l'URL (section "Trouvaille sensible" ci-dessus). Autrement dit,
   dans le projet d'origine, **une seule cle (celle de l'auteur) est
   partagee par tous les utilisateurs** ; chacun ne renseigne que son
   pseudo PSN. La cle authentifie l'**appelant/l'application**, le
   `psn_name` selectionne **quel profil** consulter.

2. **Aucune documentation developpeur publique** n'existe sur
   pocketpsn.com : l'API `/PSTrophyDisplay/` est privee et non documentee,
   les cles sont delivrees manuellement/personnellement par le
   proprietaire.

Consequence pour notre cas : la cle de 31 caracteres remise a l'utilisateur
est une cle **nouvelle et distincte** de celle du binaire. Un `HTTP 200` +
corps vide pour une requete au format correct, avec un pseudo reellement
suivi (gaz91610 a une page publique), est le symptome le plus coherent
avec une **cle pas encore activee cote serveur**, ou **pas encore
autorisee pour ce `psn_name`** -- une etape de provisioning que seul le
proprietaire de Pocket PSN peut effectuer/verifier sur son backend.

**Une seule piste cote client reste bon marche a tester** (ambiguite reelle
du README, qui appelle le parametre "PSN Username" mais le decrit comme
"PSN **display name**") : essayer le nom d'affichage a la place de
l'onlineId, c.-a-d. `python probe.py N3X2R --key-file ...` au lieu de
`gaz91610`. Peu probable (la page publique s'indexe bien sur l'onlineId
`gaz91610`), mais gratuit a verifier. Si cela ne renvoie rien non plus,
**il n'y a plus de test client-side utile** : la balle est cote
proprietaire Pocket PSN.

### Test discriminant decisif (2026-07-19) : la reponse NE depend PAS du pseudo

Investigation approfondie (voir aussi le repo jumeau Steam de l'auteur
ci-dessous). Un fait objectif du binaire oriente le test : le firmware
d'origine contient **deux messages d'erreur distincts** --
`Empty response from PocketPSN.` (corps vide) ET `Not on PocketPSN`
(+ `PocketPSN status: `, qui lit un champ `Status` du JSON). Autrement dit,
un pseudo **inconnu** avec une cle **valide** devrait produire un **corps
non vide** (un JSON portant un `Status`), et non un corps vide.

Trois requetes reelles ont donc ete faites avec la meme vraie cle, en ne
changeant que le `psn_name` (la cle n'apparait jamais, ni en argument ni en
sortie) :

| `psn_name` teste | Statut du pseudo | Resultat |
|---|---|---|
| `gaz91610` | reel, suivi (page publique existe) | `200`, corps vide |
| `PowerPyx` | temoin, quasi certainement suivi (page archivee `200` sur pocketpsn.com) | `200`, corps vide |
| `Zzq7NoSuchUser9981xyz` | inexistant / invente | `200`, corps vide |

**Les trois donnent un corps vide identique.** Consequence logique : le
backend **ne traite jamais le pseudo** (sinon le pseudo inexistant
renverrait un JSON `Status`, et PowerPyx renverrait ses stats). Il
court-circuite **en amont**, a la seule etape qui precede le lookup du
pseudo dans une API `psn_name`+`key` : **la validation de la cle**.

### Demonstration : pourquoi les autres hypotheses sont ecartees

- **Format de requete** (URL, slash, methode, Content-Type, params, casse,
  encodage, User-Agent, corps brut vs url-encode, caracteres invisibles) :
  chaque variante a ete testee, et surtout le binaire confirme que le
  firmware d'origine **n'envoie rien de plus** (un seul en-tete applicatif,
  `Content-Type` ; deux parametres seulement, `psn_name` + `key` ; pas de
  3e parametre ; `PSTrophyDisplayESP` est le nom d'appareil/hostname, pas
  un en-tete d'API). Si le format etait en cause, il n'y aurait aucune
  raison que le comportement soit **identique** pour trois pseudos.
- **Pseudo / liaison compte** : ecarte, la reponse est identique pour un
  pseudo reel suivi, un temoin suivi, et un pseudo inexistant.
- **Cle mal saisie / tronquee** : ecarte (longueur stable 31, aucun
  caractere invisible, resultat identique par fichier et par saisie).
- **Cause restante** : la cle n'est pas acceptee par le backend au moment
  de la validation -- tres probablement **pas encore activee/provisionnee**
  cote serveur Pocket PSN (ou desactivee).

**Seule reserve honnete** : une panne backend globale (le serveur
renverrait un corps vide pour *tout le monde*, y compris le vrai dispositif
de l'auteur) resterait indistinguable de notre cote et serait, elle aussi,
un probleme serveur. Dans les deux cas, **la resolution est cote Pocket
PSN**, pas cote client.

**Niveau de confiance : tres eleve.** Toutes les hypotheses client
testables ont ete eliminees, et le test a trois pseudos isole la cause a
l'etape de validation de la cle.

**Recommandation** : redemander confirmation directement au proprietaire
de Pocket PSN (voir le message type prepare dans `HANDOFF_PROGRESS.md`) --
il est le seul a pouvoir verifier/activer la cle sur son serveur. Preuve
concrete a lui transmettre : *meme un pseudo PSN inexistant renvoie
`HTTP 200` + corps vide, donc le backend ne valide jamais la cle avant de
chercher le profil*.

**Resolu le 2026-07-21** : le proprietaire a confirme avoir "oublie de
mettre a jour un endroit" cote serveur. Apres correction de son cote, la
meme requete (memes cle/pseudo, aucun changement client) renvoie desormais
une vraie reponse (691 octets). Voir section suivante pour le format reel.

## Format reel observe et confirme (2026-07-21)

Deux requetes reelles independantes (a 16 minutes d'intervalle) ont
renvoye un corps **identique au caractere pres** (seul `World Rank` a
varie de quelques places, fluctuation normale d'un classement en direct)
-- le format ci-dessous est donc considere stable, pas un aléa ponctuel.
Exemple anonymise complet : `test/fixtures/pocketpsn_response_real.json`.

```json
{"Username": "...", "Country": "fr", "PSN Level": 314, "Plus": 1,
 "PSN Level Progress": 38, "PSN Level Remaining": 555, "Avatar": "A2117_l.png",
 "Trophies Bronze": 2651, "Trophies Silver": 539, "Trophies Gold": 185,
 "Trophies Plats": 1, "Trophies Total": 3376, "Trophy Points": 72885,
 "Pocket Points": 14976, "Total Games": 376, "World Rank": 284939, "Country Rank": 13349,
 "Quick Stats": [
   {"Title": "Games Completed", "Stat": "1", "Percentile": 0.55},
   {"Title": "Completion Average", "Stat": "22%", "Percentile": 0.46},
   {"Title": "Average Rarity", "Stat": "38.76%", "Percentile": 0.46},
   {"Title": "Unearned Trophies", "Stat": "10,743", "Percentile": 0.04},
   {"Title": "Hours Played", "Stat": "2,582", "Percentile": 0.59}
 ],}
```

Ecarts confirmes par rapport a l'ancien format suppose (inference passive
du binaire, jamais verifiee avant ce test) :

| Point | Ancienne hypothese | Realite confirmee |
|---|---|---|
| `Status` | Present meme en succes | **Absent** en cas de succes (present uniquement, semble-t-il, en cas d'erreur -- ex. `{"Status": "Not on PocketPSN"}`) |
| `Plus` | Non documente | **Nouveau champ reel**, entier `0`/`1` (abonnement PS Plus) |
| `Avatar` | Non documente | **Nouveau champ reel**, nom de fichier (ex. `"A2117_l.png"`) |
| `Quick Stats` | Objet unique avec cles nommees | **Tableau** de `{"Title", "Stat", "Percentile"}` |
| Valeurs dans `Quick Stats` | Nombres/flottants | **Chaines formattees** : `"22%"`, `"38.76%"`, `"10,743"` (virgule de milliers) |
| `Trophies Per Day` | Documente dans Quick Stats | **Absent** de cette API (present uniquement sur la page HTML publique, voir `PocketPsnHtmlParser`) |
| Fin du JSON | JSON strictement valide | **Virgule trainante illegale** avant le `}` final (`...}],}`) -- confirmee dans les octets bruts de deux requetes independantes, cote serveur (construction du JSON par concatenation, pas par un vrai serialiseur) |
| Reste des champs (`Username`, `Country`, `PSN Level`, `PSN Level Progress`, `PSN Level Remaining`, `Trophies Bronze/Silver/Gold/Plats/Total`, `Trophy Points`, `Pocket Points`, `Total Games`, `World Rank`, `Country Rank`) | -- | **Conformes** a l'hypothese initiale |

**`PocketPsnParser` mis a jour en consequence** (voir `src/data/PocketPsnParser.cpp`) :
nettoyage explicite de la virgule trainante avant parsing (jamais une
simple esperance de tolerance d'ArduinoJson), lecture de `Quick Stats`
comme tableau avec conversion des chaines formattees (pourcent, virgule de
milliers) en nombres, compatibilite conservee avec l'ancien format objet
au cas ou l'API y reviendrait un jour, `Plus`/`Avatar` optionnels (jamais
exiges), `Status` jamais exige en cas de succes. Couvert par 25 nouvelles
assertions `--selftest` (voir `runPocketPsnParserSelfTest()`).

## Ce qui reste inconnu

- Les codes HTTP d'erreur reels autres que "cle non activee" (pseudo
  introuvable avec cle active, quota depasse...) -- non testes, la cle
  n'a ete confirmee active qu'apres la resolution du 2026-07-21.
- D'éventuelles routes additionnelles de l'API Pocket PSN au-delà de
  `/PSTrophyDisplay/`.
- La méthode d'obtention légitime d'une clé Pocket PSN pour un tiers reste
  informelle (obtenue ici directement aupres du proprietaire, voir
  `AUDIT.md` section 0ter) -- aucune documentation développeur publique
  trouvée sur pocketpsn.com.

## Statut de `tools/pocketpsn_probe/`

Voir [`../tools/pocketpsn_probe/README.md`](../tools/pocketpsn_probe/README.md).
L'outil appelle l'endpoint confirmé ci-dessus avec un pseudo et une clé
**fournis par l'utilisateur** (jamais codés en dur), et ne devine aucun champ
ni route supplémentaire. Tant qu'aucune clé valide n'a été testée, le schéma
JSON exact et `PocketPsnProvider` restent **non vérifiés**.

## Mise à jour 2026-07-18 : clé officielle obtenue, statut inchangé jusqu'à test réel

Une clé API privée et légitime a été obtenue directement auprès du
propriétaire de Pocket PSN (voir `AUDIT.md` section 0ter). Elle n'est pas
encore présente dans cet environnement — ajoutée localement par
l'utilisateur uniquement, jamais committée/journalisée/affichée.

`PocketPsnProvider` a été rendu portable (voir `src/network/IPocketPsnHttpClient.h`,
même principe que `IWiFiManager`) et `isVerified()` a maintenant une
sémantique réelle : il ne passe à `true` qu'après un vrai succès de
parsing (jamais dans `--selftest`, qui n'utilise que des fixtures
synthétiques explicitement documentées comme telles).

**Statut : endpoint confirmé joignable, schéma en attente de validation
réelle.** Rien dans cette phase ne change ce statut — seul un vrai test
local avec la clé réelle (par l'utilisateur, jamais dans ce dépôt) pourra
le faire passer à "confirmé".
