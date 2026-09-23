#pragma once

static const char INDEX_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>SASPIK Device Setup</title>
<style>
  * { box-sizing: border-box; }
  body {
    font-family: -apple-system, Segoe UI, Roboto, sans-serif;
    background: #0f172a;
    color: #e2e8f0;
    margin: 0;
    display: flex;
    justify-content: center;
    padding: 32px 16px;
  }
  .card {
    background: #1e293b;
    border-radius: 16px;
    padding: 28px;
    width: 100%;
    max-width: 480px;
    box-shadow: 0 20px 50px rgba(0,0,0,.4);
  }
  h1 { margin: 0 0 4px; font-size: 22px; }
  p.sub { color: #94a3b8; margin: 0 0 24px; font-size: 14px; }
  label { display: block; font-size: 13px; color: #94a3b8; margin: 14px 0 6px; }
  input, select {
    width: 100%;
    padding: 10px 12px;
    border-radius: 8px;
    border: 1px solid #334155;
    background: #0f172a;
    color: #e2e8f0;
    font-size: 15px;
  }
  input:focus, select:focus { outline: 2px solid #38bdf8; border-color: transparent; }
  .row { display: flex; gap: 10px; }
  .row > div { flex: 1; }
  button {
    width: 100%;
    margin-top: 22px;
    padding: 12px;
    border: none;
    border-radius: 8px;
    background: #38bdf8;
    color: #082f49;
    font-size: 16px;
    font-weight: 700;
    cursor: pointer;
  }
  button.secondary {
    background: #334155;
    color: #e2e8f0;
    margin-top: 10px;
    font-weight: 500;
  }
  button.danger { background: #f87171; color: #450a0a; margin-top: 10px; font-weight: 500; }
  .scan-btn {
    background: #475569;
    color: #e2e8f0;
    margin-top: 6px;
    padding: 8px;
    font-weight: 500;
  }
  #status { display: none; margin-top: 14px; padding: 10px; border-radius: 8px; font-size: 14px; }
  #status.ok { background: #14532d; color: #bbf7d0; }
  #status.err { background: #7f1d1d; color: #fecaca; }
</style>
</head>
<body>
<div class="card">
  <h1>SASPIK Device Setup</h1>
  <p class="sub">Настройка Wi-Fi и MQTT без перепрошивки</p>

  <form id="cfg" action="/save" method="POST">
    <label for="ssid">Wi-Fi сеть</label>
    <select id="ssid" name="ssid" required>
      <option value="">— выберите или введите —</option>
    </select>
    <button type="button" class="scan-btn" onclick="scanWifi()">Сканировать Wi-Fi</button>

    <label for="pass">Пароль Wi-Fi</label>
    <input type="password" id="pass" name="pass" placeholder="••••••••">

    <label for="mqtt_host">MQTT брокер (хост)</label>
    <input type="text" id="mqtt_host" name="mqtt_host" placeholder="192.168.0.101" required>

    <div class="row">
      <div>
        <label for="mqtt_port">MQTT порт</label>
        <input type="number" id="mqtt_port" name="mqtt_port" value="1883">
      </div>
      <div>
        <label for="mqtt_user">MQTT логин</label>
        <input type="text" id="mqtt_user" name="mqtt_user">
      </div>
    </div>

    <label for="mqtt_pass">MQTT пароль</label>
    <input type="password" id="mqtt_pass" name="mqtt_pass">

    <button type="submit">Сохранить и перезагрузить</button>
  </form>

  <button type="button" class="danger" onclick="resetConfig()">Сбросить настройки</button>

  <div id="status"></div>
</div>

<script>
function setStatus(msg, ok) {
  var s = document.getElementById('status');
  s.style.display = 'block';
  s.className = ok ? 'ok' : 'err';
  s.textContent = msg;
}

function scanWifi() {
  setStatus('Сканирование...', true);
  fetch('/scan').then(function(r){ return r.json(); }).then(function(nets){
    var sel = document.getElementById('ssid');
    var current = sel.value;
    sel.innerHTML = '<option value="">— выберите или введите —</option>';
    nets.forEach(function(n){
      var o = document.createElement('option');
      o.value = n.ssid;
      o.textContent = n.ssid + ' (' + n.rssi + ' dBm)';
      sel.appendChild(o);
    });
    if (current) sel.value = current;
    setStatus('Найдено сетей: ' + nets.length, true);
  }).catch(function(){ setStatus('Не удалось отсканировать', false); });
}

function resetConfig() {
  if (!confirm('Очистить сохранённые настройки и перезагрузить?')) return;
  var f = document.createElement('form');
  f.method = 'POST';
  f.action = '/reset';
  document.body.appendChild(f);
  f.submit();
}

scanWifi();
</script>
</body>
</html>
)rawliteral";

static const char SUCCESS_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Настройки сохранены</title>
<style>
  * { box-sizing: border-box; }
  body {
    font-family: -apple-system, Segoe UI, Roboto, sans-serif;
    background: #0f172a;
    color: #e2e8f0;
    margin: 0;
    display: flex;
    justify-content: center;
    align-items: center;
    min-height: 100vh;
    padding: 32px 16px;
  }
  .card {
    background: #1e293b;
    border-radius: 16px;
    padding: 28px;
    width: 100%;
    max-width: 480px;
    text-align: center;
    box-shadow: 0 20px 50px rgba(0,0,0,.4);
  }
  .check {
    width: 72px;
    height: 72px;
    margin: 0 auto 18px;
    border-radius: 50%;
    background: #14532d;
    color: #bbf7d0;
    font-size: 38px;
    line-height: 72px;
    font-weight: 700;
  }
  h1 { margin: 0 0 10px; font-size: 22px; }
  p { color: #94a3b8; font-size: 15px; line-height: 1.5; margin: 0; }
  .ssid {
    display: inline-block;
    margin-top: 16px;
    padding: 6px 14px;
    border-radius: 8px;
    background: #0f172a;
    border: 1px solid #334155;
    color: #e2e8f0;
    font-size: 14px;
  }
</style>
</head>
<body>
<div class="card">
  <div class="check">✓</div>
  <h1>Настройки сохранены</h1>
  <p>Устройство перезагружается.<br>Подключитесь обратно к своей Wi-Fi сети и проверьте индикацию.</p>
  <div class="ssid">Сеть: %SSID%</div>
</div>
</body>
</html>
)rawliteral";
