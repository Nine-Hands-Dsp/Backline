#!/usr/bin/env python3
"""Copie la liste des plugins du depot Download (plugins.json + visuels) dans catalog/.
Usage : python3 tools/sync_catalog.py <chemin du depot Download>
Les visuels .webp sont convertis en .jpg (format lu par l'application)."""
import json, shutil, sys
from pathlib import Path
from PIL import Image

src = Path(sys.argv[1] if len(sys.argv) > 1 else "../Download")
dst = Path(__file__).resolve().parent.parent / "catalog"
(dst / "img").mkdir(parents=True, exist_ok=True)
data = json.loads((src / "plugins.json").read_text(encoding="utf-8"))
shutil.copy(src / "plugins.json", dst / "plugins.json")
n = 0
for p in data["plugins"]:
    w = src / "img" / f"{p['depot']}.webp"
    if not w.exists():
        print("visuel manquant :", w); continue
    im = Image.open(w).convert("RGB")
    im.save(dst / "img" / f"{p['depot']}.jpg", "JPEG", quality=90, optimize=True)
    n += 1
print(f"{len(data['plugins'])} plugins, {n} visuels -> {dst}")
