#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор документации на плату узла meshrover (ESP32-S3-DevKitC-1 + DWM3000).

Выходные файлы:
    esp32s3-dwm3000-board.dxf          — чертёж (DXF R2010, мм)
    esp32s3-dwm3000-board-preview.png  — превью всех листов
    esp32s3-dwm3000-board-board.png    — плата крупно

Лист 1 — схема электрическая принципиальная.
Листы 2–3 — топология 2:1 и монтаж 1:1 — добавляются тем же генератором.

Запуск:  python3 esp32s3-dwm3000-board.py [--png]
"""

import math
import os
import sys

import ezdxf
from ezdxf.enums import TextEntityAlignment as TA

BASE = os.path.dirname(os.path.abspath(__file__))
SPEC_PATH = os.path.join(BASE, "board.yaml")

OUT_DXF = "esp32s3-dwm3000-board.dxf"
OUT_PNG = "esp32s3-dwm3000-board-preview.png"


def load_spec(path=SPEC_PATH):
    import yaml

    with open(path, encoding="utf-8") as f:
        spec = yaml.safe_load(f)
    for key in ("board", "nets", "module", "dwm3000"):
        if key not in spec:
            raise ValueError(f"board.yaml: нет раздела {key!r}")
    for key, empty in (("headers", []), ("wires", []), ("smd", []), ("bom", []),
                       ("pour", {"enabled": False})):
        spec.setdefault(key, empty)
    return spec


SPEC = load_spec()

# ----------------------------------------------------------------------------
# Документ
# ----------------------------------------------------------------------------
doc = ezdxf.new("R2010")
doc.units = ezdxf.units.MM
doc.header["$INSUNITS"] = 4
doc.header["$MEASUREMENT"] = 1
doc.header["$LUNITS"] = 2
doc.header["$AUNITS"] = 0
msp = doc.modelspace()

if "DASHED" not in doc.linetypes:
    doc.linetypes.add("DASHED", pattern=[0.6, 0.3, -0.3], description="Dashed __ __ __")

for name, font in (("RU", "arial.ttf"), ("RU_B", "arialbd.ttf")):
    if name not in doc.styles:
        doc.styles.new(name, dxfattribs={"font": font})

# ----------------------------------------------------------------------------
# Слои
# ----------------------------------------------------------------------------
LAYERS = {
    "РАМКА":        (7, 50, "CONTINUOUS"),
    "ШИНА_5В":      (1, 40, "CONTINUOUS"),
    "ШИНА_3В3":     (5, 40, "CONTINUOUS"),
    "ЗЕМЛЯ":        (3, 40, "CONTINUOUS"),
    "СИГНАЛЫ":      (7, 30, "CONTINUOUS"),
    "СИМВОЛЫ":      (7, 35, "CONTINUOUS"),
    "ТЕКСТ":        (7, 18, "CONTINUOUS"),
    "ПОЯСНЕНИЯ":    (6, 18, "CONTINUOUS"),
    "ПЛАТА_КОНТУР": (7, 60, "CONTINUOUS"),
    "ПЛАТА_ДЕТАЛИ": (4, 25, "CONTINUOUS"),
    "ПЛАТА_ПАДЫ":   (7, 30, "CONTINUOUS"),
    "ПЕРЕМЫЧКИ":    (2, 25, "DASHED"),
    "РАЗМЕРЫ":      (3, 18, "CONTINUOUS"),
    "ТАБЛИЦЫ":      (7, 18, "CONTINUOUS"),
    "ОСЕВЫЕ":       (8, 13, "DASHED"),
}
for _name, (_c, _lw, _lt) in LAYERS.items():
    if _name not in doc.layers:
        doc.layers.add(name=_name, color=_c, lineweight=_lw, linetype=_lt)

C5V, C33, CGND, CSIG = "ШИНА_5В", "ШИНА_3В3", "ЗЕМЛЯ", "СИГНАЛЫ"
CSYM, CTX, CNOTE, CFRAME = "СИМВОЛЫ", "ТЕКСТ", "ПОЯСНЕНИЯ", "РАМКА"


# ----------------------------------------------------------------------------
# Примитивы
# ----------------------------------------------------------------------------
def _attribs(layer, lw=None):
    a = {"layer": layer}
    if lw is not None:
        a["lineweight"] = lw
    return a


def pl(pts, layer, lw=None, closed=False):
    msp.add_lwpolyline(pts, close=closed, dxfattribs=_attribs(layer, lw))


def ln(a, b, layer, lw=None):
    msp.add_line(a, b, dxfattribs=_attribs(layer, lw))


def rect(x0, y0, x1, y1, layer, lw=None):
    msp.add_lwpolyline([(x0, y0), (x1, y0), (x1, y1), (x0, y1)],
                       close=True, dxfattribs=_attribs(layer, lw))


def circle(x, y, r, layer, lw=None):
    msp.add_circle((x, y), r, dxfattribs=_attribs(layer, lw))


def arc(x, y, r, a0, a1, layer, lw=None):
    msp.add_arc((x, y), r, a0, a1, dxfattribs=_attribs(layer, lw))


def dot(x, y, r=0.5, layer=CSYM):
    msp.add_circle((x, y), r, dxfattribs={"layer": layer, "lineweight": 60})


def txt(s, x, y, h=2.2, layer=CTX, align=TA.LEFT, rot=0.0, bold=False):
    e = msp.add_text(s, height=h,
                     dxfattribs={"layer": layer, "rotation": rot,
                                 "style": "RU_B" if bold else "RU"})
    e.set_placement((x, y), align=align)
    return e


def para(text, x, y, width, h=2.0, leading=3.4, layer=CNOTE, bold=False):
    words = text.split()
    line, yy = "", y
    for w in words:
        test = (line + " " + w).strip()
        if len(test) * h * 0.72 > width and line:
            txt(line, x, yy, h, layer, bold=bold)
            yy -= leading
            line = w
        else:
            line = test
    if line:
        txt(line, x, yy, h, layer, bold=bold)
        yy -= leading
    return yy


# ----------------------------------------------------------------------------
# Условные обозначения
# ----------------------------------------------------------------------------
def sym_res_v(x, yc, h=8.0, w=3.6):
    rect(x - w / 2, yc - h / 2, x + w / 2, yc + h / 2, CSYM)


def sym_res_h(xc, y, w=8.0, h=3.6):
    rect(xc - w / 2, y - h / 2, xc + w / 2, y + h / 2, CSYM)


def sym_cap_v(x, yc, gap=2.4, w=8.0, polar=False):
    ln((x - w / 2, yc + gap / 2), (x + w / 2, yc + gap / 2), CSYM)
    ln((x - w / 2, yc - gap / 2), (x + w / 2, yc - gap / 2), CSYM)


def sym_led_v(x, yc, w=7.0, h=6.0):
    pl([(x - w / 2, yc + h / 2), (x + w / 2, yc + h / 2), (x, yc - h / 2)], CSYM, closed=True)
    ln((x - w / 2, yc - h / 2), (x + w / 2, yc - h / 2), CSYM)
    for dx in (0.0, 1.4):
        ax, ay = x - w / 2 - 0.6 - dx, yc + 0.6 - dx
        ln((ax, ay), (ax - 2.4, ay + 1.9), CSYM, lw=13)


def sym_fuse_h(xc, y, w=14.0, h=7.0):
    rect(xc - w / 2, y - h / 2, xc + w / 2, y + h / 2, CSYM)
    ln((xc - w / 2, y), (xc + w / 2, y), CSYM)


def sym_gnd(x, y, rot=0):
    s, c = math.sin(math.radians(rot)), math.cos(math.radians(rot))

    def P(dx, dy):
        return (x + dx * c - dy * s, y + dx * s + dy * c)

    ln(P(0, 0), P(0, -2.6), CGND, lw=40)
    for dy, half in ((-2.6, 4.0), (-4.1, 2.7), (-5.6, 1.4)):
        ln(P(-half, dy), P(half, dy), CGND, lw=40)


def sym_flag(x, y, label, layer, up=True, h=2.0):
    d = 1 if up else -1
    ln((x, y), (x, y + d * 3.0), layer, lw=40)
    pl([(x - 1.5, y + d * 1.9), (x, y + d * 3.7), (x + 1.5, y + d * 1.9)],
       layer, closed=True, lw=40)
    txt(label, x, y + d * 5.0, h=h, layer=layer, align=TA.MIDDLE_CENTER)


def port(x, y, label, layer, h=2.0, right=True):
    """Вывод-флажок цепи (net label) в виде стрелки."""
    d = 1 if right else -1
    ln((x, y), (x + d * 2.6, y), layer, lw=30)
    pl([(x + d * 2.6, y), (x + d * 5.0, y + 1.4), (x + d * 5.0, y - 1.4)],
       layer, closed=True, lw=30)
    if right:
        txt(label, x + d * 6.2, y - 0.8, h=h, layer=layer)
    else:
        txt(label, x + d * 6.2, y - 0.8, h=h, layer=layer, align=TA.RIGHT)


def pin(x1, y1, x2, y2, num, layer=CSYM, h=1.6, dy=1.4):
    ln((x1, y1), (x2, y2), layer, lw=30)
    txt(str(num), (x1 + x2) / 2, (y1 + y2) / 2 + dy, h=h, layer=CTX,
        align=TA.MIDDLE_CENTER)


def panel(x0, y0, x1, y1, title):
    rect(x0, y0, x1, y1, CFRAME, lw=25)
    txt(title, x0 + 3, y1 - 5.0, 2.6, bold=True)


def para_center(s, x, y, h=2.0, layer=CNOTE):
    txt(s, x, y, h, layer, align=TA.MIDDLE_CENTER)


# ----------------------------------------------------------------------------
# Рамка и основная надпись
# ----------------------------------------------------------------------------
def title_block(x0, y0, line1, line2, subtitle, sheet_no, total_sheets, fmt, scale):
    w, h = 185.0, 55.0
    rect(x0, y0, x0 + w, y0 + h, CFRAME, lw=50)
    ln((x0, y0 + 30), (x0 + w, y0 + 30), CFRAME)
    ln((x0 + 104, y0), (x0 + 104, y0 + h), CFRAME)
    ln((x0 + 140, y0 + 30), (x0 + 140, y0 + h), CFRAME)
    ln((x0 + 160, y0 + 30), (x0 + 160, y0 + h), CFRAME)
    ln((x0 + 140, y0), (x0 + 140, y0 + 30), CFRAME)
    ln((x0 + 160, y0), (x0 + 160, y0 + 30), CFRAME)
    for dy in (16.0, 23.0):
        ln((x0, y0 + dy), (x0 + 104, y0 + dy), CFRAME)
    ln((x0 + 50, y0), (x0 + 50, y0 + 30), CFRAME)

    txt(line1, x0 + 3, y0 + 45.5, 3.8, bold=True)
    txt(line2, x0 + 3, y0 + 37.5, 2.3)
    txt(subtitle, x0 + 3, y0 + 33.4, 2.0, CNOTE)

    for lab, vx in (("Лит.", x0 + 106), ("Масса", x0 + 141), ("Масштаб", x0 + 161)):
        txt(lab, vx, y0 + 51.5, 1.8)
    txt(scale, x0 + 172, y0 + 42.0, 2.6, align=TA.MIDDLE_CENTER, bold=True)

    txt("Лист", x0 + 106, y0 + 27.0, 1.8)
    txt(str(sheet_no), x0 + 122, y0 + 14.0, 3.6, align=TA.MIDDLE_CENTER, bold=True)
    txt("Листов", x0 + 141, y0 + 27.0, 1.8)
    txt(str(total_sheets), x0 + 150, y0 + 14.0, 3.6, align=TA.MIDDLE_CENTER, bold=True)
    txt("Формат", x0 + 161, y0 + 27.0, 1.8)
    txt(fmt, x0 + 172, y0 + 14.0, 3.6, align=TA.MIDDLE_CENTER, bold=True)

    for lab, y in (("Разраб.", 25.4), ("Пров.", 18.4), ("Утв.", 11.4)):
        txt(lab, x0 + 2, y0 + y, 1.8)
    txt("Дата", x0 + 52, y0 + 25.4, 1.8)


def sheet_frame(x0, y0, w, h, sheet_no, total, scale, line2):
    m = 5.0
    for a, b in (((x0, y0), (x0 + w, y0)), ((x0 + w, y0), (x0 + w, y0 + h)),
                 ((x0 + w, y0 + h), (x0, y0 + h)), ((x0, y0 + h), (x0, y0))):
        ln(a, b, CFRAME, lw=50)
    rect(x0 + m, y0 + m, x0 + w - m, y0 + h - m, CFRAME)
    title_block(x0 + w - m - 185, y0 + m,
                "Плата узла meshrover", line2,
                "ESP32-S3-DevKitC-1 + DWM3000", sheet_no, total, "A3", scale)


# ----------------------------------------------------------------------------
# Лист 1. Панели
# ----------------------------------------------------------------------------
P1 = (10, 196, 128, 290)     # USB-C вход
P2 = (134, 196, 262, 290)    # 5 В и 3,3 В
P3 = (268, 196, 410, 290)    # DWM3000
P4 = (10, 120, 150, 190)     # ESP32-S3
P5 = (156, 120, 262, 190)    # кнопки, UART, USB-данные
P6 = (268, 120, 410, 190)    # таблица соединений
P7 = (10, 64, 150, 114)      # ширины дорожек
P8 = (156, 64, 262, 114)     # тепло и внимание
P9 = (10, 10, 225, 58)       # примечания


def sym_cap_h(xc, y, gap=2.4, h=8.0):
    ln((xc - gap / 2, y - h / 2), (xc - gap / 2, y + h / 2), CSYM)
    ln((xc + gap / 2, y - h / 2), (xc + gap / 2, y + h / 2), CSYM)


def sym_button(xc, y, w=10.0):
    circle(xc - w / 2, y, 0.9, CSYM)
    circle(xc + w / 2, y, 0.9, CSYM)
    ln((xc - w / 2 - 1.4, y + 2.6), (xc + w / 2 + 1.4, y + 2.6), CSYM)
    ln((xc, y + 2.6), (xc, y + 4.4), CSYM)


def pin_in(bx, y, side, name, sep=6.0, h=1.7):
    """Вывод блока: имя внутри корпуса, номер снаружи."""
    if side == "l":
        ln((bx, y), (bx - 5.0, y), CSYM, lw=30)
        txt(name, bx + 1.8, y - 0.7, h)
    else:
        ln((bx, y), (bx + 5.0, y), CSYM, lw=30)
        txt(name, bx - 1.8, y - 0.7, h, align=TA.RIGHT)


def draw_usb_input():
    """Вход питания: клеммник 5 В."""
    x0, y0, x1, y1 = P1
    panel(x0, y0, x1, y1, "1.  ВХОД 5 В")

    rect(16, 246, 32, 276, CSYM)
    txt("J1", 18, 270.0, 2.6, bold=True)
    txt("клеммник", 18, 264.0, 1.9)
    txt("2 конт.", 18, 260.4, 1.6, CNOTE)
    txt("+5 В", 18, 252.0, 1.7, C5V)
    txt("GND", 18, 248.0, 1.7, CGND)

    ln((32, 268), (40, 268), C5V)
    txt("1", 36, 269.4, 1.4, align=TA.MIDDLE_CENTER)
    ln((32, 256), (38, 256), CGND)
    txt("2", 35, 257.4, 1.4, align=TA.MIDDLE_CENTER)
    sym_gnd(38, 256)

    sym_fuse_h(47, 268, 14, 7)
    ln((54, 268), (62, 268), C5V)
    dot(62, 268)
    sym_flag(62, 268, "+5 В", C5V)
    txt("F1  1,1 А", 47, 274.4, 1.9, bold=True, align=TA.MIDDLE_CENTER)

    for x, ref, val in ((72, "C1", "10 мкФ"), (84, "C2", "100 нФ")):
        ln((62, 268), (x, 268), C5V)
        dot(x, 268)
        ln((x, 268), (x, 265), C5V)
        sym_cap_v(x, 263.7)
        ln((x, 262.5), (x, 258), CGND)
        sym_gnd(x, 258)
        txt(ref, x, 273.2, 1.8, bold=True, align=TA.MIDDLE_CENTER)

    ln((62, 268), (104, 268), C5V)
    dot(104, 268)
    ln((104, 268), (104, 265.5), C5V)
    sym_res_v(104, 262, 7)
    ln((104, 258.5), (104, 257.5), C5V)
    sym_led_v(104, 254.5)
    ln((104, 251.5), (104, 246), CGND)
    sym_gnd(104, 246)
    txt("R5", 109, 263.0, 1.8, bold=True)
    txt("680 Ом", 109, 260.4, 1.4, CNOTE)
    txt("LED1", 109, 255.0, 1.8, bold=True)
    txt("5 В есть", 109, 252.4, 1.4, CNOTE)

    txt("Питание — от любого источника 5 В (плата робота,", 12, 214.0, 1.6, CNOTE)
    txt("USB-переходник, лабораторный блок).", 12, 211.4, 1.6, CNOTE)
    txt("F1 — самовосстанавливающийся: при КЗ размыкает", 12, 207.6, 1.6, CNOTE)
    txt("5 В и восстанавливается сам.", 12, 205.0, 1.6, CNOTE)
    txt("Питание ESP32 — через вывод 5V розетки (J1.21):", 12, 200.6, 1.6, CNOTE)
    txt("на DevKit свой стабилизатор.", 12, 198.0, 1.6, CNOTE)


def draw_power():
    x0, y0, x1, y1 = P2
    panel(x0, y0, x1, y1, "2.  ПИТАНИЕ 5 В → 3,3 В")

    RAIL = 272.0
    ln((140, RAIL), (196, RAIL), C5V)
    ln((140, RAIL), (140, 267), C5V)
    txt("5 В", 143, 267.6, 1.8, C5V)

    # на розетку DevKit
    ln((148, RAIL), (148, 266), C5V)
    sym_flag(148, 266, "", C5V, up=False)
    txt("J2.21", 148, 260.0, 1.7, C5V, align=TA.MIDDLE_CENTER)

    # LED1 — индикация 5 В
    ln((170, RAIL), (170, 260.5), C5V)
    sym_res_v(170, 257, 7)
    ln((170, 253.5), (170, 252.5), C5V)
    sym_led_v(170, 249.5)
    ln((170, 246.5), (170, 241), CGND)
    sym_gnd(170, 241)
    txt("R5 680 Ом", 175, 258.4, 1.6, bold=True)
    txt("LED1 · 5 В есть", 175, 249.0, 1.6, CNOTE)

    # C3 — входной конденсатор (вынесен с линии питания, иначе площадки на дорожке)
    ln((186, RAIL), (186, 266.2), C5V)
    sym_cap_v(186, 265)
    ln((186, 263.8), (186, 258), CGND)
    sym_gnd(186, 258)
    txt("C3", 181, 266.0, 1.6, bold=True, align=TA.RIGHT)

    # LDO
    ln((196, RAIL), (196, 250), C5V)
    ln((196, 250), (204, 250), C5V)
    rect(204, 242, 238, 258, CSYM)
    txt("U1", 205, 259.4, 2.2, bold=True)
    txt("AMS1117-3.3", 221, 251.6, 1.7, align=TA.MIDDLE_CENTER, bold=True)
    txt("LDO 3,3 В / 0,8 А", 221, 248.4, 1.5, CNOTE, align=TA.MIDDLE_CENTER)
    txt("3", 201, 250.6, 1.3, align=TA.MIDDLE_CENTER)
    txt("2", 241, 250.6, 1.3, align=TA.MIDDLE_CENTER)
    ln((221, 242), (221, 236), CGND)
    sym_gnd(221, 236)

    # выход 3,3 В и его конденсаторы
    ln((238, 250), (252, 250), C33)
    txt("3,3 В", 253, 251.0, 1.8, C33)
    for x, ref in ((241, "C4"), (250, "C5")):
        ln((x, 250), (x, 245.5), C33)
        sym_cap_v(x, 244.2, w=5.0)
        ln((x, 243), (x, 238), CGND)
        sym_gnd(x, 238)
        txt(ref, x, 235.4, 1.4, bold=True, align=TA.MIDDLE_CENTER)

    txt("C1 — 10 мкФ; C2, C5, C7 — 100 нФ; C3, C4 — 22 мкФ; C6 — 10 мкФ.",
        138, 214.0, 1.5, CNOTE)
    txt("U1 питает только DWM3000 и светодиод: 0,16 А в пике;", 138, 211.4, 1.5, CNOTE)
    txt("P = (5 − 3,3)·0,16 ≈ 0,27 Вт, под SOT-223 — полигон ≥ 1 см².", 138, 208.8, 1.5, CNOTE)
    txt("ESP32 питается от стабилизатора на DevKit через вывод 5V (J1.21).", 138, 205.0, 1.5, CNOTE)


def draw_dwm3000():
    x0, y0, x1, y1 = P3
    panel(x0, y0, x1, y1, "3.  UWB-МОДУЛЬ DWM3000")

    bx0, bx1 = 300, 372
    rect(bx0, 214, bx1, 282, CSYM)
    txt("U2", 302, 277.0, 2.4, bold=True)
    txt("DWM3000", 309, 277.0, 1.9)
    txt("13 × 23 мм", 302, 262.0, 1.6)
    txt("антенна справа,", 302, 259.0, 1.6, CNOTE)
    txt("кипер-аут 10 мм", 302, 256.4, 1.6, CNOTE)
    txt("SPI mode 0, 20 МГц", 302, 219.0, 1.6, CNOTE)

    for y, name in ((274, "VDD1"), (266, "VDD3V3"), (250, "VSS")):
        pin_in(bx0, y, "l", name)
    for y, name in ((276, "SPICSn"), (270, "SPIMOSI"), (264, "SPIMISO"),
                    (258, "SPICLK"), (250, "GPIO8/IRQ"), (244, "RSTn")):
        pin_in(bx1, y, "r", name)

    # 3,3 В на питание
    ln((bx0 - 5, 274), (288, 274), C33)
    ln((bx0 - 5, 266), (288, 266), C33)
    ln((288, 266), (288, 274), C33)
    dot(288, 274)
    ln((288, 274), (288, 279), C33)
    txt("+3V3", 291, 278.6, 1.8, C33)

    # конденсаторы
    ln((288, 266), (288, 258), C33)
    ln((288, 258), (274, 258), C33)
    dot(274, 258)
    dot(284, 258)
    for x, ref, val in ((274, "C6", "10 мкФ"), (284, "C7", "100 нФ")):
        ln((x, 258), (x, 255.5), C33)
        sym_cap_v(x, 254.2)
        ln((x, 253), (x, 248), CGND)
        sym_gnd(x, 248)
        txt(ref, x, 251.4, 1.5, bold=True, align=TA.MIDDLE_CENTER)

    # земля модуля
    ln((bx0 - 5, 250), (294, 250), CGND)
    ln((294, 250), (294, 241), CGND)
    sym_gnd(294, 241)

    for y, net in ((276, "SPI_CS"), (270, "SPI_MOSI"), (264, "SPI_MISO"),
                   (258, "SPI_CLK"), (250, "UWB_IRQ"), (244, "UWB_RST")):
        port(bx1 + 5, y, net, CSIG)

    txt("WAKEUP (2), EXTON (1), GPIO0…GPIO5, GPIO7 (4) — свободны.", 270, 232.0, 1.5, CNOTE)
    txt("GPIO6/SPIPHA (9) и GPIO7/SPIPOL (4) — на землю.", 270, 229.4, 1.5, CNOTE)
    txt("Номера контактов — по Qorvo Data Sheet Rev B, Table 2.", 270, 204.0, 1.5, CNOTE)


def draw_esp32():
    x0, y0, x1, y1 = P4
    panel(x0, y0, x1, y1, "4.  ESP32-S3-DevKitC-1 (J2)")

    bx0, bx1 = 56, 120
    rect(bx0, 126, bx1, 184, CSYM)
    txt("J2", 58, 180.0, 2.4, bold=True)
    txt("розетка", 68, 180.0, 1.6)
    txt("2 × 22 конт., шаг 2,54", 58, 175.6, 1.5)
    txt("ряды 22,86 мм", 58, 172.6, 1.5)
    txt("контакт 1 — слева,", 58, 168.0, 1.5, CNOTE)
    txt("ряд J1 — верхний", 58, 165.4, 1.5, CNOTE)
    txt("использовано 11 из 44 контактов", 72, 133.0, 1.4, CNOTE)

    left = [(178, "5V", "5V", C5V), (171, "GND", None, CGND),
            (164, "GPIO43/TX", "UART_TX", CSIG), (157, "GPIO44/RX", "UART_RX", CSIG),
            (150, "GPIO48", "LED2", CSIG)]
    for y, name, net, layer in left:
        pin_in(bx0, y, "l", name)
        if net == "5V":
            port(bx0 - 5, y, "5V", C5V, right=False)
        elif net is None:
            ln((bx0 - 5, y), (bx0 - 9, y), CGND)
            sym_gnd(bx0 - 9, y)
        else:
            port(bx0 - 5, y, net, layer, right=False)

    right = [(178, "GPIO10", "SPI_CS"), (171, "GPIO11", "SPI_MOSI"),
             (164, "GPIO12", "SPI_MISO"), (157, "GPIO13", "SPI_CLK"),
             (150, "GPIO14", "UWB_RST"), (143, "GPIO15", "UWB_IRQ")]
    for y, name, net in right:
        pin_in(bx1, y, "r", name)
        port(bx1 + 5, y, net, CSIG)


def draw_buttons_uart_usb():
    x0, y0, x1, y1 = P5
    panel(x0, y0, x1, y1, "5.  ПЕРИФЕРИЯ")

    jx = 168
    rect(jx, 144, jx + 12, 178, CSYM)
    txt("J3", jx, 180.4, 2.0, bold=True)
    txt("UART", jx + 6.0, 180.4, 1.5, CNOTE)
    for i, (name, net, layer) in enumerate((("TX", "UART_TX", CSIG),
                                            ("RX", "UART_RX", CSIG),
                                            ("GND", None, CGND))):
        yy = 170 - i * 9
        txt(name, jx + 2, yy - 0.6, 1.7)
        txt(str(i + 1), jx + 10.0, yy + 0.9, 1.3, align=TA.MIDDLE_CENTER)
        ln((jx + 12, yy), (jx + 17, yy), layer, lw=30)
        if net:
            port(jx + 17, yy, net, layer)
        else:
            sym_gnd(jx + 17, yy)

    txt("Гребёнка 1×3: лог 115200. Питание переходника —", 162, 143.0, 1.5, CNOTE)
    txt("от его собственного USB, 3,3 В на плату не выдаём.", 162, 140.6, 1.5, CNOTE)

    port(214, 166, "GPIO48", CSIG, right=False)
    ln((214, 166), (222, 166), CSIG)
    sym_res_v(226, 162, 7)
    ln((226, 158.5), (226, 157.0), CSIG)
    sym_led_v(226, 154)
    ln((226, 151), (226, 146), CGND)
    sym_gnd(226, 146)
    txt("R7", 230, 163.4, 1.7, bold=True)
    txt("330 Ом", 230, 161.0, 1.4, CNOTE)
    txt("LED2", 230, 154.6, 1.7, bold=True)
    txt("статус", 230, 152.2, 1.4, CNOTE)

    txt("Кнопки BOOT и RESET — на самом DevKit:", 162, 134.0, 1.6, CNOTE)
    txt("на носителе они не дублируются (и не мешают", 162, 131.4, 1.6, CNOTE)
    txt("питанию: лишних линий и перемычек нет).", 162, 128.8, 1.6, CNOTE)
    txt("Линии USB данных на плату не выведены:", 162, 124.6, 1.6, CNOTE)
    txt("прошивка — через порт «USB» на DevKit.", 162, 122.0, 1.6, CNOTE)


def draw_connection_table():
    x0, y0, x1, y1 = P6
    panel(x0, y0, x1, y1, "6.  ТАБЛИЦА СОЕДИНЕНИЙ")

    cols = [x0 + 3, x0 + 26, x0 + 60, x0 + 104]
    for cx, hh in zip(cols, ("Цепь", "DWM3000", "J2 (розетка)", "Прошивка / примечание")):
        txt(hh, cx, y1 - 10.0, 1.7, bold=True)
    ln((x0 + 2, y1 - 12.0), (x1 - 2, y1 - 12.0), CFRAME, lw=20)

    rows = [
        ("SPI_CS", "17  SPICSn", "J1.16  GPIO10", "PIN_SPI_CS (активный низкий)"),
        ("SPI_MOSI", "18  SPIMOSI", "J1.17  GPIO11", "PIN_SPI_MOSI"),
        ("SPI_MISO", "19  SPIMISO", "J1.18  GPIO12", "PIN_SPI_MISO"),
        ("SPI_CLK", "20  SPICLK", "J1.19  GPIO13", "PIN_SPI_SCLK · 20 МГц"),
        ("UWB_RST", "3  RSTn", "J1.20  GPIO14", "PIN_DW_RST · перемычка"),
        ("UWB_IRQ", "22  GPIO8", "J1.8  GPIO15", "PIN_DW_IRQ · перемычка"),
        ("UART_TX", "—", "J3.2  GPIO43", "лог 115200"),
        ("UART_RX", "—", "J3.3  GPIO44", ""),
        ("LED2", "—", "J3.16  GPIO48", "R7 330 Ом"),
        ("3V3", "5, 6, 7", "—", "от U1, только DWM3000"),
        ("GND", "8,16,21,23,24", "J1.22, J3.1", "полигон земли"),
        ("5V", "—", "J1.21", "питание DevKit от J1"),
    ]
    yy = y1 - 16.0
    for r in rows:
        for cx, val in zip(cols, r):
            txt(val, cx, yy, 1.45)
        ln((x0 + 2, yy - 1.8), (x1 - 2, yy - 1.8), "ТАБЛИЦЫ", lw=10)
        yy -= 3.8

    txt("GPIO0 (J3.14) — кнопка BOOT на DevKit; 5V (J1.21) — питание DevKit.", x0 + 3, y0 + 3.8, 1.4, CNOTE)
    txt("Номера контактов DWM3000 — Qorvo Data Sheet Rev B, Table 2.", x0 + 3, y0 + 1.6, 1.4, CNOTE)


def draw_width_table():
    x0, y0, x1, y1 = P7
    panel(x0, y0, x1, y1, "7.  ТОКИ И ШИРИНЫ ДОРОЖЕК")

    cols = [x0 + 3, x0 + 62, x0 + 92, x0 + 118]
    for cx, hh in zip(cols, ("Цепь", "Ток", "Ширина", "Запас")):
        txt(hh, cx, y1 - 10.0, 1.7, bold=True)
    ln((x0 + 2, y1 - 12.0), (x1 - 2, y1 - 12.0), CFRAME, lw=20)

    rows = [("5 В (USB → DevKit)", "0,8 А", "1,0 мм", "×3"),
            ("5 В (USB → U1)", "0,2 А", "1,0 мм", "×5"),
            ("3,3 В (U1 → DWM3000)", "0,2 А", "0,5 мм", "×4"),
            ("Земля GND", "0,8 А", "полигон", "—"),
            ("SPI, UART, USB", "20 мА", "0,3 мм", "—"),
            ("Зазор между цепями", "—", "0,3 мм", "норма")]
    yy = y1 - 16.0
    for r in rows:
        for cx, val in zip(cols, r):
            txt(val, cx, yy, 1.6)
        ln((x0 + 2, yy - 1.8), (x1 - 2, yy - 1.8), "ТАБЛИЦЫ", lw=10)
        yy -= 4.6

    txt("A = (I/(k·ΔT⁰·⁴⁴))^(1/0,725), k = 0,048: для 0,8 А при ΔT = 10 °C —", x0 + 3, y0 + 6.4, 1.4, CNOTE)
    txt("0,22 мм; на практике берут ×3…×5 на нагрев и падение.", x0 + 3, y0 + 4.0, 1.4, CNOTE)


def draw_notes():
    x0, y0, x1, y1 = P9
    panel(x0, y0, x1, y1, "9.  ПРИМЕЧАНИЯ")

    notes = [
        "1. Плата — носитель для готового модуля ESP32-S3-DevKitC-1: он вставляется в розетку J2 и "
        "питается от своего стабилизатора через вывод 5V (J1.21). LDO U1 платы питает только DWM3000.",
        "2. DWM3000 питается строго от 3,3 В: VDD1 (5) и VDD3V3 (6, 7). Внутренние 1,8 В модуль "
        "делает сам, внешних выводов 1,8 В у него нет.",
        "3. GPIO6/SPIPHA (9) и GPIO7/SPIPOL (4) посажены на землю — это задаёт SPI mode 0.",
        "4. USB-C платы — только питание. Данные USB на разъём не выведены (контакты 0,5 мм вперемешку, "
        "одним слоем не развести): прошивка — через порт «USB» на DevKit, это тот же USB-Serial-JTAG.",
        "5. Антенна DWM3000 направлена вправо и выходит за край платы. Зона x 53…73, y 0…17 мм — "
        "кипер-аут: ни меди, ни деталей ни сверху, ни снизу.",
        "6. Цвет линии: красный — 5 В, синий — 3,3 В, зелёный — земля, чёрный — сигналы.",
    ]
    yy = y1 - 11.0
    for n in notes:
        yy = para(n, x0 + 3.0, yy, 205, h=1.75, leading=2.8)
        yy -= 0.8


def draw_thermal_and_warning():
    x0, y0, x1, y1 = P8
    panel(x0, y0, x1, y1, "8.  ТЕПЛО И ВНИМАНИЕ")

    txt("U1: P = (5 − 3,3) · I = 1,7 · I", x0 + 3, y1 - 11.0, 1.7, bold=True)
    rows = [("0,10 А", "0,17 Вт", "тепло не чувствуется"),
            ("0,16 А", "0,27 Вт", "норма, ΔT ≈ 16 °C"),
            ("0,30 А", "0,51 Вт", "предел без полигона")]
    yy = y1 - 16.0
    for r in rows:
        txt(r[0], x0 + 3, yy, 1.5)
        txt(r[1], x0 + 28, yy, 1.5)
        txt(r[2], x0 + 56, yy, 1.5)
        yy -= 3.8

    txt("ВНИМАНИЕ!", x0 + 3, y0 + 12.0, 2.0, bold=True, layer=CNOTE)
    para("5 В на GPIO и на 3,3 В выводит модули из строя. Не вставляйте два USB одновременно: "
         "источники встретятся.",
         x0 + 3, y0 + 8.4, 100, h=1.5, leading=2.6, layer=CNOTE)


def sheet1():
    sheet_frame(0, 0, 420, 297, 1, 3, "1 : 1", "Схема электрическая принципиальная")
    draw_usb_input()
    draw_power()
    draw_buttons_uart_usb()
    draw_dwm3000()
    draw_esp32()
    draw_connection_table()
    draw_width_table()
    draw_thermal_and_warning()
    draw_notes()


# ----------------------------------------------------------------------------
# Рендер PNG
# ----------------------------------------------------------------------------
def _fill_copper(ax):
    """Заливает медь патчами: HATCH с дырками matplotlib сам не рисует."""
    from matplotlib.path import Path as MPath
    from matplotlib.patches import PathPatch
    from ezdxf.colors import aci2rgb
    from shapely.geometry.polygon import orient

    if COPPER is None:
        return
    for ox, oy, s_ in ((BX0, BY0, BSCALE), (ART_X, ART_Y, 1.0)):
        for lay, geom in COPPER.items():
            try:
                aci = doc.layers.get(lay).color
                r, g, b = (0, 0, 0) if aci == 7 else aci2rgb(aci)
            except Exception:
                r, g, b = 0, 0, 0
            colour = (r / 255, g / 255, b / 255)
            for poly in _polys(geom):
                poly = orient(poly, sign=1.0)
                verts, codes = [], []
                for ring in [poly.exterior] + list(poly.interiors):
                    pts = [(ox + x * s_, oy + y * s_) for x, y in ring.coords[:-1]]
                    if len(pts) < 3:
                        continue
                    verts.extend(pts + [pts[0]])
                    codes.append(MPath.MOVETO)
                    codes.extend([MPath.LINETO] * (len(pts) - 1))
                    codes.append(MPath.CLOSEPOLY)
                if not verts:
                    continue
                ax.add_patch(PathPatch(MPath(verts, codes), facecolor=colour,
                                       edgecolor="none", linewidth=0,
                                       alpha=0.45 if lay == "ЗЕМЛЯ" else 0.85,
                                       antialiased=False, zorder=0))


def render_png(path, bbox=(0, 0, 420, 297), dpi=150):
    import matplotlib
    matplotlib.use("Agg")
    matplotlib.rcParams["font.family"] = "Arial"
    import matplotlib.pyplot as plt
    from ezdxf.addons.drawing import Frontend, RenderContext
    from ezdxf.addons.drawing.config import (
        BackgroundPolicy, ColorPolicy, Configuration, HatchPolicy, LineweightPolicy,
    )
    from ezdxf.addons.drawing.matplotlib import MatplotlibBackend

    w = (bbox[2] - bbox[0]) / 25.4
    h = (bbox[3] - bbox[1]) / 25.4
    fig = plt.figure(figsize=(w, h))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()
    ctx = RenderContext(doc)
    cfg = Configuration(background_policy=BackgroundPolicy.WHITE,
                        color_policy=ColorPolicy.COLOR,
                        lineweight_policy=LineweightPolicy.ABSOLUTE,
                        lineweight_scaling=1.0,
                        hatch_policy=HatchPolicy.IGNORE)
    Frontend(ctx, MatplotlibBackend(ax), config=cfg).draw_layout(msp, finalize=True)
    ax.set_aspect("auto")
    fig.set_size_inches(w, h)
    ax.set_position([0, 0, 1, 1])
    ax.set_xlim(bbox[0], bbox[2])
    ax.set_ylim(bbox[1], bbox[3])
    _fill_copper(ax)
    fig.savefig(path, dpi=dpi, facecolor="white")
    plt.close(fig)


# ----------------------------------------------------------------------------
# Дополнение к отчёту: разделы, которые make_report.py не знает
# ----------------------------------------------------------------------------
DWM_FUNC = {
    1: ("EXTON", "выход, управление внешним DC-DC", "не подключён"),
    2: ("WAKEUP", "вход пробуждения", "не подключён (по даташиту можно на GND)"),
    3: ("RSTn", "сброс, активный низкий", "GPIO14 (J1.20)"),
    4: ("GPIO7 / SPIPOL", "полярность SPI при старте", "GND → SPIPOL = 0"),
    5: ("VDD1", "питание AON-домена, 1,62…3,6 В", "+3V3"),
    6: ("VDD3V3", "питание 3 В", "+3V3"),
    7: ("VDD3V3", "питание 3 В", "+3V3"),
    8: ("VSS", "земля", "полигон GND"),
    9: ("GPIO6 / SPIPHA", "фаза SPI при старте", "GND → SPIPHA = 0"),
    10: ("GPIO5", "GPIO", "не подключён"),
    11: ("GPIO4", "GPIO", "не подключён"),
    12: ("GPIO3 / TXLED", "GPIO", "не подключён"),
    13: ("GPIO2 / RXLED", "GPIO", "не подключён"),
    14: ("GPIO1 / SFDLED", "GPIO", "не подключён"),
    15: ("GPIO0 / RXOKLED", "GPIO", "не подключён"),
    16: ("VSS", "земля", "полигон GND"),
    17: ("SPICSn", "выбор кристалла, активный низкий", "GPIO10 (J1.16)"),
    18: ("SPIMOSI", "данные в модуль", "GPIO11 (J1.17)"),
    19: ("SPIMISO", "данные из модуля", "GPIO12 (J1.18)"),
    20: ("SPICLK", "тактовая частота", "GPIO13 (J1.19)"),
    21: ("VSS", "земля", "полигон GND"),
    22: ("GPIO8 / IRQ", "прерывание к хосту", "GPIO15 (J1.8)"),
    23: ("VSS", "земля", "полигон GND"),
    24: ("VSS", "земля", "полигон GND"),
}


def append_report(path):
    """Дописывает в отчёт разделы про DWM3000 и расчёты."""
    spec = SPEC
    dwm = spec["dwm3000"]
    calc = spec.get("calculations", {})
    lines = ["", "## Модуль DWM3000 (U2)", "",
             "Корпус 13 × 23 мм, 24 контакта, площадки 1,6 × 1,0 мм с шагом 1,4 мм "
             "(Qorvo DWM3000 Data Sheet Rev B, § 7.2).", "",
             "| Конт. | Сигнал | Назначение | Куда идёт |",
             "| ---: | :--- | :--- | :--- |"]
    for pin in range(1, 25):
        name, func, dest = DWM_FUNC[pin]
        lines.append(f"| {pin} | {name} | {func} | {dest} |")
    lines += ["", f"Статус цоколёвки: **{dwm.get('pinout_status')}** "
                  f"({dwm.get('source')}).", ""]
    lines += ["## Расчёт питания", "",
              "| Величина | Значение |", "| :--- | :--- |",
              f"| Вход | {calc.get('input', '—')} |",
              f"| Пиковый ток 5 В (ESP32 Wi-Fi + DWM3000 + светодиоды) | {calc.get('rail_5v_peak_a')} А |",
              f"| Ток по 3,3 В (только DWM3000) | {calc.get('rail_3v3_a')} А |",
              f"| Падение на U1 | {calc.get('ldo_drop_v')} В |",
              f"| Рассеиваемая мощность U1 | {calc.get('ldo_power_w')} Вт |",
              f"| Перегрев U1 без радиатора | ≈ {calc.get('ldo_temp_rise_c')} °C |",
              f"| Порог срабатывания F1 | {calc.get('f1_trip_a')} А |", ""]
    lengths = {}
    for net, pts, w in TRACES:
        lengths[net] = lengths.get(net, 0.0) + sum(
            math.dist(pts[i], pts[i + 1]) for i in range(len(pts) - 1))
    lines += ["## Проложенная медь", "",
              "| Цепь | Ширина, мм | Длина, мм |", "| :--- | ---: | ---: |"]
    for net in sorted(lengths):
        w = next(w for n, _, w in TRACES if n == net)
        lines.append(f"| {net} | {w:.1f} | {lengths[net]:.1f} |")
    if COPPER:
        parts = sorted([q for q in _polys(COPPER["ЗЕМЛЯ"]) if q.area > 0.05],
                       key=lambda q: -q.area)
        lines += ["", "## Связность земли", "",
                  f"Полигон земли распадается на {len(parts)} куска: дорожки разрезают "
                  "его между рядами розетки и вокруг модуля.", "",
                  "| Кусков | Площади, мм² |", "| ---: | :--- |",
                  f"| {len(parts)} | {', '.join(f'{q.area:.0f}' for q in parts)} |", "",
                  "Оставшиеся без меди куски соединены проволочными перемычками: GND модуля "
                  "(WP3), GND полигона (WP4), GND конденсаторов (WP5) — все они есть в "
                  "перечне элементов и показаны пунктиром на листе 2.", ""]
    lines += ["", "## Что учтено из даташита", "",
              "- Кипер-аут антенны: полоса x 53…73, y 0…17 мм без меди и деталей; "
              "антенна модуля направлена за край платы.",
              "- Заливка земли по всей остальной площади — требование § 6.1 даташита.",
              "- VDD2/VDD3 у DWM3000 наружу не выведены: 1,8 В делает внутренний регулятор модуля.",
              "- `d` (расстояние от антенны до металла) по даташиту — 10 мм; выдержано "
              "кипер-аутом внутри платы (антенна смотрит вправо, к правому краю).", ""]
    with open(path, "a", encoding="utf-8") as f:
        f.write("\n".join(lines))
    print("дополнено:", path)


# ----------------------------------------------------------------------------
# Листы 2–3. Плата: контур, медь, шелкография, кипер-аут
# ----------------------------------------------------------------------------
BW, BH = 80.0, 55.0
KEEPOUT = (53.0, 0.0, 73.0, 17.0)

BSCALE = 2.0
SHEET_GAP = 23.0
SHEET3_Y = 0.0
SHEET2_Y = 297.0 + SHEET_GAP
SHEET1_Y = SHEET2_Y + 297.0 + SHEET_GAP

BX0, BY0 = 14.0, SHEET2_Y + 110.0
ART_X, ART_Y = 58.0, SHEET3_Y + 104.0

PAD = "ПЛАТА_ПАДЫ"
SILK = "ПЛАТА_ДЕТАЛИ"
EDGE = "ПЛАТА_КОНТУР"
KO = "КИПЕРАУТ"
JMP = "ПЕРЕМЫЧКИ"

for _n, _c, _lw, _lt in ((KO, 2, 13, "DASHED"), (JMP, 2, 25, "DASHED")):
    if _n not in doc.layers:
        doc.layers.add(name=_n, color=_c, lineweight=_lw, linetype=_lt)

doc.layers.get("ПЛАТА_ДЕТАЛИ").dxf.color = 8
doc.layers.get("КИПЕРАУТ").dxf.color = 1

NET_LAYER = {"5V": "ШИНА_5В", "5V_IN": "ШИНА_5В", "3V3": "ШИНА_3В3", "GND": "ЗЕМЛЯ"}
NETS = ["5V", "5V_IN", "3V3", "GND", "SPI_CS", "SPI_MOSI", "SPI_MISO", "SPI_CLK",
        "UWB_RST", "UWB_IRQ", "UART_TX", "UART_RX", "LED2", "LED1_A"]


def layer_of(net):
    return NET_LAYER.get(net, "СИГНАЛЫ")


# ---- посадочные места: (net, x, y, w, h, drill) ------------------------------
PADS = []


def pad(net, x, y, w, h=0.0, drill=0.0):
    PADS.append((net, float(x), float(y), float(w), float(h or w), float(drill)))


def smd0805(net_a, net_b, x, y, vertical=True, pw=1.0, ph=1.2, pitch=1.9):
    if vertical:
        pad(net_a, x, y - pitch / 2, pw, ph)
        pad(net_b, x, y + pitch / 2, pw, ph)
    else:
        pad(net_a, x - pitch / 2, y, pw, ph)
        pad(net_b, x + pitch / 2, y, pw, ph)


def smd1812(net_a, net_b, x, y, vertical=False):
    if vertical:
        pad(net_a, x, y - 1.8, 1.4, 2.0)
        pad(net_b, x, y + 1.8, 1.4, 2.0)
    else:
        pad(net_a, x - 1.8, y, 2.0, 1.4)
        pad(net_b, x + 1.8, y, 2.0, 1.4)


# --- розетка 2 x 22 ----------------------------------------------------------
J1_NET = {1: "3V3_DEV", 3: "UWB_IRQ", 8: "UWB_IRQ", 16: "SPI_CS", 17: "SPI_MOSI",
          18: "SPI_MISO", 19: "SPI_CLK", 20: "UWB_RST", 21: "5V", 22: "GND"}
J1_NET[3] = "NC"          # EN не используется: кнопка RESET на DevKit
J3_NET = {1: "GND", 2: "UART_TX", 3: "UART_RX", 14: "NC", 16: "LED2",
          21: "GND", 22: "GND"}
for k in range(22):
    x = 18.25 + 2.54 * k
    pad(J1_NET.get(k + 1, "NC"), x, 19.0, 1.6, drill=1.0)
    pad(J3_NET.get(k + 1, "NC"), x, 41.86, 1.6, drill=1.0)

# --- DWM3000 -----------------------------------------------------------------
DWM_NET = {1: "NC", 2: "NC", 3: "UWB_RST", 4: "GND", 5: "3V3", 6: "3V3", 7: "3V3",
           8: "GND", 9: "GND", 10: "NC", 11: "NC", 12: "NC", 13: "NC", 14: "NC",
           15: "NC", 16: "GND", 17: "SPI_CS", 18: "SPI_MOSI", 19: "SPI_MISO",
           20: "SPI_CLK", 21: "GND", 22: "UWB_IRQ", 23: "GND", 24: "GND"}
DWM_POS = {}
for i, x in enumerate((52.31, 50.91, 49.51, 48.11, 46.71, 45.31, 43.91, 42.51)):
    DWM_POS[i + 1] = (x, 3.0, 1.0, 1.6)
for i, x in enumerate((42.51, 43.91, 45.31, 46.71, 48.11, 49.51, 50.91, 52.31)):
    DWM_POS[i + 17] = (x, 14.0, 1.0, 1.6)
for i, y in enumerate((3.6, 5.0, 6.4, 7.8, 9.2, 10.6, 12.0, 13.4)):
    DWM_POS[i + 9] = (41.0, y, 1.6, 1.0)
DATASHEET_PADS = set()
for pin, (x, y, w, h) in DWM_POS.items():
    DATASHEET_PADS.add(len(PADS))
    pad(DWM_NET[pin], x, y, w, h)

# --- питание -----------------------------------------------------------------
pad("5V_IN", 6, 31.0, 3.0, drill=1.3)        # J1.1
pad("GND", 6, 36.08, 3.0, drill=1.3)         # J1.2
smd1812("5V_IN", "5V", 11, 36)               # F1
smd0805("GND", "5V", 15, 33)                 # C1
smd0805("GND", "5V", 18, 33)                 # C2
pad("GND", 25, 7, 1.0, 1.8)                  # U1.1
pad("3V3", 25, 9.5, 1.0, 1.8)                # U1.2
pad("5V", 25, 12, 1.0, 1.8)                  # U1.3
pad("3V3", 31.5, 9.5, 3.4, 1.8)              # U1 tab
smd0805("GND", "5V", 22, 4)                  # C3
smd0805("GND", "3V3", 29, 4)                 # C5
smd0805("GND", "3V3", 32, 4)                 # C4
smd0805("LED1_A", "5V", 66, 33)              # R5
smd0805("GND", "LED1_A", 66, 30)             # LED1


# --- периферия ---------------------------------------------------------------
for i, net in enumerate(("UART_TX", "UART_RX", "GND")):
    pad(net, 10 + 2.54 * i, 50, 1.8, drill=1.0)

smd0805("LED2", "LED2_A", 66, 46)            # R7: GPIO48 -> LED2_A
smd0805("LED2_A", "GND", 66, 49)             # LED2: -> GND

# ---- трассы -----------------------------------------------------------------
TRACES = []


def tr(net, *pts, w=None):
    width = w if w is not None else (1.0 if net in ("5V",) else
                                     0.5 if net == "3V3" else
                                     0.8 if net == "5V_IN" else 0.3)
    TRACES.append((net, [(float(x), float(y)) for x, y in pts], width))


tr("5V_IN", (6, 31), (9.2, 31), (9.2, 36))
tr("5V", (12.8, 36), (69.05, 36), (69.05, 19.9))
tr("5V", (16.5, 36), (16.5, 12), (25, 12), w=0.6)
tr("5V", (22, 12), (22, 4.95), w=0.5)
tr("5V", (15, 36), (15, 33.95), w=0.4)
tr("5V", (18, 36), (18, 33.95), w=0.4)

tr("5V", (66, 36), (66, 33.95), w=0.4)

tr("3V3", (25, 9.5), (34.76, 9.5), (34.76, 1.55), (46.71, 1.55), w=0.5)
tr("3V3", (43.91, 1.55), (43.91, 2.2), w=0.4)
tr("3V3", (45.31, 1.55), (45.31, 2.2), w=0.4)
tr("3V3", (46.71, 1.55), (46.71, 2.2), w=0.4)
tr("3V3", (29, 9.5), (29, 4.95), w=0.4)
tr("3V3", (32, 9.5), (32, 4.95), w=0.4)

tr("SPI_CS", (42.51, 14.8), (42.51, 17.45), (56.35, 17.45), (56.35, 18.4))
tr("SPI_MOSI", (43.91, 14.8), (43.91, 16.85), (58.89, 16.85), (58.89, 18.4))
tr("SPI_MISO", (45.31, 14.8), (45.31, 16.25), (61.43, 16.25), (61.43, 18.4))
tr("SPI_CLK", (46.71, 14.8), (46.71, 15.65), (63.97, 15.65), (63.97, 18.4))

tr("UWB_RST", (49.51, 2.2), (49.51, 0.8), (27.14, 0.8), (27.14, 8.4), w=0.25)
tr("UWB_RST", (27.14, 10.6), (27.14, 20.55), (66.51, 20.55), (66.51, 19.6), w=0.25)
tr("UWB_IRQ", (34.76, 17.5), (35.6, 18.4), (36.03, 18.6))
pad("UWB_IRQ", 34.76, 17.5, 0.9)

tr("UART_TX", (20.79, 42.76), (20.79, 45), (10, 45), (10, 50))
tr("UART_RX", (23.33, 42.76), (23.33, 46.5), (12.54, 46.5), (12.54, 50))
tr("LED2", (56.35, 42.76), (56.35, 44.5), (66, 44.5), (66, 45.05))
tr("LED2_A", (66, 46.95), (66, 48.05), w=0.4)
tr("LED1_A", (66, 32.05), (66, 30.95), w=0.4)

# --- мосты земли: соединяют карманы полигона с основным массивом ---


# землю соединяем перемычками (см. JMP_GND / JMP_GND3): медные мосты
# пересекали бы веер SPI и шину RST

JMP_PAD = (34.76, 17.5)
JMP_RST = ((27.14, 8.4), (27.14, 10.6))
JMP_GND = (50.0, 23.0)
pad("GND", JMP_GND[0], JMP_GND[1], 1.0)
JMP_GND3 = ((32.0, 3.05), (25.0, 7.0))


def chamfer(pts, d=0.45):
    if len(pts) < 3:
        return list(pts)
    out = [pts[0]]
    for i in range(1, len(pts) - 1):
        p0, p1, p2 = pts[i - 1], pts[i], pts[i + 1]
        v1 = (p1[0] - p0[0], p1[1] - p0[1])
        v2 = (p2[0] - p1[0], p2[1] - p1[1])
        l1, l2 = math.hypot(*v1), math.hypot(*v2)
        if l1 < 1e-9 or l2 < 1e-9:
            continue
        u1 = (v1[0] / l1, v1[1] / l1)
        u2 = (v2[0] / l2, v2[1] / l2)
        if u1[0] * u2[0] + u1[1] * u2[1] > 0.999:
            continue
        dd = min(d, l1 / 2.0, l2 / 2.0)
        out.append((p1[0] - u1[0] * dd, p1[1] - u1[1] * dd))
        out.append((p1[0] + u2[0] * dd, p1[1] + u2[1] * dd))
    out.append(pts[-1])
    return out


TRACES = [(n, chamfer(p), w) for n, p, w in TRACES]

# ---------------------------------------------------------------------------
# Зеркало по вертикали: плата читается «сверху-вниз» так же, как модули
# вставляются в розетку. Зеркало — изометрия, зазоры и длины сохраняются.
# ---------------------------------------------------------------------------
# Зеркало по вертикали. Включено по требованию заказчика: посадки на чертеже
# зеркальны относительно вида со стороны пайки, то есть модули и элементы
# ставятся со стороны, ПРОТИВОПОЛОЖНОЙ меди. Для монтажа медью вверх
# (обычная сборка) поставить MIRROR_Y = False — разводка пересчитается сама.
MIRROR_Y = True


def MY(y):
    """Координата подписи с учётом зеркала (позиции, заданные вручную)."""
    return BH - y if MIRROR_Y else y


def DY(d):
    return -d if MIRROR_Y else d


def apply_mirror_y():
    global PADS, TRACES, KEEPOUT, SILK_ITEMS, ART_Y
    global JMP_PAD, JMP_RST, JMP_GND, JMP_GND3, DWM_POS
    if not MIRROR_Y:
        return
    PADS = [(net, x, BH - y, w, h, d) for net, x, y, w, h, d in PADS]
    TRACES = [(net, [(x, BH - y) for x, y in pts], w) for net, pts, w in TRACES]
    KEEPOUT = (KEEPOUT[0], BH - KEEPOUT[3], KEEPOUT[2], BH - KEEPOUT[1])
    SILK_ITEMS = [(x0, BH - y1, x1, BH - y0, ref, rx, BH - ry)
                  for x0, y0, x1, y1, ref, rx, ry in SILK_ITEMS]
    DWM_POS = {k: (x, BH - y, w, h) for k, (x, y, w, h) in DWM_POS.items()}
    JMP_PAD = (JMP_PAD[0], BH - JMP_PAD[1])
    JMP_RST = tuple((x, BH - y) for x, y in JMP_RST)
    JMP_GND = (JMP_GND[0], BH - JMP_GND[1])
    JMP_GND3 = tuple((x, BH - y) for x, y in JMP_GND3)

# ---- медь -------------------------------------------------------------------
POUR_CLR = 0.4
_rounded_cache = {}


def _polys(g):
    if g is None or g.is_empty:
        return []
    if g.geom_type == "Polygon":
        return [g]
    return [p for p in g.geoms if not p.is_empty]


def build_copper():
    from shapely.geometry import LineString, Point, box as sbox
    from shapely.ops import unary_union

    by_layer, drills = {}, {}
    for net, pts, w in TRACES:
        g = LineString(pts).buffer(w / 2.0, quad_segs=16, cap_style="round",
                                   join_style="round")
        by_layer.setdefault(layer_of(net), []).append(g)

    gnd_shapes = []
    for net, x, y, w, h, drill in PADS:
        lay = layer_of(net)
        if drill > 0:
            geom = Point(x, y).buffer(w / 2.0, quad_segs=24)
            drills.setdefault(lay, []).append(Point(x, y).buffer(drill / 2.0, quad_segs=20))
        else:
            geom = sbox(x - w / 2, y - h / 2, x + w / 2, y + h / 2)
        by_layer.setdefault(lay, []).append(geom)
        if net == "GND":
            gnd_shapes.append(geom)

    copper = {}
    for lay, shapes in by_layer.items():
        g = unary_union(shapes)
        if lay in drills:
            g = g.difference(unary_union(drills[lay]))
        copper[lay] = g

    # полигон земли
    from shapely.geometry import Polygon
    board = _rounded_board().buffer(-0.4, join_style=1)
    keep = Polygon([(KEEPOUT[0], KEEPOUT[1]), (KEEPOUT[2], KEEPOUT[1]),
                    (KEEPOUT[2], KEEPOUT[3]), (KEEPOUT[0], KEEPOUT[3])])
    others = [g for lay, g in copper.items() if lay != "ЗЕМЛЯ"]
    pour = board.difference(keep)
    if others:
        pour = pour.difference(unary_union(others).buffer(POUR_CLR))
    for hole in SPEC["board"]["holes"]:
        hx, hy, hd = float(hole["x"]), float(hole["y"]), float(hole["d"])
        pour = pour.difference(Point(hx, hy).buffer(hd / 2.0 + 0.6, quad_segs=20))

    gnd = copper.get("ЗЕМЛЯ")
    if gnd is not None:
        pour = pour.union(gnd)

    # терморельеф на сквозных площадках земли
    th_gnd = [(x, y, w) for net, x, y, w, h, drill in PADS
              if net == "GND" and drill > 0]
    rings, spokes = [], []
    for x, y, r_pad in th_gnd:
        ring = Point(x, y).buffer(r_pad / 2 + POUR_CLR, quad_segs=24).difference(
            Point(x, y).buffer(r_pad / 2 + 0.02, quad_segs=24))
        rings.append(ring)
        for ang in (45, 135, 225, 315):
            a = math.radians(ang)
            L = r_pad / 2 + POUR_CLR + 0.3
            dx, dy = math.cos(a), math.sin(a)
            spokes.append(LineString([(x + dx * r_pad / 2, y + dy * r_pad / 2),
                                      (x + dx * L, y + dy * L)]).buffer(
                0.35, cap_style="flat"))
    if rings:
        avoid = unary_union(others).buffer(0.25) if others else None
        sp = unary_union(spokes)
        if avoid is not None:
            sp = sp.difference(avoid)
        pour = pour.difference(unary_union(rings)).union(sp)
    # убираем мелкие острова, не связанные ни с одной площадкой земли
    from shapely.geometry import Point as _P
    pad_pts = [_P(x, y) for net, x, y, w, h, drill in PADS if net == "GND"]
    keep_parts, dropped = [], 0
    for part in _polys(pour):
        if any(part.buffer(0.01).contains(q) for q in pad_pts):
            keep_parts.append(part)
        else:
            dropped += 1
    pour = unary_union(keep_parts) if keep_parts else pour
    copper["ЗЕМЛЯ"] = pour
    return copper


def check_design(clr=0.3):
    """Зазоры между цепями и проходы дорожек между соседними контактами."""
    from shapely.geometry import LineString, Point, box as sbox

    pads = []
    for i, (net, x, y, w, h, drill) in enumerate(PADS):
        g = (Point(x, y).buffer(w / 2.0, quad_segs=24) if drill > 0
             else sbox(x - w / 2, y - h / 2, x + w / 2, y + h / 2))
        pads.append((i, net, x, y, w, g))

    items = [(net, "площадка", f"{x:.2f},{y:.2f}", g) for i, net, x, y, w, g in pads
             if net != "NC"]
    for net, pts, w in TRACES:
        items.append((net, "дорожка", f"{pts[0][0]:.1f},{pts[0][1]:.1f}",
                      LineString(pts).buffer(w / 2.0, cap_style="round",
                                             join_style="round")))

    bad = []
    for i in range(len(pads)):
        _, ni, xi, yi, wi, gi = pads[i]
        for j in range(i + 1, len(pads)):
            _, nj, xj, yj, wj, gj = pads[j]
            if ni == nj:
                continue
            if i in DATASHEET_PADS and j in DATASHEET_PADS:
                continue          # посадочное место задано даташитом
            d = gi.distance(gj)
            if d < clr:
                bad.append((d, ni, "площадка", f"{xi:.2f},{yi:.2f}",
                            nj, "площадка", f"{xj:.2f},{yj:.2f}"))
    for net, pts, w in TRACES:
        line = LineString(pts).buffer(w / 2.0, cap_style="round", join_style="round")
        for i, pnet, px, py, pw, pg in pads:
            if pnet == net:
                continue
            d = line.distance(pg)
            if d < clr:
                bad.append((d, net, "дорожка", f"{pts[0][0]:.1f},{pts[0][1]:.1f}",
                            pnet, "площадка", f"{px:.2f},{py:.2f}"))
    for i in range(len(TRACES)):
        ni, pi, wi = TRACES[i]
        gi = LineString(pi).buffer(wi / 2.0, cap_style="round", join_style="round")
        for j in range(i + 1, len(TRACES)):
            nj, pj, wj = TRACES[j]
            if ni == nj:
                continue
            gj = LineString(pj).buffer(wj / 2.0, cap_style="round", join_style="round")
            if gi.distance(gj) < clr:
                bad.append((gi.distance(gj), ni, "дорожка", "", nj, "дорожка", ""))

    # проходы дорожек между соседними контактами (гребёнки и розетки)
    rows = {}
    for i, net, x, y, w, g in pads:
        rows.setdefault(round(y, 2), []).append((x, net, w, g))
    passages = []
    for net, pts, w in TRACES:
        line = LineString(pts)
        for y, row in rows.items():
            row.sort()
            if len(row) < 4:
                continue
            if not all(abs((row[k + 1][0] - row[k][0]) - 2.54) < 0.2
                       for k in range(len(row) - 1)):
                continue
            for a, b in zip(row, row[1:]):
                if not (2.0 < b[0] - a[0] < 3.2):
                    continue
                if net in (a[1], b[1]):
                    continue
                seg = LineString([(a[0], y), (b[0], y)])
                if not line.intersects(seg) or line.touches(seg):
                    continue
                d = min(line.distance(a[3]), line.distance(b[3]))
                passages.append((d, net, f"{a[0]:.1f}…{b[0]:.1f}", y))

    bad.sort()
    print(f"зазоров меньше {clr} мм: {len(bad)}")
    for d, ni, ki, si, nj, kj, sj in bad[:12]:
        print(f"  {ni} ({ki} {si}) ↔ {nj} ({kj} {sj}): {d:.2f} мм")
    print(f"проходов между контактами: {len(passages)}")
    for d, net, span, y in sorted(passages):
        print(f"  {net} между контактами {span} (ряд y={y}): зазор {d:.3f} мм")
    return bad + [p for p in passages if p[0] < clr]


def _rounded_board():
    from shapely.geometry import Polygon
    r, n = 3.0, 10
    pts = []
    for cx, cy, a0 in ((BW - r, r, -90), (BW - r, BH - r, 0),
                       (r, BH - r, 90), (r, r, 180)):
        for i in range(n + 1):
            a = math.radians(a0 + 90.0 * i / n)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    return Polygon(pts)


COPPER = None


def emit_copper(ox, oy, s, hatch=True, lw=13):
    from ezdxf.lldxf import const
    for lay, geom in COPPER.items():
        try:
            aci = doc.layers.get(lay).color
        except Exception:
            aci = 7
        for poly in _polys(geom):
            ext = [(ox + x * s, oy + y * s) for x, y in poly.exterior.coords[:-1]]
            holes = [[(ox + x * s, oy + y * s) for x, y in r.coords[:-1]]
                     for r in poly.interiors]
            msp.add_lwpolyline(ext, close=True, dxfattribs={"layer": lay, "lineweight": lw})
            for ring in holes:
                msp.add_lwpolyline(ring, close=True, dxfattribs={"layer": lay, "lineweight": lw})
            if not hatch:
                continue
            h = msp.add_hatch(dxfattribs={"layer": lay})
            h.set_solid_fill(color=aci, style=0)
            h.paths.add_polyline_path(ext, is_closed=True,
                                      flags=const.BOUNDARY_PATH_EXTERNAL)
            for ring in holes:
                h.paths.add_polyline_path(ring, is_closed=True,
                                          flags=const.BOUNDARY_PATH_DEFAULT)
SILK_ITEMS = [
    (3.5, 31.0, 9.0, 41.0, "J1", 9.8, 40.4),
    (9.0, 34.6, 13.0, 37.4, "F1", 9.6, 38.4),
    (14.0, 31.9, 16.0, 34.1, "C1", 16.4, 34.6),
    (17.0, 31.9, 19.0, 34.1, "C2", 19.4, 34.6),
    (23.4, 7.4, 31.2, 13.6, "U1", 23.2, 14.6),
    (20.0, 2.9, 22.0, 5.1, "C3", 20.0, 1.9),
    (28.0, 2.9, 30.0, 5.1, "C5", 28.0, 1.9),
    (31.0, 2.9, 33.0, 5.1, "C4", 31.0, 1.9),
    (65.0, 31.9, 67.0, 34.1, "R5", 67.6, 33.6),
    (65.0, 28.9, 67.0, 31.1, "LED1", 67.6, 29.6),
    (65.0, 44.9, 67.0, 47.1, "R7", 67.6, 46.6),
    (65.0, 47.9, 67.0, 50.1, "LED2", 67.6, 49.6),
    (9.0, 48.6, 16.6, 51.4, "J3", 9.0, 52.6),
    (40.0, 2.0, 63.0, 15.0, "U2", 45.0, 9.0),
    (17.0, 18.0, 72.5, 42.9, "J2", 43.0, 30.0),
]


apply_mirror_y()


def draw_silk(ox, oy, s):
    for x0, y0, x1, y1, ref, rx, ry in SILK_ITEMS:
        if ref in ("J2", "U2"):
            ln((ox + x0 * s, oy + y0 * s), (ox + x1 * s, oy + y0 * s), SILK, lw=18)
            ln((ox + x1 * s, oy + y0 * s), (ox + x1 * s, oy + y1 * s), SILK, lw=18)
            ln((ox + x1 * s, oy + y1 * s), (ox + x0 * s, oy + y1 * s), SILK, lw=18)
            ln((ox + x0 * s, oy + y1 * s), (ox + x0 * s, oy + y0 * s), SILK, lw=18)
        else:
            rect(ox + x0 * s, oy + y0 * s, ox + x1 * s, oy + y1 * s, SILK, lw=13)
        txt(ref, ox + rx * s, oy + ry * s, 1.4 if s < 1.5 else 1.7, SILK)
    # метка контакта 1 розетки и антенны
    txt("1", ox + 17.0 * s, oy + MY(21.2) * s, 1.6 if s > 1.5 else 1.2, SILK, bold=True)
    txt("АНТЕННА", ox + 55.0 * s, oy + MY(9.0) * s, 1.3 if s > 1.5 else 1.0, KO,
        align=TA.MIDDLE_CENTER)
    txt("КИПЕР-АУТ: без меди и деталей", ox + 63.0 * s, oy + MY(3.4) * s,
        1.2 if s > 1.5 else 0.9, KO, align=TA.MIDDLE_CENTER)


def draw_keepout(ox, oy, s):
    x0, y0, x1, y1 = KEEPOUT
    rect(ox + x0 * s, oy + y0 * s, ox + x1 * s, oy + y1 * s, KO, lw=13)
    for i in range(int((x1 - x0) / 2.5)):
        xa = x0 + i * 2.5
        ln((ox + xa * s, oy + y1 * s), (ox + (xa + 2.5) * s, oy + y0 * s), KO, lw=9)


def draw_rst_note(ox, oy, s):
    txt("RST между контактами 7 и 8", ox + 15.0 * s, oy + MY(21.8) * s,
        1.2 if s > 1.5 else 0.9, SILK)


def draw_jumper(ox, oy, s):
    """Перемычки: IRQ (над веером SPI), земля модуля, земля конденсаторов, RST."""
    def wire(a, b, label, dx=1.0, dy=1.2, pads=True):
        if pads:
            for px, py in (a, b):
                circle(ox + px * s, oy + py * s, 0.5 * s, PAD, lw=25)
        msp.add_lwpolyline([(ox + a[0] * s, oy + a[1] * s),
                            (ox + b[0] * s, oy + b[1] * s)],
                           dxfattribs={"layer": JMP, "lineweight": 25})
        txt(label, ox + (a[0] + dx) * s, oy + (a[1] + DY(dy)) * s,
            1.3 if s > 1.5 else 1.0, JMP)

    wire(DWM_POS[22][:2], JMP_PAD, "перемычка IRQ", dx=-13.0, dy=1.4, pads=False)
    wire(DWM_POS[21][:2], JMP_GND, "перемычка GND", dx=-2.0, dy=-2.2, pads=False)
    wire(JMP_GND3[0], JMP_GND3[1], "перемычка GND", dx=-9.0, dy=1.6, pads=False)
    wire(JMP_RST[0], JMP_RST[1], "перемычка RST", dx=1.0, dy=-1.6, pads=False)


def draw_board_dims(ox, oy, s):
    y = oy - 9
    ln((ox, oy), (ox, y + 2), "РАЗМЕРЫ", lw=13)
    ln((ox + BW * s, oy), (ox + BW * s, y + 2), "РАЗМЕРЫ", lw=13)
    ln((ox, y), (ox + BW * s, y), "РАЗМЕРЫ")
    txt("80", ox + BW * s / 2, y + 1.6, 2.2, align=TA.MIDDLE_CENTER)
    x = ox - 9
    ln((ox, oy), (x + 2, oy), "РАЗМЕРЫ", lw=13)
    ln((ox, oy + BH * s), (x + 2, oy + BH * s), "РАЗМЕРЫ", lw=13)
    ln((x, oy), (x, oy + BH * s), "РАЗМЕРЫ")
    txt("55", x - 1.6, oy + BH * s / 2, 2.2, align=TA.MIDDLE_CENTER, rot=90)


def board_sheet_frame(sheet_y, sheet_no, line2, scale):
    x0, y0, w, h = 0.0, sheet_y, 420.0, 297.0
    m = 5.0
    for a, b in (((x0, y0), (x0 + w, y0)), ((x0 + w, y0), (x0 + w, y0 + h)),
                 ((x0 + w, y0 + h), (x0, y0 + h)), ((x0, y0 + h), (x0, y0))):
        ln(a, b, CFRAME, lw=50)
    rect(x0 + m, y0 + m, x0 + w - m, y0 + h - m, CFRAME)
    title_block(x0 + w - m - 185, y0 + m, "Плата узла meshrover", line2,
                "ESP32-S3-DevKitC-1 + DWM3000", sheet_no, 3, "A3", scale)


def draw_bom(x0, top):
    txt("ПЕРЕЧЕНЬ ЭЛЕМЕНТОВ", x0, top + 4.0, 2.4, bold=True)
    rows = [("Поз.", "Наименование", "Кол.")] + [tuple(r) for r in SPEC["bom"]]
    rh = 4.6
    ln((x0, top - 1.0), (x0 + 200, top - 1.0), CFRAME, lw=25)
    for i, r in enumerate(rows):
        yy = top - 4.6 - i * rh
        bold = i == 0
        txt(r[0], x0 + 1.5, yy, 1.5, bold=bold)
        txt(r[1], x0 + 22, yy, 1.5, bold=bold)
        txt(str(r[2]), x0 + 186, yy, 1.5, bold=bold, align=TA.MIDDLE_CENTER)
        if i == 0:
            ln((x0, yy - 1.2), (x0 + 200, yy - 1.2), CFRAME, lw=20)
        else:
            ln((x0, yy - 1.2), (x0 + 200, yy - 1.2), "ТАБЛИЦЫ", lw=9)
    return top - 4.6 - len(rows) * rh


def draw_sheet2_tables(x0, top):
    txt("ШИРИНА МЕДИ И ЗАЗОРЫ", x0, top, 2.2, bold=True)
    rows = [("5 В", "0,8 А", "1,0 мм"), ("3,3 В", "0,2 А", "0,5 мм"),
            ("SPI / UART", "20 мА", "0,3 мм"), ("зазор", "—", "0,4 мм")]
    yy = top - 6.0
    for a, b, c in rows:
        txt(a, x0, yy, 1.6)
        txt(b, x0 + 40, yy, 1.6)
        txt(c, x0 + 78, yy, 1.6)
        yy -= 4.4
    yy -= 2.0
    txt("ОСОБЕННОСТИ ОДНОГО СЛОЯ", x0, yy, 2.2, bold=True)
    yy -= 5.6
    for s_ in ("переходных отверстий нет;",
               "пересечение разных цепей запрещено;",
               "единственная перемычка — UWB_IRQ:",
               "провод от контакта 22 DWM3000",
               "на площадку у розетки (виден на листе 2);",
               "земля — полигон с зазором 0,4 мм.",
               "Пайка USB-C не нужна: разъёма нет."):
        txt(s_, x0, yy, 1.5, CNOTE)
        yy -= 3.4
    return yy


def sheet2():
    board_sheet_frame(SHEET2_Y, 2, "Топология печатной платы (1 слой)", "2 : 1")
    ox, oy = BX0, BY0
    emit_copper(ox, oy, BSCALE, hatch=True, lw=0)
    draw_keepout(ox, oy, BSCALE)
    draw_silk(ox, oy, BSCALE)
    draw_rst_note(ox, oy, BSCALE)
    draw_jumper(ox, oy, BSCALE)
    draw_board_dims(ox, oy, BSCALE)

    txt("ТОПОЛОГИЯ. ВИД СВЕРХУ. 80 × 55 мм, 1 слой, 4 отв. M3",
        ox, oy + BH * BSCALE + 12, 2.6, bold=True)
    txt("Красный — 5 В, синий — 3,3 В, зелёный — земля, чёрный — сигналы.",
        ox, oy + BH * BSCALE + 7.5, 1.7, CNOTE)

    draw_bom(205, SHEET2_Y + 278)
    draw_sheet2_tables(205, SHEET2_Y + 172)

    txt("УКАЗАНИЯ ПО МОНТАЖУ", 14, SHEET2_Y + 64, 2.2, bold=True)
    for i, s_ in enumerate((
            "1. Розетка J2 — 2×22, шаг 2,54; контакт 1 — слева, ряд J1 — верхний (к DWM3000).",
            "2. DWM3000: 24 площадки 1,0 × 1,6 мм, шаг 1,4 мм; антенна смотрит вправо.",
            "3. Зона x 53…73, y 0…17 мм — кипер-аут: меди и деталей быть не должно.",
            "4. Четыре перемычки проводом 0,3…0,5 мм: IRQ, RST и две земли — они соединяют",
            "    участки полигона, разрезанные дорожками (см. пунктир на листе).",
            "5. Полигон земли — зазор 0,4 мм; отверстия GND с терморельефом.",
            "6. Норма зазора между цепями — 0,3 мм; проход RST между контактами 7 и 8 — 0,47 мм.",
            "7. J1 — клеммник 5,08 мм: плюс 5 В, минус — земля.",
            "8. Посадки зеркальны: элементы ставятся со стороны пайки (медь снизу);",
            "    для сборки медью вверх переключить MIRROR_Y в генераторе.")):
        txt(s_, 14, SHEET2_Y + 58 - i * 4.2, 1.6, CNOTE)


def sheet3():
    board_sheet_frame(SHEET3_Y, 3, "Слой меди. Рисунок для травления", "1 : 1")
    ox, oy = ART_X, ART_Y
    emit_copper(ox, oy, 1.0, hatch=True, lw=0)
    draw_keepout(ox, oy, 1.0)
    draw_silk(ox, oy, 1.0)
    draw_jumper(ox, oy, 1.0)

    y = oy - 14
    ln((ox, oy), (ox, y + 2), "РАЗМЕРЫ", lw=13)
    ln((ox + BW, oy), (ox + BW, y + 2), "РАЗМЕРЫ", lw=13)
    ln((ox, y), (ox + BW, y), "РАЗМЕРЫ")
    txt("80", ox + BW / 2, y + 1.6, 2.0, align=TA.MIDDLE_CENTER)
    x = ox - 14
    ln((ox, oy), (x + 2, oy), "РАЗМЕРЫ", lw=13)
    ln((ox, oy + BH), (x + 2, oy + BH), "РАЗМЕРЫ", lw=13)
    ln((x, oy), (x, oy + BH), "РАЗМЕРЫ")
    txt("55", x - 1.6, oy + BH / 2, 2.0, align=TA.MIDDLE_CENTER, rot=90)

    txt("МЕДЬ, 1:1 — КОНТУРЫ СООТВЕТСТВУЮТ ГРАНИЦАМ ДОРОЖЕК ПОСЛЕ ТРАВЛЕНИЯ",
        ox - 44, oy + BH + 16, 2.6, bold=True)
    txt("Печать на прозрачной плёнке (ЛУТ/фоторезист) или экспорт в CAM: масштаб 1:1, без правок.",
        ox - 44, oy + BH + 11, 1.8, CNOTE)
    txt("Полигон GND с зазором 0,4 мм; кипер-аут антенны без меди; перемычка IRQ — проводом.",
        ox - 44, oy + BH + 7, 1.8, CNOTE)

    sx, sy = ox - 44, oy - 24
    ln((sx, sy), (sx + 50, sy), "РАЗМЕРЫ", lw=18)
    for i in range(6):
        ln((sx + i * 10, sy), (sx + i * 10, sy + (3 if i % 5 == 0 else 1.8)), "РАЗМЕРЫ", lw=18)
    for i, lab in ((0, "0"), (2, "20"), (5, "50 мм")):
        txt(lab, sx + i * 10, sy - 3.2, 1.7, align=TA.MIDDLE_CENTER)

    txt("ЦВЕТ = ЦЕПЬ", ox + 90, oy + BH + 10, 2.2, bold=True)
    for i, lay in enumerate(("ШИНА_5В", "ШИНА_3В3", "ЗЕМЛЯ", "СИГНАЛЫ")):
        yy = oy + BH - 6 - i * 6
        rect(ox + 90, yy - 1.2, ox + 102, yy + 1.2, lay, lw=13)
        txt({0: "+5 В", 1: "+3,3 В", 2: "GND", 3: "сигналы"}[i], ox + 105, yy - 1.3, 1.8)
# ----------------------------------------------------------------------------
# Производственный DXF под лазер: негатив (участки без меди) + тест-купон
# ----------------------------------------------------------------------------
# Лазер снимает маску/краску там, где медь должна быть стравлена. Значит нужен
# НЕГАТИВ: замкнутые контуры всех участков без меди. Дальше в LightBurn:
#   слой NEGATIVE -> Fill (Line interval 0,1–0,15), CUT_EDGE -> Line,
#   DRILLS -> не жечь (по ним сверлить), TEST — калибровочный купон.
# ----------------------------------------------------------------------------
LASER_OUT = "esp32s3-dwm3000-laser.dxf"
LASER_LAYERS = {
    "CUT_EDGE": (7, 50),      # контур платы
    "NEGATIVE": (5, 18),      # участки без меди — замкнутые контуры, заливать
    "HATCH":    (8, 13),      # та же зона, но готовой штриховкой (если CAM без заливки)
    "DRILLS":   (1, 25),      # центры/диаметры отверстий
    "MARKS":    (3, 25),      # база для совмещения
    "TEST":     (2, 25),      # тест-купон
    "KERF":     (6, 25),      # одиночные линии для замера ширины пятна
}


def laser_doc():
    d = ezdxf.new("R2010")
    d.units = ezdxf.units.MM
    d.header["$INSUNITS"] = 4
    d.header["$MEASUREMENT"] = 1
    for name, (color, lw) in LASER_LAYERS.items():
        if name not in d.layers:
            d.layers.add(name=name, color=color, lineweight=lw)
    if "RU" not in d.styles:
        d.styles.new("RU", dxfattribs={"font": "arial.ttf"})
    return d


def laser_negative(shrink=0.15, dilate=0.0):
    """Участки без меди: плата минус вся медь, минус сверловка, минус запас по краю."""
    from shapely.geometry import Point
    from shapely.ops import unary_union
    board = _rounded_board().buffer(-shrink, join_style=1)
    copper = unary_union([g for g in COPPER.values()])
    neg = board.difference(copper)
    for net, x, y, w, h, drill in PADS:
        if drill > 0:
            neg = neg.difference(Point(x, y).buffer(drill / 2.0 + 0.25, quad_segs=24))
    if dilate > 0:
        # расширяем зону выжигания в сторону меди: маска снимается с запасом,
        # иначе у края дорожки остаётся плёнка и медь не стравливается
        neg = neg.buffer(dilate, join_style=1)
    # заливка на краю может дать тонкие лоскуты — убрать
    return unary_union([p for p in _polys(neg) if p.area > 0.05])


def laser_hatch(geom, step=0.15):
    """Штриховка строк: каждый отрезок — отдельная полилиния (без перехода по меди)."""
    from shapely.geometry import LineString
    if geom.is_empty:
        return []
    x0, y0, x1, y1 = geom.bounds
    segs, y = [], y0 + step / 2
    while y < y1:
        cut = geom.intersection(LineString([(x0 - 1, y), (x1 + 1, y)]))
        parts = cut.geoms if hasattr(cut, "geoms") else ([cut] if not cut.is_empty else [])
        for p in parts:
            if p.is_empty or p.geom_type != "LineString":
                continue
            a, b = p.coords[0], p.coords[-1]
            segs.append([a, b])
        y += step
    return segs


def laser_add_poly(doc, geom, layer, lw=18):
    msp = doc.modelspace()
    for poly in _polys(geom):
        ext = list(poly.exterior.coords)[:-1]
        if len(ext) < 3:
            continue
        msp.add_lwpolyline(ext, close=True, dxfattribs={"layer": layer, "lineweight": lw})
        for ring in poly.interiors:
            pts = list(ring.coords)[:-1]
            if len(pts) >= 3:
                msp.add_lwpolyline(pts, close=True,
                                   dxfattribs={"layer": layer, "lineweight": lw})


def laser_test_coupon(doc, ox, oy):
    """Купон: щели 0,3/0,4/0,5/0,6 мм — проверить, что лазер открывает их чисто."""
    msp = doc.modelspace()

    def text(s_, x, y, h=1.6, lay="TEST"):
        e = msp.add_text(s_, height=h, dxfattribs={"layer": lay, "style": "RU"})
        e.set_placement((x, y), align=TA.LEFT)

    msp.add_lwpolyline([(ox, oy), (ox + 40, oy), (ox + 40, oy + 30), (ox, oy + 30)],
                       close=True, dxfattribs={"layer": "TEST", "lineweight": 25})
    text("ТЕСТ-КУПОН: подбор мощности лазера", ox + 1.5, oy + 27.6, 1.8)

    # одиночные линии: замерить ширину пятна (kerf) — это главное число
    for i in range(5):
        y = oy + 24.4 - i * 0.0
        x = ox + 2.0 + i * 7.2
        msp.add_line((x, oy + 23.6), (x, oy + 27.0),
                     dxfattribs={"layer": "KERF", "lineweight": 9})
    text("одиночные линии (слой KERF): сжечь, замерить ширину — это kerf", ox + 1.5, oy + 22.4, 1.2)

    for i, g in enumerate((0.3, 0.4, 0.5, 0.6)):
        y = oy + 15.4 - i * 4.0
        # щель — то, что выжигаем
        msp.add_lwpolyline([(ox + 3, y), (ox + 3 + 14, y), (ox + 3 + 14, y + g),
                            (ox + 3, y + g)], close=True,
                           dxfattribs={"layer": "NEGATIVE", "lineweight": 15})
        text(f"щель {g:.1f} мм", ox + 19.5, y - 0.2, 1.5)
        # медные полярности по краям щели (для ориентира, не жечь)
        for yy in (y - 1.6, y + g + 0.3):
            msp.add_line((ox + 2, yy), (ox + 19, yy),
                         dxfattribs={"layer": "TEST", "lineweight": 9})

    text("Щель чистая + соседняя медь не подпалена. Затем этой мощностью — плату.", ox + 1.5, oy + 3.2, 1.2)
    text("Line interval в заливке = kerf / 2 (замерить по линиям выше).", ox + 1.5, oy + 1.2, 1.2)


def export_laser(path, step=0.15, with_hatch=True):
    d = laser_doc()
    msp = d.modelspace()

    # контур платы
    r = 3.0
    msp.add_lwpolyline([(r, 0), (BW - r, 0), (BW, r), (BW, BH - r), (BW - r, BH),
                        (r, BH), (0, BH - r), (0, r)], close=True,
                       dxfattribs={"layer": "CUT_EDGE", "lineweight": 50})

    neg = laser_negative()
    laser_add_poly(d, neg, "NEGATIVE", lw=18)

    if with_hatch:
        for pts in laser_hatch(neg, step):
            if len(pts) >= 2:
                msp.add_lwpolyline(pts, dxfattribs={"layer": "HATCH", "lineweight": 13})

    # сверловка: центр + диаметр
    for net, x, y, w, h, drill in PADS:
        if drill > 0:
            msp.add_circle((x, y), drill / 2.0, dxfattribs={"layer": "DRILLS", "lineweight": 25})
            msp.add_line((x - drill / 2 - 0.6, y), (x + drill / 2 + 0.6, y),
                         dxfattribs={"layer": "DRILLS", "lineweight": 9})
            msp.add_line((x, y - drill / 2 - 0.6), (x, y + drill / 2 + 0.6),
                         dxfattribs={"layer": "DRILLS", "lineweight": 9})

    # база для совмещения: уголки по углам платы
    for cx, cy in ((-5, -5), (BW + 5, -5), (BW + 5, BH + 5), (-5, BH + 5)):
        msp.add_line((cx - 4, cy), (cx + 4, cy), dxfattribs={"layer": "MARKS", "lineweight": 13})
        msp.add_line((cx, cy - 4), (cx, cy + 4), dxfattribs={"layer": "MARKS", "lineweight": 13})

    laser_test_coupon(d, 86.0, 4.0)

    from ezdxf import appsettings
    appsettings.update_extents(d)
    d.saveas(path)
    n_poly = len(_polys(neg))
    n_hatch = len(msp.query("LWPOLYLINE[layer=='HATCH']"))
    print(f"лазерный DXF: {path}")
    print(f"  негатив: {n_poly} полигонов, {neg.area / 100:.2f} см² под выжигание")
    if with_hatch:
        print(f"  штриховка: {n_hatch} строк, шаг {step} мм")


def export_laser_raster(path, dpi=1200):
    """Растр 1:1 (80 x 55 мм): чёрное — выжигать. Для лазеров с растровым режимом."""
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import PathPatch
    from matplotlib.path import Path as MPath
    from shapely.geometry.polygon import orient

    fig = plt.figure(figsize=(BW / 25.4, BH / 25.4))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()
    ax.set_facecolor("white")
    for poly in _polys(laser_negative()):
        poly = orient(poly, 1.0)
        verts, codes = [], []
        for ring in [poly.exterior] + list(poly.interiors):
            pts = list(ring.coords)[:-1]
            if len(pts) < 3:
                continue
            verts.extend(pts + [pts[0]])
            codes.append(MPath.MOVETO)
            codes.extend([MPath.LINETO] * (len(pts) - 1))
            codes.append(MPath.CLOSEPOLY)
        ax.add_patch(PathPatch(MPath(verts, codes), facecolor="black",
                               edgecolor="none", antialiased=False))
    ax.set_xlim(0, BW)
    ax.set_ylim(0, BH)
    fig.savefig(path, dpi=dpi, facecolor="white")
    plt.close(fig)
    print(f"растр 1:1: {path} ({int(BW / 25.4 * dpi)} x {int(BH / 25.4 * dpi)} px)")


def render_laser_png(path, bbox=(-8, -8, 132, 62), dpi=200):
    """Превью производственного файла: негатив залит, медь белая."""
    import matplotlib
    matplotlib.use("Agg")
    matplotlib.rcParams["font.family"] = "Arial"
    import matplotlib.pyplot as plt
    from matplotlib.patches import PathPatch, Rectangle
    from matplotlib.path import Path as MPath
    from shapely.geometry.polygon import orient

    w = (bbox[2] - bbox[0]) / 25.4
    h = (bbox[3] - bbox[1]) / 25.4
    fig = plt.figure(figsize=(w, h))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()
    ax.set_facecolor("white")

    neg = laser_negative()
    for poly in _polys(neg):
        poly = orient(poly, 1.0)
        verts, codes = [], []
        for ring in [poly.exterior] + list(poly.interiors):
            pts = list(ring.coords)[:-1]
            if len(pts) < 3:
                continue
            verts.extend(pts + [pts[0]])
            codes.append(MPath.MOVETO)
            codes.extend([MPath.LINETO] * (len(pts) - 1))
            codes.append(MPath.CLOSEPOLY)
        ax.add_patch(PathPatch(MPath(verts, codes), facecolor=(0.15, 0.15, 0.15),
                               edgecolor="none", antialiased=False, zorder=1))

    for net, x, y, ww, hh, drill in PADS:
        if drill > 0:
            ax.add_patch(plt.Circle((x, y), drill / 2, facecolor="white",
                                    edgecolor="black", linewidth=0.4, zorder=3))

    for i, g in enumerate((0.3, 0.4, 0.5, 0.6)):
        y = 4 + 16.5 - i * 4.0
        ax.add_patch(Rectangle((89, y), 14, g, facecolor=(0.15, 0.15, 0.15),
                               edgecolor="none", zorder=2))
    ax.add_patch(Rectangle((86, 4), 40, 26, fill=False, edgecolor="#0066cc",
                           linewidth=1.0, zorder=3))
    ax.text(87, 31.5, "тест-купон", color="#0066cc", fontsize=7)
    ax.text(2, 57.5, "НЕГАТИВ ПОД ЛАЗЕР: тёмное — выжечь (без меди)", fontsize=7)
    ax.set_aspect("equal")
    ax.set_xlim(bbox[0], bbox[2])
    ax.set_ylim(bbox[1], bbox[3])
    fig.savefig(path, dpi=dpi, facecolor="white")
    plt.close(fig)
    print("PNG:", path)


def main():
    sheet1()

    from ezdxf.math import Matrix44
    shift = Matrix44.translate(0, SHEET1_Y, 0)
    for entity in list(msp):
        entity.transform(shift)

    global COPPER
    COPPER = build_copper()

    sheet2()
    sheet3()

    from ezdxf import appsettings
    appsettings.update_extents(doc)
    doc.header["$LIMMIN"] = (0.0, 0.0)
    doc.header["$LIMMAX"] = (420.0, SHEET1_Y + 297.0)
    doc.header["$INSBASE"] = (0.0, 0.0, 0.0)

    out = os.path.join(BASE, OUT_DXF)
    doc.saveas(out)
    print("DXF:", out)

    if "--png" in sys.argv:
        png = os.path.join(BASE, OUT_PNG)
        render_png(png, bbox=(0, 0, 420, SHEET1_Y + 297.0), dpi=110)
        print("PNG:", png)

    if "--laser" in sys.argv:
        lap = os.path.join(BASE, LASER_OUT)
        export_laser(lap, step=0.15)
        render_laser_png(os.path.join(BASE, "esp32s3-dwm3000-laser-preview.png"))
        export_laser_raster(os.path.join(BASE, "esp32s3-dwm3000-laser-mask.png"))

    if "--check" in sys.argv:
        bad = check_design()
        print("итог:", "чисто" if not bad else "есть нарушения")

    if "--report" in sys.argv:
        append_report(os.path.join(BASE, "esp32s3-dwm3000-board-report.md"))


if __name__ == "__main__":
    main()
