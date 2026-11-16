#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""Проверка схемы на пересечения проводников: линии разных цепей не должны
пересекаться вне точек стыка (Т-образные соединения и углы допустимы).

    python3 check_crossings.py esp32s3-dwm3000-board.dxf [--sheet 1]

Смотрим только линии схемы (слои цепей, отрезки LINE); медь платы проверяет
check_dxf.py из скилла pcb-docs-generate.
"""
import argparse
import sys

import ezdxf
from shapely.geometry import LineString

NET_LAYERS = ("ШИНА_5В", "ШИНА_3В3", "ЗЕМЛЯ", "СИГНАЛЫ")
SHEET_H = 297.0
SHEET_GAP = 23.0


def segments(msp, layers, y0, y1):
    out = []
    for e in msp:
        if e.dxf.layer not in layers or e.dxftype() != "LINE":
            continue
        a = (e.dxf.start.x, e.dxf.start.y)
        b = (e.dxf.end.x, e.dxf.end.y)
        mid = (a[1] + b[1]) / 2.0
        if not (y0 <= mid <= y1):
            continue
        out.append((a, b))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dxf")
    ap.add_argument("--sheet", type=int, default=1,
                    help="номер листа как в основной надписи: 1 — схема (сверху), 3 — монтаж")
    a = ap.parse_args()

    doc = ezdxf.readfile(a.dxf)
    msp = doc.modelspace()
    y0 = (3 - a.sheet) * (SHEET_H + SHEET_GAP)
    segs = segments(msp, NET_LAYERS, y0, y0 + SHEET_H)
    if not segs:
        print(f"лист {a.sheet}: проводников не найдено")
        return 0

    hits = []
    for i in range(len(segs)):
        A = LineString(segs[i])
        for j in range(i + 1, len(segs)):
            B = LineString(segs[j])
            if not A.intersects(B):
                continue
            p = A.intersection(B)
            if p.is_empty or p.geom_type != "Point":
                continue
            ends = list(segs[i]) + list(segs[j])
            if any(abs(p.x - q[0]) < 0.05 and abs(p.y - q[1]) < 0.05 for q in ends):
                continue
            hits.append((round(p.x, 1), round(p.y - y0, 1)))

    print(f"лист {a.sheet}: отрезков {len(segs)}, пересечений {len(set(hits))}")
    for x, y in sorted(set(hits)):
        print(f"  пересечение в ({x}, {y}) мм листа")
    return 1 if hits else 0


if __name__ == "__main__":
    sys.exit(main())
