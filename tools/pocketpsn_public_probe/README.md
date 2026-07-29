# Pocket PSN — sonde PUBLIQUE (sans cle, sans authentification)

Cet outil verifie honnetement ce qui est recuperable sur `pocketpsn.com`
**sans aucune cle privee et sans aucune authentification**, en n'accedant
qu'a des ressources qu'un visiteur anonyme quelconque pourrait consulter.

Il complete `tools/pocketpsn_probe/`, qui teste l'endpoint prive
`api.pocketpsn.com/PSTrophyDisplay/` avec une cle fournie par l'utilisateur.
Les deux outils sont volontairement separes : celui-ci ne connait aucune
notion de cle et ne l'utilise jamais.

## Ce que cet outil NE fait PAS

- Il ne contourne **aucune** protection anti-robot (pas de navigateur
  headless, pas de resolution de defi Cloudflare/CAPTCHA, pas d'usurpation
  d'empreinte de navigateur).
- Il n'utilise, ne devine et n'embarque **aucune** cle Pocket PSN.
- Il ne teste **aucune** route qui necessiterait une authentification ou un
  parametre secret.
- Il ne devine aucune URL d'API : il ne rapporte que les URLs
  effectivement presentes dans le HTML recupere.

## Ce que cet outil fait

1. Requete GET simple (User-Agent de navigateur classique, sans cookies
   persistants) vers :
   - `https://pocketpsn.com/<pseudo>` (page de profil publique)
   - `https://pocketpsn.com/` (racine du domaine)
   - `https://pocketpsn.com/robots.txt` et `https://pocketpsn.com/sitemap.xml`
     (utilises uniquement pour verifier l'etendue d'une eventuelle
     protection anti-robot, pas pour deviner une route d'API)
2. Detecte explicitement un defi anti-robot Cloudflare (en-tete
   `Cf-Mitigated: challenge`, `Server: cloudflare`, page "Just a moment...")
   et le signale sans tenter de le resoudre.
3. Si (et seulement si) la reponse n'est pas un defi anti-robot : recherche
   des scripts JSON embarques valides et des URLs d'API publiques
   effectivement referencees dans la page.
4. Compare tout champ textuel recupere a la liste des champs attendus par
   `PocketPsnParser` (voir `docs/POCKETPSN_PROTOCOL.md`).
5. Sauvegarde :
   - le HTML brut de chaque reponse dans `output/` (gitignore, peut
     contenir le pseudo reel)
   - une copie anonymisee (pseudo remplace par `TEST_PSN_USERNAME`) dans
     `fixtures/` (committable)
   - un rapport JSON complet dans `output/` et sa version anonymisee dans
     `fixtures/report_anonymized.json`

## Utilisation

```powershell
.\run.ps1 -Psn "MON_PSEUDO"
```

ou directement :

```bash
python probe.py MON_PSEUDO
```

## Resultat observe pour le pseudo reel de l'utilisateur (2026-07-16)

Toutes les ressources testees (page de profil, racine du domaine,
`robots.txt`, `sitemap.xml`) renvoient un defi anti-robot Cloudflare :
`HTTP 403`, en-tete `Cf-Mitigated: challenge`, `Server: cloudflare`, corps
HTML titre "Just a moment...", contenant `window._cf_chl_opt` et un
`<noscript>` invitant a activer JavaScript et les cookies.

Consequence : **0 champ de profil, 0 script JSON, 0 URL d'API publique
recuperable** par cette voie depuis un client HTTP simple (curl, urllib,
navigateur headless non autorise ici). Le pseudo n'apparait dans la
reponse que dans des jetons de defi Cloudflare internes
(`__cf_chl_tk`, `__cf_chl_f_tk`, `__cf_chl_rt_tk`), jamais comme donnee de
profil reelle.

Seule une session de navigateur reelle, utilisee par un humain, pourrait
en theorie passer ce defi — ce que cet outil ne fait jamais et que
l'utilisateur devrait faire lui-meme, manuellement, s'il souhaite verifier
ce qui est visible une fois le defi resolu.

## Conclusion pour le firmware

- L'endpoint prive `api.pocketpsn.com/PSTrophyDisplay/` reste la seule
  voie theoriquement viable, et reste bloque faute d'une cle obtenue
  legitimement (voir `AUDIT.md`, section 3).
- Cette voie publique n'apporte aucune donnee exploitable.
- `/api/profile/test` reste donc a raison a l'etat `501 not_implemented` ;
  Pocket PSN ne peut pas etre presente comme fonctionnel.
