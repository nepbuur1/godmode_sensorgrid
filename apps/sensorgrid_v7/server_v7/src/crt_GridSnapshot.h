// by Marius Versteegen, 2025
// Snapshot functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_SNAPSHOT_JS[] = R"rawliteral(
    var snapFrames = [];
    var snapReplaying = false;
    var snapIdx = 0;

    // Loadcell: net als bij recordings alleen een ruwe waarde, geen loadcellGram; de gram volgt
    // bij het afspelen uit de tarra/schaal in de meta. Zoals de cirkelwaarden (emaRawValues) de
    // gefilterde waarden zijn, is dit de ruwe waarde die bij de getoonde, gefilterde gram hoort.
    function snapLoadcellRaw(s) {
      if (!s.enabled) return 0;
      if (s.filteredWeight === null) return s.lcLastRaw;
      return Math.round(s.filteredWeight * s.lcScale + s.lcTareOffset);
    }

    function snapTake() {
      const frame = {sensors: SENSOR_IDS.map(id => {
        const s = sensors[id];
        return {
          id: id,
          count: (s.enabled && s.emaRawValues) ? s.emaRawValues.length : 0,
          values: (s.enabled && s.emaRawValues) ? s.emaRawValues.map(v => Math.round(v)) : [],
          hasLoadcell: s.enabled ? s.lcVisible : false,
          loadcellRaw: snapLoadcellRaw(s)
        };
      })};
      snapFrames.push(frame);
      miContentChanged(MI_SNAP);
      snapUpdateUI();
    }

    // Een snapshot tonen: de loadcell-gram uit loadcellRaw (lcGramFromRaw). Oudere bestanden
    // bevatten nog een loadcellGram (de toen getoonde waarde); die blijft daar gelden.
    function snapApplyFrame(idx) {
      const frame = snapFrames[idx];
      applyFrame({sensors: frame.sensors.map(sd => {
        const s = sensors[sd.id];
        if (!s || !sd.hasLoadcell || typeof sd.loadcellGram === 'number') return sd;
        return Object.assign({}, sd, { loadcellGram: lcGramFromRaw(s, sd.loadcellRaw) });
      })});
    }

    function snapClear() {
      snapFrames = [];
      if (snapReplaying) snapReplayStop();
      snapIdx = 0;
      miReset(MI_SNAP);
      snapUpdateUI();
    }

    // Download via de dialogen van gridmeasinfo.js (patient-meting?, patient-id, aantekening).
    function snapDownload() {
      if (snapFrames.length === 0) return;
      miDownloadClick(MI_SNAP);
    }

    function snapUpload(event) {
      const file = event.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const parsed = JSON.parse(e.target.result);
          const split = metaSplitFromImport(parsed);
          const data = split.frames;
          if (!Array.isArray(data) || data.length === 0) {
            alert('Invalid snapshot file: expected a non-empty JSON array.');
            return;
          }
          if (snapReplaying) snapReplayStop();
          snapFrames = data;
          snapIdx = 0;
          if (split.meta) applyMetaState(split.meta);
          miLoaded(MI_SNAP, split.meta, file.name);
          snapUpdateUI();
        } catch (err) {
          alert('Failed to parse snapshot file: ' + err.message);
        }
      };
      reader.readAsText(file);
      event.target.value = '';
    }

    function snapReplayStart() {
      if (snapFrames.length === 0) return;
      snapReplaying = true;
      snapIdx = 0;
      document.getElementById('snapReplayRow').style.display = 'none';
      document.getElementById('snapControlRow').style.display = 'flex';
      snapUpdateUI();
    }

    function snapReplayStop() {
      snapReplaying = false;
      document.getElementById('snapControlRow').style.display = 'none';
      document.getElementById('snapReplayRow').style.display = 'flex';
      snapUpdateUI();
    }

    function snapStepBack() {
      if (snapIdx > 0) snapIdx--;
      snapUpdateUI();
    }

    function snapStepForward() {
      if (snapIdx < snapFrames.length - 1) snapIdx++;
      snapUpdateUI();
    }

    function snapGotoFirst() {
      snapIdx = 0;
      snapUpdateUI();
    }

    function snapGotoLast() {
      if (snapFrames.length > 0) snapIdx = snapFrames.length - 1;
      snapUpdateUI();
    }

    function snapSliderChange(val) {
      snapIdx = parseInt(val);
      document.getElementById('snapIdxInfo').textContent = 'Index: ' + snapIdx;
    }

    function snapUpdateUI() {
      document.getElementById('snapInfo').textContent = 'Snapshots: ' + snapFrames.length;
      document.getElementById('snapIdxInfo').textContent = 'Index: ' + snapIdx;
      document.getElementById('btnSnapDownload').disabled = (snapFrames.length === 0);
      document.getElementById('btnSnapReplay').disabled = (snapFrames.length === 0);
      const slider = document.getElementById('snapSlider');
      slider.max = Math.max(0, snapFrames.length - 1);
      slider.value = snapIdx;
      const showSlider = snapReplaying && snapFrames.length > 3;
      document.getElementById('snapSliderRow').style.display = showSlider ? 'flex' : 'none';
    }
)rawliteral";

} // end namespace crt
