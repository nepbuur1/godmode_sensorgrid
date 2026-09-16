// by Marius Versteegen, 2025
// Plot drawing functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_PLOT_JS[] = R"rawliteral(
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
)rawliteral";

} // end namespace crt
