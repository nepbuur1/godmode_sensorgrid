// by Marius Versteegen, 2025
// Recording circle timeline chart for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_RECCHART_JS[] = R"rawliteral(
    function updateRecChart() {
      const canvas = document.getElementById('recChart');
      if (!canvas) return;
      if (recFrames.length === 0 || selectedSensorId === null || selectedCircleIdx === null ||
          (recState !== 'playing' && recState !== 'playpaused')) {
        canvas.style.display = 'none';
        return;
      }
      canvas.style.display = 'block';
      const ctx = canvas.getContext('2d');
      const W = canvas.width;
      const H = canvas.height;
      ctx.clearRect(0, 0, W, H);

      // Extract circle values from recording frames
      const vals = [];
      for (let f = 0; f < recFrames.length; f++) {
        const frame = recFrames[f];
        let v = null;
        for (const sd of frame.sensors) {
          if (sd.id === selectedSensorId) {
            const s = sensors[selectedSensorId];
            if (sd.values && selectedCircleIdx < sd.values.length) {
              const raw = sd.values[selectedCircleIdx];
              const offsetted = s.offsets ? raw - (s.offsets[selectedCircleIdx] || 0) : raw;
              if (normIndivCap && s.indivCaps && s.indivCaps.length === sd.values.length) {
                v = indivLookup(offsetted, s.indivCaps[selectedCircleIdx]);
              } else if (normSumCap && s.maxSumCaptured != null) {
                const denom = s.maxSumCaptured - s.avResidualNoiseSum;
                if (denom > 0) {
                  const cg = getCalGrams(selectedSensorId);
                  v = cg * offsetted / denom;
                } else {
                  v = offsetted;
                }
              } else {
                v = offsetted;
              }
            }
            break;
          }
        }
        vals.push(v !== null ? v : 0);
      }

      if (vals.length === 0) { canvas.style.display = 'none'; return; }

      // Compute y range
      let yMin = vals[0], yMax = vals[0];
      for (const v of vals) {
        if (v < yMin) yMin = v;
        if (v > yMax) yMax = v;
      }
      if (yMin > 0) yMin = 0;
      if (yMax <= yMin) yMax = yMin + 1;

      function niceStep(range, targetTicks) {
        const rough = range / targetTicks;
        const mag = Math.pow(10, Math.floor(Math.log10(rough)));
        const residual = rough / mag;
        let step;
        if (residual <= 1.5) step = mag;
        else if (residual <= 3.5) step = 2 * mag;
        else if (residual <= 7.5) step = 5 * mag;
        else step = 10 * mag;
        return step;
      }

      const step = niceStep(yMax - yMin, 7);
      const tickMin = Math.floor(yMin / step) * step;
      const tickMax = Math.ceil(yMax / step) * step;
      const yRange = tickMax - tickMin;

      const pi = Math.max(0, Math.min(playIdx, vals.length - 1));
      const splitX = Math.round(W * 0.75);
      const gap = 10;

      // === LEFT FIGURE (overview) ===
      const lMarginL = 60, lMarginR = 5, lMarginT = 10, lMarginB = 30;
      const lPlotW = splitX - lMarginL - lMarginR;
      const lPlotH = H - lMarginT - lMarginB;

      // Axes
      ctx.strokeStyle = '#888';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(lMarginL, lMarginT);
      ctx.lineTo(lMarginL, lMarginT + lPlotH);
      ctx.lineTo(lMarginL + lPlotW, lMarginT + lPlotH);
      ctx.stroke();

      // Y-axis ticks and labels
      ctx.fillStyle = '#ccc';
      ctx.font = '10px monospace';
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      for (let tick = tickMin; tick <= tickMax + step * 0.01; tick += step) {
        const y = lMarginT + lPlotH - (tick - tickMin) / yRange * lPlotH;
        ctx.fillText(Math.round(tick), lMarginL - 5, y);
        ctx.strokeStyle = '#444';
        ctx.beginPath();
        ctx.moveTo(lMarginL, y);
        ctx.lineTo(lMarginL + lPlotW, y);
        ctx.stroke();
      }

      // Data line
      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      for (let i = 0; i < vals.length; i++) {
        const x = lMarginL + (vals.length > 1 ? i / (vals.length - 1) * lPlotW : lPlotW / 2);
        const y = lMarginT + lPlotH - (vals[i] - tickMin) / yRange * lPlotH;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // Playback dot
      const lDotX = lMarginL + (vals.length > 1 ? pi / (vals.length - 1) * lPlotW : lPlotW / 2);
      const lDotY = lMarginT + lPlotH - (vals[pi] - tickMin) / yRange * lPlotH;
      ctx.fillStyle = '#fff';
      ctx.beginPath();
      ctx.arc(lDotX, lDotY, 4, 0, 2 * Math.PI);
      ctx.fill();

      // X-axis labels
      ctx.fillStyle = '#888';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      ctx.fillText('Frame 0', lMarginL, lMarginT + lPlotH + 5);
      ctx.fillText('Frame ' + (vals.length - 1), lMarginL + lPlotW, lMarginT + lPlotH + 5);

      // === RIGHT FIGURE (zoomed) ===
      const rLeft = splitX + gap;
      const rMarginL = 5, rMarginR = 10, rMarginT = lMarginT, rMarginB = lMarginB;
      const rPlotW = W - rLeft - rMarginL - rMarginR;
      const rPlotH = lPlotH;
      const rOriginX = rLeft + rMarginL;

      // Axes
      ctx.strokeStyle = '#888';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(rOriginX, rMarginT);
      ctx.lineTo(rOriginX, rMarginT + rPlotH);
      ctx.lineTo(rOriginX + rPlotW, rMarginT + rPlotH);
      ctx.stroke();

      // Horizontal grid lines (same y ticks, no labels)
      for (let tick = tickMin; tick <= tickMax + step * 0.01; tick += step) {
        const y = rMarginT + rPlotH - (tick - tickMin) / yRange * rPlotH;
        ctx.strokeStyle = '#444';
        ctx.beginPath();
        ctx.moveTo(rOriginX, y);
        ctx.lineTo(rOriginX + rPlotW, y);
        ctx.stroke();
      }

      // Zoom: show 1/10 of total frames, centered on playIdx
      const zoomFactor = 10;
      const halfSpan = (vals.length - 1) / zoomFactor / 2;
      const centerIdx = pi;
      const xMinF = centerIdx - halfSpan;
      const xMaxF = centerIdx + halfSpan;

      // Data line (clipped to right panel)
      ctx.save();
      ctx.beginPath();
      ctx.rect(rOriginX, rMarginT, rPlotW, rPlotH);
      ctx.clip();
      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      let started = false;
      for (let i = 0; i < vals.length; i++) {
        const t = (xMaxF !== xMinF) ? (i - xMinF) / (xMaxF - xMinF) : 0.5;
        const x = rOriginX + t * rPlotW;
        const y = rMarginT + rPlotH - (vals[i] - tickMin) / yRange * rPlotH;
        if (!started) { ctx.moveTo(x, y); started = true; }
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.restore();

      // Playback dot (always at center)
      const rDotX = rOriginX + rPlotW / 2;
      const rDotY = rMarginT + rPlotH - (vals[pi] - tickMin) / yRange * rPlotH;
      ctx.fillStyle = '#fff';
      ctx.beginPath();
      ctx.arc(rDotX, rDotY, 4, 0, 2 * Math.PI);
      ctx.fill();
    }
)rawliteral";

} // end namespace crt
