// by Marius Versteegen, 2025
// Snapshot functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_SNAPSHOT_JS[] = R"rawliteral(
    var snapFrames = [];
    var snapReplaying = false;
    var snapIdx = 0;

    function snapTake() {
      const frame = {sensors: SENSOR_IDS.map(id => {
        const s = sensors[id];
        return {
          id: id,
          count: (s.enabled && s.emaRawValues) ? s.emaRawValues.length : 0,
          values: (s.enabled && s.emaRawValues) ? s.emaRawValues.map(v => Math.round(v)) : [],
          hasLoadcell: s.enabled ? s.lcVisible : false,
          loadcellRaw: s.enabled ? s.lcLastRaw : 0,
          loadcellGram: (s.enabled && s.filteredWeight !== null) ? parseFloat(s.filteredWeight.toFixed(1)) : 0
        };
      })};
      snapFrames.push(frame);
      snapUpdateUI();
    }

    function snapClear() {
      snapFrames = [];
      if (snapReplaying) snapReplayStop();
      snapIdx = 0;
      snapUpdateUI();
    }

    function snapDownload() {
      if (snapFrames.length === 0) return;
      const json = JSON.stringify(snapFrames);
      const blob = new Blob([json], {type: 'application/json'});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      const ts = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
      a.download = 'snapshots_' + ts + '.json';
      a.click();
      URL.revokeObjectURL(url);
    }

    function snapUpload(event) {
      const file = event.target.files[0];
      if (!file) return;
      const reader = new FileReader();
      reader.onload = function(e) {
        try {
          const data = JSON.parse(e.target.result);
          if (!Array.isArray(data) || data.length === 0) {
            alert('Invalid snapshot file: expected a non-empty JSON array.');
            return;
          }
          if (snapReplaying) snapReplayStop();
          snapFrames = data;
          snapIdx = 0;
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
