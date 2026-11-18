#!/usr/bin/env python3
"""UZDoom NX2 - icon of the .nro and launcher artwork.

    python3 assets/src/make_art.py [out_dir]     (default: assets/build)

Writes icon_256.jpg (hbmenu icon) and the launcher's images (copied to
switch/uzdoom-nx2/launcher/ by scripts/package.sh): splash.jpg, bg_main.jpg,
bg_inner.jpg, logo_300/400/620.png and card_<template>.jpg.

Plain lettering in Bebas Neue (SIL OFL, launcher/res/fonts) on gradients:
no game art. Deterministic. Needs Pillow.
"""
import os
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FONT = os.path.join(ROOT, "launcher", "res", "fonts", "BebasNeue-Regular.ttf")
CREAM = (235, 210, 170)
RED = (230, 60, 30)
MUTED = (190, 170, 150)

CARDS = {
    "freedoom2": ("FREEDOOM", (40, 70, 40)),
    "brutaldoom": ("BRUTAL", (110, 15, 10)),
    "brutality": ("BRUTALITY", (90, 30, 10)),
    "wadfusion": ("WADFUSION", (30, 40, 90)),
    "lexicon": ("LEXICON", (70, 30, 80)),
    "compendium": ("COMPENDIUM", (80, 60, 20)),
    "custom": ("PERSO", (50, 20, 15)),
}


def font(size):
    return ImageFont.truetype(FONT, size)


def gradient(w, h, top, bottom):
    img = Image.new("RGB", (w, h))
    d = ImageDraw.Draw(img)
    for y in range(h):
        t = y / max(1, h - 1)
        d.line([(0, y), (w - 1, y)], fill=tuple(int(a + (b - a) * t) for a, b in zip(top, bottom)))
    return img


def text_centered(d, cx, y, text, f, fill):
    box = d.textbbox((0, 0), text, font=f)
    d.text((cx - (box[2] - box[0]) // 2 - box[0], y - box[1]), text, font=f, fill=fill)


def glow_background(w, h):
    img = gradient(w, h, (40, 8, 6), (6, 2, 2))
    glow = Image.new("RGB", (w, h), (0, 0, 0))
    d = ImageDraw.Draw(glow)
    d.ellipse([w * 0.45, h * 0.15, w * 1.15, h * 1.1], fill=(120, 25, 10))
    glow = glow.filter(ImageFilter.GaussianBlur(120))
    return Image.blend(img, Image.eval(glow, lambda v: v), 0.45)


def logo(width):
    """'UZDOOM NX2' on a transparent background, 'width' pixels wide."""
    big = Image.new("RGBA", (1240, 300), (0, 0, 0, 0))
    d = ImageDraw.Draw(big)
    d.text((6, 0), "UZDOOM", font=font(260), fill=CREAM + (255,))
    box = d.textbbox((6, 0), "UZDOOM", font=font(260))
    d.text((box[2] + 30, 0), "NX2", font=font(260), fill=RED + (255,))
    big = big.crop(big.getbbox())
    h = round(big.height * width / big.width)
    return big.resize((width, h), Image.LANCZOS)


def main(out):
    os.makedirs(out, exist_ok=True)

    # hbmenu icon
    ic = gradient(256, 256, (70, 10, 8), (20, 4, 3))
    d = ImageDraw.Draw(ic)
    d.rectangle([6, 6, 249, 249], outline=(200, 40, 20), width=4)
    text_centered(d, 128, 40, "UZDOOM", font(92), CREAM)
    text_centered(d, 128, 140, "NX2", font(88), RED)
    text_centered(d, 128, 222, "SWITCH 2 · KAIHEN", font(22), MUTED)
    ic.save(os.path.join(out, "icon_256.jpg"), "JPEG", quality=92)

    # launcher backgrounds (1280x720 framebuffer)
    bg = glow_background(1280, 720)
    bg.save(os.path.join(out, "bg_main.jpg"), "JPEG", quality=88)
    gradient(1280, 720, (24, 8, 6), (8, 3, 3)).save(os.path.join(out, "bg_inner.jpg"), "JPEG", quality=86)
    splash = bg.copy()
    lg = logo(760)
    splash.paste(lg, ((1280 - lg.width) // 2, 230), lg)
    text_centered(ImageDraw.Draw(splash), 640, 410, "UZDOOM 5.0.3  ·  NINTENDO SWITCH 2  ·  VULKAN", font(34), MUTED)
    splash.save(os.path.join(out, "splash.jpg"), "JPEG", quality=88)

    for w in (300, 400, 620):
        logo(w).save(os.path.join(out, "logo_%d.png" % w))

    # profile cards
    for tid, (label, tint) in CARDS.items():
        c = gradient(360, 220, tuple(min(255, v + 30) for v in tint), (10, 4, 4))
        cd = ImageDraw.Draw(c)
        text_centered(cd, 180, 60, label, font(64), CREAM)
        c.save(os.path.join(out, "card_%s.jpg" % tid), "JPEG", quality=88)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "assets", "build"))
