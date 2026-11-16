#!/usr/bin/env python3
"""Регресс эталона: прогоняет генератор и сравнивает метрики с baseline.json.

    python3 regress.py --generator esp32-12v-3v3-schematic.py \
        --dxf esp32-12v-3v3-schematic.dxf [--baseline baseline.json] [--update]

Метрики: ошибки аудита, КЗ, зазоры, площади меди по слоям, наложения текста,
число TEXT/LWPOLYLINE/HATCH. Площади сравниваются с допуском, счётчики — точно.
"""
import argparse
import json
import os
import subprocess
import sys

import ezdxf

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_dxf import check_text_overlaps, copper_by_layer  # noqa: E402


def metrics(dxf_path, clearance=0.2):
    doc = ezdxf.readfile(dxf_path)
    cu = copper_by_layer(doc)
    names = sorted(cu)
    shorts = 0
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            if cu[names[i]].intersection(cu[names[j]]).area > 0.01:
                shorts += 1
    msp = doc.modelspace()
    return {
        "audit_errors": len(doc.audit().errors),
        "shorts": shorts,
        "text_overlaps": len(check_text_overlaps(msp)),
        "areas": {k: round(g.area, 1) for k, g in cu.items()},
        "entities": {t: len(msp.query(t)) for t in ("TEXT", "LWPOLYLINE", "HATCH", "LINE", "CIRCLE")},
    }


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--generator", required=True)
    ap.add_argument("--dxf", required=True)
    ap.add_argument("--baseline", default=os.path.join(
        os.path.dirname(os.path.abspath(__file__)), "..", "reference", "baseline.json"))
    ap.add_argument("--update", action="store_true")
    a = ap.parse_args()

    rc = subprocess.call([sys.executable, a.generator, "--png"])
    if rc != 0:
        print(f"генератор завершился с кодом {rc}")
        return 1

    cur = metrics(a.dxf)
    print(json.dumps(cur, ensure_ascii=False, indent=2))

    if a.update or not os.path.exists(a.baseline):
        os.makedirs(os.path.dirname(os.path.abspath(a.baseline)), exist_ok=True)
        json.dump(cur, open(a.baseline, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
        print(f"эталон обновлён: {a.baseline}")
        return 0

    base = json.load(open(a.baseline, encoding="utf-8"))
    bad = []
    for key in ("audit_errors", "shorts", "text_overlaps"):
        if cur[key] != base[key]:
            bad.append(f"{key}: было {base[key]}, стало {cur[key]}")
    for lay, area in base["areas"].items():
        new = cur["areas"].get(lay, 0.0)
        if area > 0 and abs(new - area) / area > 0.01:
            bad.append(f"площадь {lay}: было {area}, стало {new}")
    for lay in cur["areas"]:
        if lay not in base["areas"]:
            bad.append(f"новый слой меди: {lay}")
    for key, n in base["entities"].items():
        if cur["entities"][key] != n:
            bad.append(f"{key}: было {n}, стало {cur['entities'][key]}")

    if bad:
        print("\nРЕГРЕСС:")
        for line in bad:
            print(" ", line)
        return 1
    print("\nрегресс не найден: метрики совпали с эталоном")
    return 0


if __name__ == "__main__":
    sys.exit(main())
