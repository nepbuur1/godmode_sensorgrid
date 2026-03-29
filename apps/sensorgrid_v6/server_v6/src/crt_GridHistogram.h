// by Marius Versteegen, 2025
// Histogram and statistics functions for the Grid visualization page.

#pragma once

namespace crt
{
	const char GRID_HISTOGRAM_JS[] = R"rawliteral(
    function createHistogram(s) {
      s.histEl.innerHTML = "";
      s.histBars = [];
      for (let i = 0; i < NUM_BINS; i++) {
        const bar = document.createElement("div");
        bar.className = "hist-bar";
        bar.style.height = "1px";
        s.histEl.appendChild(bar);
        s.histBars.push(bar);
      }
    }

    function updateHistogram(s, values) {
      const binWidth = Math.ceil((MAX_VALUE + 1) / NUM_BINS);
      const counts = new Array(NUM_BINS).fill(0);
      for (const v of values) {
        const bin = Math.min(Math.floor(v / binWidth), NUM_BINS - 1);
        counts[bin]++;
      }
      const maxCount = Math.max(1, ...counts);
      for (let i = 0; i < NUM_BINS; i++) {
        const pct = (counts[i] / maxCount) * 100;
        s.histBars[i].style.height = Math.max(1, pct) + "%";
      }
    }

    function getStatsFilter() {
      const v = parseFloat(document.getElementById("fldStatsFilter").value);
      if (isNaN(v) || v < 0) return 0;
      if (v > 1) return 1;
      return v;
    }

    function getValueFilter() {
      const v = parseFloat(document.getElementById("fldValueFilter").value);
      if (isNaN(v) || v < 0) return 0;
      if (v > 1) return 1;
      return v;
    }

    function updateStats(s, values) {
      if (values.length === 0) return;
      let max = values[0], sum = 0;
      for (const v of values) {
        if (v > max) max = v;
        sum += v;
      }
      if (s.filteredSum === null) {
        s.filteredSum = sum;
        s.filteredMax = max;
      } else {
        const f = getStatsFilter();
        s.filteredSum = s.filteredSum * f + sum * (1 - f);
        s.filteredMax = s.filteredMax * f + max * (1 - f);
      }
      s.sumEl.textContent = s.filteredSum.toFixed(1);
      s.maxEl.textContent = s.filteredMax.toFixed(1);
    }
)rawliteral";

} // end namespace crt
