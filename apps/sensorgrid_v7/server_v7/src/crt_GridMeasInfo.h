// by Marius Versteegen, 2025
// Download dialogs and Info buttons for recordings and snapshots on the Grid visualization page:
// what kind of measurement it is, patient-id and a note.

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
    const PATIENT_ID_RE = /^[A-Za-z0-9_-]+$/;
    const MI_REC = {
      word: 'recording', title: 'Recording downloaden', infoTitle: 'Recording info',
      anchor: 'btnDownload', info: null, downloaded: false, source: null, orgDone: false,
      fileName: info => 'recording_' + miNamePart(info) + recLocalStamp() + '.json',
      data: info => metaAppendForExport(recFrames, recExportTag(), info, 'recording'),
      ask: () => miAskRecKind()
    };
    const MI_SNAP = {
      word: 'snapshots', title: 'Snapshots downloaden', infoTitle: 'Snapshots info',
      anchor: 'btnSnapDownload', info: null, downloaded: false, source: null, orgDone: false,
      fileName: info => 'snapshots_' + miNamePart(info) + recLocalStamp() + '.json',
      data: info => metaAppendForExport(snapFrames, null, info, 'snapshots'),
      ask: () => miAskSnapKind()
    };
    function miNamePart(info) {
      if (info.type === 'patient') return info.patientId + '_';
      if (info.type === 'calibration') return 'kalibratie_';
      return '';
    }

    // Naam van de upload, zonder een " (1)" die de browser eerder aan een dubbele naam toevoegde en
    // zonder _org (dat hoort alleen bij het origineel); de browser nummert zelf opnieuw als de naam
    // al in de downloadmap staat.
    function miUploadName(name) {
      return name.replace(/\.json$/i, '').replace(/\s*\(\d+\)$/, '').replace(/_org$/, '') + '.json';
    }

    function miReset(m) { m.info = null; m.downloaded = false; m.source = null; m.orgDone = false; miUpdateInfoButtons(); }
    function miContentChanged(m) { m.downloaded = false; }
    function miLoaded(m, meta, fileName) {
      m.info = miFromMeta(meta);
      m.downloaded = true;
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
      const ov = document.getElementById('miOverlay');
      if (ov) ov.remove();
    }
    document.addEventListener('keydown', ev => { if (ev.key === 'Escape') miClose(); });
    function miButton(parent, label, fn) {
      const b = miEl('button', '', label);
      b.onclick = fn;
      parent.appendChild(b);
      return b;
    }

    // Na Stop van een opname, en bij Download van een recording die nog niet gedownload is.
    function miAfterStop() {
      if (recFrames.length > 0 && !MI_REC.downloaded) miAskRecKind();
    }
    function miAskRecKind() {
      const box = miOpen('Wat voor recording is dit?');
      const col = box.appendChild(miEl('div', 'mi-choices'));
      miButton(col, 'Het is een patient meting', () => miDownloadDialog(MI_REC, true));
      const row = col.appendChild(miEl('div', 'mi-choice-row'));
      miButton(row, 'Het is een kalibratiemeting', () => {
        miClose();
        miSave(MI_REC, {type: 'calibration', patientId: '', note: ''});
      });
      row.appendChild(miEl('span', '', '(met de 3D printer)'));
      miButton(col, 'Afbreken', miClose);
    }
    function miAskSnapKind() {
      const box = miOpen('Is dit een patient-meting?');
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja', () => miDownloadDialog(MI_SNAP, true));
      miButton(row, 'Nee', () => miDownloadDialog(MI_SNAP, false));
      miButton(row, 'Afbreken', miClose);
    }

    // Dialoog "<Recording|Snapshots> downloaden": patient-id (verplicht als withPatient) en een
    // optionele aantekening, voorgevuld met de eerder ingevulde gegevens van deze reeks.
    function miDownloadDialog(m, withPatient) {
      const prev = m.info || {patientId: '', note: ''};
      const box = miOpen(m.title);
      let pid = null, hint = null;
      if (withPatient) {
        box.appendChild(miEl('label', '', 'Vul hier de patient-id in'));
        pid = box.appendChild(miEl('input'));
        pid.type = 'text';
        pid.id = 'miPatientId';
        pid.placeholder = 'letters, cijfers, - en _';
        pid.value = prev.patientId;
        hint = box.appendChild(miEl('span', 'mi-err'));
      }
      box.appendChild(miEl('label', '', 'Aantekening (optioneel)'));
      const note = box.appendChild(miEl('textarea'));
      note.id = 'miNote';
      note.rows = 5;
      note.value = prev.note;
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      const dl = miButton(row, 'Downloaden', () => {
        if (dl.disabled) return;
        const info = withPatient
          ? {type: 'patient', patientId: pid.value.trim(), note: note.value}
          : {type: 'other', patientId: '', note: note.value};
        miClose();
        miSave(m, info);
      });
      dl.id = 'miDownload';
      miButton(row, 'Afbreken', miClose);
      if (withPatient) {
        const check = () => {
          const v = pid.value.trim();
          const ok = PATIENT_ID_RE.test(v);
          const why = v === '' ? 'Vul eerst een patient-id in.'
                               : 'Het patient-id mag alleen letters, cijfers, verbindingsstreepjes (-) en underscores (_) bevatten.';
          dl.disabled = !ok;
          dl.title = ok ? '' : why;
          hint.textContent = (ok || v === '') ? '' : why;
        };
        pid.addEventListener('input', check);
        pid.addEventListener('keydown', ev => { if (ev.key === 'Enter') dl.click(); });
        check();
        pid.focus();
      } else {
        note.focus();
      }
    }

    // Download-knop: eerst waarschuwen als deze inhoud al eens gedownload is (ook een upload); dan
    // dezelfde afhandeling als de vorige keer (een kalibratiemeting meteen, anders de downloaddialoog).
    // Een upload zonder deze gegevens (ouder bestand) krijgt de vraag naar de soort meting.
    function miDownloadClick(m) {
      if (!m.downloaded) { m.ask(); return; }
      const box = miOpen('U heeft deze ' + m.word + ' al eens (ooit) gedownload, weet u het zeker?');
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja, downloaden', () => {
        if (!m.info) m.ask();
        else if (m.info.type === 'calibration') { miClose(); miSave(m, m.info); }
        else miDownloadDialog(m, m.info.type === 'patient');
      });
      miButton(row, 'Nee, afbreken', miClose);
    }

    // Upload-knoppen: een recording-upload verwijdert de huidige snapshots en een snapshots-upload
    // de huidige recording. Is die andere er, dan eerst vragen; pas bij een geslaagde upload wordt
    // hij verwijderd (recUpload / snapUpload), dus Afbreken in de bestandskeuze laat hem staan.
    function miUploadClick(m) {
      const isRec = m === MI_REC;
      const other = isRec ? MI_SNAP : MI_REC;
      const pick = () => document.getElementById(isRec ? 'recFileInput' : 'snapFileInput').click();
      if ((isRec ? snapFrames : recFrames).length === 0) { pick(); return; }
      const box = miOpen('Weet u het zeker?');
      box.appendChild(miEl('p', '', 'Een ' + (isRec ? 'recording' : 'snapshots') + '-upload verwijdert ' +
        (isRec ? 'de huidige serie snapshots.' : 'de huidige recording.') +
        (other.downloaded ? '' : ' Die is nog niet gedownload.')));
      const row = box.appendChild(miEl('div', 'mi-buttons'));
      miButton(row, 'Ja', () => { miClose(); pick(); });
      miButton(row, 'Afbreken', miClose);
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
