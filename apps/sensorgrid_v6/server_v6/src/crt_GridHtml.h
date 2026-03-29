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
    .sensor-widget.selected {
      border-color: #2a2;
      border-width: 2px;
    }
    .selected-info-panel {
      display: none;
      border: 2px solid #2a2;
      border-radius: 6px;
      padding: 0.5rem;
      margin-bottom: 1rem;
      background: #f0faf0;
      font-size: 0.75rem;
    }
    .selected-info-panel.visible {
      display: flex;
      gap: 1rem;
      align-items: center;
      justify-content: center;
    }
    .selected-info-panel label {
      display: flex;
      align-items: center;
      gap: 0.3rem;
    }
    .selected-info-panel input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 70px;
    }
    .cell.cell-selected {
      border: 2px solid #22f;
    }
    .cell.cell-captured {
      border: 2px solid #000;
    }
    .circle-info-panel {
      display: none;
      border: 2px solid #c22;
      border-radius: 6px;
      padding: 0.5rem;
      margin-bottom: 1rem;
      background: #faf0f0;
      font-size: 0.75rem;
    }
    .circle-info-panel.visible {
      display: block;
    }
    .circle-info-row {
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
      margin: 0.2rem 0;
    }
    .circle-info-row label {
      display: flex;
      align-items: center;
      gap: 0.2rem;
    }
    .circle-info-row input {
      font-size: 0.75rem;
      padding: 0.15rem 0.3rem;
      border: 1px solid #999;
      border-radius: 3px;
      width: 60px;
    }
    #status {
      margin-top: 1rem;
      text-align: center;
      color: #666;
    }
    .rec-play-panel {
      margin-bottom: 1rem;
      padding: 0.5rem;
      border: 2px solid #aaa;
      border-radius: 6px;
      background: #f5f5f5;
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
    }
    .rec-play-panel button {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .rec-play-panel button:disabled {
      opacity: 0.4;
      cursor: default;
    }
    .rec-play-panel button.rec-active {
      background: #d32f2f;
      color: #fff;
      border-color: #b71c1c;
    }
    .rec-play-panel button.pause-active {
      background: #f57c00;
      color: #fff;
      border-color: #e65100;
    }
    .rec-play-panel button.play-active {
      background: #2e7d32;
      color: #fff;
      border-color: #1b5e20;
    }
    .rec-play-panel .rec-info {
      font-size: 0.75rem;
      color: #666;
      min-width: 120px;
    }
    .snapshot-panel {
      margin-bottom: 1rem;
      padding: 0.5rem;
      border: 2px solid #b39ddb;
      border-radius: 6px;
      background: #f3e5f5;
      display: flex;
      flex-direction: column;
      gap: 0.4rem;
    }
    .snapshot-row {
      display: flex;
      gap: 0.5rem;
      align-items: center;
      justify-content: center;
    }
    .snapshot-panel button {
      padding: 0.4rem 1rem;
      border: 2px solid #999;
      border-radius: 4px;
      background: #fff;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      color: #666;
    }
    .snapshot-panel button:disabled {
      opacity: 0.4;
      cursor: default;
    }
    .snapshot-panel button.snap-replay-active {
      background: #7b1fa2;
      color: #fff;
      border-color: #4a148c;
    }
    .snapshot-panel .snap-info {
      font-size: 0.75rem;
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
        <button class="toggle-btn" id="btnNormSumCap" onclick="toggleNormSumCap()">Norm SumCap</button>
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
      <button id="btnPlay" onclick="recPlay()" disabled>Play</button>
      <button id="btnDownload" onclick="recDownload()" disabled>Download</button>
      <button id="btnRecUpload" onclick="document.getElementById('recFileInput').click()">Upload</button>
      <input type="file" id="recFileInput" accept=".json" style="display:none" onchange="recUpload(event)"/>
      <span class="rec-info" id="recInfo">Ready</span>
    </div>
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
    </div>
    <div class="selected-info-panel" id="selectedInfoPanel">
      <button class="toggle-btn" onclick="doRemoveOffset()">Remove Offset</button>
      <button class="toggle-btn" onclick="doCapture()">Capture Sum</button>
      <label>maxSumCaptured <input type="text" id="fldSumCap" readonly value="-" size="8"/></label>
      <label>avResidualNoiseSum <input type="text" id="fldResNoise" readonly value="-" size="8"/></label>
      <label>Calibrate Sum Grams <input type="text" id="fldCalGrams" value="1000" size="8" onchange="onCalGramsChange()"/></label>
      <button class="toggle-btn" onclick="resetIndivCaps()">Reset Indiv Caps</button>
      <label style="font-size:0.8rem;display:flex;align-items:center;gap:0.3rem;margin-left:0.5rem;"><input type="checkbox" id="chkEnableSensor" checked onchange="toggleEnableSensor()"/> Enable Sensor</label>
    </div>
    <div class="circle-info-panel" id="circleInfoPanel">
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
    </div>
    <div class="sensor-layout">
      <div class="sensor-widget" id="sw1" onclick="selectSensor(1)">
        <h3>Sensor 1</h3>
        <div class="panel-buttons">
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
      <div class="sensor-widget" id="sw2" onclick="selectSensor(2)">
        <h3>Sensor 2</h3>
        <div class="panel-buttons">
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
      <div class="sensor-widget" id="sw3" onclick="selectSensor(3)">
        <h3>Sensor 3</h3>
        <div class="panel-buttons">
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
      <div class="sensor-widget" id="sw4" onclick="selectSensor(4)">
        <h3>Sensor 4</h3>
        <div class="panel-buttons">
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
    let normSumCap = false;
    let normIndivCap = false;
    let selectedSensorId = null;
    let selectedCircleIdx = null;
    // Record & Play state
    // recState: 'idle' | 'recording' | 'paused' | 'stopped' | 'playing'
    let recState = 'idle';
    let recFrames = [];    // array of raw API response objects
    let playIdx = 0;

    function recRecord() {
      recFrames = [];
      playIdx = 0;
      recState = 'recording';
      document.getElementById('btnRecord').classList.add('rec-active');
      document.getElementById('btnPause').disabled = false;
      document.getElementById('btnPause').classList.remove('pause-active');
      document.getElementById('btnStop').disabled = false;
      document.getElementById('btnPlay').disabled = true;
      document.getElementById('btnPlay').classList.remove('play-active');
      document.getElementById('btnDownload').disabled = true;
      recUpdateInfo();
    }

    function recPause() {
      if (recState === 'recording') {
        recState = 'paused';
        document.getElementById('btnPause').classList.add('pause-active');
        document.getElementById('btnDownload').disabled = (recFrames.length === 0);
      } else if (recState === 'paused') {
        recState = 'recording';
        document.getElementById('btnPause').classList.remove('pause-active');
        document.getElementById('btnDownload').disabled = true;
      }
      recUpdateInfo();
    }

    function recStop() {
      if (recState !== 'recording' && recState !== 'paused') return;
      recState = 'stopped';
      document.getElementById('btnRecord').classList.remove('rec-active');
      document.getElementById('btnPause').disabled = true;
      document.getElementById('btnPause').classList.remove('pause-active');
      document.getElementById('btnStop').disabled = true;
      document.getElementById('btnPlay').disabled = (recFrames.length === 0);
      document.getElementById('btnDownload').disabled = (recFrames.length === 0);
      recUpdateInfo();
    }

    function recPlay() {
      if (recFrames.length === 0) return;
      if (recState === 'playing') {
        // Stop playback, return to stopped state
        recState = 'stopped';
        document.getElementById('btnPlay').classList.remove('play-active');
        recUpdateInfo();
        return;
      }
      recState = 'playing';
      playIdx = 0;
      document.getElementById('btnPlay').classList.add('play-active');
      document.getElementById('btnRecord').classList.remove('rec-active');
      recUpdateInfo();
    }

    function recDownload() {
      if (recFrames.length === 0) return;
      const json = JSON.stringify(recFrames);
      const blob = new Blob([json], {type: 'application/json'});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      const ts = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
      a.download = 'recording_' + ts + '.json';
      a.click();
      URL.revokeObjectURL(url);
    }

    function recUpload(event) {
      const file = event.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const data = JSON.parse(e.target.result);
          if (!Array.isArray(data) || data.length === 0) {
            alert('Invalid recording file: expected a non-empty JSON array.');
            return;
          }
          recFrames = data;
          playIdx = 0;
          recState = 'stopped';
          document.getElementById('btnRecord').classList.remove('rec-active');
          document.getElementById('btnPause').disabled = true;
          document.getElementById('btnPause').classList.remove('pause-active');
          document.getElementById('btnStop').disabled = true;
          document.getElementById('btnPlay').disabled = false;
          document.getElementById('btnPlay').classList.remove('play-active');
          document.getElementById('btnDownload').disabled = false;
          recUpdateInfo();
        } catch (err) {
          alert('Failed to parse recording file: ' + err.message);
        }
      };
      reader.readAsText(file);
      event.target.value = '';
    }

    function recUpdateInfo() {
      const el = document.getElementById('recInfo');
      if (recState === 'recording') el.textContent = 'Recording: ' + recFrames.length + ' frames';
      else if (recState === 'paused') el.textContent = 'Paused: ' + recFrames.length + ' frames';
      else if (recState === 'stopped') el.textContent = 'Stopped: ' + recFrames.length + ' frames';
      else if (recState === 'playing') el.textContent = 'Playing: ' + (playIdx) + '/' + recFrames.length;
      else el.textContent = 'Ready';
    }

    // Snapshot state
    let snapFrames = [];  // array of snapshot frames (each = {sensors: [{id, emaRawValues, hasLoadcell, loadcellRaw, loadcellGram}, ...]})
    let snapReplaying = false;
    let snapIdx = 0;

    function snapTake() {
      // Build a snapshot from current emaRawValues of all sensors
      const frame = {sensors: SENSOR_IDS.map(id => {
        const s = sensors[id];
        return {
          id: id,
          count: (s.enabled && s.emaRawValues) ? s.emaRawValues.length : 0,
          values: (s.enabled && s.emaRawValues) ? s.emaRawValues.map(v => Math.round(v)) : [],
          hasLoadcell: s.enabled ? s.lcVisible : false,
          loadcellRaw: s.enabled ? s.lcLastRaw : 0,
          loadcellGram: (s.enabled && s.filteredWeight !== null) ? parseFloat(s.filteredWeight.toFixed(1)) : 0
        };
      })};
      snapFrames.push(frame);
      snapUpdateUI();
    }

    function snapClear() {
      snapFrames = [];
      if (snapReplaying) snapReplayStop();
      snapIdx = 0;
      snapUpdateUI();
    }

    function snapDownload() {
      if (snapFrames.length === 0) return;
      const json = JSON.stringify(snapFrames);
      const blob = new Blob([json], {type: 'application/json'});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      const ts = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
      a.download = 'snapshots_' + ts + '.json';
      a.click();
      URL.revokeObjectURL(url);
    }

    function snapUpload(event) {
      const file = event.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const data = JSON.parse(e.target.result);
          if (!Array.isArray(data) || data.length === 0) {
            alert('Invalid snapshot file: expected a non-empty JSON array.');
            return;
          }
          if (snapReplaying) snapReplayStop();
          snapFrames = data;
          snapIdx = 0;
          snapUpdateUI();
        } catch (err) {
          alert('Failed to parse snapshot file: ' + err.message);
        }
      };
      reader.readAsText(file);
      event.target.value = '';  // allow re-uploading the same file
    }

    function snapReplayStart() {
      if (snapFrames.length === 0) return;
      snapReplaying = true;
      snapIdx = 0;
      document.getElementById('snapReplayRow').style.display = 'none';
      document.getElementById('snapControlRow').style.display = 'flex';
      snapUpdateUI();
    }

    function snapReplayStop() {
      snapReplaying = false;
      document.getElementById('snapControlRow').style.display = 'none';
      document.getElementById('snapReplayRow').style.display = 'flex';
      snapUpdateUI();
    }

    function snapStepBack() {
      if (snapIdx > 0) snapIdx--;
      snapUpdateUI();
    }

    function snapStepForward() {
      if (snapIdx < snapFrames.length - 1) snapIdx++;
      snapUpdateUI();
    }

    function snapGotoFirst() {
      snapIdx = 0;
      snapUpdateUI();
    }

    function snapGotoLast() {
      if (snapFrames.length > 0) snapIdx = snapFrames.length - 1;
      snapUpdateUI();
    }

    function snapUpdateUI() {
      document.getElementById('snapInfo').textContent = 'Snapshots: ' + snapFrames.length;
      document.getElementById('snapIdxInfo').textContent = 'Index: ' + snapIdx;
      document.getElementById('btnSnapDownload').disabled = (snapFrames.length === 0);
      document.getElementById('btnSnapReplay').disabled = (snapFrames.length === 0);
    }

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

    function selectSensor(id) {
      if (selectedSensorId === id) return;
      // Deselect previous
      if (selectedSensorId !== null) {
        document.getElementById("sw" + selectedSensorId).classList.remove("selected");
      }
      selectedSensorId = id;
      document.getElementById("sw" + id).classList.add("selected");
      // Show the selected sensor info panel
      document.getElementById("selectedInfoPanel").classList.add("visible");
      // Populate fields from the selected sensor's state
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
        // Clear display to show as offline
        s.gridEl.innerHTML = '<div class="no-data">Disabled</div>';
        s.cells = [];
        s.currentCount = 0;
        s.lastValues = [];
        s.filteredValues = null;
        s.filteredSum = null;
        s.filteredMax = null;
        s.filteredAvg = null;
        s.filteredStd = null;
        s.plotHistory = [];
        s.sumEl.textContent = "-";
        s.maxEl.textContent = "-";
        s.avgEl.textContent = "-";
        s.stdEl.textContent = "-";
        if (s.lcVisible) { s.lcEl.style.display = "none"; s.lcVisible = false; }
        // Clear histogram
        s.histBars.forEach(b => b.style.height = "1px");
        // Clear plot
        const ctx = s.plotCanvas.getContext("2d");
        ctx.clearRect(0, 0, s.plotCanvas.width, s.plotCanvas.height);
        // Clear 3D surface
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
      const s = sensors[selectedSensorId];
      if (s.lastValues.length === 0) return;
      // Compute sum of displayed (filtered) circle values
      const displayValues = getDisplayValues(s, selectedSensorId);
      let sum = 0;
      for (const v of displayValues) sum += v;
      s.maxSumCaptured = sum;
      document.getElementById("fldSumCap").value = sum.toFixed(1);
      setCookie("gvMaxSumCaptured" + selectedSensorId, sum, 365);
      recolorAll();
    }
    function toggleNormSumCap() {
      normSumCap = !normSumCap;
      if (normSumCap) {
        normIndivCap = false;
        document.getElementById("btnNormIndivCap").classList.remove("active");
      }
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
        // Start collecting grid-sum samples for 1 second to compute avResidualNoiseSum
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
      selectSensor(sensorId);
      selectedCircleIdx = circleIdx;
      // Update circle borders for all sensors
      SENSOR_IDS.forEach(id => updateCircleBorders(sensors[id]));
      // Show circle info panel
      const panel = document.getElementById("circleInfoPanel");
      panel.classList.add("visible");
      // Populate fields
      const s = sensors[sensorId];
      ensureIndivCaps(s, s.cells.length);
      const tuples = s.indivCaps[circleIdx];
      for (let t = 0; t < 3; t++) {
        document.getElementById("ciCap" + t).value = tuples[t].cap ? Math.round(tuples[t].cap) : "-";
        document.getElementById("ciGrams" + t).value = tuples[t].grams || "-";
      }
    }

    function captureIndiv(tupleIdx) {
      if (selectedSensorId === null || selectedCircleIdx === null) return;
      const s = sensors[selectedSensorId];
      if (s.filteredValues === null || selectedCircleIdx >= s.filteredValues.length) return;
      ensureIndivCaps(s, s.cells.length);
      const circleValue = s.filteredValues[selectedCircleIdx];
      const calGrams = parseFloat(document.getElementById("ciCalGrams" + tupleIdx).value) || 0;
      s.indivCaps[selectedCircleIdx][tupleIdx] = {cap: circleValue, grams: calGrams};
      document.getElementById("ciCap" + tupleIdx).value = Math.round(circleValue);
      document.getElementById("ciGrams" + tupleIdx).value = calGrams;
      // Save to cookie
      try { localStorage.setItem("gvIndivCaps" + selectedSensorId, JSON.stringify(s.indivCaps)); } catch(e) {}
      updateCircleBorders(s);
    }

    function resetIndivCaps() {
      if (selectedSensorId === null) return;
      const s = sensors[selectedSensorId];
      s.indivCaps = [];
      ensureIndivCaps(s, s.cells.length);
      try { localStorage.setItem("gvIndivCaps" + selectedSensorId, JSON.stringify(s.indivCaps)); } catch(e) {}
      // Update display if circle is selected
      if (selectedCircleIdx !== null) {
        for (let t = 0; t < 3; t++) {
          document.getElementById("ciCap" + t).value = "-";
          document.getElementById("ciGrams" + t).value = "-";
        }
      }
      updateCircleBorders(s);
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

    function createGrid(s, count, sensorId) {
      s.gridEl.innerHTML = "";
      s.cells = [];
      s.currentCount = count;
      ensureIndivCaps(s, count);
      const rowSizes = computeRowSizes(count);
      let cellIdx = 0;
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
          const ci = cellIdx;
          cell.addEventListener("click", function(ev) { selectCircle(sensorId, ci, ev); });
          rowEl.appendChild(cell);
          s.cells.push(cell);
          cellIdx++;
        }
        s.gridEl.appendChild(rowEl);
      });
      updateCircleBorders(s);
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

    function getValueFilter() {
      const v = parseFloat(document.getElementById("fldValueFilter").value);
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
      if (s.is3D) colorCells(s, id);
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

    function indivLookup(rawVal, tuples) {
      // Build sorted points from (0,0) + non-zero tuples
      const pts = [{x:0, y:0}];
      for (const t of tuples) {
        if (t.cap !== 0 || t.grams !== 0) {
          pts.push({x: t.cap, y: t.grams});
        }
      }
      if (pts.length === 1) return 0; // no tuples set
      pts.sort((a, b) => a.x - b.x);
      // Piecewise linear interpolation / extrapolation
      if (rawVal <= pts[0].x) {
        // Extrapolate using first segment
        const p0 = pts[0], p1 = pts[1];
        const slope = (p1.x !== p0.x) ? (p1.y - p0.y) / (p1.x - p0.x) : 0;
        return p0.y + slope * (rawVal - p0.x);
      }
      for (let i = 1; i < pts.length; i++) {
        if (rawVal <= pts[i].x) {
          const p0 = pts[i-1], p1 = pts[i];
          const t = (p1.x !== p0.x) ? (rawVal - p0.x) / (p1.x - p0.x) : 0;
          return p0.y + t * (p1.y - p0.y);
        }
      }
      // Extrapolate beyond last point
      const pA = pts[pts.length - 2], pB = pts[pts.length - 1];
      const slope = (pB.x !== pA.x) ? (pB.y - pA.y) / (pB.x - pA.x) : 0;
      return pB.y + slope * (rawVal - pB.x);
    }

    function getDisplayValues(s, id) {
      if (s.lastValues.length === 0) return [];
      const offsetted = s.offsets
        ? s.lastValues.map((v, i) => v - (s.offsets[i] || 0))
        : s.lastValues;
      if (normIndivCap && s.indivCaps.length === offsetted.length) {
        return offsetted.map((v, i) => indivLookup(v, s.indivCaps[i]));
      }
      if (normSumCap && s.maxSumCaptured != null) {
        const denom = s.maxSumCaptured - s.avResidualNoiseSum;
        if (denom > 0) {
          const cg = getCalGrams(id);
          return offsetted.map(v => cg * v / denom);
        }
      }
      return offsetted;
    }

    function colorCells(s, id) {
      if (s.lastValues.length === 0) return;
      const displayValues = getDisplayValues(s, id);
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
      SENSOR_IDS.forEach(id => colorCells(sensors[id], id));
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
      const savedKnown = getCookie("lcKnown" + id);
      if (savedTare !== null) s.lcTareOffset = parseFloat(savedTare) || 0;
      if (savedScale !== null) s.lcScale = parseFloat(savedScale) || 1;
      if (savedKnown !== null) s.lcKnownEl.value = savedKnown;
    });

    // Restore grid view settings from cookies
    (function restoreSettings() {
      const sv = (key, elId) => {
        const v = getCookie(key);
        if (v !== null) document.getElementById(elId).value = v;
      };
      sv("gvFixedMax", "fldFixedMax");
      sv("gvStatsFilter", "fldStatsFilter");
      sv("gvValueFilter", "fldValueFilter");

      // Restore per-sensor capture state
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

    function lcTare(id) {
      const s = sensors[id];
      s.lcTareOffset = s.lcLastRaw || 0;
      s.filteredWeight = null;
      setCookie("lcTare" + id, s.lcTareOffset, 365);
    }

    function lcCalibrate(id) {
      const s = sensors[id];
      const knownGrams = parseFloat(s.lcKnownEl.value);
      if (!knownGrams || knownGrams <= 0) return;
      const rawMinusTare = (s.lcLastRaw || 0) - s.lcTareOffset;
      if (rawMinusTare === 0) return;
      s.lcScale = rawMinusTare / knownGrams;
      s.filteredWeight = null;
      setCookie("lcScale" + id, s.lcScale, 365);
      setCookie("lcKnown" + id, knownGrams, 365);
    }

    function updateLoadcell(id, hasLoadcell, rawValue) {
      const s = sensors[id];
      if (!s.enabled) return;
      if (hasLoadcell) {
        if (!s.lcVisible) {
          s.lcEl.style.display = "block";
          s.lcVisible = true;
        }
        s.lcLastRaw = rawValue;
        const weightGrams = s.lcScale !== 0
          ? (rawValue - s.lcTareOffset) / s.lcScale
          : 0;
        if (s.filteredWeight === null) {
          s.filteredWeight = weightGrams;
        } else {
          const f = getStatsFilter() * 0.8;
          s.filteredWeight = s.filteredWeight * f + weightGrams * (1 - f);
        }
        s.lcWeightEl.textContent = s.filteredWeight.toFixed(1) + " g";
      } else {
        if (s.lcVisible) {
          s.lcEl.style.display = "none";
          s.lcVisible = false;
        }
      }
    }

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
      // Compute EMA of raw input values (used for snapshots)
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
      // Apply per-circle EMA filtering
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
        // Record frame after applyFrame so filteredWeight is up to date
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
        // Collect residual noise samples after offset removal
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
        // leave as-is on error
      }
      statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
    }

    // Initialize
    SENSOR_IDS.forEach(id => {
      createGrid(sensors[id], 0, id);
      createHistogram(sensors[id]);
    });
    async function pollLoop() {
      const start = Date.now();
      if (snapReplaying && snapFrames.length > 0) {
        applyFrame(snapFrames[snapIdx]);
      } else if (recState === 'playing') {
        if (playIdx < recFrames.length) {
          applyFrame(recFrames[playIdx]);
          playIdx++;
          recUpdateInfo();
        }
        if (playIdx >= recFrames.length) {
          playIdx = 0; // loop back to start
        }
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
