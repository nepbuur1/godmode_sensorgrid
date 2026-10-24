// by Marius Versteegen, 2026
// Login, sessions and the account pages + API of server_v7.

#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <esp_random.h>
#include <crt_SensorGridHttp.h>
#include "crt_AccountStore.h"
#include "crt_AccountHtml.h"

namespace crt
{
	// Sessions live in RAM: a random token in the cookie "sgsess", valid for 3 hours after
	// login. After a restart of the ESP32 everybody logs in again.
	//
	// Rights: Admin and Onderzoeker create accounts of their own or a lower role, and manage
	// (delete, reset password) accounts of a lower role. The root admin manages all other
	// accounts, including other Admins. The root admin itself cannot be deleted.
	class AccountWeb
	{
	private:
		static const uint8_t MAX_SESSIONS = 16;
		static const unsigned long SESSION_MS = 3UL * 60UL * 60UL * 1000UL;
		static const unsigned long LOGIN_FAIL_WAIT_MS = 1000;
		static constexpr const char* COOKIE_NAME = "sgsess";

		struct Session
		{
			bool used;
			char token[33];
			int idx;
			bool mustChange;      // root admin with the factory password: change it first
			unsigned long createdMs;
		};

		WebServer& server;
		AccountStore store;
		Session sessions[MAX_SESSIONS];
		bool loginFailed;
		unsigned long lastLoginFailMs;

		// --- sessions ---

		String cookieToken()
		{
			String cookies = server.header("Cookie");
			String key = String(COOKIE_NAME) + "=";
			int p = cookies.indexOf(key);
			while (p > 0 && cookies[p - 1] != ' ' && cookies[p - 1] != ';')
			{
				p = cookies.indexOf(key, p + 1);
			}
			if (p < 0) return "";
			int end = cookies.indexOf(';', p);
			return cookies.substring(p + key.length(), end < 0 ? cookies.length() : end);
		}

		Session* currentSession()
		{
			String token = cookieToken();
			if (token.length() != 32) return nullptr;
			for (Session& s : sessions)
			{
				if (s.used && token == s.token)
				{
					if (millis() - s.createdMs >= SESSION_MS || !store.isUsed(s.idx))
					{
						s.used = false;
						return nullptr;
					}
					return &s;
				}
			}
			return nullptr;
		}

		Session& newSession(int idx, bool mustChange)
		{
			Session* slot = &sessions[0];
			for (Session& s : sessions)
			{
				if (!s.used) { slot = &s; break; }
				if (s.createdMs < slot->createdMs) slot = &s;   // all in use: replace the oldest
			}
			uint8_t rnd[16];
			esp_fill_random(rnd, sizeof(rnd));
			for (int i = 0; i < 16; i++) snprintf(slot->token + 2 * i, 3, "%02x", rnd[i]);
			slot->used = true;
			slot->idx = idx;
			slot->mustChange = mustChange;
			slot->createdMs = millis();
			return *slot;
		}

		void dropSessionsOf(int idx, const Session* keep = nullptr)
		{
			for (Session& s : sessions)
			{
				if (s.used && s.idx == idx && &s != keep) s.used = false;
			}
		}

		void setSessionCookie(const char* token)
		{
			server.sendHeader("Set-Cookie", String(COOKIE_NAME) + "=" + token +
				"; Path=/; Max-Age=" + String(SESSION_MS / 1000) + "; HttpOnly; SameSite=Strict");
		}
		void clearSessionCookie()
		{
			server.sendHeader("Set-Cookie", String(COOKIE_NAME) + "=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
		}

		bool isTestClient()
		{
			return server.header(TEST_CLIENT_HEADER) == TEST_CLIENT_TOKEN;
		}

		// --- rights ---

		bool canCreate(int actor, Role newRole)
		{
			Role own = store.role(actor);
			return own >= Role::Onderzoeker && newRole <= own;
		}

		bool canManage(int actor, int target)
		{
			if (!store.isUsed(target) || target == actor || target == AccountStore::ROOT) return false;
			if (actor == AccountStore::ROOT) return true;
			return store.role(actor) >= Role::Onderzoeker && store.role(target) < store.role(actor);
		}

		// --- responses ---

		void sendOk(const String& extra = "")
		{
			server.send(200, "application/json", "{\"ok\":true" + extra + "}");
		}
		// field: the input under which the page shows the (red) message.
		void sendError(int code, const char* field, const String& msg)
		{
			server.send(code, "application/json",
				String("{\"ok\":false,\"field\":\"") + field + "\",\"error\":\"" + msg + "\"}");
		}
		void redirect(const char* location)
		{
			server.sendHeader("Location", location);
			server.send(302, "text/plain", "");
		}

		// Logged in (and not in the forced password change)? Otherwise answers 401.
		Session* requireLogin()
		{
			Session* s = currentSession();
			if (!s || s->mustChange)
			{
				sendError(401, "general", "Niet ingelogd.");
				return nullptr;
			}
			return s;
		}

		// Where a logged-in user lands: Admin and Onderzoeker go to the Grid View, a Gebruiker to the
		// Home page with its choices (crt_HomeHtml.h).
		const char* landingPage(Session* s)
		{
			return store.role(s->idx) >= Role::Onderzoeker ? "/grid" : "/";
		}

		void servePage(const char* html, Role minRole)
		{
			Session* s = currentSession();
			if (!s || s->mustChange) { redirect("/login"); return; }
			if (store.role(s->idx) < minRole) { redirect(landingPage(s)); return; }
			server.send(200, "text/html", html);
		}

		// --- API handlers ---

		void handleLogin()
		{
			if (loginFailed && millis() - lastLoginFailMs < LOGIN_FAIL_WAIT_MS)
			{
				sendError(429, "password", "Even wachten en dan opnieuw proberen.");
				return;
			}
			String name = server.arg("name");
			String pw = server.arg("password");
			int idx = store.find(name);
			if (idx == AccountStore::NOT_FOUND || !store.checkPassword(idx, pw))
			{
				loginFailed = true;
				lastLoginFailMs = millis();
				sendError(401, "password", "Onjuiste gebruikersnaam of wachtwoord.");
				return;
			}
			bool mustChange = (idx == AccountStore::ROOT) && store.rootHasFactoryPassword();
			Session& s = newSession(idx, mustChange);
			setSessionCookie(s.token);
			sendOk(String(",\"mustChange\":") + (mustChange ? "true" : "false") +
				",\"role\":" + String((int)store.role(idx)));
		}

		void handleLogout()
		{
			Session* s = currentSession();
			if (s) s->used = false;
			clearSessionCookie();
			sendOk();
		}

		void handleMe()
		{
			Session* s = currentSession();
			if (!s)
			{
				sendError(401, "general", "Niet ingelogd.");
				return;
			}
			sendOk(String(",\"name\":\"") + store.name(s->idx) + "\"" +
				",\"role\":" + String((int)store.role(s->idx)) +
				",\"isRoot\":" + (s->idx == AccountStore::ROOT ? "true" : "false") +
				",\"mustChange\":" + (s->mustChange ? "true" : "false") +
				",\"count\":" + String(store.count()) +
				",\"max\":" + String(AccountStore::MAX_ACCOUNTS));
		}

		void handleForgot()
		{
			String name = server.arg("name");
			if (name.length() == 0)
			{
				sendError(400, "name", "Vul eerst je gebruikersnaam in.");
				return;
			}
			int idx = store.find(name);
			if (idx == AccountStore::NOT_FOUND)
			{
				sendError(404, "name", "Onbekende gebruikersnaam.");
				return;
			}
			const char* text;
			if (idx == AccountStore::ROOT)
				text = "Herstart het device en houd binnen 10 seconden de BOOT-knop 2 seconden ingedrukt. Daarna geldt weer het fabriekswachtwoord.";
			else if (store.role(idx) == Role::Admin)
				text = "Vraag fysiek aan de root-admin om je wachtwoord te laten resetten.";
			else if (store.role(idx) == Role::Onderzoeker)
				text = "Vraag fysiek aan een Admin om je wachtwoord te laten resetten.";
			else
				text = "Vraag fysiek aan een Onderzoeker of Admin om je wachtwoord te laten resetten.";
			sendOk(String(",\"text\":\"") + text + "\"");
		}

		// Forced change of the root admin's factory password, right after logging in.
		void handleForcePassword()
		{
			Session* s = currentSession();
			if (!s || !s->mustChange)
			{
				sendError(403, "general", "Niet toegestaan.");
				return;
			}
			String pw = server.arg("new");
			const char* problem = AccountStore::passwordProblem(pw);
			if (problem) { sendError(400, "new", problem); return; }
			if (AccountStore::isFactoryPassword(pw))
			{
				sendError(400, "new", "Kies een ander wachtwoord dan het fabriekswachtwoord.");
				return;
			}
			if (store.setPassword(s->idx, pw) != AccountStore::Result::Ok)
			{
				sendError(500, "new", "Opslaan in flash is mislukt.");
				return;
			}
			s->mustChange = false;
			sendOk();
		}

		void handleChangePassword()
		{
			Session* s = requireLogin();
			if (!s) return;
			if (!store.checkPassword(s->idx, server.arg("current")))
			{
				sendError(400, "current", "Het huidige wachtwoord is onjuist.");
				return;
			}
			String pw = server.arg("new");
			const char* problem = AccountStore::passwordProblem(pw);
			if (problem) { sendError(400, "new", problem); return; }
			if (store.setPassword(s->idx, pw) != AccountStore::Result::Ok)
			{
				sendError(500, "new", "Opslaan in flash is mislukt.");
				return;
			}
			dropSessionsOf(s->idx, s);   // other logins of this account must log in again
			sendOk();
		}

		void handleDeleteSelf()
		{
			Session* s = requireLogin();
			if (!s) return;
			if (s->idx == AccountStore::ROOT)
			{
				sendError(403, "password", "Het root-admin account kan niet worden verwijderd.");
				return;
			}
			if (!store.checkPassword(s->idx, server.arg("password")))
			{
				sendError(400, "password", "Onjuist wachtwoord.");
				return;
			}
			int idx = s->idx;
			if (store.remove(idx) != AccountStore::Result::Ok)
			{
				sendError(500, "password", "Verwijderen uit flash is mislukt.");
				return;
			}
			dropSessionsOf(idx);
			clearSessionCookie();
			sendOk();
		}

		void handleCreate()
		{
			Session* s = requireLogin();
			if (!s) return;
			int roleNr = server.arg("role").toInt();
			if (roleNr < 0 || roleNr > (int)Role::Admin || !canCreate(s->idx, (Role)roleNr))
			{
				sendError(403, "general", "Je mag geen account met deze rol aanmaken.");
				return;
			}
			if (store.count() >= AccountStore::MAX_ACCOUNTS)
			{
				sendError(400, "general", "Het maximaal aantal accounts (20) is bereikt.");
				return;
			}
			String name = server.arg("name");
			String pw = server.arg("password");
			const char* problem = AccountStore::nameProblem(name);
			if (problem) { sendError(400, "name", problem); return; }
			if (store.find(name) != AccountStore::NOT_FOUND)
			{
				sendError(400, "name", "Deze gebruikersnaam bestaat al.");
				return;
			}
			problem = AccountStore::passwordProblem(pw);
			if (problem) { sendError(400, "password", problem); return; }
			AccountStore::Result r = store.create(name, (Role)roleNr, pw);
			if (r == AccountStore::Result::Full)
				sendError(400, "general", "Het maximaal aantal accounts (20) is bereikt.");
			else if (r != AccountStore::Result::Ok)
				sendError(500, "general", "Opslaan in flash is mislukt.");
			else
				sendOk();
		}

		// The accounts the logged-in user may manage.
		void handleList()
		{
			Session* s = requireLogin();
			if (!s) return;
			String json = ",\"accounts\":[";
			bool first = true;
			for (int i = 0; i < AccountStore::MAX_ACCOUNTS; i++)
			{
				if (!canManage(s->idx, i)) continue;
				if (!first) json += ",";
				first = false;
				json += String("{\"name\":\"") + store.name(i) + "\",\"role\":" + String((int)store.role(i)) + "}";
			}
			json += "]";
			sendOk(json);
		}

		void handleAdminDelete()
		{
			Session* s = requireLogin();
			if (!s) return;
			int target = store.find(server.arg("name"));
			if (!canManage(s->idx, target))
			{
				sendError(403, "general", "Je mag dit account niet verwijderen.");
				return;
			}
			if (store.remove(target) != AccountStore::Result::Ok)
			{
				sendError(500, "general", "Verwijderen uit flash is mislukt.");
				return;
			}
			dropSessionsOf(target);
			sendOk();
		}

		void handleAdminReset()
		{
			Session* s = requireLogin();
			if (!s) return;
			int target = store.find(server.arg("name"));
			if (!canManage(s->idx, target))
			{
				sendError(403, "name", "Je mag het wachtwoord van dit account niet resetten.");
				return;
			}
			String pw = server.arg("password");
			const char* problem = AccountStore::passwordProblem(pw);
			if (problem) { sendError(400, "password", problem); return; }
			if (store.setPassword(target, pw) != AccountStore::Result::Ok)
			{
				sendError(500, "password", "Opslaan in flash is mislukt.");
				return;
			}
			dropSessionsOf(target);
			sendOk();
		}

	public:
		AccountWeb(WebServer& webServer)
			: server(webServer), loginFailed(false), lastLoginFailMs(0)
		{
			memset(sessions, 0, sizeof(sessions));
		}

		// Call before server.begin().
		void begin()
		{
			store.begin();

			static const char* headerKeys[] = { "Cookie", TEST_CLIENT_HEADER };
			server.collectHeaders(headerKeys, 2);

			server.on("/login", HTTP_GET, [this]() {
				Session* s = currentSession();
				if (s && !s->mustChange) { redirect(landingPage(s)); return; }
				server.send(200, "text/html", LOGIN_HTML);
			});
			server.on("/account", HTTP_GET, [this]() {
				servePage(ACCOUNT_HTML, Role::Gebruiker);
			});
			server.on("/account_aanmaken", HTTP_GET, [this]() {
				servePage(ACCOUNT_CREATE_HTML, Role::Onderzoeker);
			});
			server.on("/accounts_beheren", HTTP_GET, [this]() {
				servePage(ACCOUNTS_MANAGE_HTML, Role::Onderzoeker);
			});
			server.on("/account.css", HTTP_GET, [this]() {
				server.send(200, "text/css", ACCOUNT_CSS);
			});
			server.on("/account.js", HTTP_GET, [this]() {
				server.send(200, "application/javascript", ACCOUNT_JS);
			});

			server.on("/api/login", HTTP_POST, [this]() { handleLogin(); });
			server.on("/api/logout", HTTP_POST, [this]() { handleLogout(); });
			server.on("/api/me", HTTP_GET, [this]() { handleMe(); });
			server.on("/api/forgot", HTTP_POST, [this]() { handleForgot(); });
			server.on("/api/password_force", HTTP_POST, [this]() { handleForcePassword(); });
			server.on("/api/password", HTTP_POST, [this]() { handleChangePassword(); });
			server.on("/api/delete_self", HTTP_POST, [this]() { handleDeleteSelf(); });
			server.on("/api/accounts", HTTP_GET, [this]() { handleList(); });
			server.on("/api/accounts/create", HTTP_POST, [this]() { handleCreate(); });
			server.on("/api/accounts/delete", HTTP_POST, [this]() { handleAdminDelete(); });
			server.on("/api/accounts/reset", HTTP_POST, [this]() { handleAdminReset(); });
		}

		// For pages that need a login (/grid): true if allowed, otherwise redirects to /login.
		bool allowPage()
		{
			if (isTestClient()) return true;
			Session* s = currentSession();
			if (s && !s->mustChange) return true;
			redirect("/login");
			return false;
		}

		void resetRootPassword()
		{
			store.resetRootPassword();
			dropSessionsOf(AccountStore::ROOT);
		}
	};

} // end namespace crt
