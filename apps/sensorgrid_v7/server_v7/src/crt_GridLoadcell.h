// by Marius Versteegen, 2025
// Loadcell support functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_LOADCELL_JS[] = R"rawliteral(
    // Laatste LC_HIST_N ruwe loadcell-monsters (1 s bij 100 ms poll). Tare en Calibrate gebruiken
    // de mediaan daarvan i.p.v. een los monster, zodat een toevallige verstoring bij het klikken
    // niet in tarra of schaal terechtkomt.
    const LC_HIST_N = 10;
    function lcMedianRaw(s) {
      const h = s.lcRawHist;
      if (!h || h.length === 0) return s.lcLastRaw || 0;
      const a = h.slice().sort((x, y) => x - y);
      const m = a.length >> 1;
      return (a.length % 2 === 1) ? a[m] : (a[m - 1] + a[m]) / 2;
    }
    // Ongefilterde gram uit een ruwe waarde met de tarra/schaal van de module.
    function lcGramFromRaw(s, raw) {
      return s.lcScale !== 0 ? (raw - s.lcTareOffset) / s.lcScale : 0;
    }

    function lcTare(id) {
      const s = sensors[id];
      s.lcTareOffset = lcMedianRaw(s);
      s.filteredWeight = null;
      setCookie("lcTare" + id, s.lcTareOffset, 365);
    }

    function lcCalibrate(id) {
      const s = sensors[id];
      const knownGrams = parseFloat(s.lcKnownEl.value);
      if (!knownGrams || knownGrams <= 0) return;
      const rawMinusTare = lcMedianRaw(s) - s.lcTareOffset;
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
        if (!s.lcRawHist) s.lcRawHist = [];
        s.lcRawHist.push(rawValue);
        if (s.lcRawHist.length > LC_HIST_N) s.lcRawHist.shift();
        if (typeof recordedGram === "number") {
          // Playback / snapshot replay: show the given value directly. For a recording this
          // is already the Value-Filtered series (recApplyFrame in gridrecplay.js), so the
          // window matches the green curve; a snapshot holds the value as it was shown.
          s.filteredWeight = recordedGram;
        } else {
          const weightGrams = lcGramFromRaw(s, rawValue);
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
