// by Marius Versteegen, 2026

#pragma once
#include <Arduino.h>
#include <dirent.h>
#include <sys/stat.h>
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "sd_protocol_defs.h"   // for SD_OCR_SDHC_CAP

namespace crt
{
	// Represents the microSD card that is connected via SPI.
	// Owns the SPI bus, the card handle and the FAT filesystem mount,
	// and offers file access on top of it.
	class SdCard
	{
	public:
		static constexpr const char* MOUNT_POINT = "/sdcard";

		struct Properties
		{
			char name[8];             // manufacturer's card name, from the CID register
			uint64_t capacityBytes;
			size_t sectorSizeBytes;
			int busFreqKhz;           // the speed the card and host actually agreed on
			bool isHighCapacity;      // SDHC/SDXC rather than plain SD
		};

		struct DirEntry
		{
			char name[64];
			size_t sizeBytes;
			bool isDirectory;
		};

	private:
		const gpio_num_t pinCs;
		const gpio_num_t pinMosi;
		const gpio_num_t pinMiso;
		const gpio_num_t pinSclk;
		const int requestedFreqKhz;

		sdmmc_card_t* card;
		bool busInitialized;

	public:
		SdCard(gpio_num_t pinCs, gpio_num_t pinMosi, gpio_num_t pinMiso, gpio_num_t pinSclk, int requestedFreqKhz) :
			pinCs(pinCs), pinMosi(pinMosi), pinMiso(pinMiso), pinSclk(pinSclk),
			requestedFreqKhz(requestedFreqKhz), card(nullptr), busInitialized(false)
		{
		}

		bool isMounted() const
		{
			return card != nullptr;
		}

		gpio_num_t getMisoPin() const
		{
			return pinMiso;
		}

		// Diagnostic, to be used before mounting.
		// A powered, connected module holds MISO high (the card's DAT0 idles high behind the
		// module's pull-up), which wins from our much weaker internal pull-down.
		// Reading low therefore points at missing power or a missing MISO connection,
		// rather than at anything the card is or isn't saying.
		bool probeMisoPulledHigh()
		{
			pinMode(pinMiso, INPUT_PULLDOWN);
			delay(2);
			bool isHigh = (digitalRead(pinMiso) == HIGH);
			pinMode(pinMiso, INPUT);
			return isHigh;
		}

		// Initialises the SPI bus (with DMA) and mounts the FAT filesystem.
		bool mount()
		{
			if (isMounted())
			{
				return true;
			}

			spi_bus_config_t busConfig = {};
			busConfig.mosi_io_num = pinMosi;
			busConfig.miso_io_num = pinMiso;
			busConfig.sclk_io_num = pinSclk;
			busConfig.quadwp_io_num = -1;
			busConfig.quadhd_io_num = -1;
			busConfig.max_transfer_sz = 4096;

			esp_err_t result = spi_bus_initialize(SPI2_HOST, &busConfig, SPI_DMA_CH_AUTO);
			if (result != ESP_OK)
			{
				ESP_LOGE("SdCard", "SPI bus init failed: %s", esp_err_to_name(result));
				return false;
			}
			busInitialized = true;

			sdmmc_host_t hostConfig = SDSPI_HOST_DEFAULT();
			hostConfig.slot = SPI2_HOST;
			hostConfig.max_freq_khz = requestedFreqKhz;

			sdspi_device_config_t deviceConfig = SDSPI_DEVICE_CONFIG_DEFAULT();
			deviceConfig.gpio_cs = pinCs;
			deviceConfig.host_id = SPI2_HOST;

			esp_vfs_fat_sdmmc_mount_config_t mountConfig = {};
			mountConfig.format_if_mount_failed = false;   // never silently wipe the user's card
			mountConfig.max_files = 5;
			mountConfig.allocation_unit_size = 16 * 1024;

			result = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &hostConfig, &deviceConfig, &mountConfig, &card);
			if (result != ESP_OK)
			{
				card = nullptr;
				if (result == ESP_FAIL)
				{
					ESP_LOGE("SdCard", "Mount failed: card responds, but no FAT filesystem was found.");
					ESP_LOGE("SdCard", "Is the card formatted as FAT32?");
				}
				else if (requestedFreqKhz > SDMMC_FREQ_DEFAULT)
				{
					// Above 20MHz the driver first switches the card into SDR25 high-speed mode.
					// Cards that do not survive that switch fail here while the bus is still at
					// its 400kHz probing speed - so this is not about signal integrity, and
					// suspecting the wiring would send you the wrong way.
					ESP_LOGE("SdCard", "Mount failed: %s", esp_err_to_name(result));
					ESP_LOGE("SdCard", "%d kHz was requested, which triggers the high-speed mode switch.", requestedFreqKhz);
					ESP_LOGE("SdCard", "Try SDMMC_FREQ_DEFAULT (20MHz) first: not every card accepts that switch.");
				}
				else
				{
					ESP_LOGE("SdCard", "Mount failed: %s", esp_err_to_name(result));
					ESP_LOGE("SdCard", "The card did not respond. Check the power (5V) and the wiring.");
				}
				releaseBus();
				return false;
			}
			return true;
		}

		void unmount()
		{
			if (isMounted())
			{
				esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
				card = nullptr;
			}
			releaseBus();
		}

		Properties getProperties() const
		{
			Properties properties = {};
			if (!isMounted())
			{
				return properties;
			}
			memcpy(properties.name, card->cid.name, sizeof(properties.name));
			properties.sectorSizeBytes = card->csd.sector_size;
			properties.capacityBytes = ((uint64_t)card->csd.capacity) * card->csd.sector_size;
			properties.busFreqKhz = card->max_freq_khz;
			properties.isHighCapacity = ((card->ocr & SD_OCR_SDHC_CAP) != 0);
			return properties;
		}

		// Writes a file consisting of 'repeatCount' consecutive copies of the given chunk.
		// Chunking keeps the RAM footprint independent of the resulting file size.
		bool writeFile(const char* path, const uint8_t* chunk, size_t chunkLength, size_t repeatCount = 1)
		{
			FILE* file = fopen(path, "wb");
			if (file == nullptr)
			{
				ESP_LOGE("SdCard", "Could not open %s for writing", path);
				return false;
			}
			bool succeeded = true;
			for (size_t i = 0; i < repeatCount; i++)
			{
				if (fwrite(chunk, 1, chunkLength, file) != chunkLength)
				{
					ESP_LOGE("SdCard", "Write to %s failed at chunk %u", path, (unsigned)i);
					succeeded = false;
					break;
				}
			}
			// fclose flushes; a write only really succeeded once it has done so.
			if (fclose(file) != 0)
			{
				ESP_LOGE("SdCard", "Could not close %s after writing", path);
				succeeded = false;
			}
			return succeeded;
		}

		// Reads at most bufferLength bytes into the buffer. Returns the number of bytes read.
		size_t readFile(const char* path, uint8_t* buffer, size_t bufferLength)
		{
			FILE* file = fopen(path, "rb");
			if (file == nullptr)
			{
				ESP_LOGE("SdCard", "Could not open %s for reading", path);
				return 0;
			}
			size_t bytesRead = fread(buffer, 1, bufferLength, file);
			fclose(file);
			return bytesRead;
		}

		// Streams the whole file through the given scratch buffer, discarding the contents.
		// Returns the total number of bytes read.
		size_t readEntireFile(const char* path, uint8_t* scratchBuffer, size_t scratchLength)
		{
			FILE* file = fopen(path, "rb");
			if (file == nullptr)
			{
				ESP_LOGE("SdCard", "Could not open %s for reading", path);
				return 0;
			}
			size_t totalBytesRead = 0;
			size_t bytesRead = 0;
			while ((bytesRead = fread(scratchBuffer, 1, scratchLength, file)) > 0)
			{
				totalBytesRead += bytesRead;
			}
			fclose(file);
			return totalBytesRead;
		}

		bool removeFile(const char* path)
		{
			return (unlink(path) == 0);
		}

		// Fills the given array with the entries of the root directory.
		// Returns the number of entries that were written.
		size_t readRootDirectory(DirEntry* entries, size_t maxEntries)
		{
			DIR* directory = opendir(MOUNT_POINT);
			if (directory == nullptr)
			{
				ESP_LOGE("SdCard", "Could not open %s", MOUNT_POINT);
				return 0;
			}
			size_t count = 0;
			struct dirent* entry = nullptr;
			while ((count < maxEntries) && ((entry = readdir(directory)) != nullptr))
			{
				// The precision keeps the (much longer) d_name within our own buffers.
				snprintf(entries[count].name, sizeof(entries[count].name), "%.*s",
					(int)(sizeof(entries[count].name) - 1), entry->d_name);
				entries[count].isDirectory = (entry->d_type == DT_DIR);

				// Build the path from the already-truncated name, so it always fits.
				char fullPath[128];
				snprintf(fullPath, sizeof(fullPath), "%s/%s", MOUNT_POINT, entries[count].name);
				struct stat entryStat;
				entries[count].sizeBytes = (stat(fullPath, &entryStat) == 0) ? (size_t)entryStat.st_size : 0;
				count++;
			}
			closedir(directory);
			return count;
		}

	private:
		void releaseBus()
		{
			if (busInitialized)
			{
				spi_bus_free(SPI2_HOST);
				busInitialized = false;
			}
		}
	};
}
