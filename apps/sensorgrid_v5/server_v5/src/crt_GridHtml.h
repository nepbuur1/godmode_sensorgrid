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
      margin-bottom: 1rem;
      display: flex;
      gap: 1rem;
      justify-content: center;
      align-items: flex-start;
    }
    .ctrl-buttons {
      display: flex;
      flex-wrap: wrap;
      gap: 0.4rem;
    }
    .ctrl-fields {
      display: flex;
      flex-direction: column;
      gap: 0.3rem;
      font-size: 0.75rem;
    }
    .ctrl-fields label {
      display: flex;
      align-items: center;
      gap: 0.3rem;
    }
    .ctrl-fields input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 70px;
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
    .offset-btn {
      display: block;
      margin: 0 auto 0.3rem;
      font-size: 0.7rem;
      padding: 0.2rem 0.5rem;
    }
    .sensor-widget .no-data {
      text-align: center;
      color: #999;
      padding: 2rem 0;
    }
    .grid-container {
      display: flex;
      flex-direction: column;
      align-items: flex-start;
      gap: 0;
      margin: 0 auto;
      width: fit-content;
    }
    .row {
      display: flex;
      gap: 2px;
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
    .running-plot {
      display: block;
      width: 100%;
      height: 120px;
      margin: 0.3rem auto 0;
      border: 1px solid #ccc;
      background: #111;
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
      <div class="ctrl-buttons">
        <button class="toggle-btn" id="btnNormalize" onclick="toggleNormalize()">Norm Display</button>
        <button class="toggle-btn" id="btnMaxFixed" onclick="toggleMaxFixed()">MaxFixed Display</button>
        <button class="toggle-btn" id="btnColorize" onclick="toggleColorize()">Color Display</button>
        <button class="toggle-btn" onclick="doCapture()">Capture</button>
        <button class="toggle-btn" id="btnNormMaxCap" onclick="toggleNormMaxCap()">Norm MaxCap</button>
        <button class="toggle-btn" id="btnNormSumCap" onclick="toggleNormSumCap()">Norm SumCap</button>
      </div>
      <div class="ctrl-fields">
        <label>maxCaptured <input type="text" id="fldMaxCap" readonly value="-" size="8"/></label>
        <label>maxSumCaptured <input type="text" id="fldSumCap" readonly value="-" size="8"/></label>
        <label>Calibrate Grams <input type="text" id="fldCalGrams" value="1000" size="8"/></label>
        <label>Fixed Max Display <input type="text" id="fldFixedMax" value="2500" size="8"/></label>
        <label>Stats Filter <input type="text" id="fldStatsFilter" value="0.9" size="8"/></label>
      </div>
    </div>
    <div class="sensor-layout">
      <div class="sensor-widget" id="sw1">
        <h3>Sensor 1</h3>
        <button class="toggle-btn offset-btn" onclick="removeOffset(1)">Remove offset</button>
        <div class="grid-container" id="grid1"></div>
        <div class="histogram" id="hist1"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum1">-</td><td id="max1">-</td><td id="avg1">-</td><td id="std1">-</td></tr></table>
        <canvas class="running-plot" id="plot1" width="250" height="120"></canvas>
      </div>
      <div class="sensor-widget" id="sw2">
        <h3>Sensor 2</h3>
        <button class="toggle-btn offset-btn" onclick="removeOffset(2)">Remove offset</button>
        <div class="grid-container" id="grid2"></div>
        <div class="histogram" id="hist2"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum2">-</td><td id="max2">-</td><td id="avg2">-</td><td id="std2">-</td></tr></table>
        <canvas class="running-plot" id="plot2" width="250" height="120"></canvas>
      </div>
      <div class="sensor-widget" id="sw3">
        <h3>Sensor 3</h3>
        <button class="toggle-btn offset-btn" onclick="removeOffset(3)">Remove offset</button>
        <div class="grid-container" id="grid3"></div>
        <div class="histogram" id="hist3"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum3">-</td><td id="max3">-</td><td id="avg3">-</td><td id="std3">-</td></tr></table>
        <canvas class="running-plot" id="plot3" width="250" height="120"></canvas>
      </div>
      <div class="sensor-widget" id="sw4">
        <h3>Sensor 4</h3>
        <button class="toggle-btn offset-btn" onclick="removeOffset(4)">Remove offset</button>
        <div class="grid-container" id="grid4"></div>
        <div class="histogram" id="hist4"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum4">-</td><td id="max4">-</td><td id="avg4">-</td><td id="std4">-</td></tr></table>
        <canvas class="running-plot" id="plot4" width="250" height="120"></canvas>
      </div>
    </div>
    <div id="status">...</div>
  </div>

  <script>
    const MAX_VALUE = 65535;
    const POLL_MS = 100;
    const NUM_BINS = 50;
    const SENSOR_IDS = [1, 2, 3, 4];

    let normalized = false;
    let maxFixed = false;
    let colorized = false;
    let normMaxCap = false;
    let normSumCap = false;
    let maxCaptured = null;
    let maxSumCaptured = null;

    // Per-sensor state
    const sensors = {};
    SENSOR_IDS.forEach(id => {
      sensors[id] = {
        gridEl: document.getElementById("grid" + id),
        histEl: document.getElementById("hist" + id),
        sumEl: document.getElementById("sum" + id),
        maxEl: document.getElementById("max" + id),
        avgEl: document.getElementById("avg" + id),
        stdEl: document.getElementById("std" + id),
        cells: [],
        histBars: [],
        currentCount: 0,
        lastValues: [],
        offsets: null,
        filteredSum: null,
        filteredMax: null,
        filteredAvg: null,
        filteredStd: null,
        plotCanvas: document.getElementById("plot" + id),
        plotHistory: []
      };
    });
    const statusEl = document.getElementById("status");

    function toggleNormalize() {
      normalized = !normalized;
      if (normalized && maxFixed) {
        maxFixed = false;
        document.getElementById("btnMaxFixed").classList.remove("active");
      }
      document.getElementById("btnNormalize").classList.toggle("active", normalized);
      recolorAll();
    }
    function toggleMaxFixed() {
      maxFixed = !maxFixed;
      if (maxFixed && normalized) {
        normalized = false;
        document.getElementById("btnNormalize").classList.remove("active");
      }
      document.getElementById("btnMaxFixed").classList.toggle("active", maxFixed);
      recolorAll();
    }
    function toggleColorize() {
      colorized = !colorized;
      document.getElementById("btnColorize").classList.toggle("active", colorized);
      recolorAll();
    }
    function getCalGrams() {
      return parseFloat(document.getElementById("fldCalGrams").value) || 1000;
    }
    function getFixedMax() {
      return parseFloat(document.getElementById("fldFixedMax").value) || 2500;
    }
    function doCapture() {
      // Find the sensor grid with the maximum individual measurement value
      // Only consider non-stub sensors (those where "Remove offset" was pressed)
      let bestId = null;
      let bestMax = -1;
      SENSOR_IDS.forEach(id => {
        const s = sensors[id];
        if (!s.offsets) return;
        if (s.lastValues.length === 0) return;
        const vals = s.lastValues.map((v, i) => Math.max(0, v - (s.offsets[i] || 0)));
        for (const v of vals) {
          if (v > bestMax) {
            bestMax = v;
            bestId = id;
          }
        }
      });
      if (bestId === null) return;
      const s = sensors[bestId];
      const vals = s.lastValues.map((v, i) => Math.max(0, v - (s.offsets[i] || 0)));
      maxCaptured = bestMax;
      let sum = 0;
      for (const v of vals) sum += v;
      maxSumCaptured = sum;
      document.getElementById("fldMaxCap").value = maxCaptured;
      document.getElementById("fldSumCap").value = maxSumCaptured;
      recolorAll();
    }
    function toggleNormMaxCap() {
      normMaxCap = !normMaxCap;
      if (normMaxCap) {
        normSumCap = false;
        document.getElementById("btnNormSumCap").classList.remove("active");
      }
      document.getElementById("btnNormMaxCap").classList.toggle("active", normMaxCap);
      recolorAll();
    }
    function toggleNormSumCap() {
      normSumCap = !normSumCap;
      if (normSumCap) {
        normMaxCap = false;
        document.getElementById("btnNormMaxCap").classList.remove("active");
      }
      document.getElementById("btnNormSumCap").classList.toggle("active", normSumCap);
      recolorAll();
    }

    function removeOffset(id) {
      const s = sensors[id];
      if (s.lastValues.length > 0) {
        s.offsets = [...s.lastValues];
      }
    }

    const COLS = 8;

    function computeRowSizes(n) {
      if (n <= 0) return [];
      const rows = [];
      let remaining = n;
      while (remaining > 0) {
        const s = Math.min(COLS, remaining);
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
      rowSizes.forEach((size, rowIdx) => {
        const rowEl = document.createElement("div");
        rowEl.className = "row";
        if (rowIdx % 2 === 1) {
          rowEl.style.marginLeft = "12px";
        }
        for (let i = 0; i < size; i++) {
          const cell = document.createElement("div");
          cell.className = "cell";
          cell.textContent = "?";
          rowEl.appendChild(cell);
          s.cells.push(cell);
        }
        s.gridEl.appendChild(rowEl);
      });
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

    function getStatsFilter() {
      const v = parseFloat(document.getElementById("fldStatsFilter").value);
      if (isNaN(v) || v < 0) return 0;
      if (v > 1) return 1;
      return v;
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
      if (s.filteredSum === null) {
        s.filteredSum = sum;
        s.filteredMax = max;
        s.filteredAvg = avg;
        s.filteredStd = std;
      } else {
        const f = getStatsFilter();
        s.filteredSum = s.filteredSum * f + sum * (1 - f);
        s.filteredMax = s.filteredMax * f + max * (1 - f);
        s.filteredAvg = s.filteredAvg * f + avg * (1 - f);
        s.filteredStd = s.filteredStd * f + std * (1 - f);
      }
      s.sumEl.textContent = s.filteredSum.toFixed(1);
      s.maxEl.textContent = s.filteredMax.toFixed(1);
      s.avgEl.textContent = s.filteredAvg.toFixed(1);
      s.stdEl.textContent = s.filteredStd.toFixed(1);
    }

    const PLOT_SLOTS = 20;

    function getPlotMax() {
      if (normalized) return null;
      if (maxFixed) return getFixedMax();
      return MAX_VALUE;
    }

    function updatePlot(s, displayValues) {
      if (displayValues.length === 0) return;
      const sorted = [...displayValues].sort((a, b) => b - a);
      const top3 = [sorted[0], sorted[Math.min(1, sorted.length - 1)], sorted[Math.min(2, sorted.length - 1)]];
      s.plotHistory.push(top3);
      if (s.plotHistory.length > PLOT_SLOTS) s.plotHistory.shift();

      const canvas = s.plotCanvas;
      const ctx = canvas.getContext("2d");
      const w = canvas.width;
      const h = canvas.height;
      ctx.clearRect(0, 0, w, h);

      let yMax = getPlotMax();
      if (yMax === null || yMax <= 0) {
        yMax = 1;
        for (const entry of s.plotHistory) {
          for (const v of entry) {
            if (v > yMax) yMax = v;
          }
        }
      }

      const colors = ["#ff3333", "#33cc33", "#3399ff"];
      const len = s.plotHistory.length;
      const xStep = len > 1 ? w / (PLOT_SLOTS - 1) : 0;
      const xOff = (PLOT_SLOTS - len) * xStep;

      for (let c = 0; c < 3; c++) {
        ctx.beginPath();
        ctx.strokeStyle = colors[c];
        ctx.lineWidth = 1.5;
        for (let i = 0; i < len; i++) {
          const x = xOff + i * xStep;
          const y = h - (s.plotHistory[i][c] / yMax) * h;
          if (i === 0) ctx.moveTo(x, y);
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }
    }

    function colorForValue(v, minV, maxV) {
      const lo = normalized ? minV : 0;
      const hi = normalized ? maxV : (maxFixed ? getFixedMax() : MAX_VALUE);
      const r = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / r));

      if (!colorized) {
        const gray = Math.round(255 * t);
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

    function getDisplayValues(s) {
      if (s.lastValues.length === 0) return [];
      const offsetted = s.offsets
        ? s.lastValues.map((v, i) => Math.max(0, v - (s.offsets[i] || 0)))
        : s.lastValues;
      if (normMaxCap && maxCaptured != null && maxCaptured > 0) {
        const cg = getCalGrams();
        return offsetted.map(v => cg * v / maxCaptured);
      }
      if (normSumCap && maxSumCaptured != null && maxSumCaptured > 0) {
        const cg = getCalGrams();
        return offsetted.map(v => cg * v / maxSumCaptured);
      }
      return offsetted;
    }

    function colorCells(s) {
      if (s.lastValues.length === 0) return;
      const displayValues = getDisplayValues(s);
      if (displayValues.length === 0) return;
      const [mn, mx] = minMax(displayValues);
      displayValues.forEach((v, i) => {
        if (i < s.cells.length) {
          const c = colorForValue(v, mn, mx);
          s.cells[i].style.background = c.bg;
          s.cells[i].textContent = Math.round(v);
          s.cells[i].style.color = c.dark ? "#ddd" : "#444";
        }
      });
    }

    function recolorAll() {
      SENSOR_IDS.forEach(id => colorCells(sensors[id]));
    }

    function updateSensor(id, data) {
      const s = sensors[id];
      if (data.count === 0) {
        s.gridEl.innerHTML = '<div class="no-data">No data</div>';
        s.cells = [];
        s.currentCount = 0;
        s.lastValues = [];
        return;
      }
      if (s.currentCount !== data.count) {
        createGrid(s, data.count);
      }
      s.lastValues = data.values;
      const displayValues = getDisplayValues(s);
      if (displayValues.length === 0) return;
      const [mn, mx] = minMax(displayValues);
      displayValues.forEach((v, i) => {
        if (i < s.cells.length) {
          const c = colorForValue(v, mn, mx);
          s.cells[i].style.background = c.bg;
          s.cells[i].textContent = Math.round(v);
          s.cells[i].style.color = c.dark ? "#ddd" : "#444";
        }
      });
      updateHistogram(s, displayValues);
      updateStats(s, displayValues);
      updatePlot(s, displayValues);
    }

    async function fetchAll() {
      try {
        const res = await fetch("/api/allmeasurements");
        if (!res.ok) return;
        const all = await res.json();
        for (const data of all.sensors) {
          updateSensor(data.id, data);
        }
      } catch (e) {
        // leave as-is on error
      }
      statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
    }

    // Initialize
    SENSOR_IDS.forEach(id => {
      createGrid(sensors[id], 0);
      createHistogram(sensors[id]);
    });
    async function pollLoop() {
      const start = Date.now();
      await fetchAll();
      const remaining = Math.max(0, POLL_MS - (Date.now() - start));
      setTimeout(pollLoop, remaining);
    }
    pollLoop();
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
