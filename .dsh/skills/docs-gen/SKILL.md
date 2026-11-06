---
name: docs-gen
description: Пересобирает docs.html — единую HTML-страницу из README.md и .md-файлов docs/ всех подпроектов, с навигацией и переписанными ссылками.
whenToUse: Документация изменилась и надо обновить docs.html (GitHub Pages) либо проверить, что страница собирается.
---

# Docs Generator

Генерирует `docs.html` в корне — single-page HTML со всеми README и `.md` из `docs/` каждого подпроекта, с боковой навигацией. Файл публикуется на GitHub Pages: `https://efnushtaev.github.io/saspik-project/docs.html`. `index.html` (промо-лендинг) не перезаписывается.

Скрипт: [`scripts/generate-docs.js`](scripts/generate-docs.js).

## Что попадает в сборку

- Корневой `README.md`.
- `<проект>/README.md` для каталогов `saspik-*` (кроме `saspik-private`).
- `README.md` из `<проект>/software`, `<проект>/saspik-cluster-app`, `<проект>/saspik-iot-sketches`.
- Все `.md` из `<проект>/docs/`, кроме файлов с префиксом `wip_` (черновики).

Ссылки, указывающие внутрь сборника, переписываются в якоря `#anchor`; внешние остаются как есть.

## Запуск

Из корня репозитория (скрипт берёт `process.cwd()`). `marked@4` ставим в временную папку, чтобы не засорять репозиторий:

```bash
DEPS=$(mktemp -d /tmp/dsh-marked-XXXX)
mkdir -p "$DEPS/node_modules/marked"
curl -sL https://registry.npmjs.org/marked/-/marked-4.3.0.tgz \
  | tar xz -C "$DEPS/node_modules/marked" --strip-components=1

NODE_PATH="$DEPS/node_modules" node .dsh/skills/docs-gen/scripts/generate-docs.js
```

Ожидаемый вывод: `OK: docs.html created`.

Альтернатива — `npm install marked@4`, если npm рабочий. На этой машине системный npm 6 падает на root-owned кэше (`~/.npm`), поэтому основной способ — tarball.

Проверено: в изолированной копии дерева генератор собирает 31 секцию, `docs.html` ≈ 240 КБ.

## Проверка

1. Файл `docs.html` обновился (размер меняется, `git diff --stat docs.html`).
2. В навигации есть все проекты и новые документы.
3. Внутренние ссылки ведут на якоря (`#...`), а не на файлы.

## Ограничения

- `saspik-private` не сканируется — приватные документы в публичную страницу не попадают.
- Файлы `wip_*` не включаются: это черновики.
- Не редактируй `docs.html` вручную — только перегенерация.
