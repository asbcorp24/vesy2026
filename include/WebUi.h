#pragma once

constexpr char kFallbackIndexHtml[] PROGMEM = R"HTML(
<!doctype html>
<html lang="ru">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SmartBasket</title>
  <style>
    body { font-family: Arial, sans-serif; margin: 0; background: #f6f7fb; color: #1f2937; }
    .wrap { max-width: 860px; margin: 0 auto; padding: 24px; }
    .card { background: #fff; border-radius: 16px; padding: 20px; margin-bottom: 16px; box-shadow: 0 10px 30px rgba(0,0,0,.08); }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 12px; }
    .value { font-size: 28px; font-weight: bold; }
    .label { color: #6b7280; font-size: 14px; }
    button { padding: 10px 14px; border: 0; border-radius: 10px; background: #0f766e; color: #fff; }
  </style>
</head>
<body>
  <div class="wrap">
    <div class="card">
      <h1>SmartBasket</h1>
      <p>Загрузите файл <code>data/index.html</code> в LittleFS для полной версии интерфейса.</p>
      <button onclick="refreshStatus()">Обновить</button>
    </div>
    <div class="grid" id="stats"></div>
  </div>
  <script>
    async function refreshStatus() {
      const res = await fetch('/api/status');
      const data = await res.json();
      const stats = [
        ['Вес', `${data.totalWeight.toFixed(1)} г`],
        ['Количество', data.quantity],
        ['Температура', `${data.temperature.toFixed(1)} C`],
        ['Влажность', `${data.humidity.toFixed(1)} %`]
      ];
      document.getElementById('stats').innerHTML = stats.map(([label, value]) =>
        `<div class="card"><div class="label">${label}</div><div class="value">${value}</div></div>`
      ).join('');
    }
    refreshStatus();
  </script>
</body>
</html>
)HTML";
