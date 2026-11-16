#!/usr/bin/env python3
"""PNG-превью DXF платы с корректной заливкой меди.

Зачем свой рендер: matplotlib-backend ezdxf заливает HATCH по внешнему контуру
и игнорирует отверстия — полигон земли закрашивает всю плату. Здесь заливка
строится из контуров HATCH (внешний + дырки) через правило чётности.

    python3 render_png.py board.dxf board.png [--bbox 0,0,1300,297] [--dpi 130]
"""
import argparse
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.path import Path as MPath
from matplotlib.patches import PathPatch

import ezdxf
from ezdxf.addons.drawing import Frontend, RenderContext
from ezdxf.addons.drawing.config import (
    BackgroundPolicy,
    ColorPolicy,
    Configuration,
    HatchPolicy,
    LineweightPolicy,
)
from ezdxf.addons.drawing.matplotlib import MatplotlibBackend


def _hatch_polygon(hatch):
    """Собирает контур HATCH: самый большой путь — внешний, остальные — дырки."""
    loops = []
    for path in hatch.paths:
        pts = [(v[0], v[1]) for v in path.vertices]
        if len(pts) >= 3:
            loops.append(pts)
    if not loops:
        return None
    loops.sort(key=lambda p: abs(_area(p)), reverse=True)
    return loops


def _area(pts):
    s = 0.0
    for i in range(len(pts)):
        x1, y1 = pts[i]
        x2, y2 = pts[(i + 1) % len(pts)]
        s += x1 * y2 - x2 * y1
    return s / 2.0


def _fill_layer(ax, hatch, colour):
    loops = _hatch_polygon(hatch)
    if not loops:
        return
    verts, codes = [], []
    for ring in loops:
        verts.append((ring[0]))
        codes.append(MPath.MOVETO)
        for p in ring[1:]:
            verts.append(p)
            codes.append(MPath.LINETO)
        verts.append(ring[0])
        codes.append(MPath.CLOSEPOLY)
    ax.add_patch(
        PathPatch(
            MPath(verts, codes),
            facecolor=colour,
            edgecolor="none",
            linewidth=0,
            antialiased=False,
            zorder=0,
        )
    )


def render(dxf_path, png_path, bbox=None, dpi=160):
    doc = ezdxf.readfile(dxf_path)
    msp = doc.modelspace()
    if bbox is None:
        bbox = (0, 0, 860, 297)
    w = (bbox[2] - bbox[0]) / 25.4
    h = (bbox[3] - bbox[1]) / 25.4

    fig = plt.figure(figsize=(w, h))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()

    cfg = Configuration(
        background_policy=BackgroundPolicy.WHITE,
        color_policy=ColorPolicy.COLOR,
        lineweight_policy=LineweightPolicy.ABSOLUTE,
        lineweight_scaling=1.0,
        hatch_policy=HatchPolicy.IGNORE,
    )
    Frontend(RenderContext(doc), MatplotlibBackend(ax), config=cfg).draw_layout(
        msp, finalize=True
    )

    # заливка меди — после отрисовки (backend чистит оси), zorder ниже линий
    for hatch in msp.query("HATCH"):
        layer = doc.layers.get(hatch.dxf.layer)
        aci = layer.color
        r, g, b = (0, 0, 0) if aci == 7 else ezdxf.colors.aci2rgb(aci)
        _fill_layer(ax, hatch, (r / 255, g / 255, b / 255))

    ax.set_aspect("auto")
    fig.set_size_inches(w, h)
    ax.set_position([0, 0, 1, 1])
    ax.set_xlim(bbox[0], bbox[2])
    ax.set_ylim(bbox[1], bbox[3])
    fig.savefig(png_path, dpi=dpi, facecolor="white")
    plt.close(fig)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("dxf")
    ap.add_argument("png")
    ap.add_argument("--bbox", default=None, help="x0,y0,x1,y1 в мм модели")
    ap.add_argument("--dpi", type=int, default=160)
    a = ap.parse_args()
    bbox = tuple(float(v) for v in a.bbox.split(",")) if a.bbox else None
    render(a.dxf, a.png, bbox, a.dpi)
    print(f"PNG: {a.png}")


if __name__ == "__main__":
    main()
