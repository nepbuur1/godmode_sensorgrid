// by Marius Versteegen, 2025
// Grid visualization page: shows sensor 1's measurements as gray-scale circles
// arranged in a diamond pattern, with a histogram of value distribution below.

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
      max-width: 720px;
      margin: 1.5rem auto;
      padding: 1rem;
      background: #fff;
      border: 1px solid #ddd;
    }
    h1, h2 {
      text-align: center;
      margin-bottom: 1rem;
    }
    h2 {
      margin-top: 2rem;
      font-size: 1.1rem;
    }
    .info {
      text-align: center;
      color: #666;
      margin-bottom: 1rem;
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
    .grid-container {
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 4px;
      margin: 0 auto;
    }
    .row {
      display: flex;
      gap: 4px;
      justify-content: center;
    }
    .cell {
      width: 48px;
      height: 48px;
      border-radius: 50%;
      border: 1px solid #ccc;
      display: flex;
      align-items: center;
      justify-content: center;
      font-size: 0.7rem;
      color: #888;
      transition: background 0.2s ease-out;
    }
    .histogram {
      max-width: 500px;
      margin: 0 auto;
      display: flex;
      align-items: flex-end;
      gap: 2px;
      height: 120px;
    }
    .hist-bar {
      flex: 1;
      background: #555;
      border-radius: 1px 1px 0 0;
      transition: height 0.2s ease-out;
      min-height: 1px;
    }
    .hist-axis {
      max-width: 500px;
      margin: 2px auto 0;
      display: flex;
      justify-content: space-between;
      font-size: 0.65rem;
      color: #888;
    }
    .stats-table {
      max-width: 400px;
      margin: 1.5rem auto 0;
      border-collapse: collapse;
      width: 100%;
    }
    .stats-table th, .stats-table td {
      border: 1px solid #ccc;
      padding: 0.4rem 0.8rem;
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
    <h1>Sensor 1 - Grid View</h1>
    <div class="info">Each circle shows one measurement. Arranged in a diamond pattern.</div>
    <div class="controls">
      <button class="toggle-btn" id="btnNormalize" onclick="toggleNormalize()">Normalize</button>
      <button class="toggle-btn" id="btnColorize" onclick="toggleColorize()">Colorize</button>
    </div>
    <div class="grid-container" id="grid"></div>
    <h2>Value Distribution</h2>
    <div class="histogram" id="histogram"></div>
    <div class="hist-axis"><span>0</span><span>256</span><span>512</span><span>768</span><span>1023</span></div>
    <h2>Statistics</h2>
    <table class="stats-table">
      <tr><th>max</th><th>average</th><th>sqrt(var)</th></tr>
      <tr><td id="statMax">-</td><td id="statAvg">-</td><td id="statStd">-</td></tr>
    </table>
    <div id="status">...</div>
  </div>

  <script>
    const MAX_VALUE = 1023;
    const POLL_MS = 500;
    const NUM_BINS = 50;

    const gridEl = document.getElementById("grid");
    const histEl = document.getElementById("histogram");
    const statMaxEl = document.getElementById("statMax");
    const statAvgEl = document.getElementById("statAvg");
    const statStdEl = document.getElementById("statStd");
    const statusEl = document.getElementById("status");
    let cells = [];
    let currentCount = 0;
    let histBars = [];

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

    function createGrid(count) {
      gridEl.innerHTML = "";
      cells = [];
      currentCount = count;
      const rowSizes = computeRowSizes(count);
      for (const size of rowSizes) {
        const rowEl = document.createElement("div");
        rowEl.className = "row";
        for (let i = 0; i < size; i++) {
          const cell = document.createElement("div");
          cell.className = "cell";
          cell.textContent = "?";
          rowEl.appendChild(cell);
          cells.push(cell);
        }
        gridEl.appendChild(rowEl);
      }
    }

    function createHistogram() {
      histEl.innerHTML = "";
      histBars = [];
      for (let i = 0; i < NUM_BINS; i++) {
        const bar = document.createElement("div");
        bar.className = "hist-bar";
        bar.style.height = "1px";
        histEl.appendChild(bar);
        histBars.push(bar);
      }
    }

    function updateHistogram(values) {
      const binWidth = Math.ceil((MAX_VALUE + 1) / NUM_BINS);
      const counts = new Array(NUM_BINS).fill(0);
      for (const v of values) {
        const bin = Math.min(Math.floor(v / binWidth), NUM_BINS - 1);
        counts[bin]++;
      }
      const maxCount = Math.max(1, ...counts);
      for (let i = 0; i < NUM_BINS; i++) {
        const pct = (counts[i] / maxCount) * 100;
        histBars[i].style.height = Math.max(1, pct) + "%";
      }
    }

    function updateStats(values) {
      if (values.length === 0) return;
      let max = values[0];
      let sum = 0;
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
      const variance = sumSqDiff / values.length;
      const std = Math.sqrt(variance);
      statMaxEl.textContent = max;
      statAvgEl.textContent = avg.toFixed(1);
      statStdEl.textContent = std.toFixed(1);
    }

    let normalized = false;
    let colorized = false;
    let lastValues = [];

    function toggleNormalize() {
      normalized = !normalized;
      document.getElementById("btnNormalize").classList.toggle("active", normalized);
      recolor();
    }
    function toggleColorize() {
      colorized = !colorized;
      document.getElementById("btnColorize").classList.toggle("active", colorized);
      recolor();
    }

    function colorForValue(v, minV, maxV) {
      // Map v to 0..1 range
      const range = (maxV > minV) ? (maxV - minV) : 1;
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
        const s = t / 0.25;
        cr = 0; cg = 0; cb = Math.round(255 * s);
      } else if (t < 0.5) {
        const s = (t - 0.25) / 0.25;
        cr = 0; cg = Math.round(255 * s); cb = Math.round(255 * (1 - s));
      } else if (t < 0.75) {
        const s = (t - 0.5) / 0.25;
        cr = Math.round(255 * s); cg = 255; cb = 0;
      } else {
        const s = (t - 0.75) / 0.25;
        cr = 255; cg = Math.round(255 * (1 - s)); cb = 0;
      }
      const lum = 0.299 * cr + 0.587 * cg + 0.114 * cb;
      return { bg: `rgb(${cr},${cg},${cb})`, dark: lum < 128 };
    }

    function recolor() {
      if (lastValues.length === 0) return;
      let minV = lastValues[0], maxV = lastValues[0];
      for (const v of lastValues) {
        if (v < minV) minV = v;
        if (v > maxV) maxV = v;
      }
      lastValues.forEach((v, i) => {
        if (i < cells.length) {
          const c = colorForValue(v, minV, maxV);
          cells[i].style.background = c.bg;
          cells[i].style.color = c.dark ? "#ddd" : "#444";
        }
      });
    }

    async function fetchMeasurements() {
      try {
        const res = await fetch("/api/measurements/1");
        if (!res.ok) throw new Error(res.status);
        const data = await res.json();

        if (currentCount !== data.count) {
          createGrid(data.count);
        }

        lastValues = data.values;
        let minV = data.values[0], maxV = data.values[0];
        for (const v of data.values) {
          if (v < minV) minV = v;
          if (v > maxV) maxV = v;
        }

        data.values.forEach((v, i) => {
          if (i < cells.length) {
            const c = colorForValue(v, minV, maxV);
            cells[i].style.background = c.bg;
            cells[i].textContent = v;
            cells[i].style.color = c.dark ? "#ddd" : "#444";
          }
        });

        updateHistogram(data.values);
        updateStats(data.values);

        statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
      } catch (e) {
        statusEl.textContent = "Fout: " + e.message;
      }
    }

    createGrid(50);
    createHistogram();
    setInterval(fetchMeasurements, POLL_MS);
    fetchMeasurements();
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
