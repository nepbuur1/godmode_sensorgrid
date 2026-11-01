// by Marius Versteegen, 2026
// Recordings and snapshots on the microSD card of the server, plus the HTTP API the Grid page
// uses for its Opslaan and Laden buttons.

#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <dirent.h>
#include <sys/stat.h>
#include <crt_SdCard.h>       // apps/hello_simcard/src: the SPI card driver, tested there
#include "crt_AccountWeb.h"

namespace crt
{
	// Folders on the card:
	//   kalibraties/            recording_kalibratie_<stamp>_org.json
	//   patientmetingen/<id>/   recording_<id>_<stamp>_org.json, snapshots_<id>_<stamp>_org.json
	//   snapshots_andere/       snapshots_andere_<stamp>_org.json (snapshots that are no patient measurement)
	//   current_state/          reserved (not used yet)
	// <id> may start with the test prefix "zzz-" ("-", so "_" only ever separates the name fields).
	// <stamp> is YYYY-MM-DDTHH-MM-SS. A file is written once and never replaced: a name that
	// already exists is refused, so everything on the card is an original (_org).
	//
	// API (all calls need a login; a Gebruiker can list and read patientmetingen only):
	//   GET  /api/sd/status                     {"ok":true,"mounted":bool}
	//   GET  /api/sd/list?dir=<folder>          {"ok":true,"files":[{"name":..,"size":..}]}
	//   GET  /api/sd/file?path=<folder>/<name>  the file itself
	//   POST /api/sd/save?path=<folder>/<name>  multipart/form-data with the file; streamed to
	//                                           <name>.tmp and renamed when complete, so a power
	//                                           cut never leaves a half file under the real name
	class SdStore
	{
	private:
		static const unsigned long REMOUNT_INTERVAL_MS = 5000;
		static const size_t IO_BUFFER_SIZE = 4096;
		static const size_t MAX_ID_LENGTH = 40;

		enum class Folder : uint8_t { Invalid, Kalibraties, SnapshotsAndere, Patient };

		WebServer& server;
		AccountWeb& accounts;
		SdCard card;
		unsigned long lastMountTryMs;
		bool mountTried;

		// State of the upload in progress (the WebServer handles one request at a time).
		FILE* saveFile;
		String saveTmpPath;
		String saveFinalPath;
		int saveStatus;          // 0 = fine so far, otherwise the HTTP error code
		String saveError;
		bool saveSeen;           // the request contained a file part

		// --- names ---

		static bool isLowerAlnum(char c)
		{
			return (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
		}

		// Optional test prefix "zzz-", then 1..40 lowercase letters and digits.
		// "kalibratie" and "andere" are reserved for the file names of the other folders.
		static bool isValidPatientId(const String& id)
		{
			int start = id.startsWith("zzz-") ? 4 : 0;
			int len = id.length() - start;
			if (len < 1 || len > (int)MAX_ID_LENGTH) return false;
			for (int i = start; i < (int)id.length(); i++)
			{
				if (!isLowerAlnum(id[i])) return false;
			}
			return id != "kalibratie" && id != "andere";
		}

		// YYYY-MM-DDTHH-MM-SS
		static bool isStamp(const String& s)
		{
			static const char* PATTERN = "dddd-dd-ddTdd-dd-dd";
			if (s.length() != 19) return false;
			for (int i = 0; i < 19; i++)
			{
				char p = PATTERN[i];
				if (p == 'd' ? !isDigit(s[i]) : s[i] != p) return false;
			}
			return true;
		}

		// "kalibraties", "snapshots_andere" or "patientmetingen/<id>". part receives what the file
		// names in that folder contain after "recording_" / "snapshots_".
		static Folder parseFolder(const String& dir, String& part)
		{
			if (dir == "kalibraties") { part = "kalibratie"; return Folder::Kalibraties; }
			if (dir == "snapshots_andere") { part = "andere"; return Folder::SnapshotsAndere; }
			static const char* PATIENT = "patientmetingen/";
			if (dir.startsWith(PATIENT))
			{
				part = dir.substring(strlen(PATIENT));
				if (isValidPatientId(part)) return Folder::Patient;
			}
			return Folder::Invalid;
		}

		// A name that may be written into the folder: <word>_<part>_<stamp>_org.json, where
		// kalibraties only holds recordings and snapshots_andere only snapshots.
		static bool isWritableName(Folder folder, const String& part, const String& name)
		{
			static const char* TAIL = "_org.json";
			String rest;
			if (name.startsWith("recording_") && folder != Folder::SnapshotsAndere) rest = name.substring(10);
			else if (name.startsWith("snapshots_") && folder != Folder::Kalibraties) rest = name.substring(10);
			else return false;
			if (!rest.endsWith(TAIL)) return false;
			rest = rest.substring(0, rest.length() - strlen(TAIL));
			if (rest.length() < 21) return false;
			String stamp = rest.substring(rest.length() - 19);
			return isStamp(stamp) && rest[rest.length() - 20] == '_' && rest.substring(0, rest.length() - 20) == part;
		}

		// Any .json file that is safe to read and to put in a JSON list unescaped.
		static bool isReadableName(const String& name)
		{
			if (name.length() < 6 || name[0] == '.' || !name.endsWith(".json")) return false;
			for (int i = 0; i < (int)name.length(); i++)
			{
				char c = name[i];
				if (!isAlphaNumeric(c) && c != '_' && c != '-' && c != '.') return false;
			}
			return true;
		}

		static void splitPath(const String& path, String& dir, String& name)
		{
			int slash = path.lastIndexOf('/');
			dir = (slash < 0) ? "" : path.substring(0, slash);
			name = (slash < 0) ? path : path.substring(slash + 1);
		}

		static String fullPath(const String& rel)
		{
			return String(SdCard::MOUNT_POINT) + "/" + rel;
		}

		// --- card ---

		void createFolders()
		{
			static const char* FOLDERS[] = { "current_state", "kalibraties", "patientmetingen", "snapshots_andere" };
			for (const char* f : FOLDERS)
			{
				mkdir(fullPath(f).c_str(), 0775);   // an existing folder is fine
			}
		}

		// Mounted? Otherwise retries now and then, so a card inserted later is found too.
		bool ensureMounted()
		{
			if (card.isMounted()) return true;
			if (mountTried && millis() - lastMountTryMs < REMOUNT_INTERVAL_MS) return false;
			mountTried = true;
			lastMountTryMs = millis();
			if (!card.mount()) return false;
			createFolders();
			SdCard::Properties p = card.getProperties();
			ESP_LOGI("SdStore", "SD card %s mounted, %llu MB", p.name, p.capacityBytes / (1024ULL * 1024ULL));
			return true;
		}

		// --- responses ---

		void sendError(int code, const String& msg)
		{
			server.send(code, "application/json", "{\"ok\":false,\"error\":\"" + msg + "\"}");
		}

		// Logged in, card present and (for a Gebruiker) the folder allowed? Otherwise answers.
		bool checkAccess(Folder folder, bool forWriting)
		{
			int role = accounts.currentRole();
			if (role < 0) { sendError(401, "Niet ingelogd."); return false; }
			if (folder == Folder::Invalid) { sendError(400, "Ongeldige map of bestandsnaam."); return false; }
			if (!forWriting && role == (int)Role::Gebruiker && folder != Folder::Patient)
			{
				sendError(403, "Geen toegang tot deze map.");
				return false;
			}
			if (!ensureMounted()) { sendError(503, "Geen SD-kaart gevonden."); return false; }
			return true;
		}

		// --- handlers ---

		void handleStatus()
		{
			if (accounts.currentRole() < 0) { sendError(401, "Niet ingelogd."); return; }
			server.send(200, "application/json",
				String("{\"ok\":true,\"mounted\":") + (ensureMounted() ? "true" : "false") + "}");
		}

		void handleList()
		{
			String dir = server.arg("dir");
			String part;
			Folder folder = parseFolder(dir, part);
			if (!checkAccess(folder, false)) return;

			String json = "{\"ok\":true,\"files\":[";
			DIR* d = opendir(fullPath(dir).c_str());
			if (d != nullptr)   // a patient without files yet has no folder: an empty list
			{
				bool first = true;
				struct dirent* e;
				while ((e = readdir(d)) != nullptr)
				{
					String name = e->d_name;
					if (e->d_type == DT_DIR || !isReadableName(name)) continue;
					struct stat st;
					long size = (stat(fullPath(dir + "/" + name).c_str(), &st) == 0) ? (long)st.st_size : 0;
					json += String(first ? "" : ",") + "{\"name\":\"" + name + "\",\"size\":" + String(size) + "}";
					first = false;
				}
				closedir(d);
			}
			json += "]}";
			server.send(200, "application/json", json);
		}

		void handleFile()
		{
			String dir, name, part;
			splitPath(server.arg("path"), dir, name);
			Folder folder = parseFolder(dir, part);
			if (!isReadableName(name)) folder = Folder::Invalid;
			if (!checkAccess(folder, false)) return;

			String path = fullPath(dir + "/" + name);
			struct stat st;
			FILE* f = (stat(path.c_str(), &st) == 0) ? fopen(path.c_str(), "rb") : nullptr;
			if (f == nullptr) { sendError(404, "Bestand niet gevonden."); return; }

			uint8_t* buf = (uint8_t*)malloc(IO_BUFFER_SIZE);
			if (buf == nullptr) { fclose(f); sendError(500, "Te weinig geheugen."); return; }
			server.setContentLength(st.st_size);
			server.sendHeader("Cache-Control", "no-store");
			server.send(200, "application/json", "");
			size_t n;
			while ((n = fread(buf, 1, IO_BUFFER_SIZE, f)) > 0)
			{
				server.sendContent((const char*)buf, n);
			}
			free(buf);
			fclose(f);
		}

		void failSave(int code, const String& msg)
		{
			if (saveFile != nullptr)
			{
				fclose(saveFile);
				saveFile = nullptr;
				remove(saveTmpPath.c_str());
			}
			if (saveStatus == 0)
			{
				saveStatus = code;
				saveError = msg;
			}
		}

		// Called for every piece of the uploaded file.
		void handleSaveUpload()
		{
			HTTPUpload& up = server.upload();
			if (up.status == UPLOAD_FILE_START)
			{
				saveSeen = true;
				saveStatus = 0;
				saveError = "";
				saveFile = nullptr;

				String dir, name, part;
				splitPath(server.arg("path"), dir, name);
				Folder folder = parseFolder(dir, part);
				if (accounts.currentRole() < 0) { failSave(401, "Niet ingelogd."); return; }
				if (folder == Folder::Invalid || !isWritableName(folder, part, name))
				{
					failSave(400, "Ongeldige map of bestandsnaam.");
					return;
				}
				if (!ensureMounted()) { failSave(503, "Geen SD-kaart gevonden."); return; }

				if (folder == Folder::Patient) mkdir(fullPath(dir).c_str(), 0775);
				saveFinalPath = fullPath(dir + "/" + name);
				saveTmpPath = saveFinalPath + ".tmp";
				struct stat st;
				if (stat(saveFinalPath.c_str(), &st) == 0)
				{
					failSave(409, "Dit bestand staat al op de SD-kaart; opnieuw opslaan wordt niet ondersteund.");
					return;
				}
				remove(saveTmpPath.c_str());   // left over from an interrupted earlier attempt
				saveFile = fopen(saveTmpPath.c_str(), "wb");
				if (saveFile == nullptr) { failSave(500, "Kan niet schrijven op de SD-kaart."); return; }
				setvbuf(saveFile, nullptr, _IOFBF, IO_BUFFER_SIZE);
			}
			else if (up.status == UPLOAD_FILE_WRITE)
			{
				if (saveFile != nullptr && fwrite(up.buf, 1, up.currentSize, saveFile) != up.currentSize)
				{
					failSave(500, "Schrijven op de SD-kaart mislukt (kaart vol?).");
				}
			}
			else if (up.status == UPLOAD_FILE_END)
			{
				if (saveFile == nullptr) return;
				bool closed = (fclose(saveFile) == 0);
				saveFile = nullptr;
				if (!closed) { remove(saveTmpPath.c_str()); failSave(500, "Schrijven op de SD-kaart mislukt."); return; }
				if (rename(saveTmpPath.c_str(), saveFinalPath.c_str()) != 0)
				{
					remove(saveTmpPath.c_str());
					failSave(409, "Dit bestand staat al op de SD-kaart; opnieuw opslaan wordt niet ondersteund.");
					return;
				}
				ESP_LOGI("SdStore", "Saved %s (%u bytes)", saveFinalPath.c_str(), (unsigned)up.totalSize);
			}
			else if (up.status == UPLOAD_FILE_ABORTED)
			{
				failSave(500, "Opslaan afgebroken.");
			}
		}

		// Called once the whole request has been received.
		void handleSaveDone()
		{
			if (!saveSeen) sendError(400, "Geen bestand ontvangen.");
			else if (saveStatus != 0) sendError(saveStatus, saveError);
			else server.send(200, "application/json", "{\"ok\":true}");
			saveSeen = false;
		}

	public:
		SdStore(WebServer& webServer, AccountWeb& accountWeb,
				gpio_num_t pinCs, gpio_num_t pinMosi, gpio_num_t pinMiso, gpio_num_t pinSclk, int freqKhz)
			: server(webServer), accounts(accountWeb),
			  card(pinCs, pinMosi, pinMiso, pinSclk, freqKhz),
			  lastMountTryMs(0), mountTried(false),
			  saveFile(nullptr), saveStatus(0), saveSeen(false)
		{
		}

		// Call before server.begin(). A missing card is not fatal: the API then answers 503
		// and the card is looked for again on later requests.
		void begin()
		{
			if (!ensureMounted()) ESP_LOGW("SdStore", "No SD card: Opslaan/Laden unavailable until one is inserted");

			server.on("/api/sd/status", HTTP_GET, [this]() { handleStatus(); });
			server.on("/api/sd/list", HTTP_GET, [this]() { handleList(); });
			server.on("/api/sd/file", HTTP_GET, [this]() { handleFile(); });
			server.on("/api/sd/save", HTTP_POST, [this]() { handleSaveDone(); }, [this]() { handleSaveUpload(); });
		}
	};

} // end namespace crt
