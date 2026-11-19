#!/usr/bin/env python3
"""Copie la liste des plugins du depot Download (plugins.json + visuels) dans catalog/.
Usage : python3 tools/sync_catalog.py <chemin du depot Download>
Les visuels .webp sont convertis en .jpg (format lu par l'application)."""
import hashlib, json, shutil, sys
from pathlib import Path
from PIL import Image

src = Path(sys.argv[1] if len(sys.argv) > 1 else "../Download")
dst = Path(__file__).resolve().parent.parent / "catalog"
(dst / "img").mkdir(parents=True, exist_ok=True)
data = json.loads((src / "plugins.json").read_text(encoding="utf-8"))

# fusion avec la liste deja publiee : on ne perd aucun plugin ajoute ici directement,
# et chaque plugin garde la version la plus recente des deux listes
def vkey(v):
    return [int(x) if x.isdigit() else 0 for x in str(v).split(".")]
cur_file = dst / "plugins.json"
if cur_file.exists():
    cur = json.loads(cur_file.read_text(encoding="utf-8"))
    by = {p["depot"]: p for p in data["plugins"]}
    for c in cur.get("plugins", []):
        p = by.get(c["depot"])
        if p is None:
            data["plugins"].append(c); continue
        if vkey(c.get("version", "0")) > vkey(p.get("version", "0")):
            for k in ("version", "sortie", "sortie_version"):
                if k in c: p[k] = c[k]
    if str(cur.get("date", "")) > str(data.get("date", "")) and len(str(cur.get("date", ""))) == len(str(data.get("date", ""))):
        data["date"] = cur["date"]
n = 0
for p in data["plugins"]:
    w = src / "img" / f"{p['depot']}.webp"
    j = dst / "img" / f"{p['depot']}.jpg"
    if w.exists():
        Image.open(w).convert("RGB").save(j, "JPEG", quality=90, optimize=True)
    elif not j.exists():
        print("visuel manquant :", w); continue
    p["imgHash"] = hashlib.md5(j.read_bytes()).hexdigest()[:12]   # l'application retelecharge la vignette si l'empreinte change
    n += 1
(dst / "plugins.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
print(f"{len(data['plugins'])} plugins, {n} visuels -> {dst}")
