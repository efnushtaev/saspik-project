#!/usr/bin/env node
// Генератор интерактивной страницы статуса: status.html в корне репозитория.
// Источники: IN_PROGRESS.md проектов и TODO.md. Без зависимостей.
// Запуск из корня: node .dsh/skills/project-status/scripts/generate-status.js

const fs = require('fs')
const path = require('path')

const rootDir = path.resolve(process.cwd())

const PROJECTS = [
  { key: 'meshrover', name: 'saspik-meshrover', emoji: '🤖', journal: 'saspik-meshrover/IN_PROGRESS.md' },
  { key: 'iot', name: 'saspik-iot', emoji: '📡', journal: 'saspik-iot/IN_PROGRESS.md' },
  { key: 'cluster', name: 'saspik-cluster', emoji: '🗄️', journal: 'saspik-cluster/IN_PROGRESS.md' },
  { key: 'greenhouse', name: 'saspik-greenhouse', emoji: '🌱', journal: 'saspik-greenhouse/IN_PROGRESS.md' }
]

const BADGES = {
  'В работе': 'busy',
  'На доработку': 'rework',
  'Нужно взять в работу': 'todo'
}

function parseJournal(text) {
  const lines = text.split('\n')
  const declared = (text.match(/>\s*Незавершённых задач:\s*(\d+)/) || [])[1]
  const tasks = []
  let current = null

  for (const line of lines) {
    const task = line.match(/^- \[([^\]]+)\]\s+(.+?)\s*$/)
    if (task) {
      current = { tag: task[1], title: task[2], status: '', items: [] }
      tasks.push(current)
      continue
    }
    if (!current) continue
    const status = line.match(/^\s{2}Статус:\s*(.+?)\s*$/)
    if (status) {
      current.status = status[1]
      continue
    }
    const item = line.match(/^\s{2}-\s+(.+?)\s*$/)
    if (item) current.items.push(item[1])
  }

  return {
    declared: declared === undefined ? null : Number(declared),
    unfinished: tasks.filter(t => t.tag !== 'Выполнено'),
    tasks
  }
}

function parseTodo(text) {
  const sections = {}
  let section = null
  let current = null
  for (const line of text.split('\n')) {
    const heading = line.match(/^###\s+(.+?)\s*$/) || line.match(/^##\s+(.+?)\s*$/)
    if (heading) {
      section = heading[1]
      sections[section] = sections[section] || []
      current = null
      continue
    }
    const item = line.match(/^- \[ \]\s+(.+?)\s*$/)
    if (item && section) {
      current = { title: item[1], notes: [] }
      sections[section].push(current)
      continue
    }
    // Подстроки задачи с отступом: «Зачем:», «Готово, когда:» и прочие пояснения.
    const note = line.match(/^\s{2}(\S.+?)\s*$/)
    if (note && current) current.notes.push(note[1])
  }
  return sections
}

function esc(s) {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
}

// Мини-разметка: `код`, **жирный**, [текст](ссылка).
function inline(s) {
  return esc(s)
    .replace(/`([^`]+)`/g, '<code>$1</code>')
    .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>')
}

function todoSectionFor(sections, key) {
  const name = Object.keys(sections).find(k => k.toLowerCase().includes(key))
  return name ? sections[name] : []
}

const projects = PROJECTS.map(p => {
  const journalPath = path.join(rootDir, p.journal)
  const text = fs.existsSync(journalPath) ? fs.readFileSync(journalPath, 'utf-8') : ''
  return { ...p, ...parseJournal(text), backlog: [] }
})

const todoPath = path.join(rootDir, 'TODO.md')
const todo = fs.existsSync(todoPath) ? parseTodo(fs.readFileSync(todoPath, 'utf-8')) : {}
projects.forEach(p => { p.backlog = todoSectionFor(todo, p.key) })
const other = todoSectionFor(todo, 'прочее')

const today = new Date().toLocaleDateString('ru-RU')

const rows = projects.map(p => (
  '<tr><td class="proj">' + p.emoji + ' ' + p.name + '</td>'
  + '<td class="num" data-lab="незавершённых">' + p.unfinished.length + '</td>'
  + '<td class="num" data-lab="бэклог">' + p.backlog.length + '</td></tr>'
)).join('')

const cards = projects.map(p => {
  const warn = p.declared !== null && p.declared !== p.unfinished.length
    ? '<p class="warn">⚠ счётчик расходится: в файле ' + p.declared + ', фактически ' + p.unfinished.length + '</p>'
    : ''
  const tasks = p.unfinished.length
    ? p.unfinished.map((t, i) => {
      const body = (t.status ? '<p class="status">' + inline(t.status) + '</p>' : '')
        + (t.items.length ? '<ul>' + t.items.map(x => '<li>' + inline(x) + '</li>').join('') + '</ul>' : '')
      return '<details class="task">'
        + '<summary><span class="badge ' + (BADGES[t.tag] || 'todo') + '">' + t.tag + '</span> ' + inline(t.title) + '</summary>'
        + body + '</details>'
    }).join('')
    : ''
  const backlog = p.backlog.length
    ? '<details class="backlog"><summary><strong>Бэклог · ' + p.backlog.length + '</strong></summary><ul>'
      + p.backlog.map(x => '<li>' + inline(x.title)
        + (x.notes.length ? '<p class="note">' + x.notes.map(inline).join('<br>') + '</p>' : '')
        + '</li>').join('') + '</ul></details>'
    : '<p class="empty">бэклог пуст</p>'

  return '<section class="card"><h2>' + p.name + '</h2>'
    + '<p class="counter">Незавершённых: <strong>' + p.unfinished.length + '</strong></p>'
    + warn + tasks + backlog + '</section>'
}).join('\n')

const otherBlock = other.length
  ? '<section class="card"><h2>Прочее</h2><ul>' + other.map(x => '<li>' + inline(x.title)
    + (x.notes.length ? '<p class="note">' + x.notes.map(inline).join('<br>') + '</p>' : '')
    + '</li>').join('') + '</ul></section>'
  : ''

const html = '<!DOCTYPE html>\n<html lang="ru">\n<head>\n<meta charset="UTF-8">\n'
  + '<meta name="viewport" content="width=device-width, initial-scale=1.0">\n'
  + '<meta http-equiv="Cache-Control" content="no-store">\n'
  + '<title>Статус проектов · САСПИК</title>\n<style>\n'
  + ':root { --bg:#0d1117; --fg:#c9d1d9; --muted:#7d8590; --accent:#58a6ff; --line:#21262d; --chip:#161b22; }\n'
  + '* { box-sizing:border-box; margin:0; padding:0; }\n'
  + 'body { font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Helvetica,Arial,sans-serif; color:var(--fg); background:var(--bg); font-size:16px; line-height:1.55; padding:32px; }\n'
  + 'main { max-width:760px; margin:0 auto; }\n'
  + 'h1 { font-size:1.35em; font-weight:600; letter-spacing:0.02em; margin-bottom:4px; }\n'
  + '.meta { color:var(--muted); font-size:13.5px; margin-bottom:26px; }\n'
  + 'table { border-collapse:collapse; width:100%; margin-bottom:30px; font-size:14.5px; }\n'
  + 'th, td { padding:10px 0; border-bottom:1px solid var(--line); text-align:left; font-weight:400; }\n'
  + 'th { color:var(--muted); font-size:11px; text-transform:uppercase; letter-spacing:0.08em; border-bottom:none; }\n'
  + '.num { text-align:right; width:110px; font-variant-numeric:tabular-nums; }\n'
  + '.card { padding:32px 0; border-bottom:1px solid #3d444d; }\n'
  + '.card:last-of-type { border-bottom:none; }\n'
  + '.card:last-of-type ul { margin-left:16px; }\n'
  + '.card h2 { font-size:20px; font-weight:600; text-transform:uppercase; letter-spacing:0.08em; color:var(--muted); margin-bottom:0px; }\n'
  + '.counter { color:var(--muted); font-size:13.5px; margin-bottom:8px; }\n'
  + 'details { margin:2px 0; }\n'
  + 'summary { cursor:pointer; padding:5px 0; list-style:none; line-height:1.7; }\n'
  + 'summary::-webkit-details-marker { display:none; }\n'
  + 'summary::before { content:"\\203A"; display:inline-block; width:16px; color:var(--muted); }\n'
  + 'details[open] > summary::before { content:"\\2304"; }\n'
  + 'summary:hover { color:var(--accent); }\n'
  + '.status { color:var(--muted); font-size:13.5px; margin:2px 0 8px 16px; }\n'
  + '.task ul { margin:4px 0 10px 32px; font-size:14px; color:var(--muted); }\n'
  + '.backlog ul { margin:4px 0 10px 32px; font-size:14px; }\n'
  + '.backlog { margin-top:0; }\n'
  + '.backlog summary { color:var(--muted); font-size:13.5px; }\n'
  + '.note { color:var(--muted); font-size:13px; line-height:1.6; margin-top:4px; }\n'
  + '.badge { font-size:11px; text-transform:uppercase; letter-spacing:0.07em; margin-right:2px; border-radius:4px; padding:2px 4px; border:1px solid; }\n'
  + '.busy { color:var(--accent); }\n'
  + '.rework { color:#d29922; }\n'
  + '.todo { color:#a371f7; }\n'
  + '.empty { color:var(--muted); font-size:13.5px; font-style:italic; }\n'
  + '.warn { color:#f85149; font-size:13.5px; margin-bottom:8px; }\n'
  + 'code { background:var(--chip); color:var(--fg); padding:1px 4px; border-radius:3px; font-size:0.95em; font-family:"SF Mono",ui-monospace,monospace; }\n'
  + 'a { color:var(--accent); text-decoration:none; }\n'
  + 'a:hover { text-decoration:underline; }\n'
  + 'footer { color:var(--muted); font-size:12.5px; margin-top:28px; }\n'
  + '@media (max-width:620px) {\n'
  + '  body { padding:18px 16px; font-size:15.5px; }\n'
  + '  h1 { font-size:1.2em; }\n'
  + '  .meta { font-size:12.5px; margin-bottom:20px; }\n'
  + '  table { display:block; margin-bottom:22px; }\n'
  + '  thead, tbody, tr, th, td { display:block; }\n'
  + '  thead { display:none; }\n'
  + '  tr { display:grid; grid-template-columns:1fr auto auto; gap:12px; align-items:baseline; padding:10px 0; border-bottom:1px solid var(--line); }\n'
  + '  td { border:none; padding:0; font-size:14.5px; }\n'
  + '  td.num { text-align:right; width:auto; }\n'
  + '  td.num::before { content:attr(data-lab); display:block; font-size:10px; letter-spacing:0.06em; text-transform:uppercase; color:var(--muted); }\n'
  + '  .card h2 { font-size:16px; margin-bottom:0px; }\n'
  + '  .status { margin-left:14px; }\n'
  + '  .task ul, .backlog ul { margin-left:22px; font-size:13.5px; }\n'
  + '  footer { font-size:11.5px; }\n'
  + '}\n'
  + '</style>\n</head>\n<body>\n<main>\n'
  + '<h1>Статус проектов · САСПИК</h1>\n'
  + '<p class="meta">' + today + ' · источники: журналы IN_PROGRESS.md, бэклог TODO.md</p>\n'
  + '<table><tr><th>Проект</th><th class="num">Незавершённых</th><th class="num">Бэклог</th></tr>' + rows + '</table>\n'
  + cards + '\n' + otherBlock + '\n'
  + '<footer>Сгенерировано ' + new Date().toLocaleString('ru-RU') + ' · пересобрать: node .dsh/skills/project-status/scripts/generate-status.js</footer>\n'
  + '</main>\n</body>\n</html>'

fs.writeFileSync(path.join(rootDir, 'status.html'), html, 'utf-8')
console.log('OK: status.html created')
