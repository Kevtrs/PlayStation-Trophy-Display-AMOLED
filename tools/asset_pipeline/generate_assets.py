#!/usr/bin/env python3
"""Genere les assets graphiques du theme premium (100% procedural, PIL +
numpy, aucune image tierce) et les convertit en tableaux C LVGL
(lv_img_dsc_t, format TRUE_COLOR_ALPHA, RGB565 + alpha -- coherent avec
LV_COLOR_DEPTH=16 / LV_COLOR_16_SWAP=0 defini dans include/lv_conf.h).

Voir docs/ASSET_LICENSES.md : toutes les images sont creees par ce script,
aucune ressource externe n'est utilisee -- pas de question de licence tierce.

Usage : python tools/asset_pipeline/generate_assets.py
Sorties :
  tools/asset_pipeline/png_src/*.png   (previsualisation humaine)
  src/ui/assets/*.c                    (donnees LVGL, compilees par le firmware ET le simulateur)
  src/ui/assets/assets.h               (declarations extern)
"""
import math
import os

import numpy as np
from PIL import Image, ImageDraw, ImageFilter

SCALE = 4  # supersampling pour des formes lisses
REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
PNG_DIR = os.path.join(os.path.dirname(__file__), "png_src")
C_DIR = os.path.join(REPO_ROOT, "src", "ui", "assets")
os.makedirs(PNG_DIR, exist_ok=True)
os.makedirs(C_DIR, exist_ok=True)

# Palette (coherente avec src/ui/Theme.h)
ELECTRIC_BLUE = (0x3D, 0x5A, 0xFE)
PURPLE = (0x8B, 0x5C, 0xF6)
CYAN = (0x22, 0xD3, 0xEE)
WHITE = (0xFF, 0xFF, 0xFF)

PLATINUM = ((0xE9, 0xF3, 0xFC), (0x7E, 0x9B, 0xBE))
GOLD = ((0xFF, 0xE8, 0x9A), (0x9C, 0x6A, 0x10))
SILVER = ((0xF0, 0xF2, 0xF5), (0x74, 0x7B, 0x87))
BRONZE = ((0xF0, 0xB8, 0x85), (0x6B, 0x39, 0x18))


def lerp(a, b, t):
    return tuple(a[i] + (b[i] - a[i]) * t for i in range(3))


def diagonal_gradient(size, top_left, bottom_right):
    """Degrade diagonal (haut-gauche -> bas-droite), retourne un array HxWx3 uint8."""
    w, h = size
    xs = np.linspace(0, 1, w)
    ys = np.linspace(0, 1, h)
    grid_x, grid_y = np.meshgrid(xs, ys)
    t = (grid_x + grid_y) / 2.0
    out = np.zeros((h, w, 3), dtype=np.uint8)
    for c in range(3):
        out[:, :, c] = (top_left[c] + (bottom_right[c] - top_left[c]) * t).astype(np.uint8)
    return out


def radial_alpha(size, center=None, feather=1.0):
    """Alpha radial 0..255, 255 au centre, 0 a un rayon circulaire propre
    (base sur la moitie du plus petit cote, pas la diagonale -- evite un
    halo en forme de carre arrondi observe lors d'un premier essai)."""
    w, h = size
    cx, cy = center or (w / 2, h / 2)
    ys, xs = np.mgrid[0:h, 0:w]
    dist = np.sqrt((xs - cx) ** 2 + (ys - cy) ** 2)
    max_radius = min(cx, cy, w - cx, h - cy) if center else min(w, h) / 2
    a = 1.0 - np.clip(dist / (max_radius * feather), 0, 1)
    a = np.clip(a, 0, 1) ** 1.6
    return (a * 255).astype(np.uint8)


def save_png(img: Image.Image, name: str):
    path = os.path.join(PNG_DIR, f"{name}.png")
    img.save(path)
    print(f"PNG  : {path} ({img.width}x{img.height})")
    return img


# ---------------------------------------------------------------------------
# 1. Halos / glows radiaux
# ---------------------------------------------------------------------------
def make_glow(name, size, color_a, color_b, feather=0.9):
    grad = diagonal_gradient((size, size), color_a, color_b)
    alpha = radial_alpha((size, size), feather=feather)
    rgba = np.dstack([grad, alpha])
    img = Image.fromarray(rgba, mode="RGBA")
    img = img.filter(ImageFilter.GaussianBlur(size * 0.03))
    return save_png(img, name)


# ---------------------------------------------------------------------------
# 2. Trophee central illustre (ecran Welcome)
# ---------------------------------------------------------------------------
def make_hero_trophy(name="trophy_hero", w=176, h=208):
    W, H = w * SCALE, h * SCALE
    mask = Image.new("L", (W, H), 0)
    d = ImageDraw.Draw(mask)

    cx = W / 2
    # Coupe (bowl) : ellipse superieure + col
    bowl_top = int(H * 0.06)
    bowl_bottom = int(H * 0.46)
    bowl_rx = W * 0.34
    d.ellipse([cx - bowl_rx, bowl_top, cx + bowl_rx, bowl_bottom + bowl_rx * 0.5], fill=255)
    d.rectangle([cx - bowl_rx, bowl_top + bowl_rx * 0.25, cx + bowl_rx, bowl_bottom], fill=255)
    # Anses (handles)
    handle_r = W * 0.13
    for sign in (-1, 1):
        hx = cx + sign * (bowl_rx + handle_r * 0.55)
        hy = (bowl_top + bowl_bottom) / 2
        d.ellipse([hx - handle_r, hy - handle_r * 1.15, hx + handle_r, hy + handle_r * 1.15], fill=255)
        # trou de l'anse (creuse) -> repasse a 0 en dessous, applique apres
    # Col (stem) qui se resserre
    neck_top_y = bowl_bottom + bowl_rx * 0.15
    neck_bottom_y = int(H * 0.72)
    d.polygon(
        [
            (cx - bowl_rx * 0.5, neck_top_y),
            (cx + bowl_rx * 0.5, neck_top_y),
            (cx + W * 0.07, neck_bottom_y),
            (cx - W * 0.07, neck_bottom_y),
        ],
        fill=255,
    )
    # Base (pied)
    base_top = neck_bottom_y - H * 0.02
    base_bottom = int(H * 0.80)
    d.polygon(
        [
            (cx - W * 0.10, base_top),
            (cx + W * 0.10, base_top),
            (cx + W * 0.16, base_bottom),
            (cx - W * 0.16, base_bottom),
        ],
        fill=255,
    )
    # Socle
    plinth_top = base_bottom - H * 0.015
    plinth_bottom = int(H * 0.92)
    d.rounded_rectangle([cx - W * 0.24, plinth_top, cx + W * 0.24, plinth_bottom], radius=W * 0.03, fill=255)

    # Trous des anses (vide interieur)
    hole = Image.new("L", (W, H), 255)
    dh = ImageDraw.Draw(hole)
    for sign in (-1, 1):
        hx = cx + sign * (bowl_rx + handle_r * 0.55)
        hy = (bowl_top + bowl_bottom) / 2
        inner_r = handle_r * 0.55
        dh.ellipse([hx - inner_r, hy - inner_r * 1.15, hx + inner_r, hy + inner_r * 1.15], fill=0)
    mask = Image.fromarray(np.minimum(np.array(mask), np.array(hole)))

    mask = mask.resize((w, h), Image.LANCZOS)

    # Remplissage : degrade metallique bleu electrique -> cyan -> violet fonce,
    # avec un reflet specular en haut a gauche.
    grad = diagonal_gradient((w, h), (0xBF, 0xE9, 0xFF), (0x3B, 0x2E, 0x86))
    mid = diagonal_gradient((w, h), (0x6FB8FF), (0x6FB8FF)) if False else None
    img_rgb = grad.astype(np.float32)
    # bande centrale bleu electrique
    ys, xs = np.mgrid[0:h, 0:w]
    band = np.exp(-(((xs / w - 0.42) ** 2) / 0.05))
    for c in range(3):
        img_rgb[:, :, c] = img_rgb[:, :, c] * (1 - band * 0.5) + np.array(ELECTRIC_BLUE)[c] * band * 0.5
    img_rgb = np.clip(img_rgb, 0, 255).astype(np.uint8)

    rgba = np.dstack([img_rgb, np.array(mask)])
    trophy = Image.fromarray(rgba, mode="RGBA")

    # Reflet specular (highlight) en haut a gauche de la coupe
    highlight = Image.new("L", (w, h), 0)
    dh2 = ImageDraw.Draw(highlight)
    dh2.ellipse([w * 0.22, h * 0.10, w * 0.48, h * 0.30], fill=200)
    highlight = highlight.filter(ImageFilter.GaussianBlur(w * 0.03))
    highlight_rgba = Image.merge(
        "RGBA", [Image.new("L", (w, h), 255)] * 3 + [Image.fromarray(np.minimum(np.array(highlight), np.array(mask)))]
    )
    trophy = Image.alpha_composite(trophy, highlight_rgba)

    # Contour sombre discret pour la definition
    edge = mask.filter(ImageFilter.FIND_EDGES)
    edge_rgba = Image.merge("RGBA", [Image.new("L", (w, h), 0x10)] * 3 + [edge.point(lambda p: min(255, p * 3))])
    trophy = Image.alpha_composite(trophy, edge_rgba)

    return save_png(trophy, name)


# ---------------------------------------------------------------------------
# 3. Medailles Platine / Or / Argent / Bronze
# ---------------------------------------------------------------------------
def make_medal(name, size, top_color, bottom_color, ribbon_color):
    W = size * SCALE
    img_rgb = diagonal_gradient((W, W), top_color, bottom_color)

    mask = Image.new("L", (W, W), 0)
    d = ImageDraw.Draw(mask)
    r = W * 0.40
    cx = cy = W / 2
    # Rubans (derriere le disque)
    ribbon_w = W * 0.16
    d.polygon(
        [(cx - ribbon_w, cy), (cx - ribbon_w * 0.3, cy), (cx - ribbon_w * 1.6, W), (cx - ribbon_w * 2.4, W)],
        fill=255,
    )
    d.polygon(
        [(cx + ribbon_w, cy), (cx + ribbon_w * 0.3, cy), (cx + ribbon_w * 1.6, W), (cx + ribbon_w * 2.4, W)],
        fill=255,
    )
    ribbon_mask = mask.copy()
    # Disque (par-dessus)
    d.ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)
    disc_mask = Image.new("L", (W, W), 0)
    ImageDraw.Draw(disc_mask).ellipse([cx - r, cy - r, cx + r, cy + r], fill=255)

    rgba = np.dstack([img_rgb, np.array(mask)])
    medal = Image.fromarray(rgba, mode="RGBA").resize((size, size), Image.LANCZOS)

    # recolore les rubans (zone hors disque) avec ribbon_color
    ribbon_only = np.array(ribbon_mask.resize((size, size), Image.LANCZOS)).astype(np.float32) / 255.0
    disc_only = np.array(disc_mask.resize((size, size), Image.LANCZOS)).astype(np.float32) / 255.0
    ribbon_only = np.clip(ribbon_only - disc_only, 0, 1)
    arr = np.array(medal).astype(np.float32)
    for c in range(3):
        arr[:, :, c] = arr[:, :, c] * (1 - ribbon_only) + ribbon_color[c] * ribbon_only
    medal = Image.fromarray(np.clip(arr, 0, 255).astype(np.uint8), mode="RGBA")

    # Anneau interieur + etoile centrale (embleme)
    d2 = ImageDraw.Draw(medal)
    ring_r = int(r / SCALE * 0.82)
    cx2, cy2 = size // 2, size // 2
    d2.ellipse([cx2 - ring_r, cy2 - ring_r, cx2 + ring_r, cy2 + ring_r], outline=(255, 255, 255, 90), width=max(1, size // 40))
    star_r = ring_r * 0.55
    pts = []
    for i in range(10):
        ang = -math.pi / 2 + i * math.pi / 5
        rr = star_r if i % 2 == 0 else star_r * 0.45
        pts.append((cx2 + rr * math.cos(ang), cy2 + rr * math.sin(ang)))
    d2.polygon(pts, fill=(255, 255, 255, 210))

    # Reflet
    highlight = Image.new("L", (size, size), 0)
    ImageDraw.Draw(highlight).ellipse([size * 0.24, size * 0.12, size * 0.62, size * 0.4], fill=140)
    highlight = highlight.filter(ImageFilter.GaussianBlur(size * 0.05))
    hl_rgba = Image.merge("RGBA", [Image.new("L", (size, size), 255)] * 3 + [highlight])
    medal = Image.alpha_composite(medal, hl_rgba)

    return save_png(medal, name)


# ---------------------------------------------------------------------------
# 4. Icones de statistiques (glyphes plats simples, coherents)
# ---------------------------------------------------------------------------
def make_icon_controller(name, size, color):
    W = size * SCALE
    img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    body_w, body_h = W * 0.8, W * 0.4
    x0, y0 = (W - body_w) / 2, W * 0.32
    d.rounded_rectangle([x0, y0, x0 + body_w, y0 + body_h], radius=body_h * 0.5, fill=color + (255,))
    # boutons
    r = W * 0.045
    for dx, dy in [(0.68, 0.44), (0.78, 0.5), (0.73, 0.38), (0.73, 0.5)]:
        d.ellipse([W * dx - r, W * dy - r, W * dx + r, W * dy + r], fill=(255, 255, 255, 200))
    # croix directionnelle
    cx, cy = W * 0.27, W * 0.5
    d.rectangle([cx - r * 2.2, cy - r * 0.7, cx + r * 2.2, cy + r * 0.7], fill=(20, 20, 30, 255))
    d.rectangle([cx - r * 0.7, cy - r * 2.2, cx + r * 0.7, cy + r * 2.2], fill=(20, 20, 30, 255))
    img = img.resize((size, size), Image.LANCZOS)
    return save_png(img, name)


def make_icon_rank(name, size, color):
    W = size * SCALE
    img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = W / 2
    bars = [(0.28, 0.62), (0.5, 0.42), (0.72, 0.52)]
    bw = W * 0.16
    for bx, btop in bars:
        d.rounded_rectangle([W * bx - bw / 2, W * btop, W * bx + bw / 2, W * 0.82], radius=bw * 0.25, fill=color + (255,))
    d.polygon([(cx, W * 0.12), (cx + W * 0.09, W * 0.30), (cx - W * 0.09, W * 0.30)], fill=(255, 255, 255, 220))
    img = img.resize((size, size), Image.LANCZOS)
    return save_png(img, name)


def make_icon_clock(name, size, color):
    W = size * SCALE
    img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    r = W * 0.38
    cx = cy = W / 2
    d.ellipse([cx - r, cy - r, cx + r, cy + r], outline=color + (255,), width=int(W * 0.06))
    d.line([cx, cy, cx, cy - r * 0.55], fill=color + (255,), width=int(W * 0.06))
    d.line([cx, cy, cx + r * 0.4, cy + r * 0.1], fill=color + (255,), width=int(W * 0.05))
    d.ellipse([cx - W * 0.03, cy - W * 0.03, cx + W * 0.03, cy + W * 0.03], fill=(255, 255, 255, 255))
    img = img.resize((size, size), Image.LANCZOS)
    return save_png(img, name)


def make_icon_target(name, size, color):
    W = size * SCALE
    img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    cx = cy = W / 2
    for rr, w in [(0.38, 0.05), (0.24, 0.05)]:
        d.ellipse([cx - W * rr, cy - W * rr, cx + W * rr, cy + W * rr], outline=color + (255,), width=int(W * w))
    d.ellipse([cx - W * 0.08, cy - W * 0.08, cx + W * 0.08, cy + W * 0.08], fill=color + (255,))
    img = img.resize((size, size), Image.LANCZOS)
    return save_png(img, name)


def make_icon_trophy_small(name, size, color):
    """Petite icone trophee (silhouette simplifiee) pour les badges du dashboard."""
    W = size * SCALE
    mask = Image.new("L", (W, W), 0)
    d = ImageDraw.Draw(mask)
    cx = W / 2
    d.ellipse([cx - W * 0.30, W * 0.10, cx + W * 0.30, W * 0.55], fill=255)
    d.rectangle([cx - W * 0.30, W * 0.20, cx + W * 0.30, W * 0.45], fill=255)
    for sign in (-1, 1):
        hx = cx + sign * W * 0.36
        d.ellipse([hx - W * 0.10, W * 0.18, hx + W * 0.10, W * 0.40], fill=255)
    d.polygon([(cx - W * 0.12, W * 0.52), (cx + W * 0.12, W * 0.52), (cx + W * 0.06, W * 0.68), (cx - W * 0.06, W * 0.68)], fill=255)
    d.rounded_rectangle([cx - W * 0.22, W * 0.68, cx + W * 0.22, W * 0.80], radius=W * 0.03, fill=255)
    mask = mask.resize((size, size), Image.LANCZOS)
    solid = Image.new("RGBA", (size, size), color + (0,))
    solid.putalpha(mask)
    return save_png(solid, name)


# ---------------------------------------------------------------------------
# 5. Texture de fond discrete (motif hexagonal tres attenue)
# ---------------------------------------------------------------------------
def make_background_pattern(name="bg_pattern", size=466):
    W = size * SCALE
    img = Image.new("RGBA", (W, W), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    step = W / 14
    r = step * 0.52
    for row in range(-1, 16):
        for col in range(-1, 16):
            x = col * step * 1.5
            y = row * step * math.sqrt(3)
            if col % 2:
                y += step * math.sqrt(3) / 2
            pts = [
                (x + r * math.cos(math.radians(a)), y + r * math.sin(math.radians(a))) for a in range(0, 360, 60)
            ]
            d.polygon(pts, outline=(90, 110, 220, 26))
    img = img.resize((size, size), Image.LANCZOS)
    return save_png(img, name)


# ---------------------------------------------------------------------------
# Conversion PNG (RGBA) -> tableau C LVGL (TRUE_COLOR_ALPHA, RGB565+A8)
# ---------------------------------------------------------------------------
def rgba_to_lvgl_c(img: Image.Image, c_name: str) -> str:
    img = img.convert("RGBA")
    w, h = img.size
    arr = np.array(img)
    r = (arr[:, :, 0].astype(np.uint16) >> 3)
    g = (arr[:, :, 1].astype(np.uint16) >> 2)
    b = (arr[:, :, 2].astype(np.uint16) >> 3)
    rgb565 = (r << 11) | (g << 5) | b
    lo = (rgb565 & 0xFF).astype(np.uint8)
    hi = (rgb565 >> 8).astype(np.uint8)
    alpha = arr[:, :, 3].astype(np.uint8)
    packed = np.dstack([lo, hi, alpha]).reshape(-1)

    lines = []
    row = []
    for i, byte in enumerate(packed.tolist()):
        row.append(f"0x{byte:02x}")
        if len(row) == 16:
            lines.append("    " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("    " + ", ".join(row) + ",")
    body = "\n".join(lines)

    return f"""// Genere par tools/asset_pipeline/generate_assets.py -- ne pas editer a la main.
// Voir docs/ASSET_LICENSES.md (asset cree par ce projet, aucune source tierce).
#include \"lvgl.h\"

const LV_ATTRIBUTE_MEM_ALIGN uint8_t {c_name}_map[] = {{
{body}
}};

const lv_img_dsc_t {c_name} = {{
  .header.cf = LV_IMG_CF_TRUE_COLOR_ALPHA,
  .header.always_zero = 0,
  .header.reserved = 0,
  .header.w = {w},
  .header.h = {h},
  .data_size = {w} * {h} * LV_IMG_PX_SIZE_ALPHA_BYTE,
  .data = {c_name}_map,
}};
"""


def write_c_file(img: Image.Image, c_name: str):
    content = rgba_to_lvgl_c(img, c_name)
    path = os.path.join(C_DIR, f"{c_name}.c")
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"C    : {path}")


def main():
    assets = {}

    assets["glow_hero"] = make_glow("glow_hero", 300, (0x5C, 0xC8, 0xFF), (0x6A, 0x3F, 0xC9), feather=0.85)
    assets["glow_ring"] = make_glow("glow_ring", 260, ELECTRIC_BLUE, PURPLE, feather=0.9)
    assets["glow_small"] = make_glow("glow_small", 140, CYAN, ELECTRIC_BLUE, feather=0.95)

    assets["trophy_hero"] = make_hero_trophy()

    assets["medal_platinum"] = make_medal("medal_platinum", 104, PLATINUM[0], PLATINUM[1], (0x4A, 0x5A, 0x72))
    assets["medal_gold"] = make_medal("medal_gold", 104, GOLD[0], GOLD[1], (0x6E, 0x4A, 0x10))
    assets["medal_silver"] = make_medal("medal_silver", 104, SILVER[0], SILVER[1], (0x4E, 0x52, 0x59))
    assets["medal_bronze"] = make_medal("medal_bronze", 104, BRONZE[0], BRONZE[1], (0x4A, 0x28, 0x12))

    # Variantes plus petites (taille native, sans zoom LVGL -- plus previsible
    # pour l'alignement que lv_img_set_zoom, voir docs/DEVELOPMENT.md).
    assets["medal_platinum_sm"] = make_medal("medal_platinum_sm", 60, PLATINUM[0], PLATINUM[1], (0x4A, 0x5A, 0x72))
    assets["medal_gold_sm"] = make_medal("medal_gold_sm", 60, GOLD[0], GOLD[1], (0x6E, 0x4A, 0x10))
    assets["medal_silver_sm"] = make_medal("medal_silver_sm", 60, SILVER[0], SILVER[1], (0x4E, 0x52, 0x59))
    assets["medal_bronze_sm"] = make_medal("medal_bronze_sm", 60, BRONZE[0], BRONZE[1], (0x4A, 0x28, 0x12))

    assets["icon_controller"] = make_icon_controller("icon_controller", 64, ELECTRIC_BLUE)
    assets["icon_rank"] = make_icon_rank("icon_rank", 64, PURPLE)
    assets["icon_clock"] = make_icon_clock("icon_clock", 64, CYAN)
    assets["icon_target"] = make_icon_target("icon_target", 64, ELECTRIC_BLUE)
    assets["icon_trophy_small"] = make_icon_trophy_small("icon_trophy_small", 48, (0xE8, 0xD9, 0xA8))
    assets["icon_platinum_small"] = make_icon_trophy_small("icon_platinum_small", 40, PLATINUM[0])

    assets["bg_pattern"] = make_background_pattern()

    for c_name, img in assets.items():
        write_c_file(img, c_name)

    header_lines = ["#pragma once", '#include "lvgl.h"', ""]
    for c_name in assets:
        header_lines.append(f"extern const lv_img_dsc_t {c_name};")
    header = "\n".join(header_lines) + "\n"
    with open(os.path.join(C_DIR, "assets.h"), "w", encoding="utf-8") as f:
        f.write(header)
    print(f"H    : {os.path.join(C_DIR, 'assets.h')}")
    print(f"\n{len(assets)} assets generes.")


if __name__ == "__main__":
    main()
