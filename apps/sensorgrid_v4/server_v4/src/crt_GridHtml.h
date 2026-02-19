// by Marius Versteegen, 2025
// Grid visualization page: shows sensor 1's measurements as gray-scale circles
// arranged in a diamond pattern.

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
    h1 {
      text-align: center;
      margin-bottom: 1rem;
    }
    .info {
      text-align: center;
      color: #666;
      margin-bottom: 1rem;
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
    <div class="info">Each circle shows one measurement. Gray intensity is proportional to value (0=white, 1023=black). Arranged in a diamond pattern.</div>
    <div class="grid-container" id="grid"></div>
    <div id="status">...</div>
  </div>

  <script>
    const MAX_VALUE = 1023;
    const POLL_MS = 500;

    const gridEl = document.getElementById("grid");
    const statusEl = document.getElementById("status");
    let cells = [];
    let currentCount = 0;

    function computeRowSizes(n) {
      if (n <= 0) return [];
      const w = Math.ceil(Math.sqrt(n));
      const rows = [];
      let remaining = n;
      // Ascending: 1, 2, ..., w
      for (let r = 1; r <= w && remaining > 0; r++) {
        const s = Math.min(r, remaining);
        rows.push(s);
        remaining -= s;
      }
      // Descending: w-1, w-2, ..., 1
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

    function grayValue(v) {
      const clamped = Math.max(0, Math.min(MAX_VALUE, v));
      const gray = Math.round(255 * (1 - clamped / MAX_VALUE));
      return gray;
    }

    async function fetchMeasurements() {
      try {
        const res = await fetch("/api/measurements/1");
        if (!res.ok) throw new Error(res.status);
        const data = await res.json();

        if (currentCount !== data.count) {
          createGrid(data.count);
        }

        data.values.forEach((v, i) => {
          if (i < cells.length) {
            const g = grayValue(v);
            cells[i].style.background = `rgb(${g},${g},${g})`;
            cells[i].textContent = v;
            cells[i].style.color = g < 128 ? "#ddd" : "#444";
          }
        });

        statusEl.textContent = "Laatste update: " + new Date().toLocaleTimeString();
      } catch (e) {
        statusEl.textContent = "Fout: " + e.message;
      }
    }

    createGrid(50);
    setInterval(fetchMeasurements, POLL_MS);
    fetchMeasurements();
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
