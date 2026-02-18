# client_v1 Test Results

## Test Environment
- **Server**: server_v1 running on ESP32-S3 (ACM0), WiFi AP "SCOLIOSE"
- **Sensors**: sensor_v1 running on two ESP32-S3 devices (ACM1 with SENSOR_ID=1, ACM2 with SENSOR_ID=2)
- **Client**: client_v1 running on ESP32-S3 (ACM3)
- **Date**: 2026-02-18

## Test Results

| # | Test | Result | Details |
|---|------|--------|---------|
| 1 | GET / (dashboard) | PASS | HTML OK: title, heading, script, api reference present |
| 2 | GET /api/sensors (structure) | PASS | JSON structure OK: now, sensors[], id, seen, value fields present |
| 3 | Sensor data present | PASS | At least one sensor has seen:true |
| 4 | Sensor values updating | PASS | Sensor data changed between two polls (500ms apart) |
| 5 | Download button (CSV export) | PASS | Download button present, CSV generation JS verified (button, text/csv, headers, filename) |
| 6 | GET /nonexistent (404) | PASS | Correctly returned HTTP 404 |

## Summary
**6/6 tests passed, 0 failed**

**Note on test 5**: The download/CSV export is implemented as client-side JavaScript (the button builds a CSV blob in the browser and triggers a download). Since the ESP32 test client cannot execute JavaScript, the test validates that the HTML contains the download button element, the CSV MIME type, CSV column headers, and the expected filename — confirming the JS code is correctly served.

## Serial Output (excerpt)
```
I (412) main: === CLIENT TEST NODE v1 ===
I (413) ClientNode: Client test node starting...
I (554) ClientNode: Connecting to WiFi AP 'SCOLIOSE'...
I (3554) ClientNode: WiFi connected, IP: 192.168.4.3
I (3554) ClientNode: ========================================
I (3554) ClientNode: Starting server_v1 web endpoint tests...
I (3559) ClientNode: ========================================
I (3604) ClientNode: [PASS] GET / (dashboard): HTML OK: title, heading, script, api reference present
I (3641) ClientNode: [PASS] GET /api/sensors (structure): JSON structure OK: now, sensors[], id, seen, value fields present
I (3666) ClientNode: [PASS] Sensor data present: At least one sensor has seen:true
I (4214) ClientNode: [PASS] Sensor values updating: Sensor data changed between two polls (500ms apart)
I (4256) ClientNode: [PASS] Download button (CSV export): Download button present, CSV generation JS verified (button, text/csv, headers, filename)
I (4280) ClientNode: [PASS] GET /nonexistent (404): Correctly returned HTTP 404
I (4281) ClientNode: ========================================
I (4282) ClientNode: Test summary: 6/6 passed, 0 failed
I (4287) ClientNode: ========================================
```
