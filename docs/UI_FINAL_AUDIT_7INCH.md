# UI final audit 7 pouces

Date : 2026-08-14

## Contexte

Passe de finition visuelle sur l'interface LVGL 7 pouces 800 x 480, en conservant la direction graphique validee.
La logique PocketPSN, le parser, le cache, le reseau et `SyncService` ne sont pas modifies.

## Dashboard

Corrections appliquees :

- les trois cartes inferieures utilisent maintenant un axe central interne pour les valeurs et libelles ;
- les barres des cartes inferieures sont centrees avec une marge symetrique de 22 px ;
- le badge pays centre le groupe drapeau + `FR` dans la capsule 56 x 28 px ;
- la pastille `NIVEAU PSN` centre le texte dans sa capsule 96 x 26 px.

Mesures controlees :

- cartes inferieures : 132/132/144 x 80 px ;
- zone texte interne des cartes : marge 8 px, alignement centre ;
- barre interne des cartes : marge 22 px ;
- badge pays : capsule 56 x 28 px, groupe interne 41 px centre ;
- pastille niveau : 96 x 26 px, texte centre.

Statut : valide dans le simulateur et flashe sur la cible 7 pouces.

## Ecrans verifies

| Ecran | Verification |
| --- | --- |
| Dashboard | Alignements internes corriges, profil/niveau/anneau lisibles |
| Trophees | Lignes equilibrees, valeurs alignees a droite |
| Platine | Carte detail centree, trophée lisible |
| Or | Carte detail centree, valeur et barre propres |
| Argent | Carte detail centree, valeur et barre propres |
| Bronze | Variante console centree conservee |
| Statistiques | 6 tuiles regulieres, icones centrees, textes a distance des barres |
| Synchronisation | Anneau et icone centres, texte stable |
| Hors ligne | Carte d'etat lisible, trait bas retire, action centree avec marge basse 14 px |
| Erreur | Carte d'etat lisible, trait bas retire, action centree avec marge basse 14 px |
| A propos | Attribution Pocket PSN lisible |

## Captures

Captures propres :

- `simulator_wide_screenshots/final-audit-2026-08-14-clean/*.png`

Captures avec mesures :

- `simulator_wide_screenshots/dashboard-family-v1/*-analysis.png`

Planches :

- `simulator_wide_screenshots/final-audit-2026-08-14/UI_ALL_SCREENS_CLEAN.png`
- `simulator_wide_screenshots/final-audit-2026-08-14/UI_ALL_SCREENS_ANALYSIS.png`

## Build et cible

- Simulateur PC : compile avec succes.
- Firmware `waveshare-7inch-rgb` : compile avec succes.
- Upload `COM6` : succes.
- LVGL : 9.5.0.
- RAM statique : 48 072 octets, soit 14,7 %.
- Flash : 2 967 434 octets, soit 70,7 %.
- API locale apres demarrage : Wi-Fi connecte, synchro `success`, source `live`, derniere synchro `20:14`.

Mise a jour apres passe globale :

- Simulateur PC : compile avec succes.
- Firmware `waveshare-7inch-rgb` : compile avec succes.
- Upload `COM6` : succes.
- RAM statique : 48 072 octets, soit 14,7 %.
- Flash : 2 967 370 octets, soit 70,7 %.
- API locale apres flash : Wi-Fi connecte, erreur nulle, synchro `success`, source `live`.

## Limites

- Le controle visuel final sur le vrai ecran reste a confirmer par inspection humaine directe.
- Le tactile GT911 est detecte et enregistre dans LVGL, mais cette passe ne modifie pas la logique tactile.
