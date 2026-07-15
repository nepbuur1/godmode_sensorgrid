// by Marius Versteegen, 2026

#pragma once
#include <Arduino.h>
#include "esp_timer.h"
#include "crt_SdCard.h"

namespace crt
{
	// Orchestrates the use case "prove that the microSD card works":
	// runs the test sequence once at startup and reports the verdict.
	class SdCardTest
	{
	private:
		static const size_t CHUNK_SIZE = 4096;
		static const size_t THROUGHPUT_CHUNK_COUNT = 64;             // 64 * 4kB = 256kB
		static const size_t THROUGHPUT_SIZE = CHUNK_SIZE * THROUGHPUT_CHUNK_COUNT;
		static const size_t MAX_DIR_ENTRIES = 32;
		static const unsigned long LED_FLASH_INTERVAL_MS = 500;

		static constexpr const char* SMALL_FILE = "/sdcard/hello.txt";
		static constexpr const char* LARGE_FILE = "/sdcard/speed.bin";
		static constexpr const char* SMALL_FILE_CONTENTS = "Hello simcard! - written by hello_simcard.";

		SdCard& sdCard;
		bool allTestsPassed;
		bool testsCompleted;
		uint8_t* chunkBuffer;        // scratch buffer, heap allocated to keep it off the stack
		unsigned long lastLedToggleMs;
		bool ledOn;

	public:
		SdCardTest(SdCard& sdCard) :
			sdCard(sdCard), allTestsPassed(false), testsCompleted(false),
			chunkBuffer(nullptr), lastLedToggleMs(0), ledOn(false)
		{
		}

		void init()
		{
			neopixelWrite(RGB_BUILTIN, 0, 0, 0);

			chunkBuffer = (uint8_t*)malloc(CHUNK_SIZE);
			if (chunkBuffer == nullptr)
			{
				ESP_LOGE("SdCardTest", "Could not allocate the %u byte scratch buffer", (unsigned)CHUNK_SIZE);
				finish(false);
				return;
			}
			for (size_t i = 0; i < CHUNK_SIZE; i++)
			{
				chunkBuffer[i] = (uint8_t)(i & 0xFF);
			}

			runAllTests();
		}

		void update()
		{
			// Green flashes: everything passed. Red flashes: something failed.
			unsigned long now = millis();
			if (now - lastLedToggleMs >= LED_FLASH_INTERVAL_MS)
			{
				lastLedToggleMs = now;
				ledOn = !ledOn;
				uint8_t brightness = ledOn ? 20 : 0;
				if (allTestsPassed)
				{
					neopixelWrite(RGB_BUILTIN, 0, brightness, 0);
				}
				else
				{
					neopixelWrite(RGB_BUILTIN, brightness, 0, 0);
				}
			}
		}

	private:
		void runAllTests()
		{
			ESP_LOGI("SdCardTest", "--- 1/5 mounting the card ---");
			probeWiring();
			if (!sdCard.mount())
			{
				finish(false);
				return;
			}
			ESP_LOGI("SdCardTest", "Mounted on %s", SdCard::MOUNT_POINT);

			bool passed = true;
			passed = reportProperties() && passed;
			passed = testSmallFileRoundTrip() && passed;
			passed = testThroughput() && passed;
			passed = listRootDirectory() && passed;

			ESP_LOGI("SdCardTest", "--- unmounting ---");
			sdCard.unmount();
			ESP_LOGI("SdCardTest", "Unmounted cleanly.");

			finish(passed);
		}

		// Separates "the module is not electrically there" from "the card is not answering".
		// The card is initialised at 400kHz whatever we ask for later, so a failure to mount
		// is never a matter of the clock speed - it is power or wiring.
		void probeWiring()
		{
			if (sdCard.probeMisoPulledHigh())
			{
				ESP_LOGI("SdCardTest", "MISO is held high -> the module has power and MISO is connected.");
			}
			else
			{
				ESP_LOGW("SdCardTest", "MISO reads low with our pull-down active.");
				ESP_LOGW("SdCardTest", "That suggests the module has no power (VCC must be 5V, it has an AMS1117),");
				ESP_LOGW("SdCardTest", "or that MISO is not connected to GPIO%d.", (int)sdCard.getMisoPin());
			}
		}

		bool reportProperties()
		{
			ESP_LOGI("SdCardTest", "--- 2/5 card properties ---");
			SdCard::Properties properties = sdCard.getProperties();
			ESP_LOGI("SdCardTest", "Name        : %.8s", properties.name);
			ESP_LOGI("SdCardTest", "Type        : %s", properties.isHighCapacity ? "SDHC/SDXC" : "SDSC");
			ESP_LOGI("SdCardTest", "Capacity    : %llu MB", properties.capacityBytes / (1024ULL * 1024ULL));
			ESP_LOGI("SdCardTest", "Sector size : %u bytes", (unsigned)properties.sectorSizeBytes);
			ESP_LOGI("SdCardTest", "Bus speed   : %d kHz (negotiated)", properties.busFreqKhz);
			return (properties.capacityBytes > 0);
		}

		bool testSmallFileRoundTrip()
		{
			ESP_LOGI("SdCardTest", "--- 3/5 write, read back and verify a small file ---");
			size_t length = strlen(SMALL_FILE_CONTENTS);

			if (!sdCard.writeFile(SMALL_FILE, (const uint8_t*)SMALL_FILE_CONTENTS, length))
			{
				return false;
			}
			ESP_LOGI("SdCardTest", "Wrote %u bytes to %s", (unsigned)length, SMALL_FILE);

			char readBuffer[128] = {};
			size_t bytesRead = sdCard.readFile(SMALL_FILE, (uint8_t*)readBuffer, sizeof(readBuffer) - 1);
			if (bytesRead != length)
			{
				ESP_LOGE("SdCardTest", "Read back %u bytes, expected %u", (unsigned)bytesRead, (unsigned)length);
				return false;
			}
			if (strcmp(readBuffer, SMALL_FILE_CONTENTS) != 0)
			{
				ESP_LOGE("SdCardTest", "Contents differ! read: '%s'", readBuffer);
				return false;
			}
			ESP_LOGI("SdCardTest", "Read back and verified: '%s'", readBuffer);
			return true;
		}

		bool testThroughput()
		{
			ESP_LOGI("SdCardTest", "--- 4/5 throughput of a %u kB file ---", (unsigned)(THROUGHPUT_SIZE / 1024));

			int64_t startUs = esp_timer_get_time();
			bool written = sdCard.writeFile(LARGE_FILE, chunkBuffer, CHUNK_SIZE, THROUGHPUT_CHUNK_COUNT);
			int64_t writeUs = esp_timer_get_time() - startUs;
			if (!written)
			{
				return false;
			}
			logThroughput("Write", THROUGHPUT_SIZE, writeUs);

			startUs = esp_timer_get_time();
			size_t bytesRead = sdCard.readEntireFile(LARGE_FILE, chunkBuffer, CHUNK_SIZE);
			int64_t readUs = esp_timer_get_time() - startUs;
			if (bytesRead != THROUGHPUT_SIZE)
			{
				ESP_LOGE("SdCardTest", "Read back %u bytes, expected %u", (unsigned)bytesRead, (unsigned)THROUGHPUT_SIZE);
				return false;
			}
			logThroughput("Read ", THROUGHPUT_SIZE, readUs);

			sdCard.removeFile(LARGE_FILE);   // don't leave a 256kB test file behind
			return true;
		}

		bool listRootDirectory()
		{
			ESP_LOGI("SdCardTest", "--- 5/5 contents of the root directory ---");

			// On the heap: MAX_DIR_ENTRIES entries outgrow the 3584 byte main task stack.
			SdCard::DirEntry* entries = (SdCard::DirEntry*)malloc(MAX_DIR_ENTRIES * sizeof(SdCard::DirEntry));
			if (entries == nullptr)
			{
				ESP_LOGE("SdCardTest", "Could not allocate room for %u directory entries", (unsigned)MAX_DIR_ENTRIES);
				return false;
			}

			size_t count = sdCard.readRootDirectory(entries, MAX_DIR_ENTRIES);
			for (size_t i = 0; i < count; i++)
			{
				if (entries[i].isDirectory)
				{
					ESP_LOGI("SdCardTest", "  [DIR ] %s", entries[i].name);
				}
				else
				{
					ESP_LOGI("SdCardTest", "  [FILE] %-32s %u bytes", entries[i].name, (unsigned)entries[i].sizeBytes);
				}
			}
			ESP_LOGI("SdCardTest", "%u entries.", (unsigned)count);

			free(entries);
			return true;
		}

		void logThroughput(const char* label, size_t bytes, int64_t elapsedUs)
		{
			if (elapsedUs <= 0)
			{
				return;
			}
			// kB/s = bytes / 1024 / (us / 1e6)
			unsigned kbPerSecond = (unsigned)(((uint64_t)bytes * 1000000ULL) / ((uint64_t)elapsedUs * 1024ULL));
			ESP_LOGI("SdCardTest", "%s: %u kB in %lld ms -> %u kB/s",
				label, (unsigned)(bytes / 1024), elapsedUs / 1000, kbPerSecond);
		}

		void finish(bool passed)
		{
			allTestsPassed = passed;
			testsCompleted = true;
			if (passed)
			{
				ESP_LOGI("SdCardTest", "=== ALL TESTS PASSED (LED flashes green) ===");
			}
			else
			{
				ESP_LOGE("SdCardTest", "=== TESTS FAILED (LED flashes red) ===");
			}
		}
	};
}
