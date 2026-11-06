#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Генератор схемы «Плата питания 12 В -> 3,3 В с модулем ESP32».

Выходные файлы (в текущей папке):
    esp32-12v-3v3-schematic.dxf           - чертёж (DXF R2010, единицы - мм)
    esp32-12v-3v3-schematic-preview.png   - растровое превью для быстрой проверки

Слои DXF:
    РАМКА            - рамка листа и основная надпись
    ШИНА_12В         - цепь +12 В (красный)
    ШИНА_3В3         - цепь +3,3 В (синий)
    ЗЕМЛЯ            - цепь GND (зелёный)
    СИГНАЛЫ          - остальные связи (чёрный)
    СИМВОЛЫ          - условные графические обозначения
    ТЕКСТ            - позиционные обозначения и номиналы
    ПОЯСНЕНИЯ        - текстовые пояснения для новичков
    ПЛАТА_КОНТУР     - контур печатной платы
    ПЛАТА_ДЕТАЛИ     - посадочные места
    РАЗМЕРЫ          - размерные линии
    ТАБЛИЦЫ          - рамки таблиц

Запуск:  python3 esp32-12v-3v3-schematic.py
"""

import math
import os
import sys

import ezdxf
from ezdxf.enums import TextEntityAlignment as TA

OUT_DXF = "esp32-12v-3v3-schematic.dxf"
OUT_PNG = "esp32-12v-3v3-schematic-preview.png"

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

# тип линии для осевых/скрытых контуров
if "DASHED" not in doc.linetypes:
    doc.linetypes.add("DASHED", pattern=[0.6, 0.3, -0.3], description="Dashed __ __ __")

# Текстовые стили с поддержкой кириллицы (TrueType)
for name, font in (("RU", "arial.ttf"), ("RU_B", "arialbd.ttf")):
    if name not in doc.styles:
        doc.styles.new(name, dxfattribs={"font": font})


# ----------------------------------------------------------------------------
# Слои
# ----------------------------------------------------------------------------
LAYERS = {
    # имя:              (цвет, вес линии, тип линии)
    "РАМКА":            (7, 50, "CONTINUOUS"),
    "ШИНА_12В":         (1, 40, "CONTINUOUS"),
    "ШИНА_3В3":         (5, 40, "CONTINUOUS"),
    "ЗЕМЛЯ":            (3, 40, "CONTINUOUS"),
    "СИГНАЛЫ":          (7, 30, "CONTINUOUS"),
    "СИМВОЛЫ":          (7, 35, "CONTINUOUS"),
    "ТЕКСТ":            (7, 18, "CONTINUOUS"),
    "ПОЯСНЕНИЯ":        (6, 18, "CONTINUOUS"),
    "ПЛАТА_КОНТУР":     (7, 60, "CONTINUOUS"),
    "ПЛАТА_ДЕТАЛИ":     (4, 25, "CONTINUOUS"),
    "ДОРОЖКИ":          (1, 100, "CONTINUOUS"),
    "ПЛАТА_ПАДЫ":       (7, 30, "CONTINUOUS"),
    "РАЗМЕРЫ":          (3, 18, "CONTINUOUS"),
    "ТАБЛИЦЫ":          (7, 18, "CONTINUOUS"),
    "ОСЕВЫЕ":           (8, 13, "DASHED"),
}
for name, (color, lw, ltype) in LAYERS.items():
    if name not in doc.layers:
        doc.layers.add(name=name, color=color, lineweight=lw, linetype=ltype)

# Цвета цепей (для наглядности)
C12 = "ШИНА_12В"
C3V3 = "ШИНА_3В3"
CGND = "ЗЕМЛЯ"
CSIG = "СИГНАЛЫ"
CSYM = "СИМВОЛЫ"
CTX = "ТЕКСТ"
CNOTE = "ПОЯСНЕНИЯ"
CFRAME = "РАМКА"


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
    msp.add_lwpolyline(
        [(x0, y0), (x1, y0), (x1, y1), (x0, y1)],
        close=True, dxfattribs=_attribs(layer, lw),
    )


def circle(x, y, r, layer, lw=None):
    msp.add_circle((x, y), r, dxfattribs=_attribs(layer, lw))


def arc(x, y, r, a0, a1, layer, lw=None):
    msp.add_arc((x, y), r, a0, a1, dxfattribs=_attribs(layer, lw))


def dot(x, y, r=0.5, layer=CSYM):
    msp.add_circle((x, y), r, dxfattribs={"layer": layer, "lineweight": 80})


def txt(s, x, y, h=2.5, layer=CTX, align=TA.LEFT, rot=0.0, bold=False):
    e = msp.add_text(
        s, height=h,
        dxfattribs={"layer": layer, "rotation": rot, "style": "RU_B" if bold else "RU"},
    )
    e.set_placement((x, y), align=align)
    return e


def txt_box(s, x, y, h=2.2, w=None, layer=CTX, anchor=TA.MIDDLE_CENTER):
    """Текст с центровкой по точке."""
    return txt(s, x, y, h=h, layer=layer, align=anchor)


# ----------------------------------------------------------------------------
# Условные графические обозначения
# ----------------------------------------------------------------------------
def sym_res_v(x, yc, h=10.0, w=4.0):
    """Резистор вертикальный, центр (x, yc)."""
    rect(x - w / 2, yc - h / 2, x + w / 2, yc + h / 2, CSYM)


def sym_res_h(x, yc, w=10.0, h=4.0):
    rect(x - w / 2, yc - h / 2, x + w / 2, yc + h / 2, CSYM)


def sym_cap_v(x, yc, polar=False, gap=2.6, w=9.0):
    """Конденсатор вертикальный, центр (x, yc). polar - электролитический."""
    ln((x - w / 2, yc + gap / 2), (x + w / 2, yc + gap / 2), CSYM)
    if polar:
        arc(x, yc - gap / 2 - 2.6, w / 2, 25, 155, CSYM)
        txt("+", x + w / 2 + 1.2, yc + gap / 2 - 1.0, h=2.0, layer=CSYM)
    else:
        ln((x - w / 2, yc - gap / 2), (x + w / 2, yc - gap / 2), CSYM)


def sym_diode_h(xc, y, w=8.0, h=7.0, kind="diode"):
    """Диод горизонтальный, катод справа. Центр (xc, y)."""
    pl([(xc - w / 2, y + h / 2), (xc - w / 2, y - h / 2), (xc + w / 2, y)],
       CSYM, closed=True)
    ln((xc + w / 2, y - h / 2), (xc + w / 2, y + h / 2), CSYM)
    if kind == "zener":
        ln((xc + w / 2, y + h / 2), (xc + w / 2 + 1.6, y + h / 2 + 1.2), CSYM)
        ln((xc + w / 2, y - h / 2), (xc + w / 2 - 1.6, y - h / 2 - 1.2), CSYM)


def sym_led_v(x, yc, w=8.0, h=7.0):
    """Светодиод вертикальный, катод внизу. Центр (x, yc)."""
    pl([(x - w / 2, yc + h / 2), (x + w / 2, yc + h / 2), (x, yc - h / 2)],
       CSYM, closed=True)
    ln((x - w / 2, yc - h / 2), (x + w / 2, yc - h / 2), CSYM)
    # два "луча" излучения (слева, чтобы не мешать подписи справа)
    for dx, dy in ((0.0, 0.0), (1.6, -3.2)):
        ax, ay = x - w / 2 - 0.8 + dx, yc + 0.4 + dy
        ln((ax, ay), (ax - 3.0, ay + 2.4), CSYM, lw=13)
        ln((ax - 3.0, ay + 2.4), (ax - 1.2, ay + 2.2), CSYM, lw=13)
        ln((ax - 3.0, ay + 2.4), (ax - 2.8, ay + 0.6), CSYM, lw=13)


def sym_fuse_h(xc, y, w=14.0, h=6.0):
    rect(xc - w / 2, y - h / 2, xc + w / 2, y + h / 2, CSYM)
    ln((xc - w / 2, y), (xc + w / 2, y), CSYM)


def sym_gnd(x, y, rot=0):
    """Земля. (x, y) - точка подключения; rot=0 вниз, 90 вправо."""
    s = math.sin(math.radians(rot))
    c = math.cos(math.radians(rot))
    def P(dx, dy):
        return (x + dx * c - dy * s, y + dx * s + dy * c)
    ln(P(0, 0), P(0, -3.0), CGND, lw=40)
    for dy, half in ((-3.0, 4.5), (-4.6, 3.0), (-6.2, 1.5)):
        ln(P(-half, dy), P(half, dy), CGND, lw=40)


def sym_flag(x, y, label, layer, up=True, h=2.2, dy=4.0):
    """Флажок питания: стрелка вверх (up) или вниз от точки подключения."""
    d = 1 if up else -1
    ln((x, y), (x, y + d * 3.4), layer, lw=40)
    pl([(x - 1.6, y + d * 2.2), (x, y + d * 4.2), (x + 1.6, y + d * 2.2)],
       layer, closed=True, lw=40)
    if up:
        txt(label, x, y + d * 5.6, h=h, layer=layer, align=TA.MIDDLE_CENTER)
    else:
        txt(label, x, y + d * 5.6, h=h, layer=layer, align=TA.MIDDLE_CENTER)


def pin(x1, y1, x2, y2, layer=CSYM, num=None, num_off=(0, 1.5), h=1.8):
    ln((x1, y1), (x2, y2), layer, lw=35)
    if num is not None:
        txt(str(num), (x1 + x2) / 2 + num_off[0], (y1 + y2) / 2 + num_off[1],
            h=h, layer=CTX, align=TA.MIDDLE_CENTER)


def arrow_h(x1, x2, y, layer="РАЗМЕРЫ", tip=2.0):
    ln((x1, y), (x2, y), layer)
    for x, s in ((x1, 1), (x2, -1)):
        pl([(x, y), (x + s * tip * 1.8, y + tip * 0.55),
            (x + s * tip * 1.8, y - tip * 0.55)], layer, closed=True)


def arrow_v(y1, y2, x, layer="РАЗМЕРЫ", tip=2.0):
    ln((x, y1), (x, y2), layer)
    for y, s in ((y1, 1), (y2, -1)):
        pl([(x, y), (x + tip * 0.55, y + s * tip * 1.8),
            (x - tip * 0.55, y + s * tip * 1.8)], layer, closed=True)


def leader(x1, y1, x2, y2, layer=CNOTE, h=2.0):
    """Выноска-указатель."""
    ln((x1, y1), (x2, y2), layer, lw=13)
    ln((x2, y2), (x2 + (3.0 if x2 > x1 else -3.0), y2), layer, lw=13)


# ----------------------------------------------------------------------------
# Рамка и основная надпись
# ----------------------------------------------------------------------------
def title_block(x0, y0, line1, line2, subtitle, sheet_no, total_sheets, fmt, scale):
    """Основная надпись 185 x 55 мм."""
    w, h = 185.0, 55.0
    rect(x0, y0, x0 + w, y0 + h, CFRAME, lw=50)
    ln((x0, y0 + 30), (x0 + w, y0 + 30), CFRAME)
    ln((x0 + 104, y0), (x0 + 104, y0 + h), CFRAME)
    # правая часть, верх (30...55)
    ln((x0 + 140, y0 + 30), (x0 + 140, y0 + h), CFRAME)
    ln((x0 + 160, y0 + 30), (x0 + 160, y0 + h), CFRAME)
    # правая часть, низ (10...30)
    ln((x0 + 140, y0), (x0 + 140, y0 + 30), CFRAME)
    ln((x0 + 160, y0), (x0 + 160, y0 + 30), CFRAME)
    # низ левой части: 3 строки
    for dy in (16.0, 23.0):
        ln((x0, y0 + dy), (x0 + 104, y0 + dy), CFRAME)
    ln((x0 + 50, y0), (x0 + 50, y0 + 30), CFRAME)

    txt(line1, x0 + 3, y0 + 45.5, 4.2, bold=True)
    if line2:
        txt(line2, x0 + 3, y0 + 37.5, 2.4)
    if subtitle:
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


def sheet_frame(x0, y0, w, h):
    """Рамка листа A3 + основная надпись."""
    m = 5.0
    for a, b in (((x0, y0), (x0 + w, y0)), ((x0 + w, y0), (x0 + w, y0 + h)),
                 ((x0 + w, y0 + h), (x0, y0 + h)), ((x0, y0 + h), (x0, y0))):
        ln(a, b, CFRAME, lw=50)
    rect(x0 + m, y0 + m, x0 + w - m, y0 + h - m, CFRAME)
    title_block(x0 + w - m - 185, y0 + m,
                "Плата питания 12 В → 3,3 В", "Схема электрическая принципиальная",
                "с модулем ESP32", 1, 2, "A3", "1 : 1")


# ----------------------------------------------------------------------------
# Лист 1. Схема
# ----------------------------------------------------------------------------
SHEET1 = (0, 0, 420, 297)   # A3

RAIL = 248.0
GND_CONN = 222.0            # общая точка подключения "земли"
SEC_TITLE_Y = 276.0

# ----------------------------------------------------------------------------
# Элементы входной цепи
# ----------------------------------------------------------------------------
RAIL = 248.0
GND_CONN = 222.0
D_Y = 257.0          # позиционное обозначение
V_Y = 252.8          # номинал
SEC_TITLE_Y = 276.0


def shunt_label(x, ref, val):
    txt(ref, x, D_Y, 2.2, bold=True, align=TA.MIDDLE_CENTER)
    txt(val, x, V_Y, 1.85, align=TA.MIDDLE_CENTER)


def draw_input_chain():
    # --- J1: клеммник 12 В ---
    rect(14, 232, 26, 252, CSYM)
    ln((26, 248), (32, 248), C12)
    ln((26, 240), (32, 240), CGND)
    txt("1", 29.2, 249.6, 1.8, align=TA.MIDDLE_CENTER)
    txt("2", 29.2, 241.6, 1.8, align=TA.MIDDLE_CENTER)
    txt("J1", 13, 256.5, 2.6, bold=True)
    txt("клеммник", 13, 228.5, 1.9)
    txt("12 В вход", 13, 225.2, 1.9)
    ln((32, 240), (34.5, 240), CGND)
    sym_gnd(34.5, 240)

    # --- F1: предохранитель ---
    ln((32, 248), (40, 248), C12)
    sym_fuse_h(47, RAIL)
    ln((54, 248), (62, 248), C12)
    txt("F1", 47, D_Y, 2.2, bold=True, align=TA.MIDDLE_CENTER)
    txt("1 А", 47, 241.0, 1.9, align=TA.MIDDLE_CENTER)

    # --- D1: защита от переполюсовки ---
    sym_diode_h(66, RAIL, kind="schottky")
    ln((70, 248), (78, 248), C12)
    txt("D1", 66, D_Y, 2.2, bold=True, align=TA.MIDDLE_CENTER)
    txt("SS34", 66, 241.0, 1.9, align=TA.MIDDLE_CENTER)
    txt("защита от", 66, 236.4, 1.7, align=TA.MIDDLE_CENTER)
    txt("переполюсовки", 66, 233.6, 1.7, align=TA.MIDDLE_CENTER)

    dot(78, 248)
    txt("+12 В", 80, 245.6, 2.0, layer=C12, align=TA.MIDDLE_CENTER)


def draw_filter_and_indicator_12():
    # --- C1: электролит 220 мкФ ---
    ln((78, 248), (92, 248), C12)
    dot(92, 248)
    ln((92, 248), (92, 242.6), C12)
    sym_cap_v(92, 241.3, polar=True)
    ln((92, 240.0), (92, GND_CONN), CGND)
    sym_gnd(92, GND_CONN)
    shunt_label(92, "C1", "220 мкФ / 25 В")

    # --- C2: керамика 10 мкФ ---
    ln((92, 248), (116, 248), C12)
    dot(116, 248)
    ln((116, 248), (116, 242.6), C12)
    sym_cap_v(116, 241.3)
    ln((116, 240.0), (116, GND_CONN), CGND)
    sym_gnd(116, GND_CONN)
    shunt_label(116, "C2", "10 мкФ")

    # --- R1 + LED1: индикация 12 В ---
    ln((116, 248), (140, 248), C12)
    dot(140, 248)
    sym_res_v(140, 242.0)
    ln((140, 237.0), (140, 233.5), C12)
    sym_led_v(140, 230.0)
    ln((140, 226.5), (140, GND_CONN), CGND)
    sym_gnd(140, GND_CONN)
    shunt_label(140, "R1", "1,2 кОм")
    txt("LED1", 146.5, 231.0, 1.9, bold=True)
    txt("зелёный", 146.5, 227.6, 1.7)


def draw_regulator():
    # --- U1: AMS1117-3.3 ---
    ln((140, 248), (156, 248), C12)
    rect(162, 232, 196, 256, CSYM)
    ln((156, 248), (162, 248), C12)
    txt("3", 159, 249.6, 1.8, align=TA.MIDDLE_CENTER)
    ln((196, 248), (200, 248), C3V3)
    txt("2", 198, 249.6, 1.8, align=TA.MIDDLE_CENTER)
    ln((179, 232), (179, 226), CGND)
    txt("1", 181.4, 229.6, 1.8)
    ln((179, 226), (179, GND_CONN), CGND)
    sym_gnd(179, GND_CONN)

    txt("U1", 162, 259.5, 2.6, bold=True)
    txt("AMS1117-3.3", 179, 250.0, 2.3, align=TA.MIDDLE_CENTER, bold=True)
    txt("стабилизатор 3,3 В", 179, 245.4, 1.85, align=TA.MIDDLE_CENTER)
    txt("(линейный, LDO)", 179, 242.0, 1.85, align=TA.MIDDLE_CENTER)

    ln((200, 248), (285, 248), C3V3)
    txt("+3,3 В", 204, 245.6, 2.0, layer=C3V3, align=TA.MIDDLE_CENTER)


def draw_filter_and_indicator_33():
    # --- C3: 22 мкФ ---
    dot(212, 248)
    ln((212, 248), (212, 242.6), C3V3)
    sym_cap_v(212, 241.3)
    ln((212, 240.0), (212, GND_CONN), CGND)
    sym_gnd(212, GND_CONN)
    shunt_label(212, "C3", "22 мкФ")

    # --- C4: 100 нФ ---
    dot(236, 248)
    ln((236, 248), (236, 242.6), C3V3)
    sym_cap_v(236, 241.3)
    ln((236, 240.0), (236, GND_CONN), CGND)
    sym_gnd(236, GND_CONN)
    shunt_label(236, "C4", "100 нФ")

    # --- R2 + LED2: индикация 3,3 В ---
    dot(260, 248)
    sym_res_v(260, 242.0)
    ln((260, 237.0), (260, 233.5), C3V3)
    sym_led_v(260, 230.0)
    ln((260, 226.5), (260, GND_CONN), CGND)
    sym_gnd(260, GND_CONN)
    shunt_label(260, "R2", "330 Ом")
    txt("LED2", 266.5, 231.0, 1.9, bold=True)
    txt("зелёный", 266.5, 227.6, 1.7)


def draw_esp32_block():
    """J2 - розетка под плату ESP32 DevKit + гребёнки J3, J4."""
    x0, y0, x1, y1 = 285, 136, 368, 258
    rect(x0, y0, x1, y1, CSYM)
    txt("J2", x0, 262.0, 3.0, bold=True)
    txt("РОЗЕТКА 2x19", x0 + 4, 254.0, 2.6, bold=True)
    txt("сюда вставляется плата", x0 + 4, 170.0, 1.85)
    txt("ESP32 DevKit V1", x0 + 4, 166.2, 1.85, bold=True)
    txt("(38 выводов, шаг 2,54 мм)", x0 + 4, 162.4, 1.7)
    txt("остальные выводы", x0 + 4, 192.5, 1.7)
    txt("не подключаются", x0 + 4, 189.3, 1.7)

    # левые выводы питания
    ln((278, 248), (285, 248), C3V3)
    dot(285, 248)
    txt("3V3", 289, 249.4, 1.9)
    ln((285, 236), (278, 236), CGND)
    ln((278, 236), (278, GND_CONN), CGND)
    sym_gnd(278, GND_CONN)
    txt("GND", 289, 237.4, 1.9)

    # правые выводы GPIO
    pins = [("36", 240), ("39", 234), ("34", 228), ("35", 222), ("32", 216), ("33", 210),
            ("25", 186), ("26", 180), ("27", 174), ("14", 168), ("12", 162), ("13", 156)]
    for name, y in pins:
        ln((x1, y), (386, y), CSIG)
        txt(name, 382.0, y + 1.1, 1.9, align=TA.RIGHT)

    # --- J3 ---
    rect(386, 206, 390, 256, CSYM)
    txt("J3", 383.5, 259.0, 2.6, bold=True, align=TA.RIGHT)
    for i, y in enumerate([252, 246, 240, 234, 228, 222, 216, 210], start=1):
        txt(str(i), 388, y + 0.8, 1.5, align=TA.MIDDLE_CENTER)
    ln((390, 252), (394, 252), C3V3)
    sym_flag(394, 252, "3,3 В", C3V3, h=1.9)
    ln((390, 246), (396, 246), CGND)
    sym_gnd(396, 246, rot=90)
    txt("3V3", 383.5, 253.0, 1.7, align=TA.RIGHT)
    txt("GND", 383.5, 247.0, 1.7, align=TA.RIGHT)

    # --- J4 ---
    rect(386, 140, 390, 190, CSYM)
    txt("J4", 383.5, 193.0, 2.6, bold=True, align=TA.RIGHT)
    for i, y in enumerate([186, 180, 174, 168, 162, 156, 150, 144], start=1):
        txt(str(i), 388, y + 0.8, 1.5, align=TA.MIDDLE_CENTER)
    ln((390, 150), (396, 150), CGND)
    sym_gnd(396, 150, rot=90)
    ln((390, 144), (394, 144), C3V3)
    sym_flag(394, 144, "3,3 В", C3V3, up=False, h=1.9)
    txt("GND", 383.5, 151.0, 1.7, align=TA.RIGHT)
    txt("3V3", 383.5, 145.0, 1.7, align=TA.RIGHT)


def draw_section_titles():
    items = [
        ("1.  ВХОД И ЗАЩИТА", 14),
        ("2.  ФИЛЬТР 12 В", 84),
        ("3.  СТАБИЛИЗАТОР", 156),
        ("4.  КОНТРОЛЬ 3,3 В", 206),
        ("5.  ESP32", 285),
    ]
    for text, x in items:
        txt(text, x, SEC_TITLE_Y, 2.6, bold=True)
        ln((x, SEC_TITLE_Y - 2.4), (x + len(text) * 2.95, SEC_TITLE_Y - 2.4), CFRAME, lw=13)


# ----------------------------------------------------------------------------
# Лист 1. Пояснения, легенда, таблица нагрева
# ----------------------------------------------------------------------------
def para(text, x, y, width, h=2.2, leading=3.9, layer=CNOTE, bold=False):
    """Абзац с переносом по словам. Возвращает координату y следующей строки."""
    words = text.split()
    line = ""
    yy = y
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


def draw_notes():
    """Блок примечаний."""
    x0, y0, x1, y1 = 12, 12, 218, 82
    rect(x0, y0, x1, y1, CFRAME, lw=25)
    txt("ПРИМЕЧАНИЯ", x0 + 3, y1 - 5.0, 3.0, bold=True)
    ln((x0 + 3, y1 - 6.6), (x0 + 32, y1 - 6.6), CFRAME, lw=25)

    notes = [
        "1. Вход: 12 В постоянного тока (допустимо 9...15 В), ток до 1 А. Плюс — на контакт 1 "
        "клеммника J1. При переполюсовке диод D1 закроется и плата останется целой.",
        "2. Выход: 3,3 В (не 3,0 В). ESP32 работает при 3,0...3,6 В, номинал — 3,3 В. "
        "Длительный ток нагрузки — до 300 мА, см. тепловой расчёт ниже.",
        "3. U1 — линейный стабилизатор, лишнее напряжение он превращает в тепло: "
        "P = (12 - 3,3) x I. Под корпусом SOT-223 нужен медный полигон не менее 2 см2.",
        "4. Никогда не подавайте 12 В на выводы 3V3, GPIO, EN модуля ESP32 — он выйдет из строя.",
        "5. Не подключайте USB к плате ESP32 одновременно с питанием 12 В: источники будут "
        "мешать друг другу. Питание — либо от USB, либо от 12 В.",
        "6. Импульсы Wi-Fi (до 500 мА) вызывают просадку 3,3 В. Для стабильной работы с Wi-Fi "
        "замените U1 на импульсный модуль (MP1584, LM2596) — схема подключения та же.",
        "7. Если будете часто прошивать плату по USB, впаяйте дополнительный диод Шоттки "
        "(SS14) между выходом и входом U1, катодом к входу — он защитит стабилизатор от "
        "обратного напряжения.",
        "8. Цвет линии на схеме: красный — 12 В, синий — 3,3 В, зелёный — земля (GND), "
        "чёрный — сигналы.",
    ]
    yy = y1 - 11.5
    for n in notes:
        yy = para(n, x0 + 3.0, yy, 198, h=2.05, leading=3.3)
        yy -= 0.9


def draw_thermal_table():
    """Таблица нагрева линейного стабилизатора."""
    x0, y0, x1, y1 = 12, 86, 218, 140
    rect(x0, y0, x1, y1, CFRAME, lw=25)
    txt("ТЕПЛОВОЙ РАСЧЁТ U1  (P = (12 В - 3,3 В) x I = 8,7 В x I)", x0 + 3, y1 - 5.2, 2.6, bold=True)

    col = [x0 + 3, x0 + 62, x0 + 108, x1 - 2]
    top = y1 - 10.0
    row_h = 7.2
    heads = ["Ток нагрузки", "Мощность на U1", "Что будет на практике"]
    for (cx, head) in zip(col, heads):
        txt(head, cx, top - 2.0, 2.2, bold=True)
    ln((x0 + 2, top - 4.4), (x1 - 2, top - 4.4), CFRAME, lw=25)

    rows = [
        ("100 мА", "0,87 Вт", "нормально, корпус тёплый"),
        ("200 мА", "1,74 Вт", "горячо; обязателен полигон 2 см2"),
        ("300 мА", "2,61 Вт", "предел для SOT-223, нужен радиатор"),
        ("500 мА (Wi-Fi)", "4,35 Вт", "перегрев, просадка 3,3 В, перезагрузка"),
    ]
    yy = top - 8.0
    for r in rows:
        for cx, val in zip(col, r):
            txt(val, cx, yy, 2.1)
        ln((x0 + 2, yy - 2.2), (x1 - 2, yy - 2.2), "ТАБЛИЦЫ", lw=13)
        yy -= row_h

    txt("Вывод: для одного ESP32 без Wi-Fi и нескольких датчиков 200...250 мА — рабочий режим.",
        x0 + 3, y0 + 3.5, 2.0, CNOTE)


def draw_explanation():
    """Пошаговое объяснение работы схемы."""
    x0, y0, x1, y1 = 12, 146, 265, 202
    rect(x0, y0, x1, y1, CFRAME, lw=25)
    txt("КАК ЭТО РАБОТАЕТ — ПО ШАГАМ", x0 + 3, y1 - 5.2, 2.8, bold=True)

    steps = [
        "1.  12 В приходит на клеммник J1 (плюс — контакт 1, минус — контакт 2).",
        "2.  F1 отключает плату, если ток превысит 1 А (короткое замыкание, ошибка монтажа).",
        "3.  D1 пропускает ток только в одну сторону: перепутали плюс и минус — плата цела.",
        "4.  C1 (220 мкФ) и C2 (10 мкФ) сглаживают пульсации и помехи входного напряжения.",
        "5.  U1 (AMS1117-3.3) понижает 12 В до 3,3 В; разница напряжений уходит в тепло.",
        "6.  C3 и C4 фильтруют выход и не дают U1 самовозбуждаться.",
        "7.  R1+LED1 сигналят о наличии 12 В, R2+LED2 — о наличии 3,3 В.",
        "8.  Плата ESP32 вставляется в розетку J2 и получает 3,3 В; её выводы выведены на J3 и J4.",
    ]
    yy = y1 - 11.0
    for s in steps:
        txt(s, x0 + 3, yy, 2.1)
        yy -= 4.6


def draw_legend():
    """Легенда: что означает каждый символ и цвет линии."""
    x0, y0, x1, y1 = 275, 90, 408, 132
    rect(x0, y0, x1, y1, CFRAME, lw=25)
    txt("УСЛОВНЫЕ ОБОЗНАЧЕНИЯ", x0 + 3, y1 - 4.4, 2.4, bold=True)

    sx = x0 + 6
    sym_res_h(sx, 121.0, w=8, h=3.2)
    txt("резистор", sx + 7, 120.2, 1.9)
    sym_cap_v(sx, 115.0, w=8)
    txt("конденсатор", sx + 7, 114.2, 1.9)
    sym_diode_h(sx, 108.6, w=6.5, h=4.6)
    txt("диод", sx + 7, 107.8, 1.9)
    sym_gnd(sx, 106.4)
    txt("земля (GND)", sx + 8.5, 102.4, 1.9)

    lx = x0 + 62
    ln((lx, 121.0), (lx + 11, 121.0), C12, lw=40)
    txt("12 В", lx + 13, 120.2, 1.9)
    ln((lx, 115.0), (lx + 11, 115.0), C3V3, lw=40)
    txt("3,3 В", lx + 13, 114.2, 1.9)
    ln((lx, 109.0), (lx + 11, 109.0), CGND, lw=40)
    txt("земля", lx + 13, 108.2, 1.9)
    ln((lx, 103.0), (lx + 11, 103.0), CSIG, lw=40)
    txt("сигналы", lx + 13, 102.2, 1.9)




def draw_warning():
    x0, y0, x1, y1 = 270, 66, 408, 88
    rect(x0, y0, x1, y1, CNOTE, lw=35)
    txt("ВНИМАНИЕ!", x0 + 3, y1 - 4.6, 2.8, bold=True, layer=CNOTE)
    para("Не подавайте 12 В на выводы 3V3 и GPIO модуля ESP32 — он сгорит. "
         "Перед включением проверьте полярность на клеммнике J1.",
         x0 + 3, y1 - 9.6, 132, h=2.2, leading=4.0, layer=CNOTE)


# ----------------------------------------------------------------------------
# Лист 2. Контур платы + перечень элементов
# ----------------------------------------------------------------------------
# ----------------------------------------------------------------------------
# Лист 2. Топология платы (разводка) + перечень элементов
# ----------------------------------------------------------------------------
SHEET2_X = 440.0
BW, BH = 100.0, 70.0        # размеры платы, мм
BSCALE = 2.0
BX0, BY0 = 455.0, 105.0     # левый нижний угол платы на листе

PITCH = 2.54
J2X = 48.0                  # первый контакт розетки J2
J2_TOP = 46.43              # верхний ряд (столбец A платы ESP32)
J2_BOT = 23.57              # нижний ряд
J3_Y = 62.0                 # ряд контактов гребёнок J3/J4
J4X = 68.32

# Ширина дорожек, мм (используется и как вес линии в DXF)
W_PWR = 1.0
W_33 = 0.5
W_SIG = 0.3


def bx(x):
    return BX0 + x * BSCALE


def by(y):
    return BY0 + y * BSCALE


def bpath(pts, layer, width_mm):
    """Дорожка: ломаная с весом линии, равным ширине меди."""
    msp.add_lwpolyline(
        [(bx(x), by(y)) for x, y in pts],
        dxfattribs={"layer": layer, "lineweight": int(round(width_mm * 100))},
    )


def pad_th(x, y, d=1.8, hole=1.0):
    circle(bx(x), by(y), d / 2 * BSCALE, "ПЛАТА_ПАДЫ")
    circle(bx(x), by(y), hole / 2 * BSCALE, "ПЛАТА_ПАДЫ")


def pad_smd(x, y, w, h):
    rect(bx(x - w / 2), by(y - h / 2), bx(x + w / 2), by(y + h / 2), "ПЛАТА_ПАДЫ")


def via(x, y, d=1.6, hole=0.8):
    circle(bx(x), by(y), d / 2 * BSCALE, "ПЛАТА_ПАДЫ", lw=35)
    circle(bx(x), by(y), hole / 2 * BSCALE, "ПЛАТА_ПАДЫ")


def silk_rect(x0, y0, x1, y1):
    rect(bx(x0), by(y0), bx(x1), by(y1), "ПЛАТА_ДЕТАЛИ")


# ----------------------------------------------------------------------------
# Данные платы
# ----------------------------------------------------------------------------
# гребёнки и розетка
J3_PADS = [(J2X + i * PITCH, J3_Y) for i in range(8)]
J4_PADS = [(J4X + i * PITCH, J3_Y) for i in range(8)]
J2_TOP_PADS = [(J2X + i * PITCH, J2_TOP) for i in range(19)]
J2_BOT_PADS = [(J2X + i * PITCH, J2_BOT) for i in range(19)]

# SMD-контакты: (x, y, w, h)
SMD = [
    (15.2, 36.2, 1.6, 1.8), (17.0, 36.2, 1.6, 1.8),        # F1
    (20.3, 36.2, 2.2, 2.4), (23.2, 36.2, 2.2, 2.4),        # D1
    (27.0, 36.2, 1.3, 1.5), (27.0, 33.8, 1.3, 1.5),        # C2
    (38.5, 58.2, 1.3, 1.5), (38.5, 55.8, 1.3, 1.5),        # C3
    (41.0, 58.2, 1.3, 1.5), (41.0, 55.8, 1.3, 1.5),        # C4
    (32.0, 32.0, 1.0, 2.0), (34.25, 32.0, 1.0, 2.0),       # U1 pin1, pin2
    (36.5, 32.0, 1.0, 2.0), (34.0, 35.5, 3.4, 2.2),        # U1 pin3, tab
    (18.5, 14.0, 1.3, 1.5), (20.5, 14.0, 1.3, 1.5),        # R1
    (22.5, 14.0, 1.6, 1.8), (24.5, 14.0, 1.6, 1.8),        # LED1
    (45.5, 14.0, 1.3, 1.5), (47.5, 14.0, 1.3, 1.5),        # R2
    (49.0, 14.0, 1.6, 1.8), (51.0, 14.0, 1.6, 1.8),        # LED2
]

# Один слой меди; цвет дорожки = цепь (как на листе 1).
TRACES = [
    # --- +12 В ---
    (W_PWR, [(8.0, 37.5), (10.0, 37.5), (11.3, 36.2), (15.2, 36.2)], "ШИНА_12В"),
    (W_PWR, [(17.0, 36.2), (20.3, 36.2)], "ШИНА_12В"),
    (W_PWR, [(23.2, 36.2), (27.0, 36.2)], "ШИНА_12В"),
    (W_PWR, [(27.0, 36.2), (30.0, 36.2), (30.0, 32.0), (32.0, 32.0)], "ШИНА_12В"),
    (W_PWR, [(13.0, 36.2), (13.0, 27.5)], "ШИНА_12В"),
    (W_SIG, [(18.5, 36.2), (18.5, 14.0)], "ШИНА_12В"),
    # --- +3,3 В ---
    (W_33, [(34.0, 35.5), (34.0, 58.0), (42.5, 58.0)], "ШИНА_3В3"),
    (W_33, [(45.5, 58.0), (48.0, 58.0), (48.0, 62.0)], "ШИНА_3В3"),
    (W_33, [(48.0, 58.0), (48.0, 46.43)], "ШИНА_3В3"),
    (W_33, [(42.5, 58.0), (42.5, 66.5), (86.1, 66.5), (86.1, 62.0)], "ШИНА_3В3"),
    (W_SIG, [(45.5, 58.0), (45.5, 14.0)], "ШИНА_3В3"),
    # --- GND ---
    (W_PWR, [(8.0, 6.0), (51.5, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(8.0, 32.5), (8.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(13.0, 20.5), (13.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(27.0, 33.8), (27.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(36.5, 32.0), (36.5, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(38.5, 55.8), (38.5, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(41.0, 55.8), (41.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(44.0, 63.8), (44.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(24.5, 14.0), (24.5, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(51.0, 14.0), (51.0, 6.0)], "ЗЕМЛЯ"),
    (W_PWR, [(44.0, 63.8), (83.56, 63.8)], "ЗЕМЛЯ"),
    (W_PWR, [(50.54, 62.0), (50.54, 63.8)], "ЗЕМЛЯ"),
    (W_PWR, [(83.56, 62.0), (83.56, 63.8)], "ЗЕМЛЯ"),
    (W_PWR, [(83.56, 46.43), (83.56, 62.0)], "ЗЕМЛЯ"),
    # --- связи светодиодов ---
    (W_SIG, [(20.5, 14.0), (22.5, 14.0)], "СИГНАЛЫ"),
    (W_SIG, [(47.5, 14.0), (49.0, 14.0)], "СИГНАЛЫ"),
]

# Сигнальные дорожки J2 -> J3/J4
for i in range(6):
    x = J2X + (i + 2) * PITCH
    TRACES.append((W_SIG, [(x, J2_TOP), (x, J3_Y)], "СИГНАЛЫ"))
for i in range(6):
    x = J4X + i * PITCH
    TRACES.append((W_SIG, [(x, J2_TOP), (x, J3_Y)], "СИГНАЛЫ"))

JP1 = (42.5, 58.0, 45.5, 58.0)   # перемычка через шину GND


def board_sheet_frame():
    x0, y0, w, h = SHEET2_X, 0.0, 420.0, 297.0
    m = 5.0
    for a, b in (((x0, y0), (x0 + w, y0)), ((x0 + w, y0), (x0 + w, y0 + h)),
                 ((x0 + w, y0 + h), (x0, y0 + h)), ((x0, y0 + h), (x0, y0))):
        ln(a, b, CFRAME, lw=50)
    rect(x0 + m, y0 + m, x0 + w - m, y0 + h - m, CFRAME)
    title_block(x0 + w - m - 185, y0 + m,
                "Плата питания 12 В → 3,3 В", "Топология печатной платы (1 слой)",
                "с модулем ESP32", 2, 2, "A3", "2 : 1")


def draw_board_outline():
    r = 3.0
    ln((bx(r), by(0)), (bx(BW - r), by(0)), "ПЛАТА_КОНТУР", lw=60)
    ln((bx(BW), by(r)), (bx(BW), by(BH - r)), "ПЛАТА_КОНТУР", lw=60)
    ln((bx(BW - r), by(BH)), (bx(r), by(BH)), "ПЛАТА_КОНТУР", lw=60)
    ln((bx(0), by(BH - r)), (bx(0), by(r)), "ПЛАТА_КОНТУР", lw=60)
    arc(bx(BW - r), by(r), r * BSCALE, 270, 360, "ПЛАТА_КОНТУР", lw=60)
    arc(bx(BW - r), by(BH - r), r * BSCALE, 0, 90, "ПЛАТА_КОНТУР", lw=60)
    arc(bx(r), by(BH - r), r * BSCALE, 90, 180, "ПЛАТА_КОНТУР", lw=60)
    arc(bx(r), by(r), r * BSCALE, 180, 270, "ПЛАТА_КОНТУР", lw=60)

    for hx, hy in ((4, 4), (96, 4), (4, 66), (96, 66)):
        circle(bx(hx), by(hy), 3.2, "ПЛАТА_КОНТУР")
        circle(bx(hx), by(hy), 1.6, "ПЛАТА_КОНТУР")
        ln((bx(hx) - 4.5, by(hy)), (bx(hx) + 4.5, by(hy)), "ОСЕВЫЕ")
        ln((bx(hx), by(hy) - 4.5), (bx(hx), by(hy) + 4.5), "ОСЕВЫЕ")


def draw_board_traces():
    for w, pts, lay in TRACES:
        bpath(pts, lay, w)

    # контактные площадки
    pad_th(8.0, 37.5, d=2.0, hole=1.1)
    pad_th(8.0, 32.5, d=2.0, hole=1.1)
    for x, y in J2_TOP_PADS + J2_BOT_PADS + J3_PADS + J4_PADS:
        pad_th(x, y)
    for x, y, w, h in SMD:
        pad_smd(x, y, w, h)
    pad_th(13.0, 27.5, d=2.6, hole=1.1)
    pad_th(13.0, 20.5, d=2.6, hole=1.1)

    # перемычка JP1
    pad_th(JP1[0], JP1[1], d=1.6, hole=0.9)
    pad_th(JP1[2], JP1[3], d=1.6, hole=0.9)
    arc(bx((JP1[0] + JP1[2]) / 2), by(JP1[1]) + 0.6 * BSCALE, 1.3 * BSCALE,
        0, 180, "ПЛАТА_ДЕТАЛИ", lw=30)
    txt("JP1", bx(43.2), by(61.0), 1.7, bold=True)

    # шелкография
    silk_rect(2.0, 31.0, 12.0, 39.0)                      # J1
    silk_rect(13.9, 35.1, 18.1, 37.3)                     # F1
    silk_rect(19.2, 34.3, 24.3, 38.1)                     # D1
    circle(bx(13.0), by(24.0), 4.0 * BSCALE, "ПЛАТА_ДЕТАЛИ")
    txt("+", bx(16.6), by(27.0), 2.4, bold=True)
    silk_rect(26.2, 33.0, 27.8, 37.0)                     # C2
    silk_rect(37.7, 55.0, 39.3, 59.0)                     # C3
    silk_rect(40.2, 55.0, 41.8, 59.0)                     # C4
    silk_rect(31.0, 32.0, 37.5, 35.5)                     # U1
    silk_rect(17.8, 13.2, 21.2, 14.8)                     # R1
    silk_rect(21.7, 13.0, 25.3, 15.0)                     # LED1
    silk_rect(44.8, 13.2, 48.2, 14.8)                     # R2
    silk_rect(48.2, 13.0, 51.8, 15.0)                     # LED2
    rect(bx(44.5), by(21.0), bx(99.5), by(49.0), "ОСЕВЫЕ")

    txt("J1", bx(4.0), by(41.5), 2.0, bold=True)
    txt("F1", bx(15.0), by(33.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("D1", bx(21.7), by(32.8), 1.8, align=TA.MIDDLE_CENTER)
    txt("C1", bx(8.6), by(24.8), 1.8, bold=True)
    txt("C2", bx(28.6), by(39.4), 1.8, align=TA.MIDDLE_CENTER)
    txt("C3", bx(38.5), by(60.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("C4", bx(41.0), by(60.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("U1", bx(31.2), by(30.4), 1.8, bold=True)
    txt("R1", bx(17.5), by(11.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("LED1", bx(21.5), by(11.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("R2", bx(44.5), by(11.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("LED2", bx(48.0), by(11.6), 1.8, align=TA.MIDDLE_CENTER)
    txt("J2  (розетка 2×19)", bx(48.0), by(52.4), 2.0, bold=True)
    txt("J3", bx(J2X - 2.0), by(64.6), 1.8, align=TA.RIGHT, bold=True)
    txt("J4", bx(J4X - 2.0), by(64.6), 1.8, align=TA.RIGHT, bold=True)
    txt("ESP32", bx(70.0), by(34.0), 2.6, bold=True, align=TA.MIDDLE_CENTER)
    txt("габарит платы модуля", bx(70.0), by(19.6), 1.7, align=TA.MIDDLE_CENTER)


def draw_board_dims():
    y = by(0) - 8
    arrow_h(bx(0), bx(BW), y)
    txt("100", (bx(0) + bx(BW)) / 2, y + 2.0, 2.4, align=TA.MIDDLE_CENTER)
    ln((bx(0), by(0)), (bx(0), y - 2), "РАЗМЕРЫ", lw=13)
    ln((bx(BW), by(0)), (bx(BW), y - 2), "РАЗМЕРЫ", lw=13)

    x = bx(0) - 8
    arrow_v(by(0), by(BH), x)
    txt("70", x - 2.2, (by(0) + by(BH)) / 2, 2.4, align=TA.MIDDLE_CENTER, rot=90)
    ln((bx(0), by(0)), (x - 2, by(0)), "РАЗМЕРЫ", lw=13)
    ln((bx(0), by(BH)), (x - 2, by(BH)), "РАЗМЕРЫ", lw=13)

    y = by(BH) + 7
    arrow_h(bx(4), bx(96), y)
    txt("92 (межцентровое)", (bx(4) + bx(96)) / 2, y + 2.0, 2.0, align=TA.MIDDLE_CENTER)
    x = bx(BW) + 8
    arrow_v(by(4), by(66), x)
    txt("62", x + 2.4, (by(4) + by(66)) / 2, 2.0, align=TA.MIDDLE_CENTER, rot=90)

    txt("ТОПОЛОГИЯ ПЛАТЫ.  ВИД СВЕРХУ.  МАСШТАБ 2:1", bx(0), by(BH) + 14, 3.0, bold=True)
    txt("Плата 100 × 70 мм, 1 слой, 4 отверстия M3", bx(0), by(0) - 15, 2.1)


def draw_board_legend():
    """Легенда слоёв и обозначений."""
    x0, y0 = bx(0), by(0) - 23
    txt("ОБОЗНАЧЕНИЯ", x0, y0, 2.4, bold=True)
    txt("Плата односторонняя: вся медь на одном слое,", x0, y0 - 5.0, 1.7, CNOTE)
    txt("переходных отверстий нет.", x0, y0 - 7.6, 1.7, CNOTE)
    rows = [
        ("ШИНА_12В", "цепь +12 В"),
        ("ШИНА_3В3", "цепь +3,3 В"),
        ("ЗЕМЛЯ", "цепь GND"),
        ("СИГНАЛЫ", "сигналы GPIO, светодиоды"),
    ]
    yy = y0 - 13.0
    for lay, desc in rows:
        if lay == "JP1":
            circle(x0 + 3.0, yy + 0.6, 1.1, "ПЛАТА_ПАДЫ")
            arc(x0 + 3.0, yy + 1.2, 1.1, 0, 180, "ПЛАТА_ДЕТАЛИ")
        else:
            ln((x0, yy + 0.8), (x0 + 12, yy + 0.8), lay, lw=45)
        txt(desc, x0 + 15, yy, 1.9, CNOTE)
        yy -= 4.6


def draw_trace_table():
    """Таблица рекомендуемых ширин дорожек."""
    x0, x1 = 672.0, 845.0
    top = 148.0
    txt("ШИРИНА ДОРОЖЕК", x0, top + 16, 2.8, bold=True)
    txt("Медь 35 мкм (1 oz), внешний слой", x0, top + 11.5, 1.9, CNOTE)

    heads = ("Цепь", "Ток", "Ширина")
    rows = [
        ("12 В (J1 → U1)", "до 1 А", "1,0 мм"),
        ("Земля GND", "до 1 А", "1,0 мм"),
        ("3,3 В", "до 0,5 А", "0,5 мм"),
        ("Сигналы GPIO", "до 20 мА", "0,3 мм"),
        ("Светодиоды", "до 10 мА", "0,3 мм"),
    ]
    c1, c2, c3 = x0, x0 + 62, x0 + 108
    yy = top + 6.0
    for hx, htxt in zip((c1, c2, c3), heads):
        txt(htxt, hx, yy, 1.9, bold=True)
    ln((x0, yy - 2.4), (x1, yy - 2.4), CFRAME, lw=25)
    yy -= 6.4
    for r in rows:
        txt(r[0], c1, yy, 1.9)
        txt(r[1], c2, yy, 1.9)
        txt(r[2], c3, yy, 1.9, bold=True)
        ln((x0, yy - 2.4), (x1, yy - 2.4), "ТАБЛИЦЫ", lw=13)
        yy -= 6.2

    # обоснование
    yy -= 2.0
    txt("ОБОСНОВАНИЕ (IPC-2221)", x0, yy, 2.4, bold=True)
    yy -= 5.0
    lines = [
        "A = ( I / (k · ΔT^0,44) )^(1/0,725),  k = 0,048",
        "Для 1 А при ΔT = 10 °C: A = 16 мил², ширина 0,30 мм.",
        "На практике берут запас ×3 (тепло, падение напряжения,",
        "технология ЛУТ/заказ) — отсюда 1,0 мм для 12 В и GND.",
        "Дорожка 1 мм держит 2,3 А при ΔT = 10 °C.",
        "",
        "Зазор между дорожками — не менее 0,2 мм.",
        "Плата односторонняя — переходных отверстий нет.",
        "Перемычка JP1 — 1 шт. (пересечение 3,3 В и шины GND).",
        "Свободные места залить полигоном GND (один слой).",
    ]
    for s in lines:
        txt(s, x0, yy, 1.8, CNOTE)
        yy -= 3.6


def draw_board_notes():
    x0 = bx(0)
    y0 = by(0) - 57
    txt("УКАЗАНИЯ ПО МОНТАЖУ", x0, y0, 2.4, bold=True)
    lines = [
        "1. Плата односторонняя: все дорожки на слое «ДОРОЖКИ», второй слой не нужен.",
        "2. 12 В и GND — 1,0 мм; 3,3 В — 0,5 мм; сигналы — 0,3 мм; зазор 0,2 мм.",
        "3. JP1 — единственное пересечение (3,3 В через шину GND):",
        "    провод 0,5 мм или резистор 0 Ом в корпусе 0805, ставится последним.",
        "4. U1: вывод 2 (VOUT) не паяется — он соединён с табом внутри корпуса, ток идёт через таб.",
        "5. Разводка учебная: сверьте цоколёвку вашей платы ESP32 DevKit.",
    ]
    yy = y0 - 5.0
    for s_ in lines:
        txt(s_, x0, yy, 1.8, CNOTE)
        yy -= 4.0


def draw_bom():
    x0, w = 672.0, 173.0
    c1, c2, c3 = 24.0, 126.0, 17.0
    rows = [
        ("Поз.", "Наименование", "Кол."),
        ("J1", "Клеммник 2 контакта, шаг 5,08 мм", "1"),
        ("J2", "Розетка 2x19, шаг 2,54 мм (под ESP32 DevKit)", "1"),
        ("J3, J4", "Гребёнка 1x8, шаг 2,54 мм", "2"),
        ("JP1", "Перемычка проволочная или резистор 0 Ом", "1"),
        ("F1", "Предохранитель 1 А (вставка или самовосст.)", "1"),
        ("D1", "Диод Шоттки SS34 (3 А, 40 В)", "1"),
        ("U1", "AMS1117-3.3, корпус SOT-223", "1"),
        ("C1", "Конденсатор 220 мкФ, 25 В, электролит", "1"),
        ("C2", "Конденсатор 10 мкФ, 25 В, керамика", "1"),
        ("C3", "Конденсатор 22 мкФ, 10 В, керамика", "1"),
        ("C4", "Конденсатор 100 нФ, 50 В, керамика", "1"),
        ("R1", "Резистор 1,2 кОм, 0,25 Вт", "1"),
        ("R2", "Резистор 330 Ом, 0,25 Вт", "1"),
        ("LED1, LED2", "Светодиод 3 мм, зелёный", "2"),
        ("-", "Плата ESP32 DevKit V1 (вставляется в J2)", "1"),
    ]
    rh = 6.1
    top = 264.0
    bottom = top - (len(rows) - 1) * rh
    txt("ПЕРЕЧЕНЬ ЭЛЕМЕНТОВ", x0, top + 7.0, 2.8, bold=True)

    ln((x0, top + 3.2), (x0 + w, top + 3.2), CFRAME, lw=35)
    ln((x0, bottom - 3.2), (x0 + w, bottom - 3.2), CFRAME, lw=35)
    ln((x0, top + 3.2), (x0, bottom - 3.2), CFRAME, lw=25)
    ln((x0 + w, top + 3.2), (x0 + w, bottom - 3.2), CFRAME, lw=25)
    ln((x0 + c1, top + 3.2), (x0 + c1, bottom - 3.2), CFRAME, lw=25)
    ln((x0 + c1 + c2, top + 3.2), (x0 + c1 + c2, bottom - 3.2), CFRAME, lw=25)

    for i, r in enumerate(rows):
        yy = top - i * rh
        bold = (i == 0)
        txt(r[0], x0 + 2, yy, 1.85, bold=bold)
        txt(r[1], x0 + c1 + 2, yy, 1.85, bold=bold)
        txt(r[2], x0 + c1 + c2 + c3 / 2, yy, 1.85, align=TA.MIDDLE_CENTER, bold=bold)
        if i == 0:
            ln((x0, yy - 2.6), (x0 + w, yy - 2.6), CFRAME, lw=25)
        elif i < len(rows) - 1:
            ln((x0, yy - 2.6), (x0 + w, yy - 2.6), "ТАБЛИЦЫ", lw=13)



# ----------------------------------------------------------------------------
# Сборка
# ----------------------------------------------------------------------------
def render_png(path, bbox=None, dpi=160):
    import matplotlib
    matplotlib.use("Agg")
    matplotlib.rcParams["font.family"] = "Arial"
    import matplotlib.pyplot as plt
    from ezdxf.addons.drawing import Frontend, RenderContext
    from ezdxf.addons.drawing.config import (
        BackgroundPolicy, ColorPolicy, Configuration, LineweightPolicy,
    )
    from ezdxf.addons.drawing.matplotlib import MatplotlibBackend

    if bbox is None:
        bbox = (0, 0, 860, 297)
    w = (bbox[2] - bbox[0]) / 25.4
    h = (bbox[3] - bbox[1]) / 25.4
    fig = plt.figure(figsize=(w, h))
    ax = fig.add_axes([0, 0, 1, 1])
    ax.set_axis_off()
    ctx = RenderContext(doc)
    cfg = Configuration(
        background_policy=BackgroundPolicy.WHITE,
        color_policy=ColorPolicy.COLOR,
        lineweight_policy=LineweightPolicy.ABSOLUTE,
        lineweight_scaling=1.0,
    )
    backend = MatplotlibBackend(ax)
    Frontend(ctx, backend, config=cfg).draw_layout(msp, finalize=True)
    ax.set_aspect("auto")
    fig.set_size_inches(w, h)
    ax.set_position([0, 0, 1, 1])
    ax.set_xlim(bbox[0], bbox[2])
    ax.set_ylim(bbox[1], bbox[3])
    fig.savefig(path, dpi=dpi, facecolor="white")
    plt.close(fig)


def main():
    sheet_frame(*SHEET1)

    draw_input_chain()
    draw_filter_and_indicator_12()
    draw_regulator()
    draw_filter_and_indicator_33()
    draw_esp32_block()
    draw_section_titles()

    draw_notes()
    draw_thermal_table()
    draw_explanation()
    draw_legend()
    draw_warning()

    board_sheet_frame()
    draw_board_outline()
    draw_board_traces()
    draw_board_dims()
    draw_board_legend()
    draw_board_notes()
    draw_trace_table()
    draw_bom()

    # границы чертежа и лимиты (чтобы "Показать всё" работало корректно)
    from ezdxf import appsettings
    appsettings.update_extents(doc)
    doc.header["$LIMMIN"] = (0.0, 0.0)
    doc.header["$LIMMAX"] = (860.0, 297.0)
    doc.header["$INSBASE"] = (0.0, 0.0, 0.0)

    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), OUT_DXF)
    doc.saveas(out)

    if "--png" in sys.argv:
        png = os.path.join(os.path.dirname(os.path.abspath(__file__)), OUT_PNG)
        render_png(png, bbox=(0, 0, 860, 297), dpi=140)
        render_png(os.path.join(os.path.dirname(os.path.abspath(__file__)), "_sheet1.png"),
                   bbox=(0, 0, 420, 297), dpi=230)
        render_png(os.path.join(os.path.dirname(os.path.abspath(__file__)), "_sheet2.png"),
                   bbox=(440, 0, 860, 297), dpi=230)
        print("PNG:", png)

    print("DXF:", out)


if __name__ == "__main__":
    main()
