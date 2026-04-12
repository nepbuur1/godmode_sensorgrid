// by Marius Versteegen, 2025
// Recording circle timeline chart for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_RECCHART_JS[] = R"rawliteral(
    var recZoomFactor = 10;
    var recChartLayout = null;

    function initRecZoom() {
      const saved = getCookie("gvRecZoom");
      if (saved) {
        recZoomFactor = parseFloat(saved) || 10;
        recZoomFactor = Math.max(10, Math.min(1000, recZoomFactor));
      }
      const t = (Math.log(recZoomFactor) - Math.log(10)) / (Math.log(1000) - Math.log(10));
      document.getElementById('recZoomSlider').value = Math.round(t * 100);
      document.getElementById('recZoomVal').textContent = Math.round(recZoomFactor) + 'x';

      const canvas = document.getElementById('recChart');
      canvas.addEventListener('mousedown', recChartMouseDown);
      document.addEventListener('mousemove', recChartMouseMove);
      canvas.addEventListener('touchstart', recChartTouchStart, {passive: false});
      document.addEventListener('touchmove', recChartTouchMove, {passive: false});
      document.addEventListener('touchend', function() { recChartMouseIsDown = false; });
    }

    function recZoomChange(val) {
      const t = val / 100;
      recZoomFactor = Math.exp(Math.log(10) + t * (Math.log(1000) - Math.log(10)));
      document.getElementById('recZoomVal').textContent = Math.round(recZoomFactor) + 'x';
      setCookie("gvRecZoom", recZoomFactor, 365);
      updateRecChart();
    }

    var recChartMouseIsDown = false;
    document.addEventListener('mouseup', function() { recChartMouseIsDown = false; });

    function recChartHitTest(ev, leftOnly) {
      if (!recChartLayout || recFrames.length === 0) return -1;
      const canvas = document.getElementById('recChart');
      const rect = canvas.getBoundingClientRect();
      const scaleX = canvas.width / rect.width;
      const mx = (ev.clientX - rect.left) * scaleX;
      const L = recChartLayout;

      // Left figure
      if (mx >= L.lMarginL && mx <= L.lMarginL + L.lPlotW) {
        const t = (mx - L.lMarginL) / L.lPlotW;
        return Math.round(t * (recFrames.length - 1));
      }
      // Right figure (click only, not drag)
      if (!leftOnly && mx >= L.rOriginX && mx <= L.rOriginX + L.rPlotW) {
        const t = (mx - L.rOriginX) / L.rPlotW;
        const frame = L.xMinF + t * (L.xMaxF - L.xMinF);
        return Math.max(0, Math.min(recFrames.length - 1, Math.round(frame)));
      }
      return -1;
    }

    function recChartSeek(ev, leftOnly) {
      const idx = recChartHitTest(ev, leftOnly);
      if (idx < 0) return;
      playIdx = idx;
      recUpdateInfo();
      if (recState === 'playpaused' && playIdx > 0) {
        applyFrame(recFrames[playIdx - 1]);
      }
      updateRecChart();
    }

    function recChartMouseDown(ev) {
      ev.preventDefault();
      recChartMouseIsDown = true;
      if (recState === 'playing') {
        recPlayPause();
      }
      recChartSeek(ev);
    }

    function recChartMouseMove(ev) {
      if (recChartMouseIsDown) recChartSeek(ev, true);
    }

    function recChartTouchStart(ev) {
      ev.preventDefault();
      recChartMouseIsDown = true;
      if (recState === 'playing') {
        recPlayPause();
      }
      recChartSeek(ev.touches[0]);
    }

    function recChartTouchMove(ev) {
      if (recChartMouseIsDown) {
        ev.preventDefault();
        recChartSeek(ev.touches[0], true);
      }
    }

    function updateRecChart() {
      const canvas = document.getElementById('recChart');
      if (!canvas) return;
      const chartVisible = recFrames.length > 0 && selectedSensorId !== null && selectedCircleIdx !== null &&
          (recState === 'playing' || recState === 'playpaused');
      document.getElementById('recZoomRow').style.display = chartVisible ? 'inline-flex' : 'none';
      document.getElementById('recCircleVal').style.display = chartVisible ? 'inline' : 'none';
      if (!chartVisible) {
        canvas.style.display = 'none';
        recChartLayout = null;
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

      // Show current circle value
      const pi = Math.max(0, Math.min(playIdx, vals.length - 1));
      document.getElementById('recCircleVal').textContent = 'val=' + Math.round(vals[pi]);

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

      const splitX = Math.round(W * 0.75);
      const gap = 10;

      // === LEFT FIGURE (overview) ===
      const lMarginL = 60, lMarginR = 5, lMarginT = 10, lMarginB = 30;
      const lPlotW = splitX - lMarginL - lMarginR;
      const lPlotH = H - lMarginT - lMarginB;

      ctx.strokeStyle = '#888';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(lMarginL, lMarginT);
      ctx.lineTo(lMarginL, lMarginT + lPlotH);
      ctx.lineTo(lMarginL + lPlotW, lMarginT + lPlotH);
      ctx.stroke();

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

      const lDotX = lMarginL + (vals.length > 1 ? pi / (vals.length - 1) * lPlotW : lPlotW / 2);
      const lDotY = lMarginT + lPlotH - (vals[pi] - tickMin) / yRange * lPlotH;
      ctx.fillStyle = '#fff';
      ctx.beginPath();
      ctx.arc(lDotX, lDotY, 4, 0, 2 * Math.PI);
      ctx.fill();

      ctx.fillStyle = '#888';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      ctx.fillText('Frame 0', lMarginL, lMarginT + lPlotH + 5);
      ctx.fillText('Frame ' + (vals.length - 1), lMarginL + lPlotW, lMarginT + lPlotH + 5);

      // === RIGHT FIGURE (zoomed) ===
      const rLeft = splitX + gap;
      const rMarginL = 5, rMarginR = 10, rMarginT2 = lMarginT, rMarginB2 = lMarginB;
      const rPlotW = W - rLeft - rMarginL - rMarginR;
      const rPlotH = lPlotH;
      const rOriginX = rLeft + rMarginL;

      ctx.strokeStyle = '#888';
      ctx.lineWidth = 1;
      ctx.beginPath();
      ctx.moveTo(rOriginX, rMarginT2);
      ctx.lineTo(rOriginX, rMarginT2 + rPlotH);
      ctx.lineTo(rOriginX + rPlotW, rMarginT2 + rPlotH);
      ctx.stroke();

      for (let tick = tickMin; tick <= tickMax + step * 0.01; tick += step) {
        const y = rMarginT2 + rPlotH - (tick - tickMin) / yRange * rPlotH;
        ctx.strokeStyle = '#444';
        ctx.beginPath();
        ctx.moveTo(rOriginX, y);
        ctx.lineTo(rOriginX + rPlotW, y);
        ctx.stroke();
      }

      const zoomFactor = recZoomFactor;
      const halfSpan = (vals.length - 1) / zoomFactor / 2;
      const centerIdx = pi;
      const xMinF = centerIdx - halfSpan;
      const xMaxF = centerIdx + halfSpan;

      // Store layout for hit testing
      recChartLayout = { lMarginL, lPlotW, rOriginX, rPlotW, xMinF, xMaxF };

      ctx.save();
      ctx.beginPath();
      ctx.rect(rOriginX, rMarginT2, rPlotW, rPlotH);
      ctx.clip();
      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      let started = false;
      for (let i = 0; i < vals.length; i++) {
        const t = (xMaxF !== xMinF) ? (i - xMinF) / (xMaxF - xMinF) : 0.5;
        const x = rOriginX + t * rPlotW;
        const y = rMarginT2 + rPlotH - (vals[i] - tickMin) / yRange * rPlotH;
        if (!started) { ctx.moveTo(x, y); started = true; }
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.restore();

      const rDotX = rOriginX + rPlotW / 2;
      const rDotY = rMarginT2 + rPlotH - (vals[pi] - tickMin) / yRange * rPlotH;
      ctx.fillStyle = '#fff';
      ctx.beginPath();
      ctx.arc(rDotX, rDotY, 4, 0, 2 * Math.PI);
      ctx.fill();

      // Time span label below right figure
      const fps = 1000 / POLL_MS;
      const spanFrames = xMaxF - xMinF;
      const spanSec = spanFrames / fps;
      ctx.fillStyle = '#888';
      ctx.textAlign = 'center';
      ctx.textBaseline = 'top';
      ctx.fillText('<- ' + spanSec.toFixed(1) + 's ->', rOriginX + rPlotW / 2, rMarginT2 + rPlotH + 5);
    }
)rawliteral";

} // end namespace crt
