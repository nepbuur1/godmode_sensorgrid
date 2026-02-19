# client_v4 Test Results

## Test Environment
- Server: server_v4 running on ACM0
- Sensors: sensor_v4 (ID=1) on ACM1, sensor_v4 (ID=2) on ACM2
- Client: client_v4 on ACM3
- Date: 2026-02-19

## Test Results

| # | Test | Result | Detail |
|---|------|--------|--------|
| 1 | GET / (dashboard) | PASS | HTML OK: title, heading, script, api reference, nav bar present |
| 2 | GET /grid (grid view) | PASS | Grid page OK: title, nav, grid container, measurements API reference present |
| 3 | GET /api/sensors (structure) | PASS | JSON structure OK: now, sensors[], id, seen, value fields present |
| 4 | GET /api/measurements/1 (sensor 1 measurements) | PASS | Measurements JSON OK: id, count, values[] present |
| 5 | Sensor data present | PASS | At least one sensor has seen:true |
| 6 | Sensor values updating | PASS | Sensor data changed between two polls (500ms apart) |
| 7 | Download button (CSV export) | PASS | Download button present, CSV generation JS verified (button, text/csv, headers, filename) |
| 8 | GET /nonexistent (404) | PASS | Correctly returned HTTP 404 |

**Summary: 8/8 passed, 0 failed**
