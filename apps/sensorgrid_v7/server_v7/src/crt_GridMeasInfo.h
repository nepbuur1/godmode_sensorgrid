// by Marius Versteegen, 2025
// Download and Opslaan dialogs and Info buttons for recordings and snapshots on the Grid
// visualization page: what kind of measurement it is, patient-id and a note.

#pragma once

namespace crt
{
	const char GRID_MEASINFO_JS[] = R"rawliteral(
    // Per reeks (recording / snapshots) de bij de download ingevulde gegevens:
    // info = {type: 'patient' | 'calibration' | 'other', patientId, note}, of null.
    // Gaat als meta.measurement mee in de download en komt terug bij een upload.
    // downloaded: deze inhoud is al eens gedownload (in deze browsersessie, of het is een upload).
    // source: bestandsnaam van de upload waar de huidige inhoud vandaan komt (null = vers opgenomen);
    // een download daarvan krijgt weer die naam (miUploadName); bestaat hij al, dan nummert de browser.
    // orgDone: de eerste download van verse inhoud (na Record / Clear Snapshots) is gedaan; alleen
    // die eerste krijgt _org (origineel) achter de naam.
    // sdName: naam waaronder deze inhoud op de SD-kaart staat (opgeslagen of daarvandaan geladen),
    // anders null. Opnieuw opslaan onder dezelfde naam kan niet (gridsd.js).
    // target van de dialogen: 'pc' (Download) of 'sd' (Opslaan op de SD-kaart, gridsd.js).
    const MI_REC = {
      word: 'recording', title: 'Recording downloaden', saveTitle: 'Recording opslaan', infoTitle: 'Recording info',
      anchor: 'btnDownload', saveAnchor: 'btnRecSave',
      info: null, downloaded: false, source: null, orgDone: false, sdName: null,
      fileName: info => 'recording_' + miNamePart(info) + recLocalStamp() + '.json',
      data: info => metaAppendForExport(recFrames, recExportTag(), info, 'recording'),
      ask: target => miAskRecKind(target)
    };
    const MI_SNAP = {
      word: 'snapshots', title: 'Snapshots downloaden', saveTitle: 'Snapshots opslaan', infoTitle: 'Snapshots info',
      anchor: 'btnSnapDownload', saveAnchor: 'btnSnapSave',
      info: null, downloaded: false, source: null, orgDone: false, sdName: null,
      fileName: info => 'snapshots_' + miNamePart(info) + recLocalStamp() + '.json',
      data: info => metaAppendForExport(snapFrames, null, info, 'snapshots'),
      ask: target => miAskSnapKind(target)
    };
    function miNamePart(info) {
      if (info.type === 'patient') return info.patientId + '_';
      if (info.type === 'calibration') return 'kalibratie_';
      return 'andere_';
    }
    // Een Gebruiker heeft geen Download: na Stop volgt bij hem de Opslaan-dialoog.
    function miStopTarget() { return userRole === 0 ? 'sd' : 'pc'; }

    // Naam van de upload, zonder een " (1)" die de browser eerder aan een dubbele naam toevoegde en
    // zonder _org (dat hoort alleen bij het origineel); de browser nummert zelf opnieuw als de naam
    // al in de downloadmap staat.
    function miUploadName(name) {
      return name.replace(/\.json$/i, '').replace(/\s*\(\d+\)$/, '').replace(/_org$/, '') + '.json';
    }

    function miReset(m) { m.info = null; m.downloaded = false; m.source = null; m.orgDone = false; m.sdName = null; miUpdateInfoButtons(); }
    function miContentChanged(m) { m.downloaded = false; m.sdName = null; }
    // Geladen van de pc (Upload) of van de SD-kaart (Laden, fromSd): een upload telt als al eens
    // gedownload, een bestand van de SD-kaart als al opgeslagen.
    function miLoaded(m, meta, fileName, fromSd) {
      m.info = miFromMeta(meta);
      m.downloaded = !fromSd;
      m.sdName = fromSd ? fileName : null;
      m.source = fileName || 'upload.json';
      m.orgDone = true;
      miUpdateInfoButtons();
    }
    function miFromMeta(meta) {
      const x = meta && meta.measurement;
      if (!x || typeof x !== 'object') return null;
      return {
        type: ['patient', 'calibration', 'other'].includes(x.type) ? x.type : 'other',
        patientId: typeof x.patientId === 'string' ? x.patientId : '',
        note: typeof x.note === 'string' ? x.note : ''
      };
    }
    function miUpdateInfoButtons() {
      document.getElementById('recRow2').style.display = MI_REC.info ? 'flex' : 'none';
      document.getElementById('btnSnapInfo').style.display = MI_SNAP.info ? '' : 'none';
    }

    function miSave(m, info) {
      m.info = info;
      const blob = new Blob([JSON.stringify(m.data(info))], {type: 'application/json'});
      const url = URL.createObjectURL(blob);
      const a = document.createElement('a');
      a.href = url;
      a.download = m.source ? miUploadName(m.source) : m.fileName(info);
      if (!m.orgDone) a.download = a.download.replace(/\.json$/, '_org.json');
      m.orgDone = true;
      a.click();
      URL.revokeObjectURL(url);
      m.downloaded = true;
      miUpdateInfoButtons();
      flashPopup(m.anchor, 'download completed', 1000);
    }

    // ---- Modale dialoog: een overlay met een kader; Escape = afbreken ----
    function miEl(tag, cls, text) {
      const el = document.createElement(tag);
      if (cls) el.className = cls;
      if (text !== undefined) el.textContent = text;
      return el;
    }
    function miOpen(title) {
      miClose();
      const ov = miEl('div', 'mi-overlay');
      ov.id = 'miOverlay';
      const box = miEl('div', 'mi-box');
      box.appendChild(miEl('h3', '', title));
      ov.appendChild(box);
      document.body.appendChild(ov);
      return box;
    }
    function miClose() {
      miSubClose();
      const ov = document.getElementById('miOverlay');
      if (ov) ov.remove();
    }
    // Tweede laag boven de dialoog (vraag, uitleg), zodat de dialoog eronder blijft staan.
    // onEscape: wat Escape behalve sluiten nog doet.
    function miSubOpen(title, onEscape) {
      miSubClose();
      const ov = miEl('div', 'mi-overlay');
      ov.id = 'miSubOverlay';
      ov.onEscape = onEscape || null;
      const box = miEl('div', 'mi-box');
      box.appendChild(miEl('h3', '', title));
      ov.appendChild(box);
      document.body.appendChild(ov);
      return box;
    }
    function miSubClose() {
      const ov = document.getElementById('miSubOverlay');
      if (ov) ov.remove();
    }
    document.addEventListener('keydown', ev => {
      if (ev.key !== 'Escape') return;
      const sub = document.getElementById('miSubOverlay');
      if (!sub) { miClose(); return; }
      sub.remove();
      if (sub.onEscape) sub.onEscape();
    });
    function miButton(parent, label, fn) {
      const b = miEl('button', '', label);
      b.onclick = fn;
      parent.appendChild(b);
      return b;
    }

    // Na Stop van een opname: de dialoog voor Download (bij een Gebruiker: Opslaan), als de
    // recording daar nog niet heen is.
    function miAfterStop() {
      if (recFrames.length === 0) return;
      const target = miStopTarget();
      if (target === 'sd' ? !MI_REC.sdName : !MI_REC.downloaded) miAskRecKind(target);
    }
    function miAskRecKind(target) {
      const box = miOpen(target === 'sd' ? MI_REC.saveTitle : MI_REC.title);
      box.appendChild(miEl('p', '', 'Wat voor recording is dit?'));
      const col = box.appendChild(miEl('div', 'mi-choices'));
      miButton(col, 'Het is een patient meting', () => miDownloadDialog(MI_REC, 'patient', target));
      const row = col.appendChild(miEl('div', 'mi-choice-row'));
      miButton(row, 'Het is een kalibratiemeting', () => miDownloadDialog(MI_REC, 'calibration', target));
      row.appendChild(miEl('span', '', '(met de 3D printer)'));
      miButton(col, 'Afbreken', miClose);
    }
    function miAskSnapKind(target) {
      const box = miOpen(target === 'sd' ? MI_SNAP.saveTitle : MI_SNAP.title);
      box.appendChild(miEl('p', '', 'Is dit een patient-meting?'));
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja', () => miDownloadDialog(MI_SNAP, 'patient', target));
      miButton(row, 'Nee', () => miDownloadDialog(MI_SNAP, 'other', target));
      miButton(row, 'Afbreken', miClose);
    }

    // Dialoog "<Recording|Snapshots> downloaden" (target 'pc') of "... opslaan" (target 'sd') voor
    // soort type ('patient' | 'calibration' | 'other'): bij 'patient' het plakveld "Naam plus
    // patientID" (gridsd.js), en een optionele aantekening, voorgevuld met de eerdere gegevens van
    // deze reeks. Had de reeks al een patient-ID (eerdere download, of uit het geladen bestand),
    // dan wordt dat overgenomen; voor de SD-kaart alleen als het aan het formaat voldoet.
    function miDownloadDialog(m, type, target) {
      const sd = (target === 'sd');
      const withPatient = (type === 'patient');
      const prev = m.info || {patientId: '', note: ''};
      const hadId = withPatient && m.info !== null && m.info.type === 'patient' && prev.patientId !== '';
      const keepId = hadId && (!sd || pidProblem(prev.patientId) === '');
      const box = miOpen(sd ? m.saveTitle : m.title);
      if (type === 'calibration') box.appendChild(miEl('p', '', 'Kalibratiemeting (met de 3D printer)'));
      let field = null;
      if (keepId) {
        box.appendChild(miEl('div', 'pid-out', 'PatientID: ' + prev.patientId + ' (overgenomen)')).id = 'pidKept';
      } else if (withPatient) {
        if (hadId) {
          box.appendChild(miEl('p', '', 'Het patient-ID van deze ' + m.word + ' (' + prev.patientId +
            ') past niet in de naamgeving van de SD-kaart. Plak de naam en het patient-ID uit Excel.'));
        }
        field = pidPasteField(box, userRole !== 0, () => update());
      }
      box.appendChild(miEl('label', '', 'Aantekening (optioneel)'));
      const note = box.appendChild(miEl('textarea'));
      note.id = 'miNote';
      note.rows = 5;
      note.value = prev.note;
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      const dl = miButton(row, sd ? 'Opslaan' : 'Downloaden', () => {
        if (dl.disabled) return;
        const pid = keepId ? prev.patientId : (field ? field.id() : '');
        const info = {type, patientId: withPatient ? pid : '', note: note.value};
        miClose();
        if (sd) sdSave(m, info); else miSave(m, info);
      });
      dl.id = sd ? 'miSave' : 'miDownload';
      miButton(row, 'Afbreken', miClose);
      function update() {
        dl.disabled = (field !== null && field.id() === null);
        dl.title = dl.disabled ? 'Plak eerst de naam en het patient-ID uit Excel.' : '';
      }
      update();
      if (field) field.input.focus(); else note.focus();
    }

    // Download-knop: eerst waarschuwen als deze inhoud al eens gedownload is (ook een upload); dan
    // de downloaddialoog van dezelfde soort als de vorige keer.
    // Een upload zonder deze gegevens (ouder bestand) krijgt de vraag naar de soort meting.
    function miDownloadClick(m) {
      if (!m.downloaded) { m.ask('pc'); return; }
      const box = miOpen('U heeft deze ' + m.word + ' al eens (ooit) gedownload, weet u het zeker?');
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja, downloaden', () => {
        if (!m.info) m.ask('pc');
        else miDownloadDialog(m, m.info.type, 'pc');
      });
      miButton(row, 'Nee, afbreken', miClose);
    }

    // Upload en Laden: een recording verwijdert bij het laden de huidige snapshots en omgekeerd.
    // Is die andere er, dan eerst vragen (then = verder met laden). Pas bij een geslaagd laden wordt
    // hij verwijderd (recLoadText / snapLoadText), dus Afbreken laat hem staan.
    function miConfirmReplace(m, verb, then) {
      const isRec = m === MI_REC;
      const other = isRec ? MI_SNAP : MI_REC;
      if ((isRec ? snapFrames : recFrames).length === 0) { then(); return; }
      const box = miOpen('Weet u het zeker?');
      box.appendChild(miEl('p', '', 'Een ' + (isRec ? 'recording' : 'snapshots') + '-' + verb + ' verwijdert ' +
        (isRec ? 'de huidige serie snapshots.' : 'de huidige recording.') +
        (other.downloaded || other.sdName ? '' : userRole === 0 ? ' Die is nog niet opgeslagen.' : ' Die is nog niet gedownload.')));
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja', () => { miClose(); then(); });
      miButton(row, 'Afbreken', miClose);
    }
    function miUploadClick(m) {
      miConfirmReplace(m, 'upload', () => document.getElementById(m === MI_REC ? 'recFileInput' : 'snapFileInput').click());
    }

    // Soort van een geüpload bestand: {kind: 'recording' | 'snapshots' | 'meta' | 'other' | null, sure}.
    // Sinds 2026-10-24 staat de soort in meta.fileType. Daarvoor had een recording (sinds 2026-09-17)
    // een recording-blok in de meta en een snapshot-serie niet; meta zonder dat blok is dus
    // waarschijnlijk een snapshot-serie, maar kan ook een oudere recording zijn (sure = false).
    // Zonder meta (bestanden van voor juli 2026) is de soort onbekend (null).
    function miFileKind(parsed) {
      if (parsed && typeof parsed === 'object' && !Array.isArray(parsed) && parsed.global && parsed.sensors) return {kind: 'meta', sure: true};
      if (!Array.isArray(parsed)) return {kind: 'other', sure: true};
      const split = metaSplitFromImport(parsed);
      if (split.frames.length === 0 || !split.frames.every(f => f && Array.isArray(f.sensors))) return {kind: 'other', sure: true};
      const meta = split.meta;
      if (!meta) return {kind: null, sure: false};
      if (meta.fileType === 'recording' || meta.fileType === 'snapshots') return {kind: meta.fileType, sure: true};
      if (meta.recording) return {kind: 'recording', sure: true};
      return {kind: 'snapshots', sure: false};
    }
    // Na de bestandskeuze van een Upload: load() alleen als het bestand van de gevraagde soort is (of
    // de soort onbekend). Anders een foutmelding, of bij twijfel eerst de vraag of het toch moet.
    function miCheckUpload(m, name, parsed, load) {
      const want = m === MI_REC ? 'recording' : 'snapshots';
      const k = miFileKind(parsed);
      if (k.kind === want || k.kind === null) { load(); return; }
      if (!k.sure) {
        const box = miOpen('Is dit een recording?');
        box.appendChild(miEl('p', '', '"' + name + '" lijkt een snapshot-serie: de meta bevat geen recording-gegevens. ' +
          'Het kan ook een recording van voor 17-09-2026 zijn. Toch als recording laden?'));
        const row = box.appendChild(miEl('div', 'mi-buttons'));
        miButton(row, 'Ja', () => { miClose(); load(); });
        miButton(row, 'Afbreken', miClose);
        return;
      }
      const what = {
        recording: 'een recording. Gebruik daarvoor Upload in het record-paneel.',
        snapshots: 'een snapshot-serie. Gebruik daarvoor Upload in het snapshot-paneel.',
        meta: 'een meta-bestand. Gebruik daarvoor Upload Meta.',
        other: 'geen recording, snapshot-serie of meta-bestand van deze pagina.'
      }[k.kind];
      const box = miOpen(want === 'recording' ? 'Dit is geen recording' : 'Dit is geen snapshot-serie');
      box.appendChild(miEl('p', '', '"' + name + '" is ' + what + ' Er is niets geladen of verwijderd.'));
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'OK', miClose);
    }

    function miShowInfo(m) {
      const info = m.info;
      if (!info) return;
      const box = miOpen(m.infoTitle);
      if (m === MI_REC) {
        box.appendChild(miEl('div', '', 'Soort: ' + (info.type === 'calibration'
          ? 'kalibratiemeting (met de 3D printer)' : info.type === 'patient' ? 'patient meting' : 'overig')));
      }
      box.appendChild(miEl('div', '', info.type === 'patient' ? 'Patient-id: ' + info.patientId
                                                               : 'Geen patient-meting'));
      box.appendChild(miEl('label', '', 'Aantekening'));
      box.appendChild(miEl('div', 'mi-text', info.note.trim() ? info.note : '(geen)'));
      miButton(box.appendChild(miEl('div', 'mi-buttons')), 'Sluiten', miClose);
    }
)rawliteral";

} // end namespace crt
