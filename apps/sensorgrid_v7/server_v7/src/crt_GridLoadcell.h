// by Marius Versteegen, 2025
// Loadcell support functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_LOADCELL_JS[] = R"rawliteral(
    function lcTare(id) {
      const s = sensors[id];
      s.lcTareOffset = s.lcLastRaw || 0;
      s.filteredWeight = null;
      setCookie("lcTare" + id, s.lcTareOffset, 365);
    }

    function lcCalibrate(id) {
      const s = sensors[id];
      const knownGrams = parseFloat(s.lcKnownEl.value);
      if (!knownGrams || knownGrams <= 0) return;
      const rawMinusTare = (s.lcLastRaw || 0) - s.lcTareOffset;
      if (rawMinusTare === 0) return;
      s.lcScale = rawMinusTare / knownGrams;
      s.filteredWeight = null;
      setCookie("lcScale" + id, s.lcScale, 365);
      setCookie("lcKnown" + id, knownGrams, 365);
    }

    function updateLoadcell(id, hasLoadcell, rawValue, recordedGram) {
      const s = sensors[id];
      if (!s.enabled) return;
      if (hasLoadcell) {
        if (!s.lcVisible) {
          s.lcEl.style.display = "block";
          s.lcVisible = true;
        }
        s.lcLastRaw = rawValue;
        if (typeof recordedGram === "number") {
          // Playback / snapshot replay: show the value exactly as it was recorded
          // (already filtered live at record time). Showing it directly avoids a
          // second EMA pass - which lagged the loadcell window ~2-3 samples behind
          // the green curve - and is robust to tare/scale changes since recording.
          s.filteredWeight = recordedGram;
        } else {
          const weightGrams = s.lcScale !== 0
            ? (rawValue - s.lcTareOffset) / s.lcScale
            : 0;
          if (s.filteredWeight === null) {
            s.filteredWeight = weightGrams;
          } else {
            const f = getStatsFilter() * 0.8;
            s.filteredWeight = s.filteredWeight * f + weightGrams * (1 - f);
          }
        }
        s.lcWeightEl.textContent = s.filteredWeight.toFixed(1) + " g";
      } else {
        if (s.lcVisible) {
          s.lcEl.style.display = "none";
          s.lcVisible = false;
        }
      }
    }
)rawliteral";

} // end namespace crt
