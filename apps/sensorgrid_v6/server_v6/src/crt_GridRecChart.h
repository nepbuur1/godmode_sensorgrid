// by Marius Versteegen, 2025
// Recording circle timeline chart for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_RECCHART_JS[] = R"rawliteral(
    function updateRecChart() {
      const canvas = document.getElementById('recChart');
      if (!canvas) return;
      if (recFrames.length === 0 || selectedSensorId === null || selectedCircleIdx === null) {
        canvas.style.display = 'none';
        return;
      }
      canvas.style.display = 'block';
      const ctx = canvas.getContext('2d');
      const W = canvas.width;
      const H = canvas.height;
      ctx.clearRect(0, 0, W, H);

      const marginL = 60, marginR = 10, marginT = 10, marginB = 30;
      const plotW = W - marginL - marginR;
      const plotH = H - marginT - marginB;

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

      // Compute nice tick step
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

      // Draw axes
      ctx.strokeStyle = '#888';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(marginL, marginT);
      ctx.lineTo(marginL, marginT + plotH);
      ctx.lineTo(marginL + plotW, marginT + plotH);
      ctx.stroke();

      // Y-axis ticks and labels
      ctx.fillStyle = '#ccc';
      ctx.font = '10px monospace';
      ctx.textAlign = 'right';
      ctx.textBaseline = 'middle';
      for (let tick = tickMin; tick <= tickMax + step * 0.01; tick += step) {
        const y = marginT + plotH - (tick - tickMin) / yRange * plotH;
        ctx.fillText(Math.round(tick), marginL - 5, y);
        ctx.strokeStyle = '#444';
        ctx.beginPath();
        ctx.moveTo(marginL, y);
        ctx.lineTo(marginL + plotW, y);
        ctx.stroke();
      }

      // Draw data line
      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      for (let i = 0; i < vals.length; i++) {
        const x = marginL + (vals.length > 1 ? i / (vals.length - 1) * plotW : plotW / 2);
        const y = marginT + plotH - (vals[i] - tickMin) / yRange * plotH;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // X-axis label
      ctx.fillStyle = '#888';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      ctx.fillText('Frame 0', marginL, marginT + plotH + 5);
      ctx.fillText('Frame ' + (vals.length - 1), marginL + plotW, marginT + plotH + 5);
    }
)rawliteral";

} // end namespace crt
