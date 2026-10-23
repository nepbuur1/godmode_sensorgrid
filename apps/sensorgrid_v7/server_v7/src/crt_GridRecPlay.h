// by Marius Versteegen, 2025
// Record and play functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_RECPLAY_JS[] = R"rawliteral(
    // recState: 'idle' | 'recording' | 'paused' | 'stopped' | 'playing' | 'playpaused'
    var recState = 'idle';
    var recFrames = [];
    var playIdx = 0;
    // Tag van de geladen/opgenomen recording (meta v2, B8): null = gewone opname;
    // {kind:'calibration', calibratedAt, tool, version} na "Herkalibreer druksensoren" of na
    // upload van zo'n bestand. Gaat mee in de download (metaAppendForExport).
    var recordingTag = null;
    function recExportTag() {
      return recordingTag || {kind: 'normal', calibratedAt: null, tool: 'web', version: 1};
    }
    // Tijdstempel in LOKALE tijd (CEST) voor bestandsnamen: YYYY-MM-DDTHH-MM-SS.
    // (De datums in de data zelf, zoals calibDate, blijven ISO/UTC met 'Z'.)
    function recLocalStamp(d) {
      d = d || new Date();
      var p = function(n) { return String(n).padStart(2, '0'); };
      return d.getFullYear() + '-' + p(d.getMonth() + 1) + '-' + p(d.getDate()) + 'T' +
             p(d.getHours()) + '-' + p(d.getMinutes()) + '-' + p(d.getSeconds());
    }
    // ISO-tijd (UTC) leesbaar in lokale tijd: "YYYY-MM-DD HH:MM" (of met seconden).
    function recFmtLocal(iso, withSeconds) {
      var d = new Date(iso);
      if (isNaN(d.getTime())) return String(iso);
      var s = recLocalStamp(d).replace('T', ' ');
      s = s.slice(0, 10) + ' ' + s.slice(11).replace(/-/g, ':');
      return withSeconds ? s : s.slice(0, 16);
    }
    function recTagText() {
      if (!recordingTag || recordingTag.kind !== 'calibration') return '';
      var d = recordingTag.calibratedAt ? recFmtLocal(recordingTag.calibratedAt) : '?';
      return ' \u00b7 kalibratie-opname (' + d + ')';
    }

    function recRecord() {
      recFrames = [];
      playIdx = 0;
      recordingTag = null;
      miReset(MI_REC);
      if (typeof acOnRecordingLoaded === 'function') acPreview = null;
      recState = 'recording';
      document.getElementById('btnRecord').classList.add('rec-active');
      document.getElementById('btnPause').disabled = false;
      document.getElementById('btnPause').classList.remove('pause-active');
      document.getElementById('btnStop').disabled = false;
      document.getElementById('btnPlay').disabled = true;
      document.getElementById('btnPlay').classList.remove('play-active');
      document.getElementById('btnPlayPause').disabled = true;
      document.getElementById('btnPlayPause').classList.remove('pause-active');
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
        miContentChanged(MI_REC);
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
      if (typeof acOnRecordingLoaded === 'function') acOnRecordingLoaded();
      recUpdateInfo();
      updateRecChart();
      miAfterStop();
    }

    // Bij Play: als er nog geen sensor geselecteerd is, de eerste ingeschakelde sensor met data
    // in de recording selecteren, en daarvan (als er nog geen cirkel geselecteerd is) de cirkel
    // met de laagste index - met hetzelfde effect als een muisklik.
    function recSensorCount(id) {
      for (const fr of recFrames) {
        const sd = fr.sensors.find(function(x) { return x.id === id; });
        if (sd && sd.count > 0) return sd.count;
      }
      return 0;
    }
    function recAutoSelect() {
      if (selectedSensorId === null) {
        const first = SENSOR_IDS.find(function(id) { return sensors[id].enabled && recSensorCount(id) > 0; });
        if (first === undefined) return;
        selectSensor(first);
      }
      if (selectedCircleIdx !== null) return;
      const s = sensors[selectedSensorId];
      const count = recSensorCount(selectedSensorId);
      if (!s.enabled || count === 0) return;
      if (s.cells.length === 0) createGrid(s, count, selectedSensorId);
      selectCircle(selectedSensorId, 0, { stopPropagation: function() {} });
    }

    function recPlay() {
      if (recFrames.length === 0) return;
      if (recState === 'playing' || recState === 'playpaused') {
        recState = 'stopped';
        document.getElementById('btnPlay').classList.remove('play-active');
        document.getElementById('btnPlayPause').classList.remove('pause-active');
        document.getElementById('btnPlayPause').disabled = true;
        recUpdateInfo();
        updateRecChart();
        return;
      }
      recState = 'playing';
      playIdx = 0;
      recResetFilters();
      recAutoSelect();
      document.getElementById('btnPlay').classList.add('play-active');
      document.getElementById('btnPlayPause').disabled = false;
      document.getElementById('btnRecord').classList.remove('rec-active');
      recUpdateInfo();
      updateRecChart();
    }

    function recPlayPause() {
      if (recState === 'playing') {
        recState = 'playpaused';
        document.getElementById('btnPlayPause').classList.add('pause-active');
      } else if (recState === 'playpaused') {
        recState = 'playing';
        recAutoSelect();
        document.getElementById('btnPlayPause').classList.remove('pause-active');
      }
      recUpdateInfo();
      updateRecChart();
    }

    // Download via de dialogen van gridmeasinfo.js (soort meting, patient-id, aantekening).
    function recDownload() {
      if (recFrames.length === 0) return;
      miDownloadClick(MI_REC);
    }

    function recUpload(event) {
      const file = event.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const parsed = JSON.parse(e.target.result);
          const split = metaSplitFromImport(parsed);
          const data = split.frames;
          if (!Array.isArray(data) || data.length === 0) {
            alert('Invalid recording file: expected a non-empty JSON array.');
            return;
          }
          recFrames = data;
          playIdx = 0;
          recordingTag = (split.meta && split.meta.recording && split.meta.recording.kind === 'calibration')
            ? split.meta.recording : null;
          if (split.meta) applyMetaState(split.meta);
          miLoaded(MI_REC, split.meta, file.name);
          if (typeof acOnRecordingLoaded === 'function') acOnRecordingLoaded();
          recState = 'stopped';
          document.getElementById('btnRecord').classList.remove('rec-active');
          document.getElementById('btnPause').disabled = true;
          document.getElementById('btnPause').classList.remove('pause-active');
          document.getElementById('btnStop').disabled = true;
          document.getElementById('btnPlay').disabled = false;
          document.getElementById('btnPlay').classList.remove('play-active');
          document.getElementById('btnPlayPause').disabled = true;
          document.getElementById('btnPlayPause').classList.remove('pause-active');
          document.getElementById('btnDownload').disabled = false;
          recUpdateInfo();
          updateRecChart();
        } catch (err) {
          alert('Failed to parse recording file: ' + err.message);
        }
      };
      reader.readAsText(file);
      event.target.value = '';
    }

    function recSliderChange(val) {
      playIdx = parseInt(val);
      recUpdateInfo();
      updateRecChart();
    }

    function recUpdateInfo() {
      const el = document.getElementById('recInfo');
      if (recState === 'recording') el.textContent = 'Recording: ' + recFrames.length + ' frames';
      else if (recState === 'paused') el.textContent = 'Paused: ' + recFrames.length + ' frames';
      else if (recState === 'stopped') el.textContent = 'Stopped: ' + recFrames.length + ' frames' + recTagText();
      else if (recState === 'playing') el.textContent = 'Playing: ' + playIdx + '/' + recFrames.length + recTagText();
      else if (recState === 'playpaused') el.textContent = 'Paused: ' + playIdx + '/' + recFrames.length + recTagText();
      else el.textContent = 'Ready';
      if (typeof acUpdateRecalibButton === 'function') acUpdateRecalibButton();
      var sl = document.getElementById('recSlider');
      var show = (recState === 'playing' || recState === 'playpaused');
      document.getElementById('recSliderRow').style.display = show ? 'block' : 'none';
      if (show) { sl.max = Math.max(0, recFrames.length - 1); sl.value = playIdx; }
    }

    // ---- Afspelen: zelfde Value Filter als de cirkels, ook voor de loadcell ----
    // Filtertoestand van de cellen wissen, zodat de EMA (updateSensor) op het volgende frame
    // opnieuw begint - zoals de grafiek, die vanaf frame 0 filtert.
    function recResetFilters() {
      SENSOR_IDS.forEach(function(id) { sensors[id].filteredValues = null; sensors[id].emaRawValues = null; });
    }
    // Frame idx op het grid zetten: de sensorwaarden gaan door de EMA van updateSensor; de
    // loadcell-gram wordt vervangen door de op dezelfde manier gefilterde reeks
    // (recLcFilteredSeries, gridrecchart.js), zodat loadcell-venster en groene curve gelijk zijn.
    function recApplyFrame(idx, quiet) {
      const fr = recFrames[idx];
      if (!fr) return;
      const copy = { sensors: fr.sensors.map(function(sd) {
        if (!sd.hasLoadcell || !sensors[sd.id]) return sd;
        return Object.assign({}, sd, { loadcellGram: recLcFilteredSeries(sd.id)[idx] });
      }) };
      applyFrame(copy, quiet);
    }
    // Aantal frames waarna de EMA zijn startwaarde tot 0,1 % is vergeten (max 400).
    function recWarmupFrames(vf) {
      if (vf <= 0) return 0;
      if (vf >= 1) return 400;
      return Math.min(400, Math.ceil(Math.log(1e-3) / Math.log(vf)));
    }
    // Weergave in pauze of na een seek: de cel-EMA wordt stil opnieuw opgebouwd vanaf frame
    // idx-K en eindigt op frame idx. De cellen tonen dan dezelfde gefilterde waarde als de
    // grafiek, i.p.v. bij herhaald toepassen naar de ongefilterde framewaarde te convergeren.
    function recShowFrame(idx) {
      if (idx < 0 || idx >= recFrames.length) return;
      const start = Math.max(0, idx - recWarmupFrames(getValueFilter()));
      recResetFilters();
      for (let f = start; f < idx; f++) recApplyFrame(f, true);
      recApplyFrame(idx, false);
    }
)rawliteral";

} // end namespace crt
