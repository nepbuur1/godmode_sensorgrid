# client_v3 Test Results

## Test Environment
- Server: server_v3 running on ACM0
- Sensors: sensor_v3 (ID=1) on ACM1, sensor_v3 (ID=2) on ACM2
- Client: client_v3 on ACM3
- Date: 2026-02-18

## Test Results

| # | Test | Result | Detail |
|---|------|--------|--------|
| 1 | GET / (dashboard) | PASS | HTML OK: title, heading, script, api reference present |
| 2 | GET /api/sensors (structure) | PASS | JSON structure OK: now, sensors[], id, seen, value fields present |
| 3 | Sensor data present | PASS | At least one sensor has seen:true |
| 4 | Sensor values updating | PASS | Sensor data changed between two polls (500ms apart) |
| 5 | Download button (CSV export) | PASS | Download button present, CSV generation JS verified (button, text/csv, headers, filename) |
| 6 | GET /nonexistent (404) | PASS | Correctly returned HTTP 404 |

**Summary: 6/6 passed, 0 failed**
