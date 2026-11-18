#!/usr/bin/env python3
"""UZDoom NX2 - .nro icon and launcher artwork, made from the official UZDoom branding.

    python3 assets/src/make_art.py [out_dir]     (default: assets/build)

Sources (engine/branding, (c) 2025 The UZDoom Team, CC BY-SA 4.0, logo designed
by Carlos "Cardboard Marty" Sanchez):
  - branding/misc/appicon.png  -> icon_256.jpg (the official app icon, unchanged
    apart from scaling)
  - branding/UZDoom Banner.svg -> logo_*.png, splash.jpg (the official banner,
    recoloured light for the launcher's dark background)
The derived images are distributed under the same licence (CC BY-SA 4.0); the
package carries the attribution in licenses/uzdoom-branding/.

Backgrounds and profile cards are plain gradients and text (Bebas Neue, SIL OFL).
Needs Pillow and ImageMagick (`magick`, to render the SVG).
"""
import os
import subprocess
import sys
import tempfile
from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BRANDING = os.path.join(ROOT, "engine", "branding")
FONT = os.path.join(ROOT, "launcher", "res", "fonts", "BebasNeue-Regular.ttf")
LIGHT = (236, 240, 242)
MUTED = (175, 185, 190)

CARDS = {
    "freedoom2": ("FREEDOOM", (40, 70, 40)),
    "brutaldoom": ("BRUTAL DOOM", (110, 15, 10)),
    "brutality": ("BRUTALITY", (90, 30, 10)),
    "wadfusion": ("WADFUSION", (30, 40, 90)),
    "lexicon": ("LEXICON", (70, 30, 80)),
    "compendium": ("COMPENDIUM", (80, 60, 20)),
    "custom": ("CUSTOM", (35, 60, 70)),
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


def official_banner():
    """The official banner (mark + 'UZDoom / Ultimate ZDoom'), light on transparent, trimmed."""
    with tempfile.TemporaryDirectory() as tmp:
        png = os.path.join(tmp, "banner.png")
        subprocess.run(["magick", "-background", "none", "-density", "600",
                        os.path.join(BRANDING, "UZDoom Banner.svg"), "-resize", "2400x", png], check=True)
        src = Image.open(png).convert("RGBA")
    alpha = src.split()[3]
    light = Image.new("RGBA", src.size, LIGHT + (0,))
    light.putalpha(alpha)
    return light.crop(alpha.getbbox())


def fit(img, max_w, max_h):
    s = min(max_w / img.width, max_h / img.height)
    return img.resize((max(1, round(img.width * s)), max(1, round(img.height * s))), Image.LANCZOS)


def background(w, h):
    img = gradient(w, h, (10, 28, 34), (3, 6, 8))
    glow = Image.new("RGB", (w, h), (0, 0, 0))
    ImageDraw.Draw(glow).ellipse([w * 0.45, h * 0.15, w * 1.15, h * 1.1], fill=(20, 110, 130))
    glow = glow.filter(ImageFilter.GaussianBlur(120))
    return Image.blend(img, glow, 0.35)


def main(out):
    os.makedirs(out, exist_ok=True)

    # hbmenu icon: the official app icon
    Image.open(os.path.join(BRANDING, "misc", "appicon.png")).convert("RGB") \
        .resize((256, 256), Image.LANCZOS).save(os.path.join(out, "icon_256.jpg"), "JPEG", quality=92)

    banner = official_banner()
    # launcher logos: home screen (fits 400x118), header of inner screens (300x58),
    # and a large one (620 wide)
    fit(banner, 400, 118).save(os.path.join(out, "logo_400.png"))
    fit(banner, 300, 58).save(os.path.join(out, "logo_300.png"))
    fit(banner, 620, 248).save(os.path.join(out, "logo_620.png"))

    # backgrounds (the launcher draws on a 1280x720 framebuffer)
    bg = background(1280, 720)
    bg.save(os.path.join(out, "bg_main.jpg"), "JPEG", quality=88)
    gradient(1280, 720, (8, 20, 24), (3, 6, 8)).save(os.path.join(out, "bg_inner.jpg"), "JPEG", quality=86)
    splash = bg.copy().convert("RGBA")
    big = fit(banner, 760, 300)
    splash.alpha_composite(big, ((1280 - big.width) // 2, 180))
    d = ImageDraw.Draw(splash)
    text_centered(d, 640, 520, "UZDOOM 5.0.3  ·  NINTENDO SWITCH 2  ·  VULKAN", font(34), MUTED)
    splash.convert("RGB").save(os.path.join(out, "splash.jpg"), "JPEG", quality=88)

    # profile cards
    for tid, (label, tint) in CARDS.items():
        c = gradient(360, 220, tuple(min(255, v + 30) for v in tint), (6, 8, 10))
        text_centered(ImageDraw.Draw(c), 180, 60, label, font(64), LIGHT)
        c.save(os.path.join(out, "card_%s.jpg" % tid), "JPEG", quality=88)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "assets", "build"))
