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

    function recRecord() {
      recFrames = [];
      playIdx = 0;
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
      recUpdateInfo();
      updateRecChart();
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
        document.getElementById('btnPlayPause').classList.remove('pause-active');
      }
      recUpdateInfo();
      updateRecChart();
    }

    function recDownload() {
      if (recFrames.length === 0) return;
      const json = JSON.stringify(metaAppendForExport(recFrames));
      const blob = new Blob([json], {type: 'application/json'});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      const ts = new Date().toISOString().replace(/[:.]/g, '-').slice(0, 19);
      a.download = 'recording_' + ts + '.json';
      a.click();
      URL.revokeObjectURL(url);
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
          if (split.meta) applyMetaState(split.meta);
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
      else if (recState === 'stopped') el.textContent = 'Stopped: ' + recFrames.length + ' frames';
      else if (recState === 'playing') el.textContent = 'Playing: ' + playIdx + '/' + recFrames.length;
      else if (recState === 'playpaused') el.textContent = 'Paused: ' + playIdx + '/' + recFrames.length;
      else el.textContent = 'Ready';
      var sl = document.getElementById('recSlider');
      var show = (recState === 'playing' || recState === 'playpaused');
      document.getElementById('recSliderRow').style.display = show ? 'block' : 'none';
      if (show) { sl.max = Math.max(0, recFrames.length - 1); sl.value = playIdx; }
    }
)rawliteral";

} // end namespace crt
