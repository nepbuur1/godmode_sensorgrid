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
  <link rel="stylesheet" href="/grid.css">
  <script src="/grid3d.js"></script>
  <script src="/gridrecplay.js"></script>
  <script src="/gridrecchart.js"></script>
  <script src="/gridsnapshot.js"></script>
  <script src="/gridhistogram.js"></script>
  <script src="/gridplot.js"></script>
  <script src="/gridcircle.js"></script>
  <script src="/gridloadcell.js"></script>
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
        <button class="toggle-btn" id="btnNormSumCap" onclick="toggleNormSumCap()" style="margin-left:1rem;">Norm SumCap</button>
        <button class="toggle-btn" id="btnNormIndivCap" onclick="toggleNormIndivCap()">Norm IndivCap</button>
      </div>
      <div class="ctrl-fields">
        <label>Fixed Max Display <input type="text" id="fldFixedMax" value="2500" size="8" onchange="setCookie('gvFixedMax',this.value,365)"/></label>
        <label>Stats Filter <input type="text" id="fldStatsFilter" value="0.9" size="8" onchange="setCookie('gvStatsFilter',this.value,365)"/></label>
        <label>Value Filter <input type="text" id="fldValueFilter" value="0.5" size="8" onchange="setCookie('gvValueFilter',this.value,365)"/></label>
      </div>
    </div>
    <div class="rec-play-panel">
      <button id="btnRecord" onclick="recRecord()">Record</button>
      <button id="btnPause" onclick="recPause()" disabled>Pause</button>
      <button id="btnStop" onclick="recStop()" disabled>Stop</button>
      <button id="btnPlay" onclick="recPlay()" disabled style="margin-left:1rem;">Play</button>
      <button id="btnPlayPause" onclick="recPlayPause()" disabled>Pause</button>
      <button id="btnDownload" onclick="recDownload()" disabled style="margin-left:1rem;">Download</button>
      <button id="btnRecUpload" onclick="document.getElementById('recFileInput').click()">Upload</button>
      <input type="file" id="recFileInput" accept=".json" style="display:none" onchange="recUpload(event)"/>
      <span class="rec-info" id="recInfo">Ready</span>
      <span id="recZoomRow" style="display:none;margin-left:auto;align-items:center;gap:0.3rem;">
        <label style="font-size:0.75rem;color:#aaa;">Zoom</label>
        <input type="range" id="recZoomSlider" min="0" max="100" value="0" style="width:80px;" oninput="recZoomChange(this.value)"/>
        <span id="recZoomVal" style="font-size:0.75rem;color:#ccc;min-width:2.5rem;">10x</span>
      </span>
    </div>
    <div id="recSliderRow" style="display:none;padding:0 0.5rem;">
      <input type="range" id="recSlider" min="0" max="0" value="0" style="width:100%;" oninput="recSliderChange(this.value)"/>
    </div>
    <canvas id="recChart" width="800" height="250" style="display:none;width:100%;background:#1a1a1a;border-radius:6px;margin:0.3rem 0;"></canvas>
    <div class="snapshot-panel">
      <div class="snapshot-row">
        <button id="btnSnapshot" onclick="snapTake()">Snapshot</button>
        <button id="btnSnapClear" onclick="snapClear()">Clear Snapshots</button>
        <button id="btnSnapDownload" onclick="snapDownload()" disabled>Download</button>
        <button id="btnSnapUpload" onclick="document.getElementById('snapFileInput').click()">Upload</button>
        <input type="file" id="snapFileInput" accept=".json" style="display:none" onchange="snapUpload(event)"/>
        <span class="snap-info" id="snapInfo">Snapshots: 0</span>
      </div>
      <div class="snapshot-row" id="snapReplayRow">
        <button id="btnSnapReplay" onclick="snapReplayStart()" disabled>Snapshot Replay</button>
      </div>
      <div class="snapshot-row" id="snapControlRow" style="display:none;">
        <button onclick="snapStepBack()">Step Back</button>
        <button onclick="snapStepForward()">Step Forward</button>
        <button onclick="snapGotoFirst()">Goto First</button>
        <button onclick="snapGotoLast()">Goto Last</button>
        <button onclick="snapReplayStop()">Stop Snapshot Replay</button>
        <span class="snap-info" id="snapIdxInfo">Index: 0</span>
      </div>
      <div class="snapshot-row" id="snapSliderRow" style="display:none;">
        <input type="range" id="snapSlider" min="0" max="0" value="0" style="flex:1;" oninput="snapSliderChange(this.value)"/>
      </div>
    </div>
    <div class="sensor-layout">
      <div class="sensor-widget" id="sw1" onclick="onSensorPanelClick(1, event)">
        <h3>Sensor 1</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" id="btn3d1" onclick="toggle3D(1)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid1"></div>
        <canvas class="surface-canvas" id="surface1" width="250" height="200"></canvas>
        <div class="histogram" id="hist1"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th></tr><tr><td id="sum1">-</td><td id="max1">-</td></tr></table>
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
      <div class="sensor-widget" id="sw2" onclick="onSensorPanelClick(2, event)">
        <h3>Sensor 2</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" id="btn3d2" onclick="toggle3D(2)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid2"></div>
        <canvas class="surface-canvas" id="surface2" width="250" height="200"></canvas>
        <div class="histogram" id="hist2"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th></tr><tr><td id="sum2">-</td><td id="max2">-</td></tr></table>
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
      <div class="sensor-widget" id="sw3" onclick="onSensorPanelClick(3, event)">
        <h3>Sensor 3</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" id="btn3d3" onclick="toggle3D(3)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid3"></div>
        <canvas class="surface-canvas" id="surface3" width="250" height="200"></canvas>
        <div class="histogram" id="hist3"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th></tr><tr><td id="sum3">-</td><td id="max3">-</td></tr></table>
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
      <div class="sensor-widget" id="sw4" onclick="onSensorPanelClick(4, event)">
        <h3>Sensor 4</h3>
        <div class="panel-buttons">
          <button class="toggle-btn offset-btn" id="btn3d4" onclick="toggle3D(4)">3D Surface</button>
        </div>
        <div class="grid-container" id="grid4"></div>
        <canvas class="surface-canvas" id="surface4" width="250" height="200"></canvas>
        <div class="histogram" id="hist4"></div>
        <div class="hist-axis"><span>0</span><span>32768</span><span>65535</span></div>
        <table class="stats-table"><tr><th>sum</th><th>max</th></tr><tr><td id="sum4">-</td><td id="max4">-</td></tr></table>
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
    <div class="selected-info-panel" id="selectedInfoPanel" onclick="onInfoPanelClick(event)">
      <button class="toggle-btn" onclick="doRemoveOffset()">Remove Offset</button>
      <button class="toggle-btn" onclick="doCapture()">Capture Sum</button>
      <label>maxSumCaptured <input type="text" id="fldSumCap" readonly value="-" size="8"/></label>
      <label>avResidualNoiseSum <input type="text" id="fldResNoise" readonly value="-" size="8"/></label>
      <label>Calibrate Sum Grams <input type="text" id="fldCalGrams" value="1000" size="8" onchange="onCalGramsChange()"/></label>
      <button class="toggle-btn" onclick="resetIndivCaps()">Reset Indiv Caps</button>
      <label style="font-size:0.8rem;display:flex;align-items:center;gap:0.3rem;margin-left:0.5rem;"><input type="checkbox" id="chkEnableSensor" checked onchange="toggleEnableSensor()"/> Enable Sensor</label>
      <span id="captureError" style="color:#c00;font-weight:600;font-size:0.8rem;display:none;"></span>
    </div>
    <div class="circle-info-panel" id="circleInfoPanel" onclick="onCircleInfoPanelClick(event)">
      <div class="circle-info-row">
        <button class="toggle-btn" onclick="captureIndiv(0)">Capture Indiv 1</button>
        <label>cap <input type="text" id="ciCap0" readonly value="-" size="6"/></label>
        <label>grams <input type="text" id="ciGrams0" readonly value="-" size="6"/></label>
        <label>Calibrate Grams Indiv <input type="text" id="ciCalGrams0" value="0" size="6"/></label>
      </div>
      <div class="circle-info-row">
        <button class="toggle-btn" onclick="captureIndiv(1)">Capture Indiv 2</button>
        <label>cap <input type="text" id="ciCap1" readonly value="-" size="6"/></label>
        <label>grams <input type="text" id="ciGrams1" readonly value="-" size="6"/></label>
        <label>Calibrate Grams Indiv <input type="text" id="ciCalGrams1" value="0" size="6"/></label>
      </div>
      <div class="circle-info-row">
        <button class="toggle-btn" onclick="captureIndiv(2)">Capture Indiv 3</button>
        <label>cap <input type="text" id="ciCap2" readonly value="-" size="6"/></label>
        <label>grams <input type="text" id="ciGrams2" readonly value="-" size="6"/></label>
        <label>Calibrate Grams Indiv <input type="text" id="ciCalGrams2" value="0" size="6"/></label>
      </div>
      <span id="captureIndivError" style="color:#c00;font-weight:600;font-size:0.8rem;display:none;"></span>
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
    let normSumCap = false;
    let normIndivCap = false;
    let selectedSensorId = null;
    let selectedCircleIdx = null;
    // Residual noise measurement state
    let residualCollecting = false;
    let residualSensorId = null;
    let residualStartTime = 0;
    let residualSamples = [];

    // Per-sensor state
    const sensors = {};
    SENSOR_IDS.forEach(id => {
      sensors[id] = {
        gridEl: document.getElementById("grid" + id),
        histEl: document.getElementById("hist" + id),
        sumEl: document.getElementById("sum" + id),
        maxEl: document.getElementById("max" + id),
        cells: [],
        histBars: [],
        currentCount: 0,
        lastValues: [],
        offsets: null,
        filteredSum: null,
        filteredMax: null,
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
        lcVisible: false,
        lcLastRaw: 0,
        filteredWeight: null,
        filteredValues: null,
        maxSumCaptured: null,
        avResidualNoiseSum: 0,
        calGrams: 1000,
        // Per-circle indiv captures: array of [{cap,grams},{cap,grams},{cap,grams}] per circle
        indivCaps: [],
        enabled: true,
        emaRawValues: null
      };
    });
    const statusEl = document.getElementById("status");

    function saveToggleStates() {
      setCookie("gvNormalized", normalized ? "1" : "0", 365);
      setCookie("gvMaxFixed", maxFixed ? "1" : "0", 365);
      setCookie("gvNormSumCap", normSumCap ? "1" : "0", 365);
      setCookie("gvNormIndivCap", normIndivCap ? "1" : "0", 365);
    }
    function toggleNormalize() {
      normalized = !normalized;
      if (normalized && maxFixed) {
        maxFixed = false;
        document.getElementById("btnMaxFixed").classList.remove("active");
      }
      document.getElementById("btnNormalize").classList.toggle("active", normalized);
      saveToggleStates();
      recolorAll();
    }
    function toggleMaxFixed() {
      maxFixed = !maxFixed;
      if (maxFixed && normalized) {
        normalized = false;
        document.getElementById("btnNormalize").classList.remove("active");
      }
      document.getElementById("btnMaxFixed").classList.toggle("active", maxFixed);
      saveToggleStates();
      recolorAll();
    }
    function toggleColorize() {
      colorized = !colorized;
      document.getElementById("btnColorize").classList.toggle("active", colorized);
      setCookie("gvColorized", colorized ? "1" : "0", 365);
      recolorAll();
    }
    function getCalGrams(id) {
      return sensors[id].calGrams || 1000;
    }
    function getFixedMax() {
      return parseFloat(document.getElementById("fldFixedMax").value) || 2500;
    }

    function deselectCircle() {
      selectedCircleIdx = null;
      document.getElementById("circleInfoPanel").classList.remove("visible");
      SENSOR_IDS.forEach(id => updateCircleBorders(sensors[id]));
      updateRecChart();
    }

    function deselectSensor() {
      deselectCircle();
      if (selectedSensorId !== null) {
        document.getElementById("sw" + selectedSensorId).classList.remove("selected");
      }
      selectedSensorId = null;
      document.getElementById("selectedInfoPanel").classList.remove("visible");
    }

    function onSensorPanelClick(id, ev) {
      if (ev.target.closest('button, input, .cell')) return;
      if (selectedSensorId === id) {
        deselectSensor();
      } else {
        selectSensor(id);
      }
    }

    function onInfoPanelClick(ev) {
      if (ev.target.closest('button, input')) return;
      deselectSensor();
    }

    function onCircleInfoPanelClick(ev) {
      if (ev.target.closest('button, input')) return;
      deselectCircle();
    }

    function selectSensor(id) {
      if (selectedSensorId === id) return;
      deselectCircle();
      if (selectedSensorId !== null) {
        document.getElementById("sw" + selectedSensorId).classList.remove("selected");
      }
      selectedSensorId = id;
      document.getElementById("sw" + id).classList.add("selected");
      document.getElementById("selectedInfoPanel").classList.add("visible");
      const s = sensors[id];
      document.getElementById("fldSumCap").value = s.maxSumCaptured != null ? s.maxSumCaptured : "-";
      document.getElementById("fldResNoise").value = s.avResidualNoiseSum ? s.avResidualNoiseSum.toFixed(1) : "-";
      document.getElementById("fldCalGrams").value = s.calGrams;
      document.getElementById("chkEnableSensor").checked = s.enabled;
    }

    function toggleEnableSensor() {
      if (selectedSensorId === null) return;
      const s = sensors[selectedSensorId];
      s.enabled = document.getElementById("chkEnableSensor").checked;
      setCookie("gvEnabled" + selectedSensorId, s.enabled ? "1" : "0", 365);
      if (!s.enabled) {
        s.gridEl.innerHTML = '<div class="no-data">Disabled</div>';
        s.cells = [];
        s.currentCount = 0;
        s.lastValues = [];
        s.filteredValues = null;
        s.filteredSum = null;
        s.filteredMax = null;
        s.plotHistory = [];
        s.sumEl.textContent = "-";
        s.maxEl.textContent = "-";
        if (s.lcVisible) { s.lcEl.style.display = "none"; s.lcVisible = false; }
        s.histBars.forEach(b => b.style.height = "1px");
        const ctx = s.plotCanvas.getContext("2d");
        ctx.clearRect(0, 0, s.plotCanvas.width, s.plotCanvas.height);
        if (s.is3D && s.gl) {
          s.gl.clearColor(0.067, 0.067, 0.067, 1);
          s.gl.clear(s.gl.COLOR_BUFFER_BIT | s.gl.DEPTH_BUFFER_BIT);
        }
      }
    }

    function onCalGramsChange() {
      if (selectedSensorId === null) return;
      const val = parseFloat(document.getElementById("fldCalGrams").value) || 1000;
      sensors[selectedSensorId].calGrams = val;
      setCookie("gvCalGrams" + selectedSensorId, val, 365);
      recolorAll();
    }

    function doCapture() {
      if (selectedSensorId === null) return;
      if (normSumCap || normIndivCap) {
        var msg = "Please first disable " + (normSumCap ? "NormSumCap" : "NormIndivCap") + " mode and wait a few seconds.";
        var el = document.getElementById("captureError");
        el.textContent = msg; el.style.display = "inline";
        return;
      }
      const s = sensors[selectedSensorId];
      if (s.lastValues.length === 0) return;
      const displayValues = getDisplayValues(s, selectedSensorId);
      let sum = 0;
      for (const v of displayValues) sum += v;
      s.maxSumCaptured = sum;
      document.getElementById("fldSumCap").value = sum.toFixed(1);
      setCookie("gvMaxSumCaptured" + selectedSensorId, sum, 365);
      recolorAll();
    }
    function clearCaptureErrors() {
      document.getElementById("captureError").style.display = "none";
      document.getElementById("captureIndivError").style.display = "none";
    }
    function toggleNormSumCap() {
      normSumCap = !normSumCap;
      if (normSumCap) {
        normIndivCap = false;
        document.getElementById("btnNormIndivCap").classList.remove("active");
      }
      if (!normSumCap && !normIndivCap) clearCaptureErrors();
      document.getElementById("btnNormSumCap").classList.toggle("active", normSumCap);
      saveToggleStates();
      recolorAll();
    }
    function toggleNormIndivCap() {
      normIndivCap = !normIndivCap;
      if (normIndivCap) {
        normSumCap = false;
        document.getElementById("btnNormSumCap").classList.remove("active");
      }
      if (!normSumCap && !normIndivCap) clearCaptureErrors();
      document.getElementById("btnNormIndivCap").classList.toggle("active", normIndivCap);
      saveToggleStates();
      recolorAll();
    }

    function doRemoveOffset() {
      if (selectedSensorId === null) return;
      removeOffset(selectedSensorId);
    }

    function removeOffset(id) {
      const s = sensors[id];
      if (s.lastValues.length > 0) {
        s.offsets = [...s.lastValues];
        residualCollecting = true;
        residualSensorId = id;
        residualStartTime = Date.now();
        residualSamples = [];
      }
    }

    function ensureIndivCaps(s, count) {
      if (count <= 0) return;
      if (s.indivCaps.length === count) return;
      const old = s.indivCaps;
      s.indivCaps = [];
      for (let i = 0; i < count; i++) {
        s.indivCaps.push(i < old.length ? old[i] : [{cap:0,grams:0},{cap:0,grams:0},{cap:0,grams:0}]);
      }
    }

    function isCircleFullyCaptured(tuples) {
      return tuples.every(t => t.cap !== 0 || t.grams !== 0);
    }

    function updateCircleBorders(s) {
      ensureIndivCaps(s, s.cells.length);
      s.cells.forEach((cell, i) => {
        cell.classList.remove("cell-selected", "cell-captured");
        if (selectedSensorId !== null && sensors[selectedSensorId] === s && selectedCircleIdx === i) {
          cell.classList.add("cell-selected");
        } else if (isCircleFullyCaptured(s.indivCaps[i])) {
          cell.classList.add("cell-captured");
        }
      });
    }

    function selectCircle(sensorId, circleIdx, ev) {
      ev.stopPropagation();
      if (selectedSensorId === sensorId && selectedCircleIdx === circleIdx) {
        deselectCircle();
        return;
      }
      selectSensor(sensorId);
      selectedCircleIdx = circleIdx;
      SENSOR_IDS.forEach(id => updateCircleBorders(sensors[id]));
      const panel = document.getElementById("circleInfoPanel");
      panel.classList.add("visible");
      const s = sensors[sensorId];
      ensureIndivCaps(s, s.cells.length);
      const tuples = s.indivCaps[circleIdx];
      for (let t = 0; t < 3; t++) {
        document.getElementById("ciCap" + t).value = tuples[t].cap ? Math.round(tuples[t].cap) : "-";
        document.getElementById("ciGrams" + t).value = tuples[t].grams || "-";
      }
      updateRecChart();
    }

    function captureIndiv(tupleIdx) {
      if (selectedSensorId === null || selectedCircleIdx === null) return;
      if (normSumCap || normIndivCap) {
        var msg = "Please first disable " + (normSumCap ? "NormSumCap" : "NormIndivCap") + " mode and wait a few seconds.";
        var el = document.getElementById("captureIndivError");
        el.textContent = msg; el.style.display = "inline";
        return;
      }
      const s = sensors[selectedSensorId];
      if (s.filteredValues === null || selectedCircleIdx >= s.filteredValues.length) return;
      ensureIndivCaps(s, s.cells.length);
      const circleValue = s.filteredValues[selectedCircleIdx];
      const calGrams = parseFloat(document.getElementById("ciCalGrams" + tupleIdx).value) || 0;
      s.indivCaps[selectedCircleIdx][tupleIdx] = {cap: circleValue, grams: calGrams};
      document.getElementById("ciCap" + tupleIdx).value = Math.round(circleValue);
      document.getElementById("ciGrams" + tupleIdx).value = calGrams;
      try { localStorage.setItem("gvIndivCaps" + selectedSensorId, JSON.stringify(s.indivCaps)); } catch(e) {}
      updateCircleBorders(s);
    }

    function resetIndivCaps() {
      if (selectedSensorId === null) return;
      const s = sensors[selectedSensorId];
      s.indivCaps = [];
      ensureIndivCaps(s, s.cells.length);
      try { localStorage.setItem("gvIndivCaps" + selectedSensorId, JSON.stringify(s.indivCaps)); } catch(e) {}
      if (selectedCircleIdx !== null) {
        for (let t = 0; t < 3; t++) {
          document.getElementById("ciCap" + t).value = "-";
          document.getElementById("ciGrams" + t).value = "-";
        }
      }
      updateCircleBorders(s);
    }

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

    SENSOR_IDS.forEach(id => {
      const s = sensors[id];
      const savedTare = getCookie("lcTare" + id);
      const savedScale = getCookie("lcScale" + id);
      const savedKnown = getCookie("lcKnown" + id);
      if (savedTare !== null) s.lcTareOffset = parseFloat(savedTare) || 0;
      if (savedScale !== null) s.lcScale = parseFloat(savedScale) || 1;
      if (savedKnown !== null) s.lcKnownEl.value = savedKnown;
    });

    (function restoreSettings() {
      const sv = (key, elId) => {
        const v = getCookie(key);
        if (v !== null) document.getElementById(elId).value = v;
      };
      sv("gvFixedMax", "fldFixedMax");
      sv("gvStatsFilter", "fldStatsFilter");
      sv("gvValueFilter", "fldValueFilter");

      SENSOR_IDS.forEach(id => {
        const s = sensors[id];
        const ms = getCookie("gvMaxSumCaptured" + id);
        if (ms !== null) s.maxSumCaptured = parseFloat(ms);
        const rn = getCookie("gvResNoise" + id);
        if (rn !== null) s.avResidualNoiseSum = parseFloat(rn);
        const cg = getCookie("gvCalGrams" + id);
        if (cg !== null) s.calGrams = parseFloat(cg) || 1000;
        try { const ic = localStorage.getItem("gvIndivCaps" + id); if (ic) s.indivCaps = JSON.parse(ic); } catch(e) {}
        const en = getCookie("gvEnabled" + id);
        if (en === "0") s.enabled = false;
      });

      if (getCookie("gvColorized") === "1") { colorized = true; document.getElementById("btnColorize").classList.add("active"); }
      if (getCookie("gvNormalized") === "1") { normalized = true; document.getElementById("btnNormalize").classList.add("active"); }
      if (getCookie("gvMaxFixed") === "1") { maxFixed = true; document.getElementById("btnMaxFixed").classList.add("active"); }
      if (getCookie("gvNormSumCap") === "1") { normSumCap = true; document.getElementById("btnNormSumCap").classList.add("active"); }
      if (getCookie("gvNormIndivCap") === "1") { normIndivCap = true; document.getElementById("btnNormIndivCap").classList.add("active"); }
    })();

    function updateSensor(id, data) {
      const s = sensors[id];
      if (!s.enabled) return;
      if (data.count === 0) {
        s.gridEl.innerHTML = '<div class="no-data">No data</div>';
        s.cells = [];
        s.currentCount = 0;
        s.lastValues = [];
        return;
      }
      if (s.currentCount !== data.count) {
        createGrid(s, data.count, id);
      }
      s.lastValues = data.values;
      const vf = getValueFilter();
      if (s.emaRawValues === null || s.emaRawValues.length !== data.values.length) {
        s.emaRawValues = [...data.values];
      } else {
        for (let i = 0; i < data.values.length; i++) {
          s.emaRawValues[i] = s.emaRawValues[i] * vf + data.values[i] * (1 - vf);
        }
      }
      const displayValues = getDisplayValues(s, id);
      if (displayValues.length === 0) return;
      if (s.filteredValues === null || s.filteredValues.length !== displayValues.length) {
        s.filteredValues = [...displayValues];
      } else {
        for (let i = 0; i < displayValues.length; i++) {
          s.filteredValues[i] = s.filteredValues[i] * vf + displayValues[i] * (1 - vf);
        }
      }
      const filteredDisplay = s.filteredValues;
      const [mn, mx] = minMax(filteredDisplay);
      if (s.is3D && s.gl) {
        renderSurface(s, filteredDisplay, mn, mx);
      } else {
        filteredDisplay.forEach((v, i) => {
          if (i < s.cells.length) {
            const c = colorForValue(v, mn, mx);
            s.cells[i].style.background = c.bg;
            s.cells[i].textContent = Math.round(v);
            s.cells[i].style.color = c.dark ? "#ddd" : "#444";
          }
        });
      }
      updateHistogram(s, filteredDisplay);
      updateStats(s, filteredDisplay);
      updatePlot(s, filteredDisplay);
      updateCircleBorders(s);
    }

    function applyFrame(all) {
      let snapshotTriggered = false;
      for (const data of all.sensors) {
        updateSensor(data.id, data);
        updateLoadcell(data.id, data.hasLoadcell, data.loadcellRaw);
        if (data.snapshotRequested) snapshotTriggered = true;
      }
      if (snapshotTriggered) snapTake();
    }

    async function fetchAll() {
      try {
        const res = await fetch("/api/allmeasurements");
        if (!res.ok) return;
        const all = await res.json();
        applyFrame(all);
        if (recState === 'recording') {
          const filtered = {sensors: all.sensors.map(d => {
            const s = sensors[d.id];
            if (s && !s.enabled) return {id: d.id, count: 0, values: [], hasLoadcell: false, loadcellRaw: 0, loadcellGram: 0};
            const gram = (s && s.filteredWeight !== null) ? parseFloat(s.filteredWeight.toFixed(1)) : 0;
            return {id: d.id, count: d.count, values: d.values, hasLoadcell: d.hasLoadcell, loadcellRaw: d.loadcellRaw, loadcellGram: gram};
          })};
          recFrames.push(filtered);
          recUpdateInfo();
        }
        if (residualCollecting && residualSensorId !== null) {
          const s = sensors[residualSensorId];
          if (s.lastValues.length > 0 && s.offsets) {
            let sum = 0;
            for (let i = 0; i < s.lastValues.length; i++) {
              sum += s.lastValues[i] - (s.offsets[i] || 0);
            }
            residualSamples.push(sum);
          }
          if (Date.now() - residualStartTime >= 1000) {
            residualCollecting = false;
            const rs = sensors[residualSensorId];
            if (residualSamples.length > 0) {
              let total = 0;
              for (const v of residualSamples) total += v;
              rs.avResidualNoiseSum = total / residualSamples.length;
            } else {
              rs.avResidualNoiseSum = 0;
            }
            setCookie("gvResNoise" + residualSensorId, rs.avResidualNoiseSum, 365);
            if (selectedSensorId === residualSensorId) {
              document.getElementById("fldResNoise").value = rs.avResidualNoiseSum.toFixed(1);
            }
          }
        }
      } catch (e) {
      }
      statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
    }

    SENSOR_IDS.forEach(id => {
      createGrid(sensors[id], 0, id);
      createHistogram(sensors[id]);
    });
    initRecZoom();
    async function pollLoop() {
      const start = Date.now();
      if (snapReplaying && snapFrames.length > 0) {
        applyFrame(snapFrames[snapIdx]);
      } else if (recState === 'playing') {
        if (playIdx < recFrames.length) {
          applyFrame(recFrames[playIdx]);
          playIdx++;
          recUpdateInfo();
          updateRecChart();
        }
        if (playIdx >= recFrames.length) { playIdx = 0; }
      } else if (recState === 'playpaused') {
        if (playIdx > 0) applyFrame(recFrames[playIdx - 1]);
      } else {
        await fetchAll();
      }
      const remaining = Math.max(0, POLL_MS - (Date.now() - start));
      setTimeout(pollLoop, remaining);
    }
    pollLoop();
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
