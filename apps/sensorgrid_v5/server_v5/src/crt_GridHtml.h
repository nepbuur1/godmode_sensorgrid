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
    .surface-canvas {
      display: none;
      width: 100%;
      height: 200px;
      margin: 0 auto;
      border: 1px solid #ccc;
      background: #111;
    }
    .panel-buttons {
      display: flex;
      gap: 0.3rem;
      justify-content: center;
      margin-bottom: 0.3rem;
    }
    .running-plot {
      display: block;
      width: 100%;
      height: 120px;
      margin: 0.3rem auto 0;
      border: 1px solid #ccc;
      background: #111;
    }
    .loadcell-section {
      margin-top: 0.4rem;
      padding: 0.3rem;
      border: 1px solid #bcd;
      border-radius: 4px;
      background: #f0f5fa;
      font-size: 0.7rem;
      display: none;
    }
    .loadcell-section h4 {
      margin: 0 0 0.2rem;
      font-size: 0.75rem;
      text-align: center;
    }
    .lc-weight {
      text-align: center;
      font-size: 1.1rem;
      font-weight: 700;
      margin: 0.2rem 0;
    }
    .lc-row {
      display: flex;
      align-items: center;
      gap: 0.3rem;
      margin: 0.15rem 0;
      justify-content: center;
    }
    .lc-row label {
      font-size: 0.65rem;
    }
    .lc-row input {
      font-size: 0.65rem;
      padding: 0.1rem 0.2rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 60px;
    }
    .lc-row button {
      font-size: 0.65rem;
      padding: 0.15rem 0.4rem;
      cursor: pointer;
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
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" onclick="removeOffset(1)">Remove offset</button>
          <button class="toggle-btn offset-btn" id="btn3d1" onclick="toggle3D(1)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid1"></div>
        <canvas class="surface-canvas" id="surface1" width="250" height="200"></canvas>
        <div class="histogram" id="hist1"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum1">-</td><td id="max1">-</td><td id="avg1">-</td><td id="std1">-</td></tr></table>
        <canvas class="running-plot" id="plot1" width="250" height="120"></canvas>
        <div class="loadcell-section" id="lc1">
          <h4>Loadcell</h4>
          <div class="lc-weight" id="lcWeight1">- g</div>
          <div class="lc-row">
            <button onclick="lcTare(1)">Tare</button>
          </div>
          <div class="lc-row">
            <label>Known weight (g):</label>
            <input type="text" id="lcKnown1" value="500"/>
            <button onclick="lcCalibrate(1)">Calibrate</button>
          </div>
        </div>
      </div>
      <div class="sensor-widget" id="sw2">
        <h3>Sensor 2</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" onclick="removeOffset(2)">Remove offset</button>
          <button class="toggle-btn offset-btn" id="btn3d2" onclick="toggle3D(2)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid2"></div>
        <canvas class="surface-canvas" id="surface2" width="250" height="200"></canvas>
        <div class="histogram" id="hist2"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum2">-</td><td id="max2">-</td><td id="avg2">-</td><td id="std2">-</td></tr></table>
        <canvas class="running-plot" id="plot2" width="250" height="120"></canvas>
        <div class="loadcell-section" id="lc2">
          <h4>Loadcell</h4>
          <div class="lc-weight" id="lcWeight2">- g</div>
          <div class="lc-row">
            <button onclick="lcTare(2)">Tare</button>
          </div>
          <div class="lc-row">
            <label>Known weight (g):</label>
            <input type="text" id="lcKnown2" value="500"/>
            <button onclick="lcCalibrate(2)">Calibrate</button>
          </div>
        </div>
      </div>
      <div class="sensor-widget" id="sw3">
        <h3>Sensor 3</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" onclick="removeOffset(3)">Remove offset</button>
          <button class="toggle-btn offset-btn" id="btn3d3" onclick="toggle3D(3)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid3"></div>
        <canvas class="surface-canvas" id="surface3" width="250" height="200"></canvas>
        <div class="histogram" id="hist3"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum3">-</td><td id="max3">-</td><td id="avg3">-</td><td id="std3">-</td></tr></table>
        <canvas class="running-plot" id="plot3" width="250" height="120"></canvas>
        <div class="loadcell-section" id="lc3">
          <h4>Loadcell</h4>
          <div class="lc-weight" id="lcWeight3">- g</div>
          <div class="lc-row">
            <button onclick="lcTare(3)">Tare</button>
          </div>
          <div class="lc-row">
            <label>Known weight (g):</label>
            <input type="text" id="lcKnown3" value="500"/>
            <button onclick="lcCalibrate(3)">Calibrate</button>
          </div>
        </div>
      </div>
      <div class="sensor-widget" id="sw4">
        <h3>Sensor 4</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" onclick="removeOffset(4)">Remove offset</button>
          <button class="toggle-btn offset-btn" id="btn3d4" onclick="toggle3D(4)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid4"></div>
        <canvas class="surface-canvas" id="surface4" width="250" height="200"></canvas>
        <div class="histogram" id="hist4"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th><th>average</th><th>sqrt(var)</th></tr><tr><td id="sum4">-</td><td id="max4">-</td><td id="avg4">-</td><td id="std4">-</td></tr></table>
        <canvas class="running-plot" id="plot4" width="250" height="120"></canvas>
        <div class="loadcell-section" id="lc4">
          <h4>Loadcell</h4>
          <div class="lc-weight" id="lcWeight4">- g</div>
          <div class="lc-row">
            <button onclick="lcTare(4)">Tare</button>
          </div>
          <div class="lc-row">
            <label>Known weight (g):</label>
            <input type="text" id="lcKnown4" value="500"/>
            <button onclick="lcCalibrate(4)">Calibrate</button>
          </div>
        </div>
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
        plotHistory: [],
        is3D: false,
        surfaceCanvas: document.getElementById("surface" + id),
        gl: null, glProg: null, glAPos: -1, glANor: -1, glACol: -1, glUMVP: null,
        glVtxBuf: null, glIdxBuf: null, glIdxCount: 0,
        meshRows: 0, meshCols: 0,
        // Loadcell state
        lcEl: document.getElementById("lc" + id),
        lcWeightEl: document.getElementById("lcWeight" + id),
        lcKnownEl: document.getElementById("lcKnown" + id),
        lcTareOffset: 0,
        lcScale: 1,
        lcVisible: false
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

    // --- WebGL 3D Surface ---
    const GL_VS = `attribute vec3 aPos; attribute vec3 aNor; attribute vec3 aCol;
      uniform mat4 uMVP; varying vec3 vCol; varying vec3 vNor; varying vec3 vPos;
      void main(){ gl_Position = uMVP * vec4(aPos, 1.0); vCol = aCol; vNor = aNor; vPos = aPos; }`;
    const GL_FS = `precision mediump float; varying vec3 vCol; varying vec3 vNor; varying vec3 vPos;
      void main(){
        vec3 n = normalize(vNor);
        vec3 ld = normalize(vec3(1.0, 1.0, 0.0));
        vec3 eye = normalize(vec3(10.0, 10.0, 16.0) - vPos);
        vec3 refl = reflect(-ld, n);
        float diff = max(dot(n, ld), 0.0);
        float spec = pow(max(dot(refl, eye), 0.0), 32.0) * 0.4;
        vec3 c = vCol * (0.3 + 0.7 * diff) + vec3(spec);
        gl_FragColor = vec4(min(c, 1.0), 1.0);
      }`;

    function m4Mul(a, b) {
      const r = new Array(16);
      for (let c = 0; c < 4; c++)
        for (let i = 0; i < 4; i++) {
          let s = 0;
          for (let k = 0; k < 4; k++) s += a[i+k*4]*b[k+c*4];
          r[i+c*4] = s;
        }
      return r;
    }
    function m4Pers(fov, asp, n, f) {
      const t = 1/Math.tan(fov/2), d = n-f;
      return [t/asp,0,0,0, 0,t,0,0, 0,0,(f+n)/d,-1, 0,0,2*f*n/d,0];
    }
    function m4LookAt(ex,ey,ez, cx,cy,cz, ux,uy,uz) {
      let fx=cx-ex, fy=cy-ey, fz=cz-ez;
      let fl=Math.hypot(fx,fy,fz); fx/=fl; fy/=fl; fz/=fl;
      let sx=fy*uz-fz*uy, sy=fz*ux-fx*uz, sz=fx*uy-fy*ux;
      let sl=Math.hypot(sx,sy,sz); sx/=sl; sy/=sl; sz/=sl;
      ux=sy*fz-sz*fy; uy=sz*fx-sx*fz; uz=sx*fy-sy*fx;
      return [sx,ux,-fx,0, sy,uy,-fy,0, sz,uz,-fz,0,
        -(sx*ex+sy*ey+sz*ez),-(ux*ex+uy*ey+uz*ez),(fx*ex+fy*ey+fz*ez),1];
    }

    function initGL(s) {
      const gl = s.surfaceCanvas.getContext("webgl");
      if (!gl) return;
      s.gl = gl;
      const vs = gl.createShader(gl.VERTEX_SHADER);
      gl.shaderSource(vs, GL_VS); gl.compileShader(vs);
      const fs = gl.createShader(gl.FRAGMENT_SHADER);
      gl.shaderSource(fs, GL_FS); gl.compileShader(fs);
      const prog = gl.createProgram();
      gl.attachShader(prog, vs); gl.attachShader(prog, fs);
      gl.linkProgram(prog);
      s.glProg = prog;
      s.glAPos = gl.getAttribLocation(prog, "aPos");
      s.glANor = gl.getAttribLocation(prog, "aNor");
      s.glACol = gl.getAttribLocation(prog, "aCol");
      s.glUMVP = gl.getUniformLocation(prog, "uMVP");
      s.glVtxBuf = gl.createBuffer();
      s.glIdxBuf = gl.createBuffer();
    }

    function colorRGB(v, mn, mx) {
      const lo = normalized ? mn : 0;
      const hi = normalized ? mx : (maxFixed ? getFixedMax() : MAX_VALUE);
      const range = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / range));
      if (!colorized) { const g = 0.5 + 0.5*t; return [g, g, g]; }
      const fl = 0.5 * (1 - t);
      let r, g, b;
      if (t < 0.25) { const p=t/0.25; r=0; g=0; b=p; }
      else if (t < 0.5) { const p=(t-0.25)/0.25; r=0; g=p; b=1-p; }
      else if (t < 0.75) { const p=(t-0.5)/0.25; r=p; g=1; b=0; }
      else { const p=(t-0.75)/0.25; r=1; g=1-p; b=0; }
      return [Math.min(1,r+fl), Math.min(1,g+fl), Math.min(1,b+fl)];
    }

    function renderSurface(s, vals, mn, mx) {
      const gl = s.gl;
      if (!gl) return;
      const nC = COLS;
      const nR = Math.ceil(vals.length / nC);
      if (nR < 2 || nC < 2) return;

      let yMax = getPlotMax();
      if (yMax === null || yMax <= 0) yMax = Math.max(1, ...vals);
      const hScale = 5;

      // Build height map
      const H = new Float32Array(nR * nC);
      for (let i = 0; i < nR * nC; i++) {
        const v = i < vals.length ? vals[i] : 0;
        H[i] = (v / yMax) * hScale;
      }

      // 9 floats per vertex: pos(3) + normal(3) + color(3), stride 36
      const verts = new Float32Array(nR * nC * 9);
      for (let r = 0; r < nR; r++) {
        for (let c = 0; c < nC; c++) {
          const idx = r * nC + c;
          const v = idx < vals.length ? vals[idx] : 0;
          const rgb = colorRGB(v, mn, mx);
          // Compute normal from height differences
          const hl = c > 0 ? H[idx-1] : H[idx];
          const hr = c < nC-1 ? H[idx+1] : H[idx];
          const hu = r > 0 ? H[idx-nC] : H[idx];
          const hd = r < nR-1 ? H[idx+nC] : H[idx];
          let nx = hl - hr, ny = 2, nz = hu - hd;
          const nl = Math.hypot(nx, ny, nz);
          nx /= nl; ny /= nl; nz /= nl;
          const vi = idx * 9;
          verts[vi]   = c - (nC-1)/2;
          verts[vi+1] = H[idx];
          verts[vi+2] = r - (nR-1)/2;
          verts[vi+3] = nx;
          verts[vi+4] = ny;
          verts[vi+5] = nz;
          verts[vi+6] = rgb[0];
          verts[vi+7] = rgb[1];
          verts[vi+8] = rgb[2];
        }
      }

      if (s.meshRows !== nR || s.meshCols !== nC) {
        const indices = [];
        for (let r = 0; r < nR-1; r++) {
          for (let c = 0; c < nC-1; c++) {
            const tl = r*nC+c, tr = tl+1, bl = tl+nC, br = bl+1;
            indices.push(tl, bl, tr, tr, bl, br);
          }
        }
        gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.glIdxBuf);
        gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint16Array(indices), gl.STATIC_DRAW);
        s.glIdxCount = indices.length;
        s.meshRows = nR; s.meshCols = nC;
      }

      gl.bindBuffer(gl.ARRAY_BUFFER, s.glVtxBuf);
      gl.bufferData(gl.ARRAY_BUFFER, verts, gl.DYNAMIC_DRAW);

      const cw = s.surfaceCanvas.width, ch = s.surfaceCanvas.height;
      const proj = m4Pers(0.8, cw/ch, 0.1, 100);
      const view = m4LookAt(10, 10, 16, 0, 1, 0, 0, 1, 0);
      const mvp = m4Mul(proj, view);

      gl.viewport(0, 0, cw, ch);
      gl.clearColor(0.067, 0.067, 0.067, 1);
      gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);
      gl.enable(gl.DEPTH_TEST);
      gl.useProgram(s.glProg);

      gl.bindBuffer(gl.ARRAY_BUFFER, s.glVtxBuf);
      gl.vertexAttribPointer(s.glAPos, 3, gl.FLOAT, false, 36, 0);
      gl.enableVertexAttribArray(s.glAPos);
      gl.vertexAttribPointer(s.glANor, 3, gl.FLOAT, false, 36, 12);
      gl.enableVertexAttribArray(s.glANor);
      gl.vertexAttribPointer(s.glACol, 3, gl.FLOAT, false, 36, 24);
      gl.enableVertexAttribArray(s.glACol);
      gl.uniformMatrix4fv(s.glUMVP, false, new Float32Array(mvp));

      gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, s.glIdxBuf);
      gl.drawElements(gl.TRIANGLES, s.glIdxCount, gl.UNSIGNED_SHORT, 0);
    }

    function toggle3D(id) {
      const s = sensors[id];
      s.is3D = !s.is3D;
      s.gridEl.style.display = s.is3D ? "none" : "";
      s.surfaceCanvas.style.display = s.is3D ? "block" : "none";
      document.getElementById("btn3d" + id).classList.toggle("active", s.is3D);
      if (s.is3D && !s.gl) initGL(s);
      if (s.is3D) colorCells(s);
    }

    function colorForValue(v, minV, maxV) {
      const lo = normalized ? minV : 0;
      const hi = normalized ? maxV : (maxFixed ? getFixedMax() : MAX_VALUE);
      const r = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / r));

      if (!colorized) {
        const gray = Math.round(128 + 127 * t);
        return { bg: `rgb(${gray},${gray},${gray})`, dark: gray < 128 };
      }
      // Color gradient: medium-gray -> blue -> green -> yellow -> red
      const fl = Math.round(128 * (1 - t));
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
      cr = Math.min(255, cr + fl);
      cg = Math.min(255, cg + fl);
      cb = Math.min(255, cb + fl);
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
      if (s.is3D && s.gl) {
        renderSurface(s, displayValues, mn, mx);
        return;
      }
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

    // --- Loadcell support ---

    function setCookie(name, value, days) {
      const d = new Date();
      d.setTime(d.getTime() + days * 86400000);
      document.cookie = name + "=" + encodeURIComponent(value) + ";expires=" + d.toUTCString() + ";path=/";
    }
    function getCookie(name) {
      const prefix = name + "=";
      const parts = document.cookie.split(";");
      for (let p of parts) {
        p = p.trim();
        if (p.startsWith(prefix)) return decodeURIComponent(p.substring(prefix.length));
      }
      return null;
    }

    // Load calibration from cookies on startup
    SENSOR_IDS.forEach(id => {
      const s = sensors[id];
      const savedTare = getCookie("lcTare" + id);
      const savedScale = getCookie("lcScale" + id);
      if (savedTare !== null) s.lcTareOffset = parseFloat(savedTare) || 0;
      if (savedScale !== null) s.lcScale = parseFloat(savedScale) || 1;
    });

    function lcTare(id) {
      const s = sensors[id];
      // Store the current raw value as tare offset
      s.lcTareOffset = s.lcLastRaw || 0;
      setCookie("lcTare" + id, s.lcTareOffset, 365);
    }

    function lcCalibrate(id) {
      const s = sensors[id];
      const knownGrams = parseFloat(s.lcKnownEl.value);
      if (!knownGrams || knownGrams <= 0) return;
      const rawMinusTare = (s.lcLastRaw || 0) - s.lcTareOffset;
      if (rawMinusTare === 0) return; // avoid division by zero
      s.lcScale = rawMinusTare / knownGrams;
      setCookie("lcScale" + id, s.lcScale, 365);
    }

    function updateLoadcell(id, hasLoadcell, rawValue) {
      const s = sensors[id];
      if (hasLoadcell) {
        if (!s.lcVisible) {
          s.lcEl.style.display = "block";
          s.lcVisible = true;
        }
        s.lcLastRaw = rawValue;
        const weightGrams = s.lcScale !== 0
          ? (rawValue - s.lcTareOffset) / s.lcScale
          : 0;
        s.lcWeightEl.textContent = weightGrams.toFixed(1) + " g";
      } else {
        if (s.lcVisible) {
          s.lcEl.style.display = "none";
          s.lcVisible = false;
        }
      }
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
      if (s.is3D && s.gl) {
        renderSurface(s, displayValues, mn, mx);
      } else {
        displayValues.forEach((v, i) => {
          if (i < s.cells.length) {
            const c = colorForValue(v, mn, mx);
            s.cells[i].style.background = c.bg;
            s.cells[i].textContent = Math.round(v);
            s.cells[i].style.color = c.dark ? "#ddd" : "#444";
          }
        });
      }
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
          updateLoadcell(data.id, data.hasLoadcell, data.loadcellRaw);
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
