// by Marius Versteegen, 2026
// Opslaan and Laden of recordings and snapshots on the SD card of the server (crt_SdStore.h),
// and the paste field "Naam plus patientID" of the Grid visualization page, served separately.

#pragma once

namespace crt
{
	const char GRID_SD_JS[] = R"rawliteral(
    // gridsd.js - Opslaan en Laden op de SD-kaart (/api/sd/..., crt_SdStore.h) en het plakveld
    // "Naam plus patientID". Een patient-ID wordt niet getypt maar samen met de naam uit Excel
    // geplakt; de naam is alleen een controle voor het oog en wordt nergens bewaard of verstuurd.
    // Gebruikt de dialogen van gridmeasinfo.js (miOpen, miSubOpen, MI_REC / MI_SNAP).

    // Patient-ID: kleine letters en cijfers, eventueel na de testprefix zzz-. 'kalibratie' en
    // 'andere' staan op die plaats in de namen van de andere bestanden.
    const PID_RE = /^(zzz-)?[a-z0-9]{1,40}$/;
    const PID_RESERVED = ['kalibratie', 'andere'];
    const PID_TEST_PREFIX = 'zzz-';
    // datum-tijd in een bestandsnaam: ..._YYYY-MM-DDTHH-MM-SS[_org][ (n)].json
    const SD_STAMP_RE = /_(\d{4}-\d\d-\d\dT\d\d-\d\d-\d\d)(?:_org)?(?:\s*\(\d+\))?\.json$/i;

    // '' als het ID bruikbaar is, anders de reden.
    function pidProblem(id) {
      if (/[A-Z]/.test(id)) return 'Het patient-ID mag geen hoofdletters bevatten, alleen kleine letters en cijfers.';
      if (!PID_RE.test(id)) return 'Het patient-ID mag alleen kleine letters en cijfers bevatten (eventueel na de testprefix zzz-).';
      if (PID_RESERVED.includes(id)) return '"' + id + '" kan geen patient-ID zijn.';
      return '';
    }

    // Geplakte tekst uit Excel: precies 2 naastgelegen cellen van een rij, "naam<TAB>ID" (Excel
    // zet er nog een regeleinde achter). Geeft {name, id, error}; error is '' als alles klopt.
    function pidParse(text) {
      const t = String(text).replace(/\r?\n$/, '');
      const cells = t.split('\t');
      if (/[\r\n]/.test(t) || cells.length !== 2) {
        return {name: '', id: '', error: 'Selecteer in Excel precies 2 naastgelegen cellen van een rij: de naam en het patient-ID.'};
      }
      const name = cells[0].trim(), id = cells[1].trim();
      if (!name || !id) return {name, id, error: 'Een van de twee cellen is leeg. Selecteer de naam en het patient-ID van de patient.'};
      return {name, id, error: pidProblem(id)};
    }

    // Het plakveld in dialoog box. Typen kan niet, alleen plakken. testCheck: bij een ID zonder
    // zzz- eerst de vraag of het om een echte patient gaat (Admin en Onderzoeker). onChange() na
    // elke wijziging. Geeft {id(): het bruikbare patient-ID of null, input}.
    function pidPasteField(box, testCheck, onChange) {
      const head = box.appendChild(miEl('div', 'pid-head'));
      head.appendChild(miEl('label', '', 'Naam plus patientID'));
      const help = head.appendChild(miEl('button', 'pid-help', '?'));
      help.type = 'button';
      help.title = 'Uitleg';
      help.onclick = pidShowHelp;
      const input = box.appendChild(miEl('input'));
      input.type = 'text';
      input.id = 'pidPaste';
      input.autocomplete = 'off';
      input.placeholder = 'Plak hier (Ctrl+V) de naam en het patient-ID uit Excel';
      const nameEl = box.appendChild(miEl('div', 'pid-out'));
      nameEl.id = 'pidName';
      const idEl = box.appendChild(miEl('div', 'pid-out'));
      idEl.id = 'pidId';
      const err = box.appendChild(miEl('span', 'mi-err'));
      err.id = 'pidErr';
      let okId = null;
      function show(p) {
        input.value = p ? p.name + '  ⇥  ' + p.id : '';
        nameEl.textContent = 'Naam: ' + (p ? p.name : '');
        idEl.textContent = 'PatientID: ' + (p ? p.id : '');
        err.textContent = p ? p.error : '';
        okId = (p && !p.error) ? p.id : null;
        onChange();
      }
      input.addEventListener('beforeinput', ev => ev.preventDefault());   // typen, knippen, wissen
      input.addEventListener('drop', ev => ev.preventDefault());
      input.addEventListener('paste', ev => {
        ev.preventDefault();
        const p = pidParse(ev.clipboardData ? ev.clipboardData.getData('text/plain') : '');
        show(p);
        if (okId !== null && testCheck && !okId.startsWith(PID_TEST_PREFIX)) pidAskTest(() => show(null));
      });
      nameEl.textContent = 'Naam:';
      idEl.textContent = 'PatientID:';
      return {id: () => okId, input};
    }

    // Admin en Onderzoeker: een ID zonder zzz- is een echte patient of een vergeten testprefix.
    // clear() maakt het plakveld leeg; ook Escape geldt als "niet bevestigd".
    function pidAskTest(clear) {
      const box = miSubOpen('Is dit een echte patient of een test?', clear);
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'het is een echte patient', miSubClose);
      miButton(row, 'het betreft een test', () => {
        clear();
        const b = miSubOpen('Geen testprefix');
        b.appendChild(miEl('p', '', "In dat geval had de patient-id moeten beginnen met de prefix 'zzz-'"));
        miButton(b.appendChild(miEl('div', 'mi-buttons')), 'OK', miSubClose);
      });
    }

    function pidShowHelp() {
      const box = miSubOpen('Naam plus patientID');
      box.appendChild(miEl('p', '', 'Plak in dit veld in een keer de selectie van 2 naastgelegen velden uit de Excel-tabel: ' +
        'het veld met de naam van de patient en het veld met diens patient-ID (naam links, patient-ID rechts).'));
      box.appendChild(miEl('p', 'pid-note', '(De patientnaam wordt niet opgeslagen; die is alleen nodig om de kans te ' +
        'verkleinen dat u het verkeerde patient-ID kopieert.)'));
      box.appendChild(miEl('p', 'pid-note', 'Belangrijk: zorg ervoor dat patient-ids niet hinten naar de naam van de ' +
        'patient, ivm privacy waarborging.'));
      box.appendChild(miEl('p', 'pid-note', 'Tip: laat Excel dubbele patient-ID\'s markeren (Voorwaardelijke opmaak > ' +
        'Markeringsregels voor cellen > Dubbele waarden). Twee patienten met hetzelfde ID kan deze pagina niet zien.'));
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'OK', miSubClose);
    }

    // ---- SD-kaart ----

    // fetch met een JSON-antwoord {ok, error, ...}; netwerk- en serverfouten worden {ok:false, error}.
    async function sdFetchJson(url, opts) {
      try {
        const r = await fetch(url, opts);
        const j = await r.json().catch(() => null);
        if (j && j.ok) return j;
        return {ok: false, error: (j && j.error) || ('Fout ' + r.status + ' van de server.')};
      } catch (e) {
        return {ok: false, error: 'Geen verbinding met de server.'};
      }
    }
    function sdMessage(title, text) {
      const box = miOpen(title);
      box.appendChild(miEl('p', '', text));
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'OK', miClose);
    }
    function sdStampText(stamp) {
      return stamp.slice(0, 10) + ' ' + stamp.slice(11).replace(/-/g, ':');
    }

    // Map en naam op de SD-kaart voor info. De datum-tijd komt uit de naam van het geladen bestand
    // als die er een heeft (dan krijgt dezelfde meting dezelfde naam), anders is het nu.
    function sdTarget(m, info) {
      const part = info.type === 'patient' ? info.patientId : info.type === 'calibration' ? 'kalibratie' : 'andere';
      const folder = info.type === 'patient' ? 'patientmetingen/' + info.patientId
                   : info.type === 'calibration' ? 'kalibraties' : 'snapshots_andere';
      const st = m.source ? m.source.match(SD_STAMP_RE) : null;
      return {folder, name: m.word + '_' + part + '_' + (st ? st[1] : recLocalStamp()) + '_org.json'};
    }

    // Opslaan-knop: wat al op de SD-kaart staat, kan niet nog eens (alles daar is een origineel).
    function miSaveClick(m) {
      if (m.sdName) {
        sdMessage(m.saveTitle, 'Deze ' + m.word + ' staat al op de SD-kaart (' + m.sdName + '). Opnieuw opslaan wordt niet ondersteund.');
        return;
      }
      m.ask('sd');
    }

    async function sdSave(m, info) {
      const t = sdTarget(m, info);
      const box = miOpen(m.saveTitle);
      box.appendChild(miEl('p', '', 'Bezig met opslaan op de SD-kaart...'));
      const fd = new FormData();
      fd.append('file', new Blob([JSON.stringify(m.data(info))], {type: 'application/json'}), t.name);
      const r = await sdFetchJson('/api/sd/save?path=' + encodeURIComponent(t.folder + '/' + t.name), {method: 'POST', body: fd});
      if (!r.ok) { sdMessage('Opslaan mislukt', r.error); return; }
      miClose();
      m.info = info;
      m.sdName = t.name;
      miUpdateInfoButtons();
      flashPopup(m.saveAnchor, 'opgeslagen op de SD-kaart', 1500);
    }

    // Laden-knop. Een Gebruiker laadt alleen patient-metingen: naam+ID plakken, dan de lijst.
    // Admin en Onderzoeker kiezen eerst de soort.
    function miLoadClick(m) {
      miConfirmReplace(m, 'laden', () => sdLoadDialog(m));
    }
    function sdLoadTitle(m) { return m === MI_REC ? 'Recording laden' : 'Snapshots laden'; }
    function sdLoadDialog(m) {
      if (userRole === 0) { sdLoadPatient(m); return; }
      const box = miOpen(sdLoadTitle(m));
      box.appendChild(miEl('p', '', 'Wat voor ' + m.word + ' wilt u laden?'));
      const col = box.appendChild(miEl('div', 'mi-choices'));
      miButton(col, 'Een patient meting', () => sdLoadPatient(m));
      if (m === MI_REC) miButton(col, 'Een kalibratiemeting', () => sdLoadFolder(m, 'kalibraties'));
      else miButton(col, 'Een andere meting', () => sdLoadFolder(m, 'snapshots_andere'));
      miButton(col, 'Afbreken', miClose);
    }
    function sdLoadPatient(m) {
      const box = miOpen(sdLoadTitle(m));
      let list = null;
      const field = pidPasteField(box, false, () => {
        if (field && field.id() !== null) sdFillList(m, list, 'patientmetingen/' + field.id());
        else if (list) list.textContent = '';
      });
      list = box.appendChild(miEl('div', 'sd-list'));
      list.id = 'sdList';
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'Afbreken', miClose);
      field.input.focus();
    }
    function sdLoadFolder(m, folder) {
      const box = miOpen(sdLoadTitle(m));
      const list = box.appendChild(miEl('div', 'sd-list'));
      list.id = 'sdList';
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'Afbreken', miClose);
      sdFillList(m, list, folder);
    }

    // De bestanden van deze soort in folder, nieuwste bovenaan; klikken laadt het bestand.
    async function sdFillList(m, list, folder) {
      const req = (list.sdReq || 0) + 1;
      list.sdReq = req;
      list.textContent = 'Bezig met zoeken...';
      const r = await sdFetchJson('/api/sd/list?dir=' + encodeURIComponent(folder));
      if (list.sdReq !== req || !document.body.contains(list)) return;   // intussen opnieuw geplakt of gesloten
      list.textContent = '';
      if (!r.ok) { list.appendChild(miEl('span', 'mi-err', r.error)); return; }
      const files = r.files.filter(f => f.name.startsWith(m.word + '_')).sort((a, b) => a.name < b.name ? 1 : -1);
      if (files.length === 0) {
        list.appendChild(miEl('span', '', 'Geen ' + (m === MI_REC ? 'recordings' : 'snapshots') + ' gevonden op de SD-kaart.'));
        return;
      }
      files.forEach(f => {
        const st = f.name.match(SD_STAMP_RE);
        const kb = Math.max(1, Math.round(f.size / 1024));
        const b = miButton(list, (st ? sdStampText(st[1]) : f.name) + '  (' + kb + ' kB)', () => sdOpen(m, folder, f.name));
        b.className = 'sd-file';
        b.title = f.name;
      });
    }

    async function sdOpen(m, folder, name) {
      const box = miOpen(sdLoadTitle(m));
      box.appendChild(miEl('p', '', 'Bezig met laden van ' + name + '...'));
      let text;
      try {
        const r = await fetch('/api/sd/file?path=' + encodeURIComponent(folder + '/' + name));
        if (!r.ok) {
          const j = await r.json().catch(() => null);
          sdMessage('Laden mislukt', (j && j.error) || ('Fout ' + r.status + ' van de server.'));
          return;
        }
        text = await r.text();
      } catch (e) {
        sdMessage('Laden mislukt', 'Geen verbinding met de server.');
        return;
      }
      miClose();
      if (m === MI_REC) recLoadText(text, name, true);
      else snapLoadText(text, name, true);
    }
)rawliteral";

} // end namespace crt
