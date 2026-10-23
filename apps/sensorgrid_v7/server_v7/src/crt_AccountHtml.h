// by Marius Versteegen, 2026
// Login and account pages of server_v7, plus the user menu (also used on the Grid page).

#pragma once

namespace crt
{
	const char ACCOUNT_CSS[] = R"rawliteral(
    body { margin: 0; font-family: system-ui, sans-serif; background: #fafafa; color: #222; }
    nav { background: #333; padding: 0.5rem 1rem; display: flex; gap: 1.5rem; align-items: center; }
    nav a { text-decoration: none; font-weight: 600; color: #ccc; }
    .page { max-width: 820px; margin: 1.5rem auto; padding: 1rem; }
    h1 { text-align: center; margin-bottom: 1.2rem; }
    .panels { display: flex; gap: 1rem; flex-wrap: wrap; justify-content: center; align-items: flex-start; }
    .panel { background: #fff; border: 1px solid #ddd; border-radius: 6px; padding: 1rem 1.2rem;
             flex: 1 1 300px; max-width: 380px; box-sizing: border-box; }
    .panel h2 { font-size: 1.05rem; margin: 0 0 0.8rem; }
    .panel p { font-size: 0.9rem; color: #444; }
    .field { margin-bottom: 0.7rem; }
    .field label { display: block; font-size: 0.8rem; color: #555; margin-bottom: 0.2rem; }
    .field input, .field select { width: 100%; box-sizing: border-box; padding: 0.4rem;
             border: 1px solid #999; border-radius: 4px; font-size: 0.9rem; }
    .err { color: #c00; font-size: 0.8rem; min-height: 1em; margin-top: 0.2rem; }
    .msg { color: #2e7d32; font-size: 0.85rem; min-height: 1em; margin-top: 0.5rem; }
    .btn { padding: 0.45rem 1rem; border: 2px solid #333; border-radius: 4px; background: #333;
           color: #fff; font-weight: 600; cursor: pointer; font-size: 0.85rem; }
    .btn:disabled { background: #ccc; border-color: #ccc; cursor: default; }
    .btn.sec { background: #fff; color: #333; }
    .note { font-size: 0.85rem; color: #444; background: #fff8e1; border: 1px solid #f0d080;
            border-radius: 4px; padding: 0.5rem; margin-top: 0.8rem; }
    .acc-list { max-height: 280px; overflow-y: auto; border: 1px solid #ddd; border-radius: 4px; }
    .acc-row { display: flex; align-items: center; padding: 0.35rem 0.6rem; border-bottom: 1px solid #eee; }
    .acc-row .role { color: #777; font-size: 0.8rem; margin-left: 0.5rem; }
    .trash { margin-left: auto; background: none; border: none; cursor: pointer; font-size: 1.05rem; }
)rawliteral";

	// Shared by all pages that show the user menu: renders it into <span id="userMenu"> and
	// calls window.accPage(me) (if the page defines it) with the /api/me result, or null when
	// not logged in.
	const char ACCOUNT_JS[] = R"rawliteral(
    const ROLE_NAMES = ['Gebruiker', 'Onderzoeker', 'Admin'];
    const PW_RE = /^[a-z0-9_!.?]{5,20}$/;
    const NAME_RE = /^[a-z0-9_!.?]{5,15}$/;

    function accVal(id) { return document.getElementById(id).value; }
    function accText(id, msg) { const el = document.getElementById(id); if (el) el.textContent = msg || ''; }
    function accPost(url, params) {
      return fetch(url, {method: 'POST', headers: {'Content-Type': 'application/x-www-form-urlencoded'},
                         body: new URLSearchParams(params)})
        .then(r => r.json())
        .catch(() => ({ok: false, field: 'general', error: 'Geen verbinding met het device.'}));
    }
    function accCharsOk(s) { return /^[a-z0-9_!.?]*$/.test(s); }
    function accPwHint(pw) {
      if (!pw) return '';
      if (!accCharsOk(pw)) return 'Alleen kleine letters, cijfers en _ ! . ? zijn toegestaan.';
      if (pw.length < 5 || pw.length > 20) return 'Het wachtwoord moet 5 tot 20 tekens lang zijn.';
      return '';
    }
    function accNameHint(n) {
      if (!n) return '';
      if (!accCharsOk(n)) return 'Alleen kleine letters, cijfers en _ ! . ? zijn toegestaan.';
      if (n.length < 5 || n.length > 15) return 'De gebruikersnaam moet 5 tot 15 tekens lang zijn.';
      return '';
    }
    // Usernames only have lowercase letters: convert while typing.
    function accLower(inp) { const v = inp.value.toLowerCase(); if (v !== inp.value) inp.value = v; }
    // New password + repeat: hints under both fields; true when valid and equal.
    function accPwPair(idNew, idRep, idNewErr, idRepErr) {
      const n = accVal(idNew), r = accVal(idRep);
      accText(idNewErr, accPwHint(n));
      accText(idRepErr, r && r !== n ? 'De wachtwoorden zijn niet gelijk.' : '');
      return PW_RE.test(n) && n === r;
    }

    const UM_CSS =
      '.um{position:relative;margin-left:auto}' +
      '.um-btn{background:#444;color:#fff;border:1px solid #777;border-radius:4px;padding:.25rem .7rem;font-weight:600;font-size:.85rem;cursor:pointer}' +
      '.um-list{position:absolute;right:0;top:calc(100% + 4px);background:#fff;border:1px solid #ccc;border-radius:4px;box-shadow:0 2px 8px rgba(0,0,0,.25);min-width:170px;z-index:100;display:none}' +
      '.um-list.open{display:block}' +
      '.um-list a{display:block;padding:.5rem .8rem;color:#222;text-decoration:none;font-weight:500}' +
      '.um-list a:hover{background:#eee}';
    function umRender(me) {
      const host = document.getElementById('userMenu');
      if (!host || !me) return;
      if (!document.getElementById('umStyle')) {
        const st = document.createElement('style');
        st.id = 'umStyle';
        st.textContent = UM_CSS;
        document.head.appendChild(st);
      }
      host.className = 'um';
      const canCreate = me.role >= 1;
      host.innerHTML = '<button class="um-btn" onclick="umToggle(event)">' + me.name + ' &#9662;</button>' +
        '<div class="um-list" id="umList">' +
        '<a href="/account">Mijn Account</a>' +
        (canCreate ? '<a href="/account_aanmaken">Account aanmaken</a><a href="/accounts_beheren">Accounts beheren</a>' : '') +
        '<a href="#" onclick="umLogout(event)">Uitloggen</a></div>';
    }
    function umToggle(ev) {
      ev.stopPropagation();
      document.getElementById('umList').classList.toggle('open');
    }
    function umLogout(ev) {
      ev.preventDefault();
      accPost('/api/logout', {}).then(() => { location.href = '/login'; });
    }
    document.addEventListener('click', () => {
      const l = document.getElementById('umList');
      if (l) l.classList.remove('open');
    });
    document.addEventListener('DOMContentLoaded', () => {
      fetch('/api/me').then(r => r.ok ? r.json() : null).catch(() => null).then(me => {
        if (me && !me.mustChange) umRender(me);
        if (typeof window.accPage === 'function') window.accPage(me);
      });
    });
)rawliteral";

	const char LOGIN_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>Inloggen</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <link rel="stylesheet" href="/account.css">
  <script src="/account.js"></script>
</head>
<body>
  <nav>
    <a href="/">Home</a>
    <a href="/grid">Grid View</a>
  </nav>
  <div class="page">
    <h1 id="title">Inloggen</h1>
    <div class="panels">
      <div class="panel" id="loginPanel">
        <form onsubmit="doLogin(event)">
          <div class="field">
            <label for="name">Gebruikersnaam</label>
            <input id="name" maxlength="15" autofocus autocomplete="username" autocapitalize="none" oninput="accLower(this);updLogin()"/>
            <div class="err" id="nameErr"></div>
          </div>
          <div class="field">
            <label for="pw">Wachtwoord</label>
            <input id="pw" type="password" maxlength="20" autocomplete="current-password" oninput="updLogin()"/>
            <div class="err" id="pwErr"></div>
          </div>
          <button class="btn" id="btnLogin" type="submit" disabled>Login</button>
          <button class="btn sec" type="button" onclick="doForgot()">Wachtwoord vergeten</button>
          <div class="note" id="forgotNote" style="display:none;"></div>
        </form>
      </div>
      <div class="panel" id="forcePanel" style="display:none;">
        <h2>Nieuw wachtwoord instellen</h2>
        <p>Je bent ingelogd met het fabriekswachtwoord. Kies eerst een eigen wachtwoord.</p>
        <div class="field">
          <label for="fNew">Nieuw wachtwoord (5 tot 20 tekens)</label>
          <input id="fNew" type="password" maxlength="20" autocomplete="new-password" oninput="updForce()"/>
          <div class="err" id="fNewErr"></div>
        </div>
        <div class="field">
          <label for="fRep">Nieuw wachtwoord herhalen</label>
          <input id="fRep" type="password" maxlength="20" autocomplete="new-password" oninput="updForce()"/>
          <div class="err" id="fRepErr"></div>
        </div>
        <button class="btn" id="btnForce" onclick="doForce()" disabled>Opslaan</button>
      </div>
    </div>
  </div>
  <script>
    window.accPage = function(me) {
      if (me && me.mustChange) showForce();
    };
    let loginWaiting = false;
    function updLogin() {
      document.getElementById('btnLogin').disabled = loginWaiting || !(accVal('name') && accVal('pw'));
    }
    function doLogin(ev) {
      ev.preventDefault();
      accText('nameErr'); accText('pwErr');
      document.getElementById('forgotNote').style.display = 'none';
      loginWaiting = true;
      updLogin();
      accPost('/api/login', {name: accVal('name'), password: accVal('pw')}).then(r => {
        if (r.ok) {
          if (r.mustChange) showForce(); else location.href = '/grid';
          return;
        }
        accText(r.field === 'name' ? 'nameErr' : 'pwErr', r.error);
        // the device accepts a new attempt 1 second after a failed one
        setTimeout(() => { loginWaiting = false; updLogin(); }, 1000);
      });
    }
    function doForgot() {
      accText('nameErr'); accText('pwErr');
      const note = document.getElementById('forgotNote');
      note.style.display = 'none';
      accPost('/api/forgot', {name: accVal('name')}).then(r => {
        if (!r.ok) { accText('nameErr', r.error); return; }
        note.textContent = r.text;
        note.style.display = 'block';
      });
    }
    function showForce() {
      document.getElementById('loginPanel').style.display = 'none';
      document.getElementById('forcePanel').style.display = 'block';
      document.getElementById('title').textContent = 'Wachtwoord wijzigen';
      document.getElementById('fNew').focus();
    }
    function updForce() {
      document.getElementById('btnForce').disabled = !accPwPair('fNew', 'fRep', 'fNewErr', 'fRepErr');
    }
    function doForce() {
      accPost('/api/password_force', {new: accVal('fNew')}).then(r => {
        if (r.ok) location.href = '/grid';
        else accText('fNewErr', r.error);
      });
    }
  </script>
</body>
</html>
)rawliteral";

	const char ACCOUNT_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>Mijn account</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <link rel="stylesheet" href="/account.css">
  <script src="/account.js"></script>
</head>
<body>
  <nav>
    <a href="/">Home</a>
    <a href="/grid">Grid View</a>
    <span id="userMenu"></span>
  </nav>
  <div class="page">
    <h1>Mijn account</h1>
    <div class="panels">
      <div class="panel">
        <h2>Wachtwoord wijzigen</h2>
        <div class="field">
          <label for="pwCur">Huidig wachtwoord</label>
          <input id="pwCur" type="password" maxlength="20" autocomplete="current-password" oninput="updPw()"/>
          <div class="err" id="pwCurErr"></div>
        </div>
        <div class="field">
          <label for="pwNew">Nieuw wachtwoord (5 tot 20 tekens)</label>
          <input id="pwNew" type="password" maxlength="20" autocomplete="new-password" oninput="updPw()"/>
          <div class="err" id="pwNewErr"></div>
        </div>
        <div class="field">
          <label for="pwRep">Nieuw wachtwoord herhalen</label>
          <input id="pwRep" type="password" maxlength="20" autocomplete="new-password" oninput="updPw()"/>
          <div class="err" id="pwRepErr"></div>
        </div>
        <button class="btn" id="btnPw" onclick="doPw()" disabled>Wijzigen</button>
        <div class="msg" id="pwMsg"></div>
      </div>
      <div class="panel">
        <h2>Account verwijderen</h2>
        <div id="delBody">
          <p>Je account wordt helemaal verwijderd.</p>
          <div class="field">
            <label for="delPw">Wachtwoord</label>
            <input id="delPw" type="password" maxlength="20" autocomplete="current-password" oninput="updDel()"/>
            <div class="err" id="delPwErr"></div>
          </div>
          <button class="btn" id="btnDel" onclick="doDel()" disabled>Account Verwijderen</button>
        </div>
      </div>
    </div>
  </div>
  <script>
    let me = null;
    window.accPage = function(m) {
      if (!m || m.mustChange) { location.href = '/login'; return; }
      me = m;
      if (me.isRoot) {
        document.getElementById('delBody').innerHTML = '<p>Het root-admin account kan niet worden verwijderd.</p>';
      }
    };
    function updPw() {
      accText('pwMsg');
      const pairOk = accPwPair('pwNew', 'pwRep', 'pwNewErr', 'pwRepErr');
      document.getElementById('btnPw').disabled = !(pairOk && accVal('pwCur'));
    }
    function doPw() {
      accText('pwCurErr');
      accPost('/api/password', {current: accVal('pwCur'), new: accVal('pwNew')}).then(r => {
        if (!r.ok) { accText(r.field === 'current' ? 'pwCurErr' : 'pwNewErr', r.error); return; }
        ['pwCur', 'pwNew', 'pwRep'].forEach(id => { document.getElementById(id).value = ''; });
        updPw();
        accText('pwMsg', 'Je wachtwoord is gewijzigd.');
      });
    }
    function updDel() {
      document.getElementById('btnDel').disabled = !accVal('delPw');
    }
    function doDel() {
      if (!confirm('Weet je zeker dat je je account "' + me.name + '" helemaal wilt verwijderen? Dit kan niet ongedaan worden gemaakt.')) return;
      accPost('/api/delete_self', {password: accVal('delPw')}).then(r => {
        if (r.ok) location.href = '/login';
        else accText('delPwErr', r.error);
      });
    }
  </script>
</body>
</html>
)rawliteral";

	const char ACCOUNT_CREATE_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>Account aanmaken</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <link rel="stylesheet" href="/account.css">
  <script src="/account.js"></script>
</head>
<body>
  <nav>
    <a href="/">Home</a>
    <a href="/grid">Grid View</a>
    <span id="userMenu"></span>
  </nav>
  <div class="page">
    <h1>Account aanmaken</h1>
    <div class="panels">
      <div class="panel">
        <div id="fullNote" class="note" style="display:none;margin-top:0;">
          Het maximaal aantal accounts (20) is bereikt. Accounthouders moeten eerst hun account
          verwijderen, voordat er nieuwe accounts kunnen worden aangemaakt.
        </div>
        <div id="createForm">
          <div class="field">
            <label>Rol</label>
            <div id="roleHost"></div>
          </div>
          <div class="field">
            <label for="name">Gebruikersnaam</label>
            <input id="name" maxlength="15" autocomplete="off" autocapitalize="none" oninput="accLower(this);updCreate()"/>
            <div class="err" id="nameErr"></div>
          </div>
          <div class="field">
            <label for="pw">Wachtwoord (5 tot 20 tekens)</label>
            <input id="pw" type="password" maxlength="20" autocomplete="new-password" oninput="updCreate()"/>
            <div class="err" id="pwErr"></div>
          </div>
          <div class="field">
            <label for="pwRep">Wachtwoord herhalen</label>
            <input id="pwRep" type="password" maxlength="20" autocomplete="new-password" oninput="updCreate()"/>
            <div class="err" id="pwRepErr"></div>
          </div>
          <button class="btn" id="btnCreate" onclick="doCreate()" disabled>Account Aanmaken</button>
          <div class="err" id="generalErr"></div>
          <div class="msg" id="createMsg"></div>
        </div>
      </div>
    </div>
  </div>
  <script>
    let me = null;
    let fixedRole = null;   // set when only one role is possible (no dropdown)
    window.accPage = function(m) {
      if (!m || m.mustChange) { location.href = '/login'; return; }
      if (m.role < 1) { location.href = '/grid'; return; }
      me = m;
      // own role or lower; default one lower than the own role
      const host = document.getElementById('roleHost');
      if (me.role === 0) {
        fixedRole = 0;
        host.innerHTML = '<b>' + ROLE_NAMES[0] + '</b>';
      } else {
        let h = '<select id="role">';
        for (let r = me.role; r >= 0; r--) {
          h += '<option value="' + r + '"' + (r === me.role - 1 ? ' selected' : '') + '>' + ROLE_NAMES[r] + '</option>';
        }
        host.innerHTML = h + '</select>';
      }
      showFull();
    };
    function showFull() {
      const full = me.count >= me.max;
      document.getElementById('fullNote').style.display = full ? 'block' : 'none';
      document.getElementById('createForm').style.display = full ? 'none' : 'block';
    }
    function updCreate() {
      accText('createMsg');
      const n = accVal('name');
      accText('nameErr', accNameHint(n));
      const pairOk = accPwPair('pw', 'pwRep', 'pwErr', 'pwRepErr');
      document.getElementById('btnCreate').disabled = !(pairOk && NAME_RE.test(n));
    }
    function doCreate() {
      accText('generalErr');
      const role = fixedRole !== null ? fixedRole : parseInt(accVal('role'));
      const name = accVal('name');
      accPost('/api/accounts/create', {name: name, role: role, password: accVal('pw')}).then(r => {
        if (!r.ok) {
          accText(r.field === 'name' ? 'nameErr' : (r.field === 'password' ? 'pwErr' : 'generalErr'), r.error);
          return;
        }
        ['name', 'pw', 'pwRep'].forEach(id => { document.getElementById(id).value = ''; });
        updCreate();
        accText('createMsg', 'Account "' + name + '" (' + ROLE_NAMES[role] + ') is aangemaakt.');
        me.count++;
        showFull();
      });
    }
  </script>
</body>
</html>
)rawliteral";

	const char ACCOUNTS_MANAGE_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>Accounts beheren</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <link rel="stylesheet" href="/account.css">
  <script src="/account.js"></script>
</head>
<body>
  <nav>
    <a href="/">Home</a>
    <a href="/grid">Grid View</a>
    <span id="userMenu"></span>
  </nav>
  <div class="page">
    <h1>Accounts beheren</h1>
    <div class="panels">
      <div class="panel">
        <h2>Accounts</h2>
        <div class="acc-list" id="accList"></div>
        <div class="err" id="listErr"></div>
      </div>
      <div class="panel">
        <h2>Wachtwoord resetten</h2>
        <div class="field">
          <label for="resetName">Account</label>
          <select id="resetName" onchange="updReset()"></select>
          <div class="err" id="resetNameErr"></div>
        </div>
        <div class="field">
          <label for="rPw">Nieuw wachtwoord (5 tot 20 tekens)</label>
          <input id="rPw" type="password" maxlength="20" autocomplete="new-password" oninput="updReset()"/>
          <div class="err" id="rPwErr"></div>
        </div>
        <div class="field">
          <label for="rRep">Nieuw wachtwoord herhalen</label>
          <input id="rRep" type="password" maxlength="20" autocomplete="new-password" oninput="updReset()"/>
          <div class="err" id="rRepErr"></div>
        </div>
        <button class="btn" id="btnReset" onclick="doReset()" disabled>Resetten</button>
        <div class="msg" id="resetMsg"></div>
      </div>
    </div>
  </div>
  <script>
    window.accPage = function(m) {
      if (!m || m.mustChange) { location.href = '/login'; return; }
      if (m.role < 1) { location.href = '/grid'; return; }
      loadAccounts();
    };
    function loadAccounts() {
      fetch('/api/accounts').then(r => r.json()).then(r => {
        if (!r.ok) { accText('listErr', r.error); return; }
        const list = document.getElementById('accList');
        const sel = document.getElementById('resetName');
        const prev = sel.value;
        if (r.accounts.length === 0) {
          list.innerHTML = '<div class="acc-row">Er zijn geen accounts die je kunt beheren.</div>';
        } else {
          list.innerHTML = r.accounts.map(a =>
            '<div class="acc-row"><span>' + a.name + '</span><span class="role">' + ROLE_NAMES[a.role] + '</span>' +
            '<button class="trash" title="Account verwijderen" onclick="doDelete(\'' + a.name + '\')">&#128465;</button></div>'
          ).join('');
        }
        sel.innerHTML = '<option value="">kies een account</option>' +
          r.accounts.map(a => '<option value="' + a.name + '">' + a.name + ' (' + ROLE_NAMES[a.role] + ')</option>').join('');
        if (r.accounts.some(a => a.name === prev)) sel.value = prev;
        updReset();
      }).catch(() => accText('listErr', 'Geen verbinding met het device.'));
    }
    function doDelete(name) {
      accText('listErr');
      if (!confirm('Weet je zeker dat je het account "' + name + '" wilt verwijderen?')) return;
      accPost('/api/accounts/delete', {name: name}).then(r => {
        if (!r.ok) accText('listErr', r.error);
        loadAccounts();
      });
    }
    function updReset() {
      accText('resetMsg');
      const pairOk = accPwPair('rPw', 'rRep', 'rPwErr', 'rRepErr');
      document.getElementById('btnReset').disabled = !(pairOk && accVal('resetName'));
    }
    function doReset() {
      accText('resetNameErr');
      const name = accVal('resetName');
      accPost('/api/accounts/reset', {name: name, password: accVal('rPw')}).then(r => {
        if (!r.ok) { accText(r.field === 'password' ? 'rPwErr' : 'resetNameErr', r.error); return; }
        ['rPw', 'rRep'].forEach(id => { document.getElementById(id).value = ''; });
        updReset();
        accText('resetMsg', 'Het wachtwoord van "' + name + '" is gereset.');
      });
    }
  </script>
</body>
</html>
)rawliteral";

} // end namespace crt
