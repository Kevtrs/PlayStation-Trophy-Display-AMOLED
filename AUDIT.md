# AUDIT — Dépôt d'origine `tomtechie/Playstation-Trophies-ESP-Display`

Date de l'audit initial : 2026-07-13. **Corrigé le 2026-07-13 (suite)** après
signalement de l'utilisateur : voir section 0bis.

## 0. Avertissement méthodologique (audit initial, partiellement erroné)

Lors de l'audit initial, une tentative de consultation automatisée du dépôt
avait renvoyé un résumé mentionnant une « Pocket PSN API » et une release
`Playstation_Trophy_API_1.0`. J'ai vérifié cela en récupérant le fichier brut
exact au commit `86f0af94...` (celui contenu dans le ZIP fourni) : aucune
occurrence de « Pocket » n'y figure, et j'en avais conclu que ce résumé était
une hallucination de l'outil de web-fetch. **Cette conclusion était fausse.**

## 0bis. Correction (2026-07-13, suite au signalement utilisateur)

Le ZIP fourni correspond au commit `86f0af94bf74c8718dae89afe43b32b812fda208`
du **22 octobre 2025**. Ce n'est **pas** le dernier commit du dépôt. L'historique
réel de `main` (vérifié via l'API GitHub, `GET /repos/.../commits?sha=main`) est :

| Commit | Date | Message |
|---|---|---|
| `e8ac02885c...` | **2025-12-06** | *"Update README.md — Rewritten code with Pocket PSN API. Easier to install for user"* |
| `86f0af94bf...` | 2025-10-22 | *(commit contenu dans le ZIP fourni)* |
| `0453e3a44e...` | 2025-10-22 | Update README.md |
| ... | ... | ... |
| `6005073cbf...` | 2025-09-22 | Initial commit |

Le commit `e8ac02885c` (2025-12-06) réécrit entièrement le README pour décrire
une **nouvelle version basée sur l'API Pocket PSN**, et correspond exactement
à la release GitHub :

- **Release** : tag `Main`, nom `Playstation_Trophy_API_1.0`, publiée le
  2025-12-06T17:12:40Z, description *"Updated version with Pocket PSN API"*
  (vérifié via `GET /repos/.../releases`).
- **Asset unique** : `PlaystationTrophy.bin`, **4 194 304 octets** — taille
  strictement identique au fichier fourni par l'utilisateur
  (`PlaystationTrophy (1).bin`). **Le `.bin` fourni est donc bien cet asset
  de release officiel**, pas un artefact de build ambigu.

**Mon erreur initiale** : j'ai vérifié l'exactitude du contenu du ZIP (ce qui
était correct — ce commit précis ne contient effectivement pas "Pocket"), mais
j'en ai conclu à tort que la mention "Pocket PSN" vue ailleurs était une pure
invention de l'outil, sans vérifier si le dépôt avait simplement évolué depuis
le commit du ZIP. Le résumé automatisé initial n'avait donc pas hallucination
sur le fond (Pocket PSN existe bien), même s'il donnait un résumé approximatif
sans citation exacte vérifiable à l'époque. Leçon retenue : distinguer
« ce commit précis ne contient pas X » de « ce dépôt ne contient pas X
actuellement ».

## 0ter. Cle Pocket PSN officielle obtenue (2026-07-18, abandon de la piste NPSSO)

L'utilisateur a contacte le createur du firmware original, qui a confirme
avoir developpe son projet directement avec le proprietaire de Pocket PSN
et l'a mis en relation avec lui. Le proprietaire de Pocket PSN a accepte de
soutenir ce projet (usage educatif, initiation de jeunes de 12 a 15 ans a
l'embarque ESP32) et a fourni une **cle API privee et legitime**, en
echange de deux conditions : crediter visiblement Pocket PSN avec un lien
vers leur site, et leur envoyer des nouvelles/photos du projet termine.

Consequence : la piste de recherche Sony NPSSO (`docs/NPSSO_VS_POCKETPSN.md`,
`tools/psn_official_probe/`) est abandonnee et retiree du depot (voir
`HANDOFF_PROGRESS.md` pour le detail de la suppression) -- elle n'est plus
necessaire puisque l'endpoint prive Pocket PSN documente en section 3
ci-dessous devient utilisable directement. La cle elle-meme n'est jamais
committee, journalisee ni affichee ; elle sera ajoutee localement par
l'utilisateur uniquement. Voir `docs/POCKETPSN_PROTOCOL.md` pour le statut
de validation reelle (toujours "endpoint confirme joignable, schema en
attente de validation reelle" tant qu'un test local avec la vraie cle n'a
pas ete effectue).

## 0quater. Cle Pocket PSN compilee dans le firmware (2026-07-24)

Decision utilisateur, pour la distribution du projet (ex: MakerWorld) : plutot
que de demander a chaque utilisateur final sa propre cle API Pocket PSN, la
cle unique obtenue en section 0ter (qui authentifie l'**application**, pas un
compte individuel — voir `docs/POCKETPSN_PROTOCOL.md`, modele du firmware
d'origine) est desormais compilee dans le firmware via
`include/secrets.h` (fichier local, non versionne — voir
`include/secrets.example.h` et `.gitignore`). Chaque utilisateur final n'a
plus qu'a renseigner son propre pseudo PSN via le portail captif ; le champ
« Cle API Pocket PSN » a ete retire du formulaire web (`data/index.html`).

`ProviderFactory::effectiveApiKey()` (`src/data/ProviderFactory.h/.cpp`)
centralise la resolution : une cle saisie manuellement (repli existant, via
l'API `/api/config`) reste prioritaire sur la cle compilee, pour un
utilisateur avance qui voudrait utiliser la sienne. La cle elle-meme n'est
jamais committee, journalisee ni affichee — voir `--selftest` du simulateur
pour la couverture de ce repli.

## 1. Deux générations bien distinctes du projet d'origine

### 1.A — Ancienne génération (contenue dans le ZIP fourni, commit du 2025-10-22)

Code source **publié en clair** dans le README (blocs de code copier-coller,
pas de dossier source séparé) :
- **Variante Arduino IDE autonome** : scraping HTML de
  `psntrophyleaders.com/user/view/<user>#games` (site tiers non officiel,
  pas de JSON), total + Plat/Or/Argent/Bronze uniquement, TLS désactivé,
  identifiants en dur, aucun portail de configuration.
- **Variante Home Assistant/ESPHome** : ne contacte jamais PSN depuis l'ESP32 ;
  lit 4 entités capteur déjà calculées par l'intégration Home Assistant
  `playstation_network` (basée elle-même sur un token NPSSO côté HA).

Ces deux variantes restent documentées dans le README actuel sous la section
repliée **"Non API Version"** (lignes 321+ du README HEAD) — l'auteur les
conserve comme alternative historique, mais ce n'est plus la version mise en
avant.

### 1.B — Nouvelle génération "Pocket PSN" (2025-12-06, `Playstation_Trophy_API_1.0`)

**Le code source de cette version n'est PAS publié.** Le README HEAD décrit son
comportement et sa configuration en détail, mais ne contient aucun bloc de
code source (contrairement à l'ancienne génération) : elle est distribuée
**uniquement sous forme du firmware précompilé `PlaystationTrophy.bin`**,
flashé via l'outil web `https://esptool.spacehuhn.com`.

Ce que le README HEAD confirme explicitement (cité) :
- *"It fetches data from the Pocket PSN API, renders configurable screens, and
  hosts a built-in web app for setup."*
- *"PSN stats powered by [Pocket PSN](https://pocketpsn.com)"*
- Toujours **ESP32-C3 + SSD1306 128×64** (le portage matériel vers l'ESP32-S3
  AMOLED reste entièrement à faire, indépendamment du fournisseur de données).
- Metrics annoncées (tableau du README) : Username, Country, PSN Level,
  PS Plus, Platinum/Gold/Silver/Bronze, Total Trophies, Trophy Points (TP),
  Pocket Points (PP), Total Games, World Rank, Country Rank, Level Progress,
  Next Level, Completed Games, Completion Average, Average Rarity, Unearned
  Trophies, Playtime.
- Rafraîchissement automatique **toutes les 30 minutes**, POST vers *"the
  Pocket PSN endpoint"* (URL exacte non donnée dans le README — voir Phase 2.5).
- Config Wi-Fi via portail **AP+STA** (`PSTrophy_Setup` / `PSN12345` /
  `192.168.4.1/ui`, **pas de captive portal** intentionnellement), persistée
  dans `/config.json` (LittleFS).
- Jusqu'à 10 écrans configurables, placement des métriques par écran/slot,
  configuration via interface web embarquée (`/getConfig`, `/saveConfig`,
  `/wifi`, `/saveWifi` — endpoints internes au device, pas Pocket PSN).

## 2. Le fichier `.bin` fourni — analyse passive (Phase 2.5)

Voir [`docs/POCKETPSN_PROTOCOL.md`](docs/POCKETPSN_PROTOCOL.md) pour le détail
complet de l'inspection passive (extraction de chaînes de caractères
imprimables, aucune tentative de désassemblage/décompilation du code, conforme
à la consigne "n'invente rien, ne reconstitue pas la logique").

Résumé des faits confirmés par cette inspection :
- Firmware ESP-IDF v5.5-1-g... (build "Jul 22 2025"), pour **ESP32-C3**
  (présence de `esp32c3` dans les chemins de composants IDF), construit
  nativement en **ESP-IDF** (composants `esp_wifi`, `lwip`, `mbedtls`,
  `network_provisioning`, `littlefs` — pas de trace de l'API Arduino ici,
  contrairement aux anciennes variantes).
- **Endpoint confirmé en clair** : `https://api.pocketpsn.com/PSTrophyDisplay/`
- **Méthode confirmée** : `POST`, corps `application/x-www-form-urlencoded`,
  paramètres confirmés : `psn_name=<pseudo>&key=<valeur>`.
- **Un paramètre `key` est requis en plus du pseudo.** Ce n'est donc pas un
  service public anonyme — un identifiant/clé est nécessaire.
- Réponse attendue en JSON (chaînes `"JSON parse error"`, `"Bad JSON"`,
  `"Non-200 response from PocketPSN"` présentes). Noms de champs JSON
  observés tels quels dans le binaire : `Status`, `Username`, `Country`,
  `PSN Level`, `PSN Level Progress`, `PSN Level Remaining`, `Trophies Bronze`,
  `Trophies Silver`, `Trophies Gold`, `Trophies Plats`, `Trophies Total`,
  `Trophy Points`, `Pocket Points`, `Total Games`, `World Rank`,
  `Country Rank`, `Quick Stats` (sous-objet contenant `Games Completed`,
  `Completion Average`, `Average Rarity`, `Unearned Trophies`,
  `Hours Played`). **Aucun champ JSON pour "PS Plus" n'a été trouvé
  explicitement** (seule l'étiquette d'affichage OLED "Plus:" existe) — champ
  d'affichage vs nom JSON réel non confirmé pour cette métrique précise.
- **Une chaîne candidate ressemblant à une clé API a été trouvée à proximité
  immédiate de l'URL de l'endpoint dans le binaire** (30 caractères
  alphanumériques). **Je ne la reproduis pas ici et ne l'utilise nulle part
  dans ce projet.** Si c'est réellement une clé d'API personnelle de l'auteur
  du firmware d'origine (`tomtechie`) délivrée par Pocket PSN, l'utiliser
  reviendrait à consommer un accès tiers sans autorisation — hors de question.
  Voir section 3.
- Les chaînes `User-Agent`, `Authorization`, `Connection`, `Accept-Encoding`
  trouvées dans le binaire sont des **constantes internes génériques du
  composant HTTPClient d'ESP-IDF/Arduino**, pas des preuves de ce que
  l'endpoint Pocket PSN exige réellement comme en-têtes.
- **Non vérifié et non vérifiable dans cet environnement** : aucune requête
  réseau réelle n'a été observée (pas de carte physique ni d'environnement
  d'exécution autorisé disponible ici pour exécuter ce firmware précis et
  capturer le trafic). Le format exact de la clé, sa méthode d'obtention, et
  la forme exacte de la réponse JSON (ordre des champs, présence de tableaux
  vs objets simples pour "Quick Stats") restent **inférés depuis les chaînes
  et le README, non confirmés par une réponse réelle capturée**.

## 3. Obtention d'une clé Pocket PSN légitime — inconnue non résolue

Une recherche sur `pocketpsn.com` confirme qu'il s'agit d'un vrai service
(tableau de bord + application mobile "Pocket PSN - PS Trophy Tracker"), mais
**aucune documentation publique d'API ni de processus d'inscription développeur
en libre-service n'a été trouvée.** Il est donc probable que la clé utilisée
par le firmware d'origine ait été négociée directement entre `tomtechie` et
Pocket PSN (accès privé, pas un programme développeur public). **Ceci reste
une inconnue bloquante pour rendre `PocketPsnProvider` réellement fonctionnel**
tant que l'utilisateur n'a pas obtenu, par un canal légitime de son choix
(compte Pocket PSN, contact avec l'éditeur, ou l'auteur du firmware d'origine),
sa propre clé. Le projet ne doit jamais essayer de contourner cela.

## 4. Ce qui change pour l'architecture

- `TrophyDataProvider` (interface abstraite) + `PocketPsnProvider` (implémentation
  utilisant l'endpoint et le schéma ci-dessus, **marquée non fonctionnelle/non
  vérifiée** tant qu'une clé légitime et un test réel n'auront pas confirmé le
  schéma exact) + `DemoDataProvider` (fonctionnel, données fictives) — voir
  `src/data/`.
- Le mode démo reste un **outil de développement temporaire** ; Pocket PSN est
  le fournisseur obligatoire visé pour la version finale (décision utilisateur).
- Le vieux fournisseur PSNTrophyLeaders / Home Assistant (section 1.A) n'est
  **plus** la cible — conservé uniquement comme contexte historique.

## 5. Licence et crédits (inchangé)

Aucun fichier `LICENSE` dans le dépôt d'origine, aucune licence explicite dans
le README (aux deux générations). Le nouveau projet doit créditer clairement
`tomtechie` et Pocket PSN (`https://pocketpsn.com`) dans le README final, et ne
pas redistribuer `ARIAL.TTF` (police Monotype/Microsoft non confirmée libre).

## 6. Ce qui reste absent / non confirmé (ne pas supposer)

- L'URL exacte n'inclut aucun chemin/paramètre au-delà de
  `/PSTrophyDisplay/` observé — l'existence d'autres routes (santé, version,
  etc.) n'est ni confirmée ni infirmée.
- Le format précis de la clé (longueur, caractères autorisés, expiration) n'est
  pas confirmé au-delà de la chaîne candidate trouvée (non réutilisée).
- Le code HTTP exact retourné en cas de pseudo introuvable, de clé invalide, ou
  de compte non enregistré sur Pocket PSN n'est pas confirmé (seuls les
  messages internes du firmware — *"Non-200 response"*, *"Not on PocketPSN"* —
  indiquent qu'un statut logique existe côté device, pas la valeur HTTP/JSON
  réelle renvoyée par le serveur).
