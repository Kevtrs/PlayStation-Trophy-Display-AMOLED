#pragma once

// Geometrie du board Waveshare ESP32-S3-Touch-LCD-7 (800x480, rectangulaire)
// -- coexiste avec layout.hpp (466x466, rond, AMOLED 1.75) sans jamais le
// modifier : les deux cibles restent independantes tant que le nouveau
// board n'a pas remplace l'ancien (voir BoardConfig7inch.h). Design valide
// par l'utilisateur le 2026-07-28 (voir maquette) : contrairement au petit
// ecran rond, cet ecran n'a pas besoin de tactile ni de defilement manuel
// -- App::tick() fait tourner Dashboard -> 4 trophees -> 4 stats tout seul
// (meme mecanisme deja en place pour l'ecran rond, voir src/ui/app.cpp).

namespace trophy {

constexpr int WIDE_WIDTH = 800;
constexpr int WIDE_HEIGHT = 480;
constexpr int WIDE_PAD = 40;

constexpr int wide_center_x(int w) {
    return (WIDE_WIDTH - w) / 2;
}
constexpr int wide_center_y(int h) {
    return (WIDE_HEIGHT - h) / 2;
}

// --- Ecran Dashboard (dense : tout visible d'un coup, pas besoin d'etre
// lisible de loin -- c'est l'ecran "je m'approche et je regarde", voir
// demande utilisateur du 2026-07-28). Deux colonnes : profil/niveau/mini-
// cartes a gauche, trophee platine en grand a droite. ---
constexpr int WIDE_DASH_STATUS_X = 600;
constexpr int WIDE_DASH_STATUS_Y = 24;
constexpr int WIDE_DASH_LEFT_X = WIDE_PAD;
constexpr int WIDE_DASH_LEFT_W = 420;
constexpr int WIDE_DASH_AVATAR_SIZE = 64;
constexpr int WIDE_DASH_AVATAR_Y = 46;
constexpr int WIDE_DASH_NAME_X = WIDE_DASH_LEFT_X + WIDE_DASH_AVATAR_SIZE + 18;
constexpr int WIDE_DASH_NAME_Y = 54;
constexpr int WIDE_DASH_META_Y = 84;
constexpr int WIDE_DASH_LEVEL_Y = 148;
constexpr int WIDE_DASH_LEVEL_W = 220;
constexpr int WIDE_DASH_LEVEL_META_X = WIDE_DASH_LEFT_X + WIDE_DASH_LEVEL_W + 24;
constexpr int WIDE_DASH_LEVEL_TAG_Y = 168;
constexpr int WIDE_DASH_XP_TRACK_Y = 196;
constexpr int WIDE_DASH_XP_TRACK_W = 190;
constexpr int WIDE_DASH_XP_CAPTION_Y = 210;
// Largeur max de la legende XP (ex: "72 % vers le niveau 328") : plafonnee
// pour ne jamais atteindre la colonne droite (voir WIDE_DASH_RIGHT_X plus
// bas) meme avec un niveau a 4 chiffres ou un texte plus long -- audit de
// chevauchement du 2026-07-28 (la boite du texte atteignait x=504, contre
// x=470 pour le bord gauche du halo avant sa reduction de taille).
constexpr int WIDE_DASH_XP_CAPTION_W = 170;
constexpr int WIDE_DASH_CARDS_Y = 320;
constexpr int WIDE_DASH_CARD_W = 190;
constexpr int WIDE_DASH_CARD_H = 92;
constexpr int WIDE_DASH_CARD_GAP = 20;
constexpr int WIDE_DASH_RIGHT_X = 480;
constexpr int WIDE_DASH_RIGHT_W = 800 - WIDE_DASH_RIGHT_X - WIDE_PAD;
constexpr int WIDE_DASH_TROPHY_SIZE = 200;
// Doit rester <= WIDE_DASH_RIGHT_W (280) : au-dela, le halo deborde de sa
// colonne notionnelle et peut chevaucher le contenu de la colonne gauche
// (ex: legende XP) -- audit de chevauchement du 2026-07-28, valait 300
// avant correction (deborait de 10px de chaque cote).
constexpr int WIDE_DASH_HALO_SIZE = 260;

// --- Ecrans Trophees / Statistiques (une seule valeur geante a la fois,
// posee et regardee de loin -- voir App::tick(), rotation_slide_for()). ---
constexpr int WIDE_HERO_HALO_SIZE = 320;
constexpr int WIDE_HERO_HALO_Y = 210;
constexpr int WIDE_HERO_TROPHY_SIZE = 200;
constexpr int WIDE_HERO_TROPHY_TOP = 46;
// Anneau des ecrans Trophees uniquement (halo/halo2/texture generee par IA) :
// SEPARE de WIDE_HERO_HALO_Y/SIZE ci-dessus (celui-ci reste l'atmosphere de
// fond generique -- ambient_rings(), partagee par Trophees/Statistiques/
// Credits, jamais touchee ici) -- l'ancien schema utilisait la meme
// constante pour l'atmosphere ET l'anneau qui encadre le trophee, sans
// jamais verifier que le trophee (centre y=146, TROPHY_TOP+SIZE/2) tombait
// bien au centre de cet anneau -- audit du 2026-07-29 : ecart mesure de
// 64px (trophee colle en haut, 124px de vide en bas, la valeur "58"
// chevauchait le bas de l'anneau sur 102px). Recentre ici sur le trophee,
// retaille pour laisser une marge nette avant WIDE_HERO_VALUE_Y (268).
constexpr int WIDE_HERO_TROPHY_HALO_Y = WIDE_HERO_TROPHY_TOP + WIDE_HERO_TROPHY_SIZE / 2;  // 146
constexpr int WIDE_HERO_TROPHY_HALO_SIZE = 230;
constexpr int WIDE_HERO_ICON_BOX = 120;
constexpr int WIDE_HERO_ICON_GLYPH = 64;
// Le disque d'icone (ecrans Statistiques) ne laissait que 2px entre son
// bord et l'anneau interieur (halo2, WIDE_HERO_HALO_SIZE-36) -- quasi colle,
// lisible comme "mal cadre" (retour utilisateur du 2026-07-28). Descendre
// le disque vers le centre de l'anneau (au lieu de s'en rapprocher du bord
// en remontant) augmente la marge : valait 70, ~22px de marge desormais.
constexpr int WIDE_HERO_ICON_TOP = 90;
constexpr int WIDE_HERO_VALUE_Y = 268;
// Ecrans Statistiques uniquement (icone au-dessus, bas a y=210) : plus de
// marge disponible que les ecrans Trophees (trophee au-dessus, bas a
// y=246) -- la valeur peut donc remonter davantage sans toucher l'icone.
// Retour utilisateur du 2026-07-28 ("78% plus haut beaucoup").
constexpr int WIDE_HERO_STAT_VALUE_Y = 228;
// WIDE_PAD (40) de marge de chaque cote, comme le Dashboard -- valait 760
// (20px de marge seulement, incoherent avec le reste) avant l'audit
// d'espacement du 2026-07-28.
constexpr int WIDE_HERO_VALUE_W = WIDE_WIDTH - 2 * WIDE_PAD;
constexpr int WIDE_HERO_CAPTION_Y = 384;
constexpr int WIDE_HERO_CAPTION_W = 700;
constexpr int WIDE_HERO_DOTS_Y = 442;

} // namespace trophy
