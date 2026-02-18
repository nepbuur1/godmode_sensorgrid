// by Marius Versteegen, 2025
// Embedded HTML dashboard for the collector node web interface.
// Adapted from _not_part_of_this_project_reference_for_inspiration/collector_node/data/index.html

#pragma once

namespace crt
{
	const char INDEX_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>ESP32-S3 sensormetingen</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <style>
    :root {
      --bar-height: 38px;
      --bar-bg: #f5f5f5;
      --bar-border: #666;
      --stale-border: #007bff;
    }
    body {
      margin: 0;
      font-family: system-ui, sans-serif;
      background: #fafafa;
    }
    .page {
      max-width: 720px;
      margin: 1.5rem auto;
      padding: 1rem;
      background: #fff;
      border: 1px solid #ddd;
    }
    h1 {
      text-align: center;
      margin-bottom: 1.5rem;
    }
    .grid {
      display: flex;
      gap: 1.5rem;
      justify-content: center;
    }
    .col {
      display: flex;
      flex-direction: column;
      gap: 0.75rem;
      min-width: 300px;
    }
    .sensor-row {
      display: flex;
      align-items: center;
      gap: 0.75rem;
    }
    .sensor-row .sensor-id {
      width: 2.2rem;
      text-align: center;
      font-weight: 600;
    }
    .sensor-row .bar-wrap {
      flex: 1;
      display: flex;
      gap: 0.5rem;
      align-items: center;
    }
    .sensor-row .bar-wrap.right {
      justify-content: flex-end;
    }
    .bar {
      flex: 1;
      background: var(--bar-bg);
      border: 1px solid var(--bar-border);
      height: var(--bar-height);
      border-radius: 4px;
      overflow: hidden;
      position: relative;
    }
    .bar .bar-fill {
      height: 100%;
      width: 0%;
      background: #ffe600;
      transition: width 0.2s ease-out, background 0.2s ease-out;
    }
    .value {
      min-width: 4rem;
      text-align: center;
      font-weight: 500;
    }
    .sensor-row.stale .bar {
      border: 2px solid var(--stale-border);
    }
    .bar.missing {
      background-image: repeating-linear-gradient(
        45deg,
        #e2e2e2 0,
        #e2e2e2 5px,
        #ffffff 5px,
        #ffffff 10px
      );
    }
    .actions {
      margin-top: 1.5rem;
      display: flex;
      gap: 1rem;
      align-items: center;
    }
    button {
      background: #4caf50;
      border: none;
      color: #fff;
      padding: 0.5rem 1.25rem;
      border-radius: 4px;
      cursor: pointer;
      font-size: 1rem;
    }
    button:hover { background: #449d48; }
  </style>
</head>
<body>
  <div class="page">
    <h1>Sensormetingen</h1>

    <div class="grid">
      <div class="col" id="leftCol"></div>
      <div class="col" id="rightCol"></div>
    </div>

    <div class="actions">
      <button id="downloadBtn">Download</button>
      <span id="status">...</span>
    </div>
  </div>

  <script>
    const SENSOR_IDS = [1,2,3,4, 8,7,6,5];
    const MAX_VALUE = 1023;
    const STALE_MS = 5000;
    const MISSING_MS = 60000;
    const POLL_MS = 200;

    const statusEl = document.getElementById("status");
    const downloadBtn = document.getElementById("downloadBtn");
    const leftCol = document.getElementById("leftCol");
    const rightCol = document.getElementById("rightCol");

    const sensorEls = {};

    SENSOR_IDS.forEach((id, i) => {
      const row = document.createElement("div");
      row.className = "sensor-row";
      row.dataset.id = id;

      if (i < 4) {
        row.innerHTML = `
          <div class="sensor-id">${id}</div>
          <div class="bar-wrap">
            <div class="bar"><div class="bar-fill"></div></div>
            <div class="value">?</div>
          </div>
        `;
        leftCol.appendChild(row);
      } else {
        row.innerHTML = `
          <div class="bar-wrap right">
            <div class="value">?</div>
            <div class="bar"><div class="bar-fill"></div></div>
          </div>
          <div class="sensor-id">${id}</div>
        `;
        rightCol.appendChild(row);
      }

      const bar = row.querySelector(".bar");
      bar.classList.add("missing");
      sensorEls[id] = row;
    });

    function lerpColor(v){
      const r = 255;
      const g = Math.round(230 * (1 - v));
      const b = 0;
      return `rgb(${r},${g},${b})`;
    }

    async function fetchSensors(){
      try{
        const res = await fetch("/api/sensors");
        if(!res.ok) throw new Error(res.status);
        const data = await res.json();

        data.sensors.forEach(s => {
          const row = sensorEls[s.id];
          if (!row) return;

          const bar  = row.querySelector(".bar");
          const fill = row.querySelector(".bar-fill");
          const val  = row.querySelector(".value");

          row.classList.remove("stale");
          bar.classList.remove("missing");

          const age = s.seen ? s.age_ms : Infinity;

          if (!s.seen || age > MISSING_MS) {
            bar.classList.add("missing");
            fill.style.width = "0%";
            val.textContent = "?";
          } else {
            const v = Math.max(0, Math.min(MAX_VALUE, s.value));
            fill.style.width = (v / MAX_VALUE * 100) + "%";
            fill.style.background = lerpColor(v / MAX_VALUE);
            val.textContent = v;

            if (age > STALE_MS) {
              row.classList.add("stale");
            }
          }
        });

        statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
      } catch (e) {
        statusEl.textContent = "Fout: " + e.message;
      }
    }

    setInterval(fetchSensors, POLL_MS);
    fetchSensors();

    downloadBtn.addEventListener("click", () => {
      let csv = "";
      const nowStr = new Date().toLocaleString("nl-NL", { dateStyle: "short", timeStyle: "medium" });
      csv += "Naam : , ...\r\n";
      csv += "Tijdstip : , " + nowStr + "\r\n\r\n";
      csv += "Sensor id,Meetwaarde\r\n";

      SENSOR_IDS.forEach(id => {
        const row = sensorEls[id];
        const val = row.querySelector(".value").textContent;
        csv += id + "," + val + "\r\n";
      });

      const blob = new Blob([csv], { type: "text/csv" });
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");
      a.href = url;
      a.download = "sensors.csv";
      a.click();
      URL.revokeObjectURL(url);
    });
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
