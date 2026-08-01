#!/usr/bin/env python3
"""One-shot generator of placeholder sprite textures for BabyKeySmash.
Creates simple RGBA PNGs ( balloon , star , heart , dino , trail dot ) in
textures/ . Real kid-friendly art can simply replace these files ; every
*.png in textures/ is picked up at startup .
Requires : python3-pil , python3-numpy"""

import os
import math
import numpy as np
from PIL import Image, ImageDraw

SIZE = 256
OUT = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "textures")


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


if __name__ == "__main__":
    balloon()
    star()
    heart()
    dino()
    trail_dot()
