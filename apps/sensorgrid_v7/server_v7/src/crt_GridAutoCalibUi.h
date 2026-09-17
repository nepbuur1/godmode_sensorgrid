// by Marius Versteegen, 2026
// Automatic pressure-sensor calibration: UI glue for the Grid page,
// served separately.

#pragma once

namespace crt
{
	const char GRID_AUTOCALIBUI_JS[] = R"rawliteral(
    // gridautocalibui.js - UI glue for "Herkalibreer druksensoren" on the Grid page.
    // Uses the DOM-free core in gridautocalib.js (acCalibrateRecording, acFinishRun, ...) and
    // the page state of grid.html (sensors[], calibVersion, calibDate, recFrames, recordingTag).
    // See LLM_kalibratieDocs/StappenPlanAutoCalibratieIntegratie.md, stap 11 and 12.

    var acLastRun = null;
    const AC_HINT = "Loadcell vooraf kalibreren in deze volgorde: Tare (leeg) → gewicht erop → Calibrate. " +
                    "De herkalibratie werkt op de geladen of afgespeelde recording en gebruikt de loadcell-parameters van de pagina.";

    // Preview: when a recording is loaded (Upload) or finished (Stop), the analysis runs once
    // (~50-300 ms) so the button can tell whether the recording contains a calibration
    // staircase at all. The result is reused when the button is pressed.
    var acPreview = null;   // {frames: recFrames, result, anyOk}
    function acOnRecordingLoaded() {
      acPreview = null;
      if (recFrames.length > 0) {
        const result = acCalibrateRecording(recFrames, acLcParamsFromPage(), 'web-autocalib');
        let anyOk = false;
        for (const mid of Object.keys(result.modules)) {
          if (result.modules[mid].sensors.some(function(r) { return r.status === 'ok'; })) anyOk = true;
        }
        acPreview = { frames: recFrames, result: result, anyOk: anyOk };
      }
      acUpdateRecalibButton();
    }
    function acPreviewValid() {
      return acPreview !== null && acPreview.frames === recFrames;
    }
    // The button is only usable on a loaded recording that is not being recorded or played,
    // and only when the preview found at least one sensor with a valid staircase.
    function acCanRecalibrate() {
      return (recState === 'stopped' || recState === 'playpaused') && recFrames.length > 0 &&
             acPreviewValid() && acPreview.anyOk;
    }
    function acUpdateRecalibButton() {
      const b = document.getElementById('btnRecalib');
      if (!b) return;
      b.disabled = !acCanRecalibrate();
      if (recFrames.length > 0 && acPreviewValid() && !acPreview.anyOk) {
        b.title = 'Geen kalibratie-trap (11 treden) gevonden in deze opname: dit is geen kalibratie-opname.';
      } else {
        b.title = 'Kalibreer de druksensoren uit de geladen recording (loadcell vooraf: Tare, gewicht erop, Calibrate)';
      }
    }

    // Loadcell parameters per module from the page state (after an upload equal to the meta
    // of the file; live: the last Tare/Calibrate).
    function acLcParamsFromPage() {
      const out = {};
      SENSOR_IDS.forEach(function(id) {
        out[id] = { lcScale: sensors[id].lcScale, lcTareOffset: sensors[id].lcTareOffset };
      });
      return out;
    }

    // "Herkalibreer druksensoren": run the core on recFrames, apply the version-byte rules,
    // store pwl/calVer/calLog, tag the recording, refresh the grid and show the result panel.
    function acRunRecalibrate() {
      if (!acCanRecalibrate()) return null;
      const t0 = Date.now();
      const result = acPreviewValid() ? acPreview.result
                                      : acCalibrateRecording(recFrames, acLcParamsFromPage(), 'web-autocalib');
      const bytesByModule = {}, okByModule = {};
      for (const mid of Object.keys(result.modules)) {
        const s = sensors[parseInt(mid)];
        if (!s) continue;
        const rs = result.modules[mid].sensors;
        ensurePwl(s, rs.length);
        bytesByModule[mid] = s.calVer.slice();
        okByModule[mid] = rs.map(function(r) { return r.status === 'ok'; });
      }
      const run = acFinishRun(bytesByModule, calibVersion, okByModule);
      for (const mid of Object.keys(result.modules)) {
        const id = parseInt(mid);
        const s = sensors[id];
        if (!s) continue;
        const rs = result.modules[mid].sensors;
        // calLog is the diagnosis of the last run, also when nothing was applied
        s.calLog = rs.map(function(r) { return { status: r.status, reason: r.reason || '', nTreads: r.nTreads || 0 }; });
        if (run.applied) {
          s.calVer = run.bytesByModule[mid];
          rs.forEach(function(r, i) { if (r.status === 'ok') s.pwl[i] = r.knots; });
        }
        saveCalibSensor(id);
        updateCircleBorders(s);
      }
      if (run.applied) {
        calibVersion = run.global;
        calibDate = new Date().toISOString();
        recordingTag = { kind: 'calibration', calibratedAt: calibDate, tool: 'web-autocalib', version: 1 };
        saveCalibGlobals();
      }
      acLastRun = { result: result, run: run, ms: Date.now() - t0, at: new Date().toISOString() };
      recolorAll();
      recUpdateInfo();
      acRenderResultPanel(acLastRun);
      if (selectedSensorId !== null && selectedCircleIdx !== null) acUpdateCircleInfo(selectedSensorId, selectedCircleIdx);
      return acLastRun;
    }

    // ---- "kalibratie_check": while the button is held, the circles of the selected sensor
    // show the version colour (acCheckColor) and the version byte instead of the value. ----
    var acCheckMode = false;
    const AC_CHECK_BG = { black: "#000000", green: "#2e7d32", orange: "#f57c00", red: "#d32f2f" };
    function acCheckStart(ev) {
      if (ev && ev.preventDefault) ev.preventDefault();
      if (selectedSensorId === null) return;
      acCheckMode = true;
      acPaintCheckCells(sensors[selectedSensorId]);
    }
    function acCheckStop() {
      if (!acCheckMode) return;
      acCheckMode = false;
      recolorAll();
    }
    // Called from updateSensor/colorCells while acCheckMode is on (selected sensor only).
    function acIsCheckPainting(s) {
      return acCheckMode && selectedSensorId !== null && sensors[selectedSensorId] === s && !s.is3D;
    }
    function acPaintCheckCells(s) {
      ensurePwl(s, s.cells.length);
      s.cells.forEach(function(cell, i) {
        const v = s.calVer[i] | 0;
        const c = acCheckColor(v, calibVersion);
        cell.style.background = AC_CHECK_BG[c];
        cell.style.color = "#ffffff";
        cell.textContent = String(v);
      });
    }

    // ---- circle info panel: auto-PWL line (knots and version of the selected circle) ----
    function acUpdateCircleInfo(sensorId, circleIdx) {
      const el = document.getElementById("ciAuto");
      if (!el) return;
      if (sensorId === null || circleIdx === null) { el.textContent = ""; return; }
      const s = sensors[sensorId];
      const pw = (s.pwl && s.pwl[circleIdx]) || [];
      const ver = (s.calVer && s.calVer[circleIdx]) | 0;
      const log = (s.calLog && s.calLog[circleIdx]) || null;
      if (pw.length >= 2) {
        el.textContent = "auto-PWL: " + pw.length + " knopen, versie " + ver + " (globaal " + calibVersion + "): " +
          pw.map(function(k) { return Math.round(k.raw) + "\u2192" + Math.round(k.gram) + " g"; }).join("  ");
      } else {
        el.textContent = "auto-PWL: geen" + (log && log.reason ? " (" + log.reason + ")" : "");
      }
    }

    function acEsc(s) {
      return String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
    }
    function acCategory(reason) {
      return reason ? reason.split(':')[0] : '';
    }

    // Result panel: per module the counts, a details table (ok + skipped except "niet belast"),
    // the version/date after the run, and the loadcell hint.
    function acRenderResultPanel(last) {
      const panel = document.getElementById('acResultPanel');
      if (!panel) return;
      const run = last.run;
      let html = '<div class="ac-row"><b>Herkalibreer druksensoren</b>';
      if (run.applied) {
        html += '<span>kalibratie-versie ' + calibVersion + ', ' + acEsc(String(calibDate).replace('T', ' ').slice(0, 19)) +
                (run.reset ? ', versie-bytes gereset (255 bereikt)' : '') + '</span>';
      } else {
        html += '<span class="ac-warn">geen enkele sensor goedgekeurd: kalibratie, versie en datum ongewijzigd</span>';
      }
      html += '<span class="ac-dim">' + last.ms + ' ms</span>' +
              '<button class="toggle-btn" onclick="acToggleDetails()">Details</button>' +
              '<button class="toggle-btn" onclick="acHideResultPanel()">Sluiten</button></div>';
      for (const mid of Object.keys(last.result.modules)) {
        const rs = last.result.modules[mid].sensors;
        const ok = rs.filter(function(r) { return r.status === 'ok'; }).length;
        const h1 = rs.filter(function(r) { return acCategory(r.reason) === 'H1'; }).length;
        const other = rs.length - ok - h1;
        html += '<div class="ac-row">Module ' + acEsc(mid) + ': <b>' + ok + ' ok</b>, ' + other +
                ' overgeslagen, ' + h1 + ' niet belast</div>';
      }
      html += '<div id="acDetails" style="display:none;">';
      for (const mid of Object.keys(last.result.modules)) {
        const rs = last.result.modules[mid].sensors;
        const rows = rs.filter(function(r) { return r.status === 'ok' || acCategory(r.reason) !== 'H1'; });
        if (rows.length === 0) continue;
        html += '<table class="ac-table"><tr><th>module</th><th>idx</th><th>status</th><th>treden</th><th>versie</th><th>reden (overgeslagen) / knopen raw→gram (ok)</th></tr>';
        for (const r of rows) {
          const s = sensors[parseInt(mid)];
          const ver = (s && s.calVer[r.idx] !== undefined) ? s.calVer[r.idx] : '';
          html += '<tr class="' + (r.status === 'ok' ? 'ac-ok' : 'ac-skip') + '"><td>' + acEsc(mid) + '</td><td>' + r.idx +
                  '</td><td>' + r.status + '</td><td>' + (r.nTreads || 0) + '</td><td>' + ver + '</td><td>' +
                  acEsc(r.reason || (r.knots ? r.knots.map(function(k) { return Math.round(k.raw) + '→' + Math.round(k.gram) + ' g'; }).join('  ') : '')) +
                  '</td></tr>';
        }
        html += '</table>';
      }
      html += '</div><div class="ac-dim">' + acEsc(AC_HINT) + '</div>';
      panel.innerHTML = html;
      panel.style.display = 'block';
    }
    function acToggleDetails() {
      const d = document.getElementById('acDetails');
      if (d) d.style.display = (d.style.display === 'none') ? 'block' : 'none';
    }
    function acHideResultPanel() {
      const panel = document.getElementById('acResultPanel');
      if (panel) panel.style.display = 'none';
    }
)rawliteral";

} // end namespace crt
