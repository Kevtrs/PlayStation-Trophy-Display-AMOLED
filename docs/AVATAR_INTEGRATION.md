# Avatar PSN - contrat UI

## Etat actuel

Pocket PSN renvoie un champ `Avatar` dans la reponse API, par exemple :

```json
"Avatar": "A2117_l.png"
```

Le parser et le cache firmware conservent deja cette valeur dans
`ProfileData::avatarFileName`.

Cette passe ajoute uniquement le chainon UI :

- `trophy::ProfileData::avatar_file_name` transporte le nom vers l'UI LVGL ;
- `WideUiBridge::mapProfile()` copie `state.profile.avatarFileName` vers l'UI 7 pouces ;
- le dashboard 7 pouces tente d'afficher une image LVGL resolue par `assets::profile_avatar_descriptor()`;
- un avatar de demo `A2117_l.png` est embarque en RGB565 + alpha8 pour valider le rendu immediatement;
- si aucune image n'est disponible, l'icone Lucide `circle-user-round` reste affichee.

## Domaine avatar Pocket PSN

Le createur de Pocket PSN a indique que les avatars et images de jeux ont ete
deplaces vers un serveur d'images separe. Le domaine prioritaire est donc :

```text
https://img.pocketpsn.com/psn_avatars/
```

Exemple d'URL construite depuis `Avatar=A2117_l.png` :

```text
https://img.pocketpsn.com/psn_avatars/A2117_l.png
```

Le 2026-08-06, le createur de Pocket PSN a confirme que l'ancien chemin
`https://img.pocketpsn.com/assets/psn_avatars/...` est incorrect. Le chemin
`https://img.pocketpsn.com/psn_avatars/...` a ete teste depuis le PC de
developpement et renvoie bien `HTTP 200 image/png` pour `A2117_l.png`.

## Asset de demo embarque

`src/assets/profile_avatar_demo.cpp` contient une version 64 x 64 de
`A2117_l.png`, convertie en `LV_COLOR_FORMAT_RGB565A8` avec masque circulaire.
Elle occupe 12 288 octets de donnees image. Son seul objectif est de valider
visuellement la chaine UI et le mode demo ; elle ne remplace pas le futur cache
dynamique pour les autres avatars.

## A raccorder cote fonctionnel

La couche reseau/cache doit rester hors de l'UI. Le raccord propre est :

1. lire `state.profile.avatarFileName`;
2. valider le nom avec `assets::is_safe_pocketpsn_avatar_name()`;
3. construire l'URL via `assets::pocketpsn_avatar_url()`;
4. telecharger l'image pendant la synchronisation, jamais pendant le rendu LVGL;
5. redimensionner/convertir l'image en petit asset LVGL RGB565 + alpha, par exemple 56 x 56 ou 64 x 64;
6. garder cet asset en cache local;
7. enregistrer un resolver avec `assets::set_profile_avatar_resolver()`;
8. retourner `nullptr` en cas d'absence, d'erreur HTTP, d'image invalide ou de cache vide.

L'UI ne doit jamais bloquer, relancer une synchro ou afficher une erreur visible
simplement parce que l'avatar manque.

## Securite

Le nom d'avatar accepte par l'UI est volontairement limite :

- extension `.png` uniquement ;
- longueur maximale 64 caracteres ;
- lettres, chiffres, `_`, `-`, `.` uniquement ;
- aucun `/`, `\` ou `..`.

Cela evite qu'un champ `Avatar` mal forme puisse devenir un chemin local ou une
URL arbitraire.

## Fallback

Si l'image n'est pas disponible, le dashboard garde le pictogramme Lucide
existant. Le layout ne change donc pas entre :

- avatar absent ;
- Cloudflare bloque encore l'image ;
- cache pas encore peuple ;
- premiere synchronisation ;
- mode demo.
