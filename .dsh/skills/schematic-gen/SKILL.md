---
name: schematic-gen
description: Генерирует принципиальные схемы и диаграммы для SASPIK — DXF-чертёж с PNG-превью (ezdxf), Mermaid-диаграмму с рендером через kroki.io; KiCad-вариант по legacy-правилам формата.
whenToUse: Нужно нарисовать или обновить схему подключения/питания, диаграмму связей либо экспортировать её в PNG.
---

# Schematic Generator

Три режима. Выбирай по запросу: «схема платы» → DXF, «диаграмма связей» → Mermaid, «KiCad-схема» → legacy-раздел.

## 1. DXF-схема (основной режим)

Референс — корневой генератор [`esp32-12v-3v3-schematic.py`](../../../esp32-12v-3v3-schematic.py): плата питания 12 В → 3,3 В с модулем ESP32. Он же образец структуры.

- Стек: `ezdxf` (документ `R2010`, `doc.units = MM`, `$INSUNITS = 4`), превью — `matplotlib` через `ezdxf.addons.drawing` (`Frontend` + `MatplotlibBackend`, `savefig(..., dpi=...)`).
- Файлы: `<имя>-schematic.py` в корне → выходы `<имя>-schematic.dxf` и `<имя>-schematic-preview.png`.
- Слои (обязательный набор): `РАМКА`, `ШИНА_12В`, `ШИНА_3В3`, `ЗЕМЛЯ`, `СИГНАЛЫ`, `СИМВОЛЫ`, `ТЕКСТ`, `ПОЯСНЕНИЯ`, `ПЛАТА_КОНТУР`, `ПЛАТА_ДЕТАЛИ`, `РАЗМЕРЫ`, `ТАБЛИЦЫ`.
- Кириллица: текстовые стили `RU` / `RU_B` с `arial.ttf` / `arialbd.ttf`.
- Композиция: функции `draw_<блок>()` на каждый узел (вход, фильтр, регулятор, ESP32, таблицы, легенда, примечания), затем контур платы и трассы; в конце — рамка и основная надпись.
- Зависимости не установлены в системном `python3` (`ezdxf` отсутствует). Ставь в venv или используй bundled Python из `load_workspace_dependencies`.

Проверка: скрипт отрабатывает без ошибок, созданы оба файла, PNG читается и не пустой, содержимое попадает в рамку листа.

## 2. Mermaid-диаграмма

- Формат: `docs/<имя>.md` с блоком ` ```mermaid `; тип `graph TB` / `graph LR`, `subgraph` по функциональным блокам (питание, контроллер, периферия).
- Цвета узлов:
  - питание, разъёмы, LDO: `fill:#e8f5e9,stroke:#2e7d32`;
  - USB-UART: `fill:#f3e5f5,stroke:#7b1fa2`;
  - MCU: `fill:#e3f2fd,stroke:#1565c0`;
  - периферия: `fill:#fce4ec,stroke:#c62828`;
  - регуляторы: `fill:#fff3e0,stroke:#e65100`.
- Рендер PNG для показа в ответе: сжать код `zlib.compress`, закодировать `base64.urlsafe_b64encode`, скачать `https://kroki.io/mermaid/png/<encoded>`, сохранить `docs/<имя>.png`. Сеть доступна (kroki.io отвечает 200).
- Дополнительно дай ссылку на kroki для открытия в браузере.

## 3. KiCad (legacy, без шаблона)

Раньше схемы генерировались скриптом `hardware/generate_schematic.py`, но **в текущем репозитории этого файла и каталога `hardware/` нет** — режим только по явному запросу и после согласования с владельцем.

Правила формата, если генерировать с нуля:

- `(version 20240124)`, координаты в deci-mil, конвертация в mm: `_dm(v) = v * 127 / 50000`.
- Pin offset для `Device:C`, `Device:R`, `Device:L` — строго `3.81` mm (не через `_dm`).
- Питание/земля: `power:PWR_FLAG` на каждую шину, `power:GND` на землю.
- Проверка через `kicad-cli` (KiCad установлен: `/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli`):

  ```bash
  /Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli sch erc \
    <путь>/<имя>.kicad_sch --severity-all --output <путь>/ERC_<имя>.json
  ```

- Ожидаемые результаты: 0 `pin_not_connected`, 0 `power_pin_not_driven`; остаточные `wire_dangling` — известный ложный баг CLI, отмечать в отчёте.

## Отчёт

В ответе: какие файлы созданы, чем проверено (запуск, ERC/размер PNG), что осталось на ручную проверку.
