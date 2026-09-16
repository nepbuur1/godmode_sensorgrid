// by Marius Versteegen, 2025
// HX711 loadcell wrapper: detects and reads an HX711 on dedicated GPIO pins.
// Used alongside the measurement provider (stub or real) to optionally
// send loadcell data to the server.
//
// Pin assignment:
//   IO6 = power for HX711
//   IO9 = ground for HX711
//   IO7 = SCK
//   IO8 = DT / DOUT

#pragma once
#include <Arduino.h>
#include <HX711.h>

namespace crt
{
	class HX711Measurement
	{
	private:
		static const int POWER_PIN = 6;
		static const int GND_PIN   = 9;
		static const int SCK_PIN   = 7;
		static const int DOUT_PIN  = 8;

		HX711* scale;
		bool detected;
		int32_t lastRawValue;

	public:
		HX711Measurement()
			: scale(nullptr), detected(false), lastRawValue(0) {}

		~HX711Measurement()
		{
			if (scale) delete scale;
		}

		// Power the HX711 via GPIO and attempt detection.
		// Returns true if an HX711 was found.
		bool init()
		{
			// Power HX711 via GPIO before initializing
			::pinMode(POWER_PIN, OUTPUT);
			::digitalWrite(POWER_PIN, HIGH);

			::pinMode(GND_PIN, OUTPUT);
			::digitalWrite(GND_PIN, LOW);

			delay(150); // let HX711 power up

			// Construct HX711 (calls begin() which sets up pin modes)
			scale = new HX711(DOUT_PIN, SCK_PIN);

			detected = scale->wait_ready_timeout(500);
			if (detected)
			{
				// Do one dummy read to settle
				scale->read();
				ESP_LOGI("HX711Measurement", "HX711 detected on IO%d/IO%d", DOUT_PIN, SCK_PIN);
			}
			else
			{
				ESP_LOGI("HX711Measurement", "HX711 not detected");
			}
			return detected;
		}

		bool isDetected() const { return detected; }

		// Non-blocking read: only reads if HX711 has data ready.
		// Call this from the measurement cycle.
		void update()
		{
			if (!detected || !scale) return;

			if (scale->is_ready())
			{
				lastRawValue = scale->read();
			}
		}

		int32_t getRawValue() const { return lastRawValue; }

	}; // end class HX711Measurement

} // end namespace crt
