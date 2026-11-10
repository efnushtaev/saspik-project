"""Render the MQTT subscription diagram (faq/mqtt-subscriptions.png).

Pillow-only fallback: kroki.io is unreachable from here, so the same content
is drawn locally. Run with the bundled Python (Pillow 12):

    python3 faq/mqtt-subscriptions.py
"""

import math
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

W, H = 1800, 1030
FONT = "/System/Library/Fonts/Supplemental/Arial.ttf"
BOLD = "/System/Library/Fonts/Supplemental/Arial Bold.ttf"

BG = (255, 255, 255)
TEXT = (33, 33, 33)
MUTED = (96, 96, 96)
GREEN = (46, 125, 50)
GREEN_BG = (232, 245, 233)
ORANGE = (230, 81, 0)
ORANGE_BG = (255, 243, 224)
BLUE = (21, 101, 192)
BLUE_BG = (227, 242, 253)
RED = (183, 28, 28)
RED_BG = (255, 235, 238)
PURPLE = (123, 31, 162)
PURPLE_BG = (243, 229, 245)
GREY = (158, 158, 158)
GREY_BG = (247, 247, 247)


def font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(BOLD if bold else FONT, size)


img = Image.new("RGB", (W, H), BG)
d = ImageDraw.Draw(img)


def rbox(x1, y1, x2, y2, fill, outline, radius=16, width=3):
    d.rounded_rectangle(
        [x1, y1, x2, y2], radius=radius, fill=fill, outline=outline, width=width
    )


def text(x, y, s, f, fill=TEXT, anchor="mm"):
    d.text((x, y), s, font=f, fill=fill, anchor=anchor)


def arrow(p1, p2, color, width=3, head=15):
    ang = math.atan2(p2[1] - p1[1], p2[0] - p1[0])
    end = (
        p2[0] - math.cos(ang) * head * 0.65,
        p2[1] - math.sin(ang) * head * 0.65,
    )
    d.line([p1, end], fill=color, width=width)
    left = (
        p2[0] - math.cos(ang - 0.48) * head,
        p2[1] - math.sin(ang - 0.48) * head,
    )
    right = (
        p2[0] - math.cos(ang + 0.48) * head,
        p2[1] - math.sin(ang + 0.48) * head,
    )
    d.polygon([p2, left, right], fill=color)


def dashed(p1, p2, color, width=3, dash=12, gap=8):
    total = math.dist(p1, p2)
    if total == 0:
        return
    ux = (p2[0] - p1[0]) / total
    uy = (p2[1] - p1[1]) / total
    t = 0.0
    while t < total:
        t2 = min(t + dash, total)
        d.line(
            [
                (p1[0] + ux * t, p1[1] + uy * t),
                (p1[0] + ux * t2, p1[1] + uy * t2),
            ],
            fill=color,
            width=width,
        )
        t += dash + gap


def xmark(cx, cy, r=12, color=RED, width=5):
    d.line([(cx - r, cy - r), (cx + r, cy + r)], fill=color, width=width)
    d.line([(cx - r, cy + r), (cx + r, cy - r)], fill=color, width=width)


def check(cx, cy, r=7, color=GREEN, width=3):
    d.line([(cx - r, cy), (cx - r * 0.25, cy + r * 0.65)], fill=color, width=width)
    d.line([(cx - r * 0.25, cy + r * 0.65), (cx + r, cy - r * 0.65)], fill=color, width=width)


# ---------------------------------------------------------------- header
text(900, 34, "MQTT-подписки: кто получает чью команду", font(34, True), anchor="ma")
text(
    900,
    82,
    "Подписка — это фильтр по строке топика, а не идентичность устройства; «+» — wildcard на один сегмент",
    font(19),
    MUTED,
    anchor="ma",
)

# ------------------------------------------------------------------- UI
rbox(690, 125, 1110, 185, PURPLE_BG, PURPLE)
text(900, 145, "UI / сервер", font(22, True))
text(900, 170, "публикует команды", font(17), MUTED)

# --------------------------------------------------------------- broker
rbox(760, 205, 1040, 245, BLUE_BG, BLUE)
text(900, 225, "MQTT broker", font(20, True))
arrow((900, 185), (900, 205), GREY)

# --------------------------------------------------------------- topics
rbox(90, 275, 760, 370, BLUE_BG, BLUE)
text(425, 305, "device/unitId2/saspik-sawqm001/command", font(19, True))
text(425, 340, "команда для нашего устройства", font(16), MUTED)

rbox(1040, 275, 1710, 370, BLUE_BG, BLUE)
text(1375, 305, "device/unitId2/saspik-aenm001/command", font(19, True))
text(1375, 340, "команда для другого объекта unitId2", font(16), MUTED)

arrow((820, 245), (425, 275), GREY)
arrow((980, 245), (1375, 275), GREY)

# ------------------------------------------------------------ applications
rbox(90, 470, 760, 690, GREEN_BG, GREEN)
text(425, 500, "MqttDeviceApp", font(26, True), GREEN)
text(425, 536, "objectId = saspik-sawqm001", font(18))
text(425, 568, "подписка (только точная):", font(16, True))
text(425, 594, "device/unitId2/saspik-sawqm001/command", font(15))
text(425, 630, "получает только свои команды", font(18, True), GREEN)
text(425, 660, "wildcard не используется", font(16), MUTED)

rbox(1040, 470, 1710, 710, ORANGE_BG, ORANGE)
text(1375, 500, "GatewayApp", font(26, True), ORANGE)
text(1375, 536, "objectId = saspik-sawqm001", font(18))
text(1375, 566, "подписка 1 (точная):", font(16, True))
text(1375, 590, "device/unitId2/saspik-sawqm001/command", font(15))
text(1375, 620, "подписка 2 (wildcard):", font(16, True))
text(1375, 646, "device/unitId2/+/command", font(17, True))
text(1375, 682, "чтобы маршрутизировать команды ESP-NOW-узлам", font(15), MUTED)

# --------------------------------------------------------------- delivery
arrow((425, 370), (425, 470), GREEN, width=4)
check(444, 410)
text(458, 410, "точное совпадение", font(17, True), GREEN, anchor="la")

arrow((1375, 370), (1375, 470), ORANGE, width=4)
check(1394, 410, color=ORANGE)
text(1408, 410, "+ = saspik-aenm001", font(17, True), ORANGE, anchor="la")

# no-delivery path: topic for another objectId -> MqttDeviceApp
dashed((1040, 323), (900, 323), RED)
dashed((900, 323), (900, 580), RED)
dashed((900, 580), (810, 580), RED)
xmark(795, 580)
xmark(907, 430, r=7, width=3)
text(922, 430, "exact не совпал", font(15, True), RED, anchor="la")
text(922, 452, "другой objectId", font(15), RED, anchor="la")
text(922, 476, "не доставлено", font(14), RED, anchor="la")
text(900, 298, "чужой objectId", font(15, True), RED)

# ------------------------------------------------------- gateway routing
arrow((1375, 710), (1375, 750), ORANGE)
rbox(1120, 750, 1630, 830, ORANGE_BG, ORANGE, radius=14)
text(1375, 790, "objectId есть в NODE_TABLE?", font(19, True))

arrow((1240, 830), (1085, 880), GREEN)
text(1135, 846, "да", font(17, True), GREEN)
arrow((1510, 830), (1565, 880), RED)
text(1550, 846, "нет", font(17, True), RED)

rbox(880, 880, 1290, 960, (252, 228, 236), (198, 40, 40))
text(1085, 905, "ESP-NOW-узел", font(19, True), (198, 40, 40))
text(1085, 935, "команда уходит узлу", font(16), MUTED)

rbox(1360, 880, 1770, 960, RED_BG, RED)
text(1565, 905, "warning", font(19, True), RED)
text(1565, 935, "command to unknown node", font(15), RED)

# ------------------------------------------------------- wildcard explainer
rbox(90, 750, 760, 985, GREY_BG, GREY, radius=16, width=2)
text(425, 778, "Как работает «+»", font(21, True))

line = "device/unitId2/+/command"
main_font = font(18, True)
text(120, 815, line, main_font, anchor="la")
prefix = "device/unitId2/"
x_plus = 120 + d.textlength(prefix, font=main_font)
w_plus = d.textlength("+", font=main_font)
d.line([(x_plus, 836), (x_plus + w_plus, 836)], fill=RED, width=3)
text(120, 850, "+ = один любой сегмент", font(15), RED, anchor="la")

check(126, 882)
text(142, 882, "device/unitId2/saspik-aenm001/command", font(15), GREEN, anchor="la")
xmark(126, 910, r=7, width=3)
text(142, 910, "device/unitId3/saspik-aenm001/command", font(15), RED, anchor="la")
xmark(126, 938, r=7, width=3)
text(142, 938, "device/unitId2/a/b/command", font(15), RED, anchor="la")
text(
    120,
    966,
    "objectId — просто сегмент строки; при «+» он не сравнивается с вашим.",
    font(14),
    MUTED,
    anchor="la",
)

# ---------------------------------------------------------------- footer
text(
    900,
    1012,
    "MQTT доставляет копию каждому подписчику, чей фильтр совпал — «перехвата» нет.",
    font(16),
    MUTED,
    anchor="ma",
)

out = Path(__file__).with_suffix(".png")
img.save(out)
print(f"saved {out} ({img.width}x{img.height})")
