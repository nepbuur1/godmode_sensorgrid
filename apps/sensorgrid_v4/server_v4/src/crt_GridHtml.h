// by Marius Versteegen, 2025
// Grid visualization page: shows sensors 1-4 measurements as circles
// arranged in diamond patterns, with histograms and statistics tables.

#pragma once

namespace crt
{
	const char GRID_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>ESP32-S3 Grid View</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    body {
      margin: 0;
      font-family: system-ui, sans-serif;
      background: #fafafa;
    }
    nav {
      background: #333;
      padding: 0.5rem 1rem;
      display: flex;
      gap: 1.5rem;
    }
    nav a {
      text-decoration: none;
      font-weight: 600;
    }
    .page {
      max-width: 1200px;
      margin: 1.5rem auto;
      padding: 1rem;
      background: #fff;
      border: 1px solid #ddd;
    }
    h1 {
      text-align: center;
      margin-bottom: 0.5rem;
    }
    .controls {
      text-align: center;
      margin-bottom: 1rem;
      display: flex;
      gap: 0.5rem;
      justify-content: center;
    }
    .toggle-btn {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .toggle-btn.active {
      background: #333;
      color: #fff;
      border-color: #333;
    }
    .sensor-layout {
      display: grid;
      grid-template-columns: 1fr 1fr 1fr 1fr;
      gap: 0.5rem;
    }
    .sensor-widget {
      border: 1px solid #ddd;
      border-radius: 6px;
      padding: 0.4rem;
      background: #fafafa;
    }
    .sensor-widget h3 {
      text-align: center;
      margin: 0 0 0.3rem;
      font-size: 0.8rem;
    }
    .sensor-widget .no-data {
      text-align: center;
      color: #999;
      padding: 2rem 0;
    }
    .grid-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 0;
      margin: 0 auto;
    }
    .row {
      display: flex;
      gap: 2px;
      justify-content: center;
      margin-top: -1px;
    }
    .row:first-child {
      margin-top: 0;
    }
    .cell {
      width: 22px;
      height: 22px;
      border-radius: 50%;
      border: 1px solid #ccc;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 0.4rem;
      color: #888;
      transition: background 0.2s ease-out;
    }
    .histogram {
      max-width: 100%;
      margin: 0.3rem auto 0;
      display: flex;
      align-items: flex-end;
      gap: 1px;
      height: 40px;
    }
    .hist-bar {
      flex: 1;
      background: #555;
      border-radius: 1px 1px 0 0;
      transition: height 0.2s ease-out;
      min-height: 1px;
    }
    .hist-axis {
      margin: 2px auto 0;
      display: flex;
      justify-content: space-between;
      font-size: 0.55rem;
      color: #888;
    }
    .stats-table {
      margin: 0.3rem auto 0;
      border-collapse: collapse;
      width: 100%;
      font-size: 0.65rem;
    }
    .stats-table th, .stats-table td {
      border: 1px solid #ccc;
      padding: 0.15rem 0.3rem;
      text-align: center;
    }
    .stats-table th {
      background: #f0f0f0;
      font-weight: 600;
    }
    #status {
      margin-top: 1rem;
      text-align: center;
      color: #666;
    }
  </style>
</head>
<body>
  <nav>
    <a href="/" style="color:#ccc;">Home</a>
    <a href="/grid" style="color:#fff;">Grid View</a>
  </nav>
  <div class="page">
    <h1>Grid View</h1>
    <div class="controls">
      <button class="toggle-btn" id="btnNormalize" onclick="toggleNormalize()">Normalize</button>
      <button class="toggle-btn" id="btnColorize" onclick="toggleColorize()">Colorize</button>
    </div>
    <div class="sensor-layout">
      <div class="sensor-widget" id="sw1">
        <h3>Sensor 1</h3>
        <div class="grid-container" id="grid1"></div>
        <div class="histogram" id="hist1"></div>
        <div class="hist-axis"><span>0</span><span>512</span><span>1023</span></div>
        <table class="stats-table"><tr><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="max1">-</td><td id="avg1">-</td><td id="std1">-</td></tr></table>
      </div>
      <div class="sensor-widget" id="sw2">
        <h3>Sensor 2</h3>
        <div class="grid-container" id="grid2"></div>
        <div class="histogram" id="hist2"></div>
        <div class="hist-axis"><span>0</span><span>512</span><span>1023</span></div>
        <table class="stats-table"><tr><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="max2">-</td><td id="avg2">-</td><td id="std2">-</td></tr></table>
      </div>
      <div class="sensor-widget" id="sw3">
        <h3>Sensor 3</h3>
        <div class="grid-container" id="grid3"></div>
        <div class="histogram" id="hist3"></div>
        <div class="hist-axis"><span>0</span><span>512</span><span>1023</span></div>
        <table class="stats-table"><tr><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="max3">-</td><td id="avg3">-</td><td id="std3">-</td></tr></table>
      </div>
      <div class="sensor-widget" id="sw4">
        <h3>Sensor 4</h3>
        <div class="grid-container" id="grid4"></div>
        <div class="histogram" id="hist4"></div>
        <div class="hist-axis"><span>0</span><span>512</span><span>1023</span></div>
        <table class="stats-table"><tr><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="max4">-</td><td id="avg4">-</td><td id="std4">-</td></tr></table>
      </div>
    </div>
    <div id="status">...</div>
  </div>

  <script>
    const MAX_VALUE = 1023;
    const POLL_MS = 500;
    const NUM_BINS = 50;
    const SENSOR_IDS = [1, 2, 3, 4];

    let normalized = false;
    let colorized = false;

    // Per-sensor state
    const sensors = {};
    SENSOR_IDS.forEach(id => {
      sensors[id] = {
        gridEl: document.getElementById("grid" + id),
        histEl: document.getElementById("hist" + id),
        maxEl: document.getElementById("max" + id),
        avgEl: document.getElementById("avg" + id),
        stdEl: document.getElementById("std" + id),
        cells: [],
        histBars: [],
        currentCount: 0,
        lastValues: []
      };
    });
    const statusEl = document.getElementById("status");

    function toggleNormalize() {
      normalized = !normalized;
      document.getElementById("btnNormalize").classList.toggle("active", normalized);
      recolorAll();
    }
    function toggleColorize() {
      colorized = !colorized;
      document.getElementById("btnColorize").classList.toggle("active", colorized);
      recolorAll();
    }

    function computeRowSizes(n) {
      if (n <= 0) return [];
      const w = Math.ceil(Math.sqrt(n));
      const rows = [];
      let remaining = n;
      for (let r = 1; r <= w && remaining > 0; r++) {
        const s = Math.min(r, remaining);
        rows.push(s);
        remaining -= s;
      }
      for (let r = w - 1; r >= 1 && remaining > 0; r--) {
        const s = Math.min(r, remaining);
        rows.push(s);
        remaining -= s;
      }
      return rows;
    }

    function createGrid(s, count) {
      s.gridEl.innerHTML = "";
      s.cells = [];
      s.currentCount = count;
      const rowSizes = computeRowSizes(count);
      for (const size of rowSizes) {
        const rowEl = document.createElement("div");
        rowEl.className = "row";
        for (let i = 0; i < size; i++) {
          const cell = document.createElement("div");
          cell.className = "cell";
          cell.textContent = "?";
          rowEl.appendChild(cell);
          s.cells.push(cell);
        }
        s.gridEl.appendChild(rowEl);
      }
    }

    function createHistogram(s) {
      s.histEl.innerHTML = "";
      s.histBars = [];
      for (let i = 0; i < NUM_BINS; i++) {
        const bar = document.createElement("div");
        bar.className = "hist-bar";
        bar.style.height = "1px";
        s.histEl.appendChild(bar);
        s.histBars.push(bar);
      }
    }

    function updateHistogram(s, values) {
      const binWidth = Math.ceil((MAX_VALUE + 1) / NUM_BINS);
      const counts = new Array(NUM_BINS).fill(0);
      for (const v of values) {
        const bin = Math.min(Math.floor(v / binWidth), NUM_BINS - 1);
        counts[bin]++;
      }
      const maxCount = Math.max(1, ...counts);
      for (let i = 0; i < NUM_BINS; i++) {
        const pct = (counts[i] / maxCount) * 100;
        s.histBars[i].style.height = Math.max(1, pct) + "%";
      }
    }

    function updateStats(s, values) {
      if (values.length === 0) return;
      let max = values[0], sum = 0;
      for (const v of values) {
        if (v > max) max = v;
        sum += v;
      }
      const avg = sum / values.length;
      let sumSqDiff = 0;
      for (const v of values) {
        const d = v - avg;
        sumSqDiff += d * d;
      }
      const std = Math.sqrt(sumSqDiff / values.length);
      s.maxEl.textContent = max;
      s.avgEl.textContent = avg.toFixed(1);
      s.stdEl.textContent = std.toFixed(1);
    }

    function colorForValue(v, minV, maxV) {
      const lo = normalized ? minV : 0;
      const hi = normalized ? maxV : MAX_VALUE;
      const r = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / r));

      if (!colorized) {
        const gray = Math.round(255 * (1 - t));
        return { bg: `rgb(${gray},${gray},${gray})`, dark: gray < 128 };
      }
      // Color gradient: black -> blue -> green -> yellow -> red
      let cr, cg, cb;
      if (t < 0.25) {
        const p = t / 0.25;
        cr = 0; cg = 0; cb = Math.round(255 * p);
      } else if (t < 0.5) {
        const p = (t - 0.25) / 0.25;
        cr = 0; cg = Math.round(255 * p); cb = Math.round(255 * (1 - p));
      } else if (t < 0.75) {
        const p = (t - 0.5) / 0.25;
        cr = Math.round(255 * p); cg = 255; cb = 0;
      } else {
        const p = (t - 0.75) / 0.25;
        cr = 255; cg = Math.round(255 * (1 - p)); cb = 0;
      }
      const lum = 0.299 * cr + 0.587 * cg + 0.114 * cb;
      return { bg: `rgb(${cr},${cg},${cb})`, dark: lum < 128 };
    }

    function minMax(values) {
      let mn = values[0], mx = values[0];
      for (const v of values) {
        if (v < mn) mn = v;
        if (v > mx) mx = v;
      }
      return [mn, mx];
    }

    function colorCells(s) {
      if (s.lastValues.length === 0) return;
      const [mn, mx] = minMax(s.lastValues);
      s.lastValues.forEach((v, i) => {
        if (i < s.cells.length) {
          const c = colorForValue(v, mn, mx);
          s.cells[i].style.background = c.bg;
          s.cells[i].style.color = c.dark ? "#ddd" : "#444";
        }
      });
    }

    function recolorAll() {
      SENSOR_IDS.forEach(id => colorCells(sensors[id]));
    }

    async function fetchSensor(id) {
      const s = sensors[id];
      try {
        const res = await fetch("/api/measurements/" + id);
        if (!res.ok) {
          s.gridEl.innerHTML = '<div class="no-data">No data</div>';
          s.cells = [];
          s.currentCount = 0;
          s.lastValues = [];
          return;
        }
        const data = await res.json();
        if (s.currentCount !== data.count) {
          createGrid(s, data.count);
        }
        s.lastValues = data.values;
        const [mn, mx] = minMax(data.values);
        data.values.forEach((v, i) => {
          if (i < s.cells.length) {
            const c = colorForValue(v, mn, mx);
            s.cells[i].style.background = c.bg;
            s.cells[i].textContent = v;
            s.cells[i].style.color = c.dark ? "#ddd" : "#444";
          }
        });
        updateHistogram(s, data.values);
        updateStats(s, data.values);
      } catch (e) {
        // leave as-is on error
      }
    }

    async function fetchAll() {
      await Promise.all(SENSOR_IDS.map(id => fetchSensor(id)));
      statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
    }

    // Initialize
    SENSOR_IDS.forEach(id => {
      createGrid(sensors[id], 0);
      createHistogram(sensors[id]);
    });
    setInterval(fetchAll, POLL_MS);
    fetchAll();
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
