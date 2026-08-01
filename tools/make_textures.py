#!/usr/bin/env python3
"""One-shot generator of placeholder sprite textures for BabyKeySmash.
Creates simple RGBA PNGs ( balloon , star , heart , dino , trail dot ) in
textures/ . Real kid-friendly art can simply replace these files ; every
*.png in textures/ is picked up at startup .
Requires : python3-pil , python3-numpy"""

import io
import os
import math
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from fontTools.ttLib import TTFont

SIZE = 256
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "textures")

FONT = "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf"

# Uppercase Greek alphabet in order , file index must match the
# latinToGreek / keysym mapping tables in src/sprites.cpp and src/main.cpp
GREEK = [
    ("alpha", "Α"), ("beta", "Β"), ("gamma", "Γ"), ("delta", "Δ"),
    ("epsilon", "Ε"), ("zeta", "Ζ"), ("eta", "Η"), ("theta", "Θ"),
    ("iota", "Ι"), ("kappa", "Κ"), ("lambda", "Λ"), ("mu", "Μ"),
    ("nu", "Ν"), ("xi", "Ξ"), ("omicron", "Ο"), ("pi", "Π"),
    ("rho", "Ρ"), ("sigma", "Σ"), ("tau", "Τ"), ("upsilon", "Υ"),
    ("phi", "Φ"), ("chi", "Χ"), ("psi", "Ψ"), ("omega", "Ω"),
]


def canvas():
    img = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
    return img, ImageDraw.Draw(img)


def save(name, img):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, name)
    img.save(path)
    print("wrote", path)


def balloon():
    img, d = canvas()
    # string
    d.line([(128, 190), (118, 210), (138, 230), (124, 250)], fill=(60, 60, 60, 255), width=3)
    # body ( red balloon ) with outline
    d.ellipse([58, 25, 198, 195], fill=(230, 40, 40, 255), outline=(150, 20, 20, 255), width=6)
    # knot
    d.ellipse([120, 188, 136, 204], fill=(150, 20, 20, 255))
    # highlight
    d.ellipse([82, 52, 118, 108], fill=(255, 200, 200, 255))
    save("balloon.png", img)


def star():
    img, d = canvas()
    outer, inner = 110, 45
    pts = []
    for i in range(10):
        r = outer if i % 2 == 0 else inner
        a = -math.pi / 2 + i * math.pi / 5
        pts.append((128 + r * math.cos(a), 128 + r * math.sin(a)))
    d.polygon(pts, fill=(255, 220, 60, 255), outline=(200, 150, 30, 255), width=6)
    save("star.png", img)


def heart():
    img, d = canvas()
    # classic parametric heart , scaled into the canvas
    pts = []
    for i in range(200):
        t = 2 * math.pi * i / 200
        x = 16 * math.sin(t) ** 3
        y = 13 * math.cos(t) - 5 * math.cos(2 * t) - 2 * math.cos(3 * t) - math.cos(4 * t)
        pts.append((128 + x * 6.5, 120 - y * 6.5))
    d.polygon(pts, fill=(250, 80, 140, 255), outline=(180, 40, 90, 255), width=6)
    save("heart.png", img)


def dino():
    img, d = canvas()
    green = (90, 200, 90, 255)
    dark = (50, 140, 50, 255)
    # a friendly long-neck dinosaur from simple shapes
    for lx in (105, 140, 165, 195):                                     # legs
        d.rectangle([lx, 185, lx + 18, 235], fill=green)
    d.ellipse([65, 115, 215, 205], fill=green, outline=dark, width=5)   # body
    d.ellipse([30, 55, 90, 125], fill=green)                            # neck
    d.ellipse([28, 31, 76, 79], fill=green)                             # head
    d.ellipse([175, 171, 255, 199], fill=green)                         # tail
    for bx, by in ((100, 120), (130, 112), (160, 115)):                 # back bumps
        d.ellipse([bx - 12, by - 12, bx + 12, by + 12], fill=dark)
    d.ellipse([40, 45, 50, 55], fill=(30, 30, 30, 255))                 # eye
    save("dino.png", img)


def trail_dot():
    # soft radial gradient dot used for the mouse trail
    yy, xx = np.mgrid[0:SIZE, 0:SIZE]
    dist = np.sqrt((xx - 128) ** 2 + (yy - 128) ** 2) / 110.0
    alpha = np.clip(1.0 - dist, 0, 1) ** 2
    arr = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)
    arr[..., 0:3] = 255
    arr[..., 3] = (alpha * 255).astype(np.uint8)
    save("trail_dot.png", Image.fromarray(arr, "RGBA"))


EMOJI_FONT = "/usr/share/fonts/truetype/noto/NotoColorEmoji.ttf"

# Curated kid-friendly single-codepoint emoji . Multi-codepoint sequences
# ( skin tones , ZWJ combos ) are avoided on purpose : single codepoints map
# directly through the font cmap
EMOJI = {
    "dog": "🐶", "cat": "🐱", "mouse": "🐭", "hamster": "🐹", "rabbit": "🐰",
    "fox": "🦊", "bear": "🐻", "panda": "🐼", "koala": "🐨", "tiger": "🐯",
    "lion": "🦁", "cow": "🐮", "pig": "🐷", "frog": "🐸", "monkey": "🐵",
    "chicken": "🐔", "penguin": "🐧", "chick": "🐤", "duck": "🦆", "owl": "🦉",
    "horse": "🐴", "unicorn": "🦄", "bee": "🐝", "butterfly": "🦋",
    "snail": "🐌", "ladybug": "🐞", "turtle": "🐢", "octopus": "🐙",
    "fish": "🐠", "dolphin": "🐬", "whale": "🐳", "elephant": "🐘",
    "giraffe": "🦒", "dino": "🦕", "trex": "🦖",
    "car": "🚗", "bus": "🚌", "firetruck": "🚒", "tractor": "🚜",
    "train": "🚂", "airplane": "✈", "rocket": "🚀", "boat": "⛵",
    "balloon": "🎈", "ball": "⚽", "rainbow": "🌈", "star": "⭐",
    "sun": "🌞", "moon": "🌙", "snowflake": "❄", "flower": "🌸",
    "sunflower": "🌻", "gift": "🎁", "teddy": "🧸", "drum": "🥁",
    "apple": "🍎", "banana": "🍌", "strawberry": "🍓", "watermelon": "🍉",
    "icecream": "🍦", "cookie": "🍪", "cake": "🎂", "pizza": "🍕",
    "heart": "❤",
}


def emoji():
    # Noto Color Emoji is a CBDT bitmap font : every glyph embeds a ready
    # 128x128 PNG which is extracted directly , no rendering involved .
    # Files are named emoji_* : sprites.cpp skips the random tint for them
    # so their original colors stay intact
    font = TTFont(EMOJI_FONT)
    cmap = font.getBestCmap()
    glyphs = font["CBDT"].strikeData[0]
    written = 0
    for name, glyph in EMOJI.items():
        code = ord(glyph)
        if code not in cmap:
            print("skipping", name, "- not in", EMOJI_FONT)
            continue
        raw = glyphs[cmap[code]].data
        png_start = raw.find(b"\x89PNG")
        if png_start < 0:
            print("skipping", name, "- no embedded PNG")
            continue
        img = Image.open(io.BytesIO(raw[png_start:])).convert("RGBA")
        img = img.resize((SIZE, SIZE), Image.LANCZOS)
        save("emoji_%s.png" % name, img)
        written += 1
    print("wrote", written, "emoji textures")


def greek_letters():
    # cv::putText ( Hershey fonts ) can not render Greek glyphs , so the
    # Greek letters used with --greek are pre-baked here with a TTF font ,
    # white with a dark outline so they get tinted at draw time like the
    # Latin letters
    font = ImageFont.truetype(FONT, 180)
    stroke = 10
    for i, (name, glyph) in enumerate(GREEK):
        probe = ImageDraw.Draw(Image.new("RGBA", (4, 4)))
        left, top, right, bottom = probe.textbbox((0, 0), glyph, font=font, stroke_width=stroke)
        pad = 20
        img = Image.new("RGBA", (right - left + 2 * pad, bottom - top + 2 * pad), (0, 0, 0, 0))
        d = ImageDraw.Draw(img)
        d.text((pad - left, pad - top), glyph, font=font,
               fill=(255, 255, 255, 255), stroke_width=stroke, stroke_fill=(40, 40, 40, 255))
        os.makedirs(os.path.join(OUT, "greek"), exist_ok=True)
        img.save(os.path.join(OUT, "greek", "greek_%02u_%s.png" % (i, name)))
    print("wrote", len(GREEK), "greek letters to", os.path.join(OUT, "greek"))


if __name__ == "__main__":
    balloon()
    star()
    heart()
    dino()
    trail_dot()
    greek_letters()
    emoji()
