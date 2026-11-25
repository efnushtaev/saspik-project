#!/usr/bin/env python3
"""Собирает -report.md: параметры платы, расчёты, результаты проверок, BOM.

    python3 make_report.py --spec board.yaml --dxf board.dxf --out board-report.md
"""
import argparse
import json
import math
import os
import sys

import ezdxf
import yaml

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from check_dxf import check_text_overlaps, copper_by_layer  # noqa: E402


def poly_len(pts):
    return sum(math.dist(pts[i], pts[i + 1]) for i in range(len(pts) - 1))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--spec", required=True)
    ap.add_argument("--dxf", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--pins", default=None)
    a = ap.parse_args()

    spec = yaml.safe_load(open(a.spec, encoding="utf-8"))
    doc = ezdxf.readfile(a.dxf)
    errs = doc.audit().errors
    cu = copper_by_layer(doc)
    overlaps = check_text_overlaps(doc.modelspace())

    nets = spec["nets"]
    boards = spec.get("tests", {}).get("boards") or []
    scale_area = sum(b["scale"] ** 2 for b in boards) or 1.0

    lines = []
    add = lines.append
    meta = spec.get("meta", {})
    add(f"# {meta.get('title', os.path.basename(a.spec))}")
    add("")
    add(f"Документ: {meta.get('doc_code', '—')}, ревизия {meta.get('revision', '—')}. "
        f"Спецификация: `{os.path.basename(a.spec)}`.")
    add("")

    b = spec["board"]
    add("## Плата")
    add("")
    add("| Параметр | Значение |")
    add("| :--- | :--- |")
    add(f"| Габарит | {b['width']} × {b['height']} мм |")
    add(f"| Слоёв меди | {b['layers']} |")
    add(f"| Толщина / медь | {b.get('thickness', '—')} мм / {b.get('copper_um', '—')} мкм |")
    add(f"| Крепёж | {len(b.get('holes', []))} отв. |")
    add("")

    add("## Цепи")
    add("")
    add("| Цепь | Слой | Ток, А | Ширина, мм | Длина меди, мм |")
    add("| :--- | :--- | ---: | ---: | ---: |")
    lengths = {n: 0.0 for n in nets}
    for w in spec["wires"]:
        lengths[w["net"]] = lengths.get(w["net"], 0.0) + poly_len([tuple(p) for p in w["points"]])
    mod = spec["module"]
    for hdr in spec["headers"]:
        for func in hdr["functions"]:
            if str(func).startswith("GPIO") and func in {v for v in mod["pins"].values()}:
                lengths["SIG"] = lengths.get("SIG", 0.0) + abs(
                    float(mod["row_top_y"]) - float(hdr["y"]))
    for name, n in nets.items():
        add(f"| {name} | {n['layer']} | {n.get('current_a', '—')} | {n['width']} | "
            f"{lengths.get(name, 0):.1f} |")
    add("")

    if spec.get("pour", {}).get("enabled"):
        p = spec["pour"]
        add(f"Полигон GND: зазор {p['clearance']} мм до чужих цепей, {p['edge']} мм до края, "
            f"терморельеф {p.get('thermal_spokes', 4)} спицы под {p.get('thermal_angle', 45)}°.")
        add("")

    add("## Контакты модуля")
    add("")
    side = {"right": "справа", "left": "слева", "top": "сверху", "bottom": "снизу"}
    add(f"{mod.get('type', 'модуль')} — {len(mod['pins'])} конт., шаг {mod['pitch']} мм, "
        f"контакт 1 {side.get(mod.get('pin1'), mod.get('pin1', '—'))}, "
        f"антенна {side.get(mod.get('antenna'), mod.get('antenna', '—'))}.")
    status = {"verified": "сверена с официальным документом и проверена скриптом",
              "user": "предоставлена заказчиком, сверка с даташитом не выполнялась",
              "unverified": "НЕ ПОДТВЕРЖДЕНА — требуется проверка по шелкографии"}
    if mod.get("source"):
        add("")
        st = status.get(mod.get("pinout_status", "unverified"), mod.get("pinout_status"))
        add(f"Источник цоколёвки: {mod['source']}.")
        add("")
        add(f"Статус цоколёвки: **{st}**.")
    add("")

    add("## Проверки")
    add("")
    add("| Проверка | Результат |")
    add("| :--- | :--- |")
    add(f"| Аудит DXF | {len(errs)} ошибок |")
    add(f"| Наложения текста | {len(overlaps)} |")
    for lay, geom in sorted(cu.items(), key=lambda kv: -kv[1].area):
        # слой лазерного негатива — не медь, в таблицу площадей не идёт
        if lay.startswith("ЛАЗЕР_"):
            continue
        add(f"| Медь «{lay}», мм² (на все листы) | {geom.area:.1f} |")
    if boards:
        add(f"| Листов | {len(spec.get('sheets') or ['схема', 'топология', 'монтаж'])} (вертикально) |")
        add(f"| Масштаб платы на листах | {' + '.join(str(x['scale']) for x in boards)} |")
    add("")

    if a.pins and os.path.exists(a.pins):
        pins = json.load(open(a.pins, encoding="utf-8"))
        add("### Площадки")
        add("")
        add("| Контакт | Координаты | Ожидание | Факт | |")
        add("| :--- | :--- | :--- | :--- | :--- |")
        for probe in pins.get("probes", []):
            from shapely.geometry import Point
            hit = "нет"
            for bd in pins.get("boards") or [{"ox": 0, "oy": 0, "scale": 1}]:
                px = bd["ox"] + probe["x"] * bd["scale"]
                py = bd["oy"] + probe["y"] * bd["scale"]
                p = Point(px, py)
                hit = next((n for n, lay in pins["layers"].items()
                            if lay in cu and cu[lay].buffer(0.001).contains(p)), "нет")
                if hit != "нет":
                    break
            add(f"| {probe['name']} | {probe['x']}, {probe['y']} | {probe['expect']} | {hit} | "
                f"{'ok' if hit == probe['expect'] else 'РАСХОЖДЕНИЕ'} |")
        add("")

    add("## Перечень элементов")
    add("")
    add("| Поз. | Наименование | Кол. |")
    add("| :--- | :--- | ---: |")
    for row in spec.get("bom", []):
        add(f"| {row[0]} | {row[1]} | {row[2]} |")
    add("")

    add("## Что осталось на ручную проверку")
    add("")
    for item in spec.get("manual_checks") or [
        "цоколёвка конкретного модуля (сверить с шелкографией)",
        "наличие и размеры компонентов",
        "тепловой режим в корпусе",
    ]:
        add(f"- {item}")
    add("")

    with open(a.out, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print(f"отчёт: {a.out}")


if __name__ == "__main__":
    main()
