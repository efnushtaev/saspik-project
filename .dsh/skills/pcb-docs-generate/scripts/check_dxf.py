#!/usr/bin/env python3
"""Геометрическая проверка DXF платы: КЗ, зазоры, площадки, габариты.

Работает по готовому DXF, без доступа к генератору. Медь берётся из HATCH
по слоям; площадки проверяются по точкам из JSON (--pins).

    python3 check_dxf.py board.dxf [--pins pins.json] [--clearance 0.2]

pins.json: {"layers": {"3V3": "ШИНА_3В3"}, "probes": [
    {"name": "J2.1", "x": 93.72, "y": 46.43, "expect": "3V3"}]}

Код возврата 1, если найдены КЗ, зазоры меньше нормы или несовпадение площадок.
"""
import argparse
import json
import sys

import ezdxf
from shapely.geometry import Polygon, Point
from shapely.ops import unary_union

SKIP_LAYERS = {"РАМКА", "ТЕКСТ", "ПОЯСНЕНИЯ", "СИМВОЛЫ", "РАЗМЕРЫ", "ТАБЛИЦЫ",
               "ПЛАТА_КОНТУР", "ПЛАТА_ДЕТАЛИ", "ОСЕВЫЕ", "HATCH", "0"}


def _ring(pts):
    return Polygon(pts) if len(pts) >= 3 else None


def _loops_to_polygons(loops):
    """HATCH может содержать несколько независимых контуров: вложенные — дырки."""
    polys = []
    for pts in loops:
        poly = Polygon(pts)
        if not poly.is_valid:
            poly = poly.buffer(0)
        if poly.area > 1e-6:
            polys.append(poly)
    polys.sort(key=lambda p: p.area, reverse=True)
    outers = []          # (внешний контур, [дырки])
    for poly in polys:
        for outer, holes in outers:
            if outer.contains(poly):
                holes.append(poly)
                break
        else:
            outers.append((poly, []))
    result = []
    for outer, holes in outers:
        p = Polygon(outer.exterior.coords, [h.exterior.coords for h in holes])
        result.append(p if p.is_valid else p.buffer(0))
    return result


def copper_by_layer(doc):
    """Слой -> shapely-геометрия (объединение всех HATCH этого слоя)."""
    acc = {}
    for hatch in doc.modelspace().query("HATCH"):
        loops = []
        for path in hatch.paths:
            pts = [(v[0], v[1]) for v in path.vertices]
            if len(pts) >= 3:
                loops.append(pts)
        if not loops:
            continue
        acc.setdefault(hatch.dxf.layer, []).extend(_loops_to_polygons(loops))
    return {k: unary_union(v) for k, v in acc.items()}


# Сущности, которые ломают переносимость: CAM и старые CAD их не читают.
NON_PORTABLE = {"PROXY", "SPLINE", "3DSOLID", "MESH", "REGION", "IMAGE",
                "OLE2FRAME", "XREF", "PDFUNDERLAY", "DWFUNDERLAY", "ACAD_PROXY_ENTITY"}


def compat_report(doc, msp):
    """Проверка переносимости DXF: версия, единицы, сущности, цвет.

    Возвращает (блокеры, предупреждения, сводка).
    """
    blockers, warnings = [], []
    ver = doc.dxfversion
    if ver != "AC1024":
        (blockers if ver > "AC1024" else warnings).append(
            f"версия {ver}: для универсальности нужна R2010 (AC1024)")
    units = doc.header.get("$INSUNITS", 0)
    if units != 4:
        blockers.append(f"$INSUNITS = {units}, нужно 4 (миллиметры)")

    from collections import Counter
    census = Counter(e.dxftype() for e in msp)
    for kind in sorted(set(census) & NON_PORTABLE):
        blockers.append(f"сущности {kind}: CAM и часть CAD их не читают")

    truecolor = sum(1 for e in msp if e.dxf.hasattr("true_color"))
    if truecolor:
        warnings.append(f"{truecolor} объектов с truecolor — часть программ покажет белым")

    ext = doc.header.get("$EXTMAX")
    if not ext or abs(ext[0]) < 1e-6 and abs(ext[1]) < 1e-6:
        warnings.append("$EXTMAX не выставлен — «Показать всё» откроет пустоту")

    summary = {
        "версия": f"{ver} ({doc.acad_release})",
        "единицы": "мм" if units == 4 else f"код {units}",
        "слоёв": len(doc.layers),
        "типов сущностей": len(census),
        "truecolor": truecolor,
        "блоков": len(doc.blocks),
        "сущности": dict(sorted(census.items())),
    }
    return blockers, warnings, summary


def text_boxes(msp, factor=0.72):
    """Приблизительные рамки TEXT: ширина ≈ 0,72·высота на символ (Arial)."""
    boxes = []
    for e in msp.query("TEXT"):
        txt = e.dxf.text
        if not txt.strip():
            continue
        h = e.dxf.height
        w = factor * h * len(txt)
        x, y = e.dxf.insert.x, e.dxf.insert.y
        rot = getattr(e.dxf, "rotation", 0.0) or 0.0
        if abs(rot) > 1e-6:
            continue                      # повёрнутый текст проверяем глазами
        halign = getattr(e.dxf, "halign", 0) or 0
        valign = getattr(e.dxf, "valign", 0) or 0
        if halign == 1:   x -= w / 2
        elif halign == 2: x -= w
        if valign == 2:   y -= h / 2
        elif valign == 3: y -= h
        boxes.append((txt, (x, y, x + w, y + h)))
    return boxes


def check_text_overlaps(msp, min_frac=0.35, min_size=0.8):
    boxes = text_boxes(msp)
    hits = []
    for i in range(len(boxes)):
        t1, b1 = boxes[i]
        for j in range(i + 1, len(boxes)):
            t2, b2 = boxes[j]
            ox = min(b1[2], b2[2]) - max(b1[0], b2[0])
            oy = min(b1[3], b2[3]) - max(b1[1], b2[1])
            if ox <= 0 or oy <= 0:
                continue
            a1 = (b1[2] - b1[0]) * (b1[3] - b1[1])
            a2 = (b2[2] - b2[0]) * (b2[3] - b2[1])
            if min(a1, a2) < min_size * min_size:
                continue
            frac = ox * oy / min(a1, a2)
            if frac > min_frac:
                hits.append((frac, t1, t2))
    return hits


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dxf")
    ap.add_argument("--pins", help="JSON с точками проверки площадок")
    ap.add_argument("--clearance", type=float, default=0.2)
    ap.add_argument("--min-area", type=float, default=0.01)
    a = ap.parse_args()

    doc = ezdxf.readfile(a.dxf)
    problems = 0

    errs = doc.audit()
    print(f"аудит DXF: {len(errs.errors)} ошибок")
    problems += len(errs.errors)

    blockers, warns, summary = compat_report(doc, doc.modelspace())
    print("переносимость DXF:")
    for key, val in summary.items():
        if key != "сущности":
            print(f"  {key:18} {val}")
    print(f"  сущности           {summary['сущности']}")
    for line in blockers:
        print(f"  БЛОКЕР: {line}")
    for line in warns:
        print(f"  предупреждение: {line}")
    problems += len(blockers)

    from ezdxf import appsettings
    ext = doc.header.get("$EXTMAX", (0, 0, 0))
    print(f"габарит модели: до {tuple(round(v, 1) for v in ext)}")

    cu = copper_by_layer(doc)
    print(f"слоёв меди: {len(cu)}")
    for lay, geom in sorted(cu.items(), key=lambda kv: -kv[1].area):
        print(f"  {lay:12} {geom.area:8.1f} мм²")

    names = sorted(cu)
    for i in range(len(names)):
        for j in range(i + 1, len(names)):
            a1, a2 = names[i], names[j]
            inter = cu[a1].intersection(cu[a2])
            if inter.area > a.min_area:
                b = [round(v, 2) for v in inter.bounds]
                print(f"  КЗ: {a1} ∩ {a2} = {inter.area:.3f} мм²  {b}")
                problems += 1
            elif inter.area > 0:
                d = cu[a1].distance(cu[a2])
                if d < a.clearance:
                    print(f"  зазор {a1}/{a2}: {d:.2f} мм (< {a.clearance})")
                    problems += 1

    if a.pins:
        with open(a.pins) as f:
            spec = json.load(f)
        layers = spec.get("layers", {})
        boards = spec.get("boards") or [{"ox": 0, "oy": 0, "scale": 1}]
        print("площадки:")
        for probe in spec.get("probes", []):
            hit = "нет"
            for b in boards:
                px = b["ox"] + probe["x"] * b["scale"]
                py = b["oy"] + probe["y"] * b["scale"]
                p = Point(px, py)
                hit = next((n for n, lay in layers.items()
                            if lay in cu and cu[lay].buffer(0.001).contains(p)), "нет")
                if hit != "нет":
                    break
            ok = hit == probe["expect"]
            if not ok:
                problems += 1
            print(f"  {probe['name']:8} {probe['x']:7.2f},{probe['y']:6.2f} "
                  f"-> {hit:10} ожид {probe['expect']:10} {'ok' if ok else '<-- РАСХОЖДЕНИЕ'}")

    overlaps = check_text_overlaps(doc.modelspace())
    print(f"наложений текста: {len(overlaps)}")
    for frac, t1, t2 in overlaps[:8]:
        print(f"  {frac:4.0%}  «{t1[:28]}»  ×  «{t2[:28]}»")
    if len(overlaps) > 8:
        print(f"  ... и ещё {len(overlaps) - 8}")
    problems += len(overlaps)

    print(f"\nпроблем: {problems}")
    sys.exit(1 if problems else 0)


if __name__ == "__main__":
    main()
