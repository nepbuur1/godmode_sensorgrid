// by Marius Versteegen, 2026
// User accounts of server_v7, stored in NVS.

#pragma once
#include <Arduino.h>
#include <Preferences.h>
#include <esp_random.h>
#include <mbedtls/pkcs5.h>

namespace crt
{
	enum class Role : uint8_t
	{
		Gebruiker = 0,
		Onderzoeker = 1,
		Admin = 2,
	};

	inline const char* roleName(Role role)
	{
		switch (role)
		{
			case Role::Admin:       return "Admin";
			case Role::Onderzoeker: return "Onderzoeker";
			default:                return "Gebruiker";
		}
	}

	// At most 20 accounts, including the fixed root admin "admin". Stored in NVS (namespace
	// "accounts"): key "root" for the root admin, keys "s0".."s18" for the other accounts.
	// Only salted PBKDF2-SHA256 hashes are stored, never passwords. NVS is independent of any
	// file system, so a later SD card (FAT on /sdcard) does not interfere with it.
	//
	// The root admin has the factory password "geheim" as long as key "root" is absent;
	// resetRootPassword() removes that key again (BOOT button, see ServerNode).
	//
	// Index 0 is the root admin, 1..19 are the other accounts.
	class AccountStore
	{
	public:
		static const uint8_t MAX_ACCOUNTS = 20;
		static const int ROOT = 0;
		static const int NOT_FOUND = -1;

		enum class Result : uint8_t
		{
			Ok,
			InvalidName,
			InvalidPassword,
			NameExists,
			Full,
			Forbidden,
			StorageError,
		};

	private:
		static const uint8_t NAME_MAX_LEN = 15;
		static const uint8_t SALT_LEN = 16;
		static const uint8_t HASH_LEN = 32;
		static const uint32_t PBKDF2_ITERATIONS = 2000;
		static const uint8_t RECORD_VERSION = 1;
		static constexpr const char* NVS_NAMESPACE = "accounts";
		static constexpr const char* ROOT_NAME = "admin";
		static constexpr const char* ROOT_FACTORY_PASSWORD = "geheim";

		struct Record
		{
			uint8_t version;
			uint8_t role;
			char name[NAME_MAX_LEN + 1];
			uint8_t salt[SALT_LEN];
			uint8_t hash[HASH_LEN];
		};

		Preferences prefs;
		Record records[MAX_ACCOUNTS];
		bool used[MAX_ACCOUNTS];
		bool rootFactoryPassword;

		static void keyOf(int idx, char* key, size_t len)
		{
			if (idx == ROOT) snprintf(key, len, "root");
			else snprintf(key, len, "s%d", idx - 1);
		}

		static void hashPassword(const char* pw, const uint8_t* salt, uint8_t* out)
		{
			mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
				(const unsigned char*)pw, strlen(pw), salt, SALT_LEN,
				PBKDF2_ITERATIONS, HASH_LEN, out);
		}

		static bool equalConstTime(const uint8_t* a, const uint8_t* b, size_t len)
		{
			uint8_t diff = 0;
			for (size_t i = 0; i < len; i++) diff |= a[i] ^ b[i];
			return diff == 0;
		}

		void setRootFactoryRecord()
		{
			memset(&records[ROOT], 0, sizeof(Record));
			records[ROOT].version = RECORD_VERSION;
			records[ROOT].role = (uint8_t)Role::Admin;
			strncpy(records[ROOT].name, ROOT_NAME, NAME_MAX_LEN);
			used[ROOT] = true;
			rootFactoryPassword = true;
		}

		bool store(int idx, const char* name, Role role, const char* pw)
		{
			Record r = {};
			r.version = RECORD_VERSION;
			r.role = (uint8_t)role;
			strncpy(r.name, name, NAME_MAX_LEN);
			esp_fill_random(r.salt, SALT_LEN);
			hashPassword(pw, r.salt, r.hash);
			char key[8];
			keyOf(idx, key, sizeof(key));
			if (prefs.putBytes(key, &r, sizeof(r)) != sizeof(r)) return false;
			records[idx] = r;
			used[idx] = true;
			return true;
		}

		static bool validChars(const String& s)
		{
			for (size_t i = 0; i < s.length(); i++)
			{
				char c = s[i];
				bool ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
						  c == '_' || c == '!' || c == '.' || c == '?';
				if (!ok) return false;
			}
			return true;
		}

	public:
		AccountStore() : rootFactoryPassword(true)
		{
			memset(used, 0, sizeof(used));
		}

		void begin()
		{
			prefs.begin(NVS_NAMESPACE, false);
			for (int i = 0; i < MAX_ACCOUNTS; i++)
			{
				char key[8];
				keyOf(i, key, sizeof(key));
				used[i] = prefs.isKey(key) &&
						  prefs.getBytes(key, &records[i], sizeof(Record)) == sizeof(Record) &&
						  records[i].version == RECORD_VERSION;
			}
			if (used[ROOT]) rootFactoryPassword = false;
			else setRootFactoryRecord();
			ESP_LOGI("AccountStore", "%d account(s), root admin %s", count(),
					 rootFactoryPassword ? "has the factory password" : "has its own password");
		}

		// Forget the root admin's own password: "geheim" applies again (with forced change).
		void resetRootPassword()
		{
			prefs.remove("root");
			setRootFactoryRecord();
			ESP_LOGW("AccountStore", "Root admin password reset to the factory password");
		}

		// Names: 5..15 characters, passwords: 5..20; both only a-z 0-9 _ ! . ?
		static const char* nameProblem(const String& name)
		{
			if (!validChars(name)) return "Alleen kleine letters, cijfers en _ ! . ? zijn toegestaan.";
			if (name.length() < 5 || name.length() > NAME_MAX_LEN) return "De gebruikersnaam moet 5 tot 15 tekens lang zijn.";
			return nullptr;
		}
		static const char* passwordProblem(const String& pw)
		{
			if (!validChars(pw)) return "Alleen kleine letters, cijfers en _ ! . ? zijn toegestaan.";
			if (pw.length() < 5 || pw.length() > 20) return "Het wachtwoord moet 5 tot 20 tekens lang zijn.";
			return nullptr;
		}
		static bool isFactoryPassword(const String& pw) { return pw == ROOT_FACTORY_PASSWORD; }

		int find(const String& name) const
		{
			for (int i = 0; i < MAX_ACCOUNTS; i++)
			{
				if (used[i] && name == records[i].name) return i;
			}
			return NOT_FOUND;
		}

		int count() const
		{
			int n = 0;
			for (int i = 0; i < MAX_ACCOUNTS; i++) if (used[i]) n++;
			return n;
		}

		bool isUsed(int idx) const { return idx >= 0 && idx < MAX_ACCOUNTS && used[idx]; }
		const char* name(int idx) const { return records[idx].name; }
		Role role(int idx) const { return (Role)records[idx].role; }
		bool rootHasFactoryPassword() const { return rootFactoryPassword; }

		bool checkPassword(int idx, const String& pw) const
		{
			if (!isUsed(idx)) return false;
			if (idx == ROOT && rootFactoryPassword) return pw == ROOT_FACTORY_PASSWORD;
			uint8_t hash[HASH_LEN];
			hashPassword(pw.c_str(), records[idx].salt, hash);
			return equalConstTime(hash, records[idx].hash, HASH_LEN);
		}

		Result create(const String& name, Role role, const String& pw)
		{
			if (nameProblem(name)) return Result::InvalidName;
			if (passwordProblem(pw)) return Result::InvalidPassword;
			if (find(name) != NOT_FOUND) return Result::NameExists;
			for (int i = 1; i < MAX_ACCOUNTS; i++)
			{
				if (!used[i])
				{
					return store(i, name.c_str(), role, pw.c_str()) ? Result::Ok : Result::StorageError;
				}
			}
			return Result::Full;
		}

		Result setPassword(int idx, const String& pw)
		{
			if (!isUsed(idx)) return Result::Forbidden;
			if (passwordProblem(pw)) return Result::InvalidPassword;
			String nm = records[idx].name;
			if (!store(idx, nm.c_str(), role(idx), pw.c_str())) return Result::StorageError;
			if (idx == ROOT) rootFactoryPassword = false;
			return Result::Ok;
		}

		Result remove(int idx)
		{
			if (idx == ROOT || !isUsed(idx)) return Result::Forbidden;
			char key[8];
			keyOf(idx, key, sizeof(key));
			if (!prefs.remove(key)) return Result::StorageError;
			used[idx] = false;
			return Result::Ok;
		}
	};

} // end namespace crt
