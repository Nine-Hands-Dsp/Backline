#!/usr/bin/env python3
"""Liste des plugins de Backline (catalog/), seule source depuis le 20/11/2026 (le depot Download est abandonne).

Usage :
  python3 tools/update_catalog.py                                         recalcule l'empreinte de toutes les vignettes
  python3 tools/update_catalog.py --thumb <depot> <image ou notice.pdf>   nouvelle vignette d'un plugin

--thumb : la pedale est detectee sur fond noir (image, ou 1re page de la notice), recadree et posee sur un fond
noir de 480 px de haut (marges ~25 px), sans agrandissement, puis enregistree dans catalog/img/<depot>.jpg.
L'empreinte imgHash de chaque vignette est ecrite dans catalog/plugins.json : Backline retelecharge une vignette
des que son empreinte change."""
import hashlib, json, subprocess, sys, tempfile
from datetime import date
from pathlib import Path
import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parent.parent / "catalog"
MOIS = ["janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre"]


def page1(pdf):
    tmp = Path(tempfile.mkdtemp())
    subprocess.run(["pdftoppm", "-f", "1", "-l", "1", "-r", "300", "-png", str(pdf), str(tmp / "p")], check=True)
    return Image.open(sorted(tmp.glob("p*.png"))[0]).convert("RGB")


def run(mask):
    """plus long bloc continu de valeurs vraies : (debut, fin)"""
    best, start = (0, 0, 0), None
    for i, on in enumerate(list(mask) + [False]):
        if on and start is None: start = i
        if not on and start is not None:
            if i - start > best[0]: best = (i - start, start, i)
            start = None
    return best[1], best[2]


def thumb(depot, src):
    im = page1(src) if str(src).lower().endswith(".pdf") else Image.open(src).convert("RGB")
    lit = np.asarray(im).astype(int).sum(2) > 60
    # plus long bloc de lignes eclairees = la pedale (les textes de la page de garde sont plus courts)
    y0, y1 = run(lit.sum(1) > lit.shape[1] * 0.08)
    x0, x1 = run(lit[y0:y1].sum(0) > (y1 - y0) * 0.2)
    # les jacks depassent sur les cotes : on elargit tant que des colonnes restent un peu eclairees
    side = (np.asarray(im).astype(int)[y0:y1].sum(2) > 12).sum(0) > 2
    while x0 > 0 and side[x0 - 1]: x0 -= 1
    while x1 < len(side) and side[x1]: x1 += 1
    p = im.crop((x0, y0, x1, y1))
    s = min(1.0, 447 / p.height)
    w, h = round(p.width * s), round(p.height * s)
    p = p.resize((w, h), Image.LANCZOS)
    c = Image.new("RGB", (max(345, w + 50), 480), (0, 0, 0))
    c.paste(p, ((c.width - w) // 2, 32 + (447 - h) // 2))
    c.save(ROOT / "img" / f"{depot}.jpg", "JPEG", quality=90, optimize=True)
    print(f"vignette {depot} : {c.size[0]} x {c.size[1]}")


def main():
    if len(sys.argv) >= 4 and sys.argv[1] == "--thumb":
        thumb(sys.argv[2], sys.argv[3])
    data = json.loads((ROOT / "plugins.json").read_text(encoding="utf-8"))
    for p in data["plugins"]:
        j = ROOT / "img" / f"{p['depot']}.jpg"
        if j.exists():
            p["imgHash"] = hashlib.md5(j.read_bytes()).hexdigest()[:12]
        else:
            print("vignette manquante :", j)
    d = date.today()
    data["date"] = f"{d.day} {MOIS[d.month - 1]} {d.year}"
    (ROOT / "plugins.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{len(data['plugins'])} plugins")


main()
