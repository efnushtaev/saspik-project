# Аппаратная схема UWB-узла (Stage 0)

## Состав на один узел

| Компонент                    | Назначение                    | Примечание                      |
| ---------------------------- | ----------------------------- | ------------------------------- |
| ESP32-S3 (ESP32-S3-WROOM-1)  | Главный контроллер            | SPI, UART, GPIO                 |
| DWM3000                      | UWB-трансивер                 | IEEE 802.15.4z, модуль с антенной |
| LDO 3.3 В (AMS1117-3.3)     | Питание                       | От LiPo / USB, фикс. 3.3В      |
| USB-UART (CH340C)            | Отладка + лог                 | Только для стенда               |
| C1 — 10μF                    | Входной фильтр LDO            | Перед AMS1117 IN–GND           |
| C3 — 10μF                    | Выходной фильтр LDO           | После AMS1117 OUT–GND          |
| C5 — 100nF                   | Высокочастотный байпас ESP32  | Рядом с VDD33                   |
| C6 — 10μF                    | Низкочастотный накопитель ESP32 | Рядом с VDD33                 |
| C7 — 100nF                   | Высокочастотный байпас DWM3000 | Рядом с VCC/VCC2               |
| C8 — 10μF                    | Развязка внутр. регулятора 1.8В DWM3000 | Между VDD и GND        |
| R1 — 10k                     | Pull-up EN ESP32              | К 3.3V                          |



## Структурная схема узла

```mermaid
graph TB
  subgraph FiveV[5V]
    J1[J1 USB]
    C1[C1 10uF]
  end

  subgraph ThreeV[3.3V]
    LDO[AMS1117-3.3]
    C3[C3 10uF]
    C5[C5 100nF]
    C6[C6 10uF]
    C7[C7 100nF]
    C8[C8 10uF]
    ESP[ESP32-S3]
    CH340[CH340C]
    DW[DWM3000]
    R1[R1 10k]
  end

  GND_BUS[ ]

  J1 --> LDO
  J1 --> C1
  LDO --> C3 & C5 & C6 & ESP & CH340
  LDO -->|VCC+VCC2| DW
  C7 --> DW
  C8 -.->|1.8V out| DW

  J2[J2 USB-UART] --> CH340
  CH340 -->|UART| ESP
  R1 -.->|EN pull-up| ESP
  ESP -->|SPI| DW
  ESP -->|GPIO| DW

  C1 --> GND_BUS
  C3 --> GND_BUS
  C5 --> GND_BUS
  C6 --> GND_BUS
  C7 --> GND_BUS
  C8 --> GND_BUS
  ESP --> GND_BUS
  CH340 --> GND_BUS
  DW --> GND_BUS
  J2 --> GND_BUS

  style J1 fill:#e8f5e9,stroke:#2e7d32
  style J2 fill:#e8f5e9,stroke:#2e7d32
  style LDO fill:#fff3e0,stroke:#e65100
  style CH340 fill:#f3e5f5,stroke:#7b1fa2
  style ESP fill:#e3f2fd,stroke:#1565c0
  style DW fill:#fce4ec,stroke:#c62828
  style GND_BUS fill:#f5f5f5,stroke:#616161,stroke-dasharray:3,color:#616161
```

## Соединение ESP32-S3 ↔ DWM3000 (SPI)

```
ESP32-S3           DWM3000 (24-pin QFN модуль)
────────           ───────────────────────────
GPIO 10 (SPI CS)   →  CS
GPIO 11 (SPI MOSI) →  MOSI
GPIO 12 (SPI MISO) ←  MISO
GPIO 13 (SPI SCK)  →  SCK
GPIO 14            →  RSTn
GPIO 15            ←  IRQ
3.3V               →  VCC / VCC2
GND                →  GND
```

> Пин-аут требует сверки с даташитом DWM3000 (Qorvo). В отличие от DWM1000, у DWM3000 часть пинов переключена. Перед разводкой платы обязательно сверить назначение по документации.

## Питание

```
LiPo 3.7V → LDO 3.3V → ESP32-S3 VDD33
                     → DWM3000 VCC + VCC2 (3.3V, оба пина)

DWM3000 VDD — выход внутреннего регулятора 1.8В (C8 между VDD и GND)
```

## Стенд для Stage 1 (два узла)

```
[USB]──[ESP32 #1]──[UWB] ~~~~~~~~ [UWB]──[ESP32 #2]──[USB]
         │                                            │
      UART(лог)                                   UART(лог)
```

- Оба узла идентичны по схеме.
- Питание через USB от ноутбука.
- Связь UWB — двусторонняя дальнометрия (TWR).

## Альтернативные пины для кастомной платы

Если SPI-порты заняты, можно использовать HSPI (GPIO 26–29) вместо SPI2:

```
HSPI: CS=26, MOSI=27, MISO=28, SCK=29
```

## Известные проблемы схемы

- Пин-аут DWM3000 требует верификации — в текущей схеме используются предположительные соединения по аналогии с DWM1000.
