const fs = require('fs')
const path = require('path')

// Корень проекта: скрипт запускается из корня saspik-project.
// Нельзя использовать __dirname — папка opencode/ в корне является симлинком
// на saspik-private/opencode, и Node резолвит __dirname в реальную папку.
const rootDir = path.resolve(process.cwd())

// Страницы, которые не попадают в публичную сборку: пути от корня репозитория.
// Исключённые README остаются в репозитории, но в docs.html не публикуются,
// а ссылки на них в других документах выводятся простым текстом без ссылки.
const EXCLUDED = new Set([
  'saspik-iot/saspik-iot-sketches/README.md',
  'saspik-iot/saspik-iot-sketches/esp32-controller/README.md',
  'saspik-iot/saspik-iot-sketches/esp32-local-mqtt/README.md',
  'saspik-iot/saspik-iot-sketches/esp32-dht22-led/README.md',
  'saspik-iot/saspik-iot-sketches/saspik-iot-node-sensor-1/README.md',
])

const readmePaths = (fs.existsSync(path.join(rootDir, 'README.md')) ? ['README.md'] : []).concat(
  fs.readdirSync(rootDir, { withFileTypes: true })
  .filter(d => d.isDirectory() && d.name.startsWith('saspik-') && d.name !== 'saspik-private')
  .flatMap(dir => {
    const sub = path.join(rootDir, dir.name)
    const results = [dir.name + '/README.md']
    // Дополнительные подкаталоги проектов: README в software/ и saspik-cluster-app/,
    // а также все .md-файлы в docs/ (документация сервисов).
    for (const crawlDir of ['software', 'saspik-cluster-app', 'docs', 'saspik-iot-sketches']) {
      const crawlPath = path.join(sub, crawlDir)
      if (!fs.existsSync(crawlPath)) continue
      const match = crawlDir === 'docs' ? (name) => name.endsWith('.md') && !name.startsWith('wip_') : (name) => name === 'README.md'
      const walk = (rel) => {
        const abs = path.join(crawlPath, rel)
        const items = fs.readdirSync(abs, { withFileTypes: true })
        for (const item of items) {
          const relPath = (rel ? rel + '/' : '') + item.name
          if (item.isDirectory() && !item.name.startsWith('.') && item.name !== 'node_modules') {
            walk(relPath)
          } else if (match(item.name)) {
            results.push(dir.name + '/' + crawlDir + '/' + relPath)
          }
        }
      }
      walk('')
    }
    return results.filter(p => fs.existsSync(path.join(rootDir, p)) && !EXCLUDED.has(p))
  }))

const pathToAnchor = {}
readmePaths.forEach(p => {
  const dir = path.dirname(p)
  const base = dir === '.' ? 'root' : dir.replace(/\//g, '-')
  const fileBase = path.basename(p, '.md')
  pathToAnchor[p] = fileBase === 'README' ? base : base + '-' + fileBase
})

function resolveLink(href, fromDir) {
  const resolved = path.resolve(fromDir, href)
  const rel = path.relative(rootDir, resolved)
  const normalized = rel.split(path.sep).join('/')
  if (pathToAnchor[normalized]) return '#' + pathToAnchor[normalized]
  if (pathToAnchor[normalized + '/README.md']) return '#' + pathToAnchor[normalized + '/README.md']
  // Страница исключена из сборки — ссылку не рисуем, чтобы не было битых якорей.
  if (EXCLUDED.has(normalized) || EXCLUDED.has(normalized + '/README.md')) return null
  return href
}

function rewriteLinks(md, fromDir) {
  return md.replace(/\[([^\]]*)\]\(([^)]+)\)/g, (m, text, href) => {
    if (href.startsWith('http') || href.startsWith('#') || href.startsWith('mailto:')) return m
    const target = resolveLink(href, fromDir)
    if (target === null) return text
    return '[' + text + '](' + target + ')'
  })
}

const { marked } = require('marked')
marked.setOptions({ breaks: true, gfm: true })

// Подпись в левом меню = заголовок H1 страницы (без знака ⧫).
// Имя проекта не подставляется: заголовок должен совпадать со страницей.
function navLabel(p, content) {
  const h1 = (content.match(/^#\s+(.+?)\s*$/m) || [])[1]
  const fallback = p === 'README.md' ? 'Главная' : p.replace('/README.md', '').replace(/\.md$/, '')
  if (!h1) return fallback
  return h1.replace(/^⧫\s*/, '').trim()
}

const sections = readmePaths.map(p => {
  const fullPath = path.join(rootDir, p)
  const content = fs.readFileSync(fullPath, 'utf-8')
  const dir = path.dirname(fullPath)
  const anchor = pathToAnchor[p]
  return {
    anchor,
    html: marked.parse(rewriteLinks(content, dir)),
    displayName: navLabel(p, content),
    depth: p.split('/').length
  }
})

const tocItems = sections.map(s => {
  const cls = s.depth === 1 ? 'root' : s.depth === 2 ? 'l2' : 'l3'
  // Названия разделов (README проектов) — капсом, как разделы в меню.
  const label = cls === 'l2' ? s.displayName.toUpperCase() : s.displayName
  return '<a href="#' + s.anchor + '" class="' + cls + '">' + label + '</a>'
}).join('\n')

const bodyHtml = sections.map(s =>
  '<section id="' + s.anchor + '">\n' + s.html + '\n</section>'
).join('\n<hr class="section-sep">\n')

const html = '<!DOCTYPE html>\n<html lang="ru">\n<head>\n'
  + '<meta charset="UTF-8">\n'
  + '<meta name="viewport" content="width=device-width, initial-scale=1.0">\n'
  + '<title>САСПИК — документация проекта</title>\n'
  + '<style>\n'
  + '  :root { --bg: #ffffff; --fg: #1f2328; --accent: #0969da; --border: #d0d7de; --code-bg: #f6f8fa; --heading: #1f2328; }\n'
  + '  * { box-sizing: border-box; margin: 0; padding: 0; }\n'
  + '  body {\n'
  + '    font-family: -apple-system, BlinkMacSystemFont, \'Segoe UI\', Helvetica, Arial, sans-serif;\n'
  + '    background: var(--bg);\n'
  + '    color: var(--fg);\n'
  + '    line-height: 1.6;\n'
  + '    display: flex;\n'
  + '    min-height: 100vh;\n'
  + '  }\n'
  + '  nav {\n'
  + '    position: fixed;\n'
  + '    top: 0; left: 0; bottom: 0;\n'
  + '    width: 260px;\n'
  + '    background: #f6f8fa;\n'
  + '    border-right: 1px solid var(--border);\n'
  + '    padding: 20px;\n'
  + '    overflow-y: auto;\n'
  + '    z-index: 10;\n'
  + '  }\n'
  + '  nav h2 { font-size: 14px; text-transform: uppercase; color: #656d76; margin-bottom: 12px; letter-spacing: 0.5px; }\n'
  + '  nav a {\n'
  + '    display: block;\n'
  + '    color: var(--accent);\n'
  + '    text-decoration: none;\n'
  + '    font-size: 13px;\n'
  + '    padding: 3px 4px;\n'
  + '    white-space: nowrap;\n'
  + '    overflow: hidden;\n'
  + '    text-overflow: ellipsis;\n'
  + '  }\n'
  + '  nav a:hover { text-decoration: underline; }\n'
  + '  nav .root { font-weight: 800; }\n'
  + '  nav .l2 { padding-left: 16px; font-size: 12px; }\n'
  + '  nav .l3 { padding-left: 32px; font-size: 11px; }\n'
  + '  nav a.active { color: #ffffff; background: #0a69da7d; border-radius: 4px; }\n'

  + '  main {\n'
  + '    margin-left: 260px;\n'
  + '    flex: 1;\n'
  + '    padding: 40px 48px;\n'
  + '    max-width: 960px;\n'
  + '  }\n'
  + '  h1, h2, h3, h4, h5, h6 { color: var(--heading); margin-top: 24px; margin-bottom: 16px; font-weight: 600; }\n'
  + '  h1 { font-size: 2em; border-bottom: 1px solid var(--border); padding-bottom: 8px; }\n'
  + '  h2 { font-size: 1.5em; border-bottom: 1px solid var(--border); padding-bottom: 6px; }\n'
  + '  h3 { font-size: 1.25em; }\n'
  + '  a { color: var(--accent); text-decoration: none; }\n'
  + '  a:hover { text-decoration: underline; }\n'
  + '  p { margin-bottom: 16px; }\n'
  + '  ul, ol { margin-bottom: 16px; padding-left: 24px; }\n'
  + '  li { margin-bottom: 4px; }\n'
  + '  code {\n'
  + '    background: var(--code-bg);\n'
  + '    padding: 2px 6px;\n'
  + '    border-radius: 4px;\n'
  + '    font-size: 0.9em;\n'
  + '    font-family: \'SF Mono\', Monaco, \'Cascadia Code\', monospace;\n'
  + '  }\n'
  + '  pre {\n'
  + '    background: var(--code-bg);\n'
  + '    border: 1px solid var(--border);\n'
  + '    border-radius: 6px;\n'
  + '    padding: 16px;\n'
  + '    overflow-x: auto;\n'
  + '    margin-bottom: 16px;\n'
  + '  }\n'
  + '  pre code { background: none; padding: 0; font-size: 0.85em; line-height: 1.45; }\n'
  + '  table { border-collapse: collapse; margin-bottom: 16px; width: 100%; }\n'
  + '  th, td { border: 1px solid var(--border); padding: 8px 12px; text-align: left; }\n'
  + '  th { background: #f6f8fa; font-weight: 600; }\n'
  + '  blockquote { border-left: 4px solid var(--accent); padding-left: 16px; color: #656d76; margin-bottom: 16px; }\n'
  + '  hr { border: none; border-top: 1px solid var(--border); margin: 24px 0; }\n'
  + '  hr.section-sep { margin: 48px 0; border-top: 2px solid var(--accent); opacity: 0.3; }\n'
  + '  section:target { animation: highlight 2s ease; }\n'
  + '  @keyframes highlight { 0% { background: rgba(88, 166, 255, 0.08); } 100% { background: transparent; } }\n'
  + '  ::-webkit-scrollbar { width: 8px; }\n'
  + '  ::-webkit-scrollbar-track { background: var(--bg); }\n'
  + '  ::-webkit-scrollbar-thumb { background: var(--border); border-radius: 4px; }\n'
  + '  @media (max-width: 768px) {\n'
  + '    nav { position: static; width: 100%; border-right: none; border-bottom: 1px solid var(--border); }\n'
  + '    main { margin-left: 0; padding: 20px; }\n'
  + '  }\n'
  + '</style>\n</head>\n<body>\n<nav>\n  <h2>\u2B07\uFE0F САСПИК</h2>\n'
  + tocItems
  + '\n</nav>\n<main>\n'
  + bodyHtml
  + '\n</main>\n<script>\n'
  + '(function () {\n'
  + '  var links = Array.prototype.slice.call(document.querySelectorAll(\'nav a[href^="#"]\'));\n'
  + '  var targets = links.map(function (a) { return document.getElementById(a.getAttribute(\'href\').slice(1)); });\n'
  + '  function update() {\n'
  + '    var offset = 120, index = 0;\n'
  + '    for (var i = 0; i < targets.length; i++) {\n'
  + '      var el = targets[i];\n'
  + '      if (el && el.getBoundingClientRect().top - offset <= 0) index = i;\n'
  + '    }\n'
  + '    var docHeight = document.documentElement.scrollHeight;\n'
  + '    if (window.innerHeight + window.scrollY >= docHeight - 2) index = targets.length - 1;\n'
  + '    for (var j = 0; j < links.length; j++) links[j].classList.toggle(\'active\', j === index);\n'
  + '  }\n'
  + '  window.addEventListener(\'scroll\', update, { passive: true });\n'
  + '  window.addEventListener(\'resize\', update);\n'
  + '  window.addEventListener(\'hashchange\', update);\n'
  + '  update();\n'
  + '})();\n'
  + '</script>\n</body>\n</html>'

fs.writeFileSync(path.join(rootDir, 'docs.html'), html, 'utf-8')
console.log('OK: docs.html created')
