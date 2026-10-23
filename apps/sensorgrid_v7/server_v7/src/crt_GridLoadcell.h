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

    // "?" in het loadcell-venster: uitleg van Tare en Calibrate (dialoog van gridmeasinfo.js).
    function lcHelp() {
      const box = miOpen('Loadcell kalibreren');
      const steps = box.appendChild(miEl('ol'));
      [
        'Gebruik live data: niet tijdens het afspelen van een recording of snapshot.',
        'Haal alle belasting van de loadcell af en wacht tot het getoonde gewicht stil staat.',
        'Klik Tare. Het nulpunt wordt de mediaan van de laatste seconde; het gewicht wordt nu ongeveer 0 g.',
        'Plaats een bekend gewicht op de loadcell, bijvoorbeeld 500 g, en vul dat in grammen in bij "Known weight (g)".',
        'Wacht een paar seconden tot de waarde stabiel is en klik Calibrate. Het getoonde gewicht wordt nu ongeveer het bekende gewicht.',
        'Controle: haal het gewicht eraf; de loadcell hoort weer ongeveer 0 g te tonen. Wijkt dat af, herhaal dan vanaf stap 2.'
      ].forEach(t => steps.appendChild(miEl('li', '', t)));
      box.appendChild(miEl('p', '', 'Is alleen het nulpunt verschoven (bijvoorbeeld na een tijdje), dan is opnieuw Tare met een onbelaste loadcell genoeg; de schaal van Calibrate blijft gelden.'));
      box.appendChild(miEl('p', '', 'Kalibreer de loadcell voordat je een kalibratiemeting opneemt voor "Herkalibreer druksensoren": die gebruikt de loadcell-grammen als referentie.'));
      box.appendChild(miEl('p', '', 'Tarra en schaal worden in deze browser bewaard en gaan mee in de meta van recording-, snapshot- en meta-downloads.'));
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'Sluiten', miClose);
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
