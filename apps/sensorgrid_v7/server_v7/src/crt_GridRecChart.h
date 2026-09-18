// by Marius Versteegen, 2025
// Recording circle timeline chart for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_RECCHART_JS[] = R"rawliteral(
    var recZoomFactor = 10;
    var recChartLayout = null;
    // "single y-axis": beide curves op een as; bereik = grootste van de loadcellwaarden en de
    // weergavewaarden van ALLE cirkels van de module over de hele opname (gecachet, want de
    // grafiek wordt bij elke poll opnieuw getekend).
    var recSingleY = false;
    var recRangeCache = null;   // {frames, sensorId, sumCap, indivCap, sMin, sMax, lcMin, lcMax}
    // y-clip (alleen bij single y-axis): de getoonde y-range wordt tot [recClipLo, recClipHi]
    // beperkt; waarden erbuiten worden op de rand getekend. Ongeldig (lo >= hi) = geen clip.
    var recClipLo = 0;
    var recClipHi = 3000;

    function recInvalidateRange() { recRangeCache = null; }

    // ---- Loadcell-gram voor de weergave ----
    // Ongefilterde gram van een frame: uit het bestand als loadcellGram daar ongefilterd in staat
    // (recording.lcGramRaw), anders (oudere opname, loadcellGram al live gefilterd) herberekend
    // uit loadcellRaw met de tarra/schaal van de module (bij upload uit de meta hersteld).
    function recLcGramOfFrame(sd, s) {
      if (recLcGramRaw || !sd.hasLoadcell || !s || !s.lcScale) return (typeof sd.loadcellGram === 'number') ? sd.loadcellGram : 0;
      return (sd.loadcellRaw - s.lcTareOffset) / s.lcScale;
    }
    // Zelfde Value Filter als de cirkels (updateSensor): EMA over de frames, gestart op frame 0.
    function recEmaSeries(arr, vf) {
      const out = new Array(arr.length);
      let acc = 0;
      for (let i = 0; i < arr.length; i++) {
        acc = (i === 0) ? arr[i] : acc * vf + arr[i] * (1 - vf);
        out[i] = acc;
      }
      return out;
    }
    // Gefilterde loadcell-gram per frame van een module (gecachet per frames/filter/tarra/schaal);
    // gebruikt door de groene curve en door recApplyFrame (loadcell-venster bij afspelen).
    var recLcCache = null;   // {frames, keys: {id: key}, series: {id: [...]}}
    function recLcFilteredSeries(sensorId) {
      const s = sensors[sensorId];
      const vf = getValueFilter();
      const key = recFrames.length + '|' + vf + '|' + (recLcGramRaw ? 1 : 0) + '|' + s.lcTareOffset + '|' + s.lcScale;
      if (!recLcCache || recLcCache.frames !== recFrames) recLcCache = { frames: recFrames, keys: {}, series: {} };
      if (recLcCache.keys[sensorId] === key) return recLcCache.series[sensorId];
      const raw = new Array(recFrames.length);
      for (let f = 0; f < recFrames.length; f++) {
        let v = 0;
        for (const sd of recFrames[f].sensors) { if (sd.id === sensorId) { v = recLcGramOfFrame(sd, s); break; } }
        raw[f] = v;
      }
      recLcCache.keys[sensorId] = key;
      recLcCache.series[sensorId] = recEmaSeries(raw, vf);
      return recLcCache.series[sensorId];
    }

    function recShowClipRow() {
      const row = document.getElementById('recClipRow');
      if (row) row.style.display = recSingleY ? 'inline-flex' : 'none';
    }
    function recSetClip(lo, hi) {
      lo = parseFloat(lo); hi = parseFloat(hi);
      recClipLo = isFinite(lo) ? lo : 0;
      recClipHi = isFinite(hi) ? hi : 3000;
      setCookie("gvRecClipLo", recClipLo, 365);
      setCookie("gvRecClipHi", recClipHi, 365);
      const fl = document.getElementById('fldClipLo'), fh = document.getElementById('fldClipHi');
      if (fl) fl.value = recClipLo;
      if (fh) fh.value = recClipHi;
      updateRecChart();
    }
    function recClipChange() {
      recSetClip(document.getElementById('fldClipLo').value, document.getElementById('fldClipHi').value);
    }
    function recClipValid() { return recClipLo < recClipHi; }

    function recSingleYChange(on) {
      recSingleY = !!on;
      setCookie("gvRecSingleY", recSingleY ? "1" : "0", 365);
      const chk = document.getElementById('chkSingleY');
      if (chk && chk.checked !== recSingleY) chk.checked = recSingleY;
      recShowClipRow();
      updateRecChart();
    }

    function recModuleRange(sensorId) {
      const c = recRangeCache;
      if (c && c.frames === recFrames && c.sensorId === sensorId && c.sumCap === normSumCap && c.indivCap === normIndivCap) return c;
      const s = sensors[sensorId];
      let sMin = Infinity, sMax = -Infinity, lcMin = Infinity, lcMax = -Infinity;
      for (let f = 0; f < recFrames.length; f++) {
        for (const sd of recFrames[f].sensors) {
          if (sd.id !== sensorId) continue;
          if (typeof sd.loadcellGram === 'number') {
            const lg = recLcGramOfFrame(sd, s);
            if (lg < lcMin) lcMin = lg;
            if (lg > lcMax) lcMax = lg;
          }
          if (sd.values) {
            const n = sd.values.length;
            for (let i = 0; i < n; i++) {
              const v = displayValueForCircle(s, sensorId, i, sd.values[i], n);
              if (v < sMin) sMin = v;
              if (v > sMax) sMax = v;
            }
          }
          break;
        }
      }
      if (sMin === Infinity) { sMin = 0; sMax = 0; }
      if (lcMin === Infinity) { lcMin = 0; lcMax = 0; }
      recRangeCache = { frames: recFrames, sensorId: sensorId, sumCap: normSumCap, indivCap: normIndivCap,
                        sMin: sMin, sMax: sMax, lcMin: lcMin, lcMax: lcMax };
      return recRangeCache;
    }

    function initRecZoom() {
      const saved = getCookie("gvRecZoom");
      if (saved) {
        recZoomFactor = parseFloat(saved) || 10;
        recZoomFactor = Math.max(10, Math.min(1000, recZoomFactor));
      }
      const t = (Math.log(recZoomFactor) - Math.log(10)) / (Math.log(1000) - Math.log(10));
      document.getElementById('recZoomSlider').value = Math.round(t * 100);
      document.getElementById('recZoomVal').textContent = Math.round(recZoomFactor) + 'x';
      recSingleY = (getCookie("gvRecSingleY") === "1");
      const chk = document.getElementById('chkSingleY');
      if (chk) chk.checked = recSingleY;
      const clo = getCookie("gvRecClipLo"), chi = getCookie("gvRecClipHi");
      if (clo !== null && isFinite(parseFloat(clo))) recClipLo = parseFloat(clo);
      if (chi !== null && isFinite(parseFloat(chi))) recClipHi = parseFloat(chi);
      const fl = document.getElementById('fldClipLo'), fh = document.getElementById('fldClipHi');
      if (fl) fl.value = recClipLo;
      if (fh) fh.value = recClipHi;
      recShowClipRow();

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
        recShowFrame(playIdx - 1);
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

      // Extract circle values (blue) and loadcell values (green) from frames. Beide gaan door
      // hetzelfde Value Filter als de cirkels (EMA over de frames vanaf frame 0), zodat curve,
      // "val=" en celtekst dezelfde waarde tonen.
      const rawVals = [];
      let hasLc = false;
      for (let f = 0; f < recFrames.length; f++) {
        const frame = recFrames[f];
        let v = null;
        for (const sd of frame.sensors) {
          if (sd.id === selectedSensorId) {
            if (sd.hasLoadcell) hasLc = true;
            const s = sensors[selectedSensorId];
            if (sd.values && selectedCircleIdx < sd.values.length) {
              // zelfde omrekening als de grid (raw / Norm SumCap / Norm IndivCap incl. auto-PWL)
              v = displayValueForCircle(s, selectedSensorId, selectedCircleIdx, sd.values[selectedCircleIdx], sd.values.length);
            }
            break;
          }
        }
        rawVals.push(v !== null ? v : 0);
      }

      if (rawVals.length === 0) { canvas.style.display = 'none'; return; }
      const vals = recEmaSeries(rawVals, getValueFilter());
      const lcVals = hasLc ? recLcFilteredSeries(selectedSensorId) : new Array(vals.length).fill(0);
      let lcMax = 0;
      for (const lv of lcVals) if (lv > lcMax) lcMax = lv;

      // Show current circle value.
      // The sensorgrid displays frame (playIdx - 1): during playback the frame is
      // applied to the grid and then playIdx is incremented before this redraw, and
      // in playpaused/seek the grid shows recFrames[playIdx - 1]. So the white dot
      // (and value label / zoom centre) must use playIdx - 1 to line up with the
      // value shown on the sensorgrid (phase 6y).
      const pi = Math.max(0, Math.min(playIdx - 1, vals.length - 1));
      document.getElementById('recCircleVal').textContent = 'val=' + Math.round(vals[pi]);

      // Compute y range
      let yMin = vals[0], yMax = vals[0];
      for (const v of vals) {
        if (v < yMin) yMin = v;
        if (v > yMax) yMax = v;
      }
      // single y-axis: het grootste bereik van de module (alle cirkels en de loadcell over de
      // hele opname), zodat gelijke hoogte gelijke waarde betekent en de as niet verspringt
      // bij het kiezen van een andere cirkel.
      const singleY = recSingleY && hasLc && lcMax > 0;
      if (singleY) {
        const r = recModuleRange(selectedSensorId);
        yMin = Math.min(yMin, r.sMin, r.lcMin);
        yMax = Math.max(yMax, r.sMax, r.lcMax);
      }
      if (yMin > 0) yMin = 0;
      const clipping = singleY && recClipValid();
      if (clipping) {
        yMin = Math.max(yMin, recClipLo);
        yMax = Math.min(yMax, recClipHi);
      }
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
      const nIntervals = Math.max(1, Math.round((tickMax - tickMin) / step));

      // Loadcell (green) right-axis scale: reuse the SAME horizontal grid lines.
      // The bottom grid line = 0; pick a round per-line step so the top line just
      // covers the maximum recorded loadcell value (phase 6z). e.g. lcMax=355 over
      // 6 intervals -> step 60 -> top 360, labels 0,60,120,180,240,300,360.
      const drawLc = hasLc && lcMax > 0;
      let lcStep = 1, lcTickMax = nIntervals;
      if (drawLc && !singleY) {
        const lcRough = lcMax / nIntervals;
        const lcMag = Math.pow(10, Math.floor(Math.log10(lcRough)));
        lcStep = Math.ceil(lcRough / lcMag) * lcMag;
        lcTickMax = lcStep * nIntervals;
      }
      // y-positie (fractie 0..1 van onder) van een loadcellwaarde: eigen rechter as, of
      // dezelfde as als de sensorcurve bij single y-axis.
      // Bij y-clip worden waarden buiten de getekende range op de rand gezet.
      const clampV = clipping ? (v => Math.max(recClipLo, Math.min(recClipHi, v))) : (v => v);
      const lcFrac = singleY ? (v => (clampV(v) - tickMin) / yRange) : (v => v / lcTickMax);

      const splitX = Math.round(W * 0.75);
      const gap = 10;

      // === LEFT FIGURE (overview) ===
      const lMarginL = 60, lMarginR = drawLc ? 40 : 5, lMarginT = 10, lMarginB = 30;
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
        if (drawLc) {
          const lcLab = singleY ? tick : Math.round((tick - tickMin) / step) * lcStep;
          ctx.fillStyle = '#4f4';
          ctx.textAlign = 'left';
          ctx.fillText((singleY || lcStep >= 1) ? Math.round(lcLab) : lcLab.toFixed(1), lMarginL + lPlotW + 5, y);
          ctx.fillStyle = '#ccc';
          ctx.textAlign = 'right';
        }
      }

      // Green loadcell curve first, so the blue sensor curve is drawn on top of it.
      if (drawLc) {
        ctx.strokeStyle = '#4f4';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        for (let i = 0; i < lcVals.length; i++) {
          const x = lMarginL + (lcVals.length > 1 ? i / (lcVals.length - 1) * lPlotW : lPlotW / 2);
          const y = lMarginT + lPlotH - lcFrac(lcVals[i]) * lPlotH;
          if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }

      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      for (let i = 0; i < vals.length; i++) {
        const x = lMarginL + (vals.length > 1 ? i / (vals.length - 1) * lPlotW : lPlotW / 2);
        const y = lMarginT + lPlotH - (clampV(vals[i]) - tickMin) / yRange * lPlotH;
        if (i === 0) ctx.moveTo(x, y);
        else ctx.lineTo(x, y);
      }
      ctx.stroke();

      // Yellow dot on the green loadcell curve at the current position.
      if (drawLc) {
        const lcDotX = lMarginL + (lcVals.length > 1 ? pi / (lcVals.length - 1) * lPlotW : lPlotW / 2);
        const lcDotY = lMarginT + lPlotH - lcFrac(lcVals[pi]) * lPlotH;
        ctx.fillStyle = '#ff0';
        ctx.beginPath();
        ctx.arc(lcDotX, lcDotY, 4, 0, 2 * Math.PI);
        ctx.fill();
      }

      const lDotX = lMarginL + (vals.length > 1 ? pi / (vals.length - 1) * lPlotW : lPlotW / 2);
      const lDotY = lMarginT + lPlotH - (clampV(vals[pi]) - tickMin) / yRange * lPlotH;
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
      recChartLayout = { lMarginL, lPlotW, rOriginX, rPlotW, xMinF, xMaxF, tickMin, tickMax, step, singleY, lcTickMax,
                         clipping, clipLo: recClipLo, clipHi: recClipHi };

      ctx.save();
      ctx.beginPath();
      ctx.rect(rOriginX, rMarginT2, rPlotW, rPlotH);
      ctx.clip();
      // Green loadcell curve first, so the blue sensor curve is drawn on top of it.
      if (drawLc) {
        ctx.strokeStyle = '#4f4';
        ctx.lineWidth = 1.5;
        ctx.beginPath();
        let lcStarted = false;
        for (let i = 0; i < lcVals.length; i++) {
          const t = (xMaxF !== xMinF) ? (i - xMinF) / (xMaxF - xMinF) : 0.5;
          const x = rOriginX + t * rPlotW;
          const y = rMarginT2 + rPlotH - lcFrac(lcVals[i]) * rPlotH;
          if (!lcStarted) { ctx.moveTo(x, y); lcStarted = true; }
          else ctx.lineTo(x, y);
        }
        ctx.stroke();
      }
      ctx.strokeStyle = '#4af';
      ctx.lineWidth = 1.5;
      ctx.beginPath();
      let started = false;
      for (let i = 0; i < vals.length; i++) {
        const t = (xMaxF !== xMinF) ? (i - xMinF) / (xMaxF - xMinF) : 0.5;
        const x = rOriginX + t * rPlotW;
        const y = rMarginT2 + rPlotH - (clampV(vals[i]) - tickMin) / yRange * rPlotH;
        if (!started) { ctx.moveTo(x, y); started = true; }
        else ctx.lineTo(x, y);
      }
      ctx.stroke();
      ctx.restore();

      // Yellow dot on the green loadcell curve (centred, like the white dot).
      if (drawLc) {
        const rLcDotY = rMarginT2 + rPlotH - lcFrac(lcVals[pi]) * rPlotH;
        ctx.fillStyle = '#ff0';
        ctx.beginPath();
        ctx.arc(rOriginX + rPlotW / 2, rLcDotY, 4, 0, 2 * Math.PI);
        ctx.fill();
      }

      const rDotX = rOriginX + rPlotW / 2;
      const rDotY = rMarginT2 + rPlotH - (clampV(vals[pi]) - tickMin) / yRange * rPlotH;
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
