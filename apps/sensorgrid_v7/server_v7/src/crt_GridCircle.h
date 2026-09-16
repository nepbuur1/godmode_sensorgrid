// by Marius Versteegen, 2025
// Grid/circle rendering functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_CIRCLE_JS[] = R"rawliteral(
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

    function colorForValue(v, minV, maxV) {
      const lo = normalized ? minV : 0;
      const hi = normalized ? maxV : (maxFixed ? getFixedMax() : MAX_VALUE);
      const r = (hi > lo) ? (hi - lo) : 1;
      const t = Math.max(0, Math.min(1, (v - lo) / r));

      if (!colorized) {
        const gray = Math.round(128 + 127 * t);
        return { bg: `rgb(${gray},${gray},${gray})`, dark: gray < 128 };
      }
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
      const pts = [{x:0, y:0}];
      for (const t of tuples) {
        if (t.cap !== 0 || t.grams !== 0) {
          pts.push({x: t.cap, y: t.grams});
        }
      }
      if (pts.length === 1) return 0;
      pts.sort((a, b) => a.x - b.x);
      if (rawVal <= pts[0].x) {
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
)rawliteral";

} // end namespace crt
