// by Marius Versteegen, 2026
// Home page of server_v7 (/): after login. Admin and Onderzoeker see who is logged in and a button
// to the Grid View; a Gebruiker chooses what to do, which opens the Grid View in that user mode.
// (The former home page "Sensormetingen" is now at /test.)

#pragma once

namespace crt
{
	const char HOME_HTML[] = R"rawliteral(<!doctype html>
<html lang="nl">
<head>
  <meta charset="utf-8" />
  <title>Home</title>
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <link rel="stylesheet" href="/account.css">
  <script src="/account.js"></script>
  <style>
    .home-panel { text-align: center; }
    .home-panel p { font-size: 1.05rem; color: #222; margin: 0.3rem 0 1rem; }
    .home-panel .btn { display: block; margin: 0.6rem auto; width: 16rem; max-width: 100%;
                       box-sizing: border-box; text-decoration: none; font-size: 0.95rem; }
  </style>
</head>
<body data-page="home">
  <nav>
    <span id="navLinks"><a href="/">Home</a></span>
    <span id="userMenu"></span>
  </nav>
  <div class="page">
    <h1>Home</h1>
    <div class="panels">
      <div class="panel home-panel" id="homePanel"></div>
    </div>
  </div>
  <script>
    window.accPage = function(me) {
      if (!me || me.mustChange) { location.href = '/login'; return; }
      const p = document.getElementById('homePanel');
      if (me.role >= 1) {
        p.innerHTML = '<p>U bent ingelogd als ' + (me.role >= 2 ? 'admin' : 'onderzoeker') + '.</p>' +
          '<a class="btn" href="/grid">Open Grid View</a>';
      } else {
        p.innerHTML = '<p>Wat wilt u doen?</p>' +
          USER_MODES.map(u => '<a class="btn" href="/grid?mode=' + u.mode + '">' + u.home + '</a>').join('');
      }
    };
  </script>
</body>
</html>)rawliteral";

} // end namespace crt
