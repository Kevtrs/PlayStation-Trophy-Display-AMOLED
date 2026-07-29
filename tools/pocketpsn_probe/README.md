# pocketpsn_probe

Outil PC independant (Python 3, aucune dependance externe) pour sonder
l'endpoint Pocket PSN identifie par inspection passive du firmware officiel
`PlaystationTrophy.bin` -- voir [`../../docs/POCKETPSN_PROTOCOL.md`](../../docs/POCKETPSN_PROTOCOL.md)
et [`../../AUDIT.md`](../../AUDIT.md) (sections 2-3) pour le detail complet et
les limites connues.

## Usage -- saisie securisee de la cle (recommande)

La maniere la plus simple et la plus sure : ne rien passer du tout, l'outil
demande la cle de facon masquee (elle ne s'affiche jamais a l'ecran, et
n'apparait donc jamais dans l'historique de votre terminal) :

```
python probe.py VotrePseudoPSN
Aucune cle fournie via --key-file ou POCKETPSN_API_KEY.
Cle API Pocket PSN (saisie masquee, jamais affichee) : ████████████
```

Alternative : un fichier local ignore par Git contenant uniquement la cle
(utile pour relancer l'outil plusieurs fois sans la ressaisir) :

```
# Une seule fois : creez le fichier vous-meme (jamais commite, voir .gitignore)
notepad tools\pocketpsn_probe\output\ma_cle.txt

python probe.py VotrePseudoPSN --key-file tools\pocketpsn_probe\output\ma_cle.txt
```

`--key` (argument direct) et la variable d'environnement `POCKETPSN_API_KEY`
restent disponibles mais sont deconseilles : un argument de commande reste
dans l'historique du shell.

L'outil affiche le code HTTP et le `Content-Type` de la reponse, enregistre le
corps brut (donnees reelles) dans `output/pocketpsn_response_<horodatage>.json`
(ou `.txt` si le `Content-Type` n'indique pas du JSON), **et** ecrit une copie
anonymisee (pseudo remplace par `TEST_PSN_USERNAME`) dans
`fixtures/pocketpsn_response_anonymized_<horodatage>.json` pour relecture
humaine avant tout usage. `output/` et `fixtures/` sont tous deux ignores par
Git : rien n'est commite automatiquement, la copie anonymisee doit etre
relue puis copiee manuellement dans `test/fixtures/` si elle est jugee propre.

L'outil affiche aussi automatiquement un tableau comparant les champs
documentes (`docs/POCKETPSN_PROTOCOL.md`) aux champs reellement presents
dans la reponse -- a lire avant toute modification de `PocketPsnParser`.

## Diagnostic HTTP 200 + corps vide

Constate reellement lors des premiers tests (cle chargee par saisie
interactive puis par `--key-file` -- erreur de saisie ecartee, longueur de
cle stable a 31 caracteres). Les variantes `User-Agent` (`PSTrophyDisplayESP`,
etc.) ont ete testees et **n'y changent rien** : corps vide identique. La
variante `sans slash final` renvoie `404`, confirmant que le slash est
obligatoire. Serveur reel : `Apache/2.4.63 (Ubuntu)`, `Content-Length: 0`.

**Hypothese principale restante : le percent-encodage du corps.** Le
firmware d'origine construit le corps par concatenation brute
(`"psn_name=" + name + "&key=" + key`), **sans url-encodage**, alors que la
premiere version de cet outil passait par `urllib.parse.urlencode()`. Si la
cle contient des caracteres transformes par l'encodage (`+`->`%2B`,
`/`->`%2F`, `=`->`%3D`, espace->`+`), le serveur recoit une cle differente
-> `200` + corps vide.

Au lancement, l'outil affiche un **diagnostic d'encodage** qui indique,
sans jamais montrer les valeurs, si le pseudo ou la cle sont modifies par
l'url-encodage :

```
=== Diagnostic d'encodage (aucune valeur affichee) ===
psn_name : longueur brute=8   longueur url-encodee=8   -> inchangee
cle      : longueur brute=31  longueur url-encodee=37  -> MODIFIEE par url-encodage (suspect)
```

Si la cle est signalee `MODIFIEE`, la variante `corps_brut_comme_firmware`
est presque certainement la bonne.

Variantes testees par defaut (une seule variable modifiee a la fois) :

- `corps_brut_comme_firmware` -- corps concatene brut, sans url-encodage,
  identique au firmware d'origine. **Hypothese principale.**
- `corps_brut_avec_user_agent` -- corps brut + `User-Agent: PSTrophyDisplayESP`
  (au cas ou les deux seraient necessaires ensemble).
- `baseline_urlencode` -- ancien comportement (url-encode), pour
  comparaison : c'est celui qui renvoyait un corps vide.

Pour ne tester qu'une variante : `--variant corps_brut_comme_firmware`
(voir `--help`).

Chaque variante affiche, sans jamais montrer la cle :
- la longueur de la cle apres `trim()` ;
- la longueur exacte du corps recu ;
- l'en-tete `Content-Length` de la reponse ;
- tous les en-tetes de reponse (en-tetes **serveur**, jamais un secret).

Un resume final compare le resultat de toutes les variantes testees.

## Important -- a propos de la cle

**Cet outil ne fournit, ne devine et n'embarque aucune cle Pocket PSN.** Une
chaine ressemblant a une cle a ete trouvee dans le firmware d'origine lors de
l'audit, mais elle n'est reproduite nulle part dans ce depot : l'utiliser
reviendrait a consommer l'acces d'un tiers sans autorisation. Aucune methode
d'obtention publique d'une cle Pocket PSN n'a ete confirmee au moment de cet
audit -- voir AUDIT.md section 3. Si vous disposez d'une cle legitime (compte
Pocket PSN, ou obtenue aupres de l'editeur/de l'auteur du firmware original),
fournissez-la via `--key` ou la variable d'environnement.

## Ce que l'outil ne fait pas

- Il n'essaie aucun autre endpoint que celui confirme
  (`https://api.pocketpsn.com/PSTrophyDisplay/`).
- Il ne contourne aucune authentification ni protection.
- Il ne journalise jamais la cle en clair (masquee dans tous les logs), et
  masque partiellement le pseudo PSN affiche a l'ecran.

## Verification effectuee

Une seule execution avec des identifiants manifestement factices a confirme
que l'endpoint existe et repond (`HTTP 200`, `Content-Type: application/json`,
corps vide pour des identifiants invalides). Voir
`docs/POCKETPSN_PROTOCOL.md` pour le detail. Le comportement avec une cle
valide n'a pas ete observe.
