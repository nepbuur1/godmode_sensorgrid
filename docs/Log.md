# Log
## Intro
This log should be maintained during development.
For each Phase, a separate chapter should be added, which is to contain compact summaries of what was done to complete the phase.

## Phase 1

### Summary
Converted the reference ESP32 sensor grid project to ESP32-S3 using ESP-IDF 5.4.3 with the Arduino component.

### Created files
- **`apps/sensorgrid_common/crt_SensorPacket.h`** - shared data structure for ESP-NOW packets
- **`apps/collector_phase1/src/`** - collector app (WiFi AP + ESP-NOW receiver + WebServer)
  - `crt_CollectorNode.h` - main class: WiFi AP "SCOLIOSE", ESP-NOW receive callback, WebServer with /api/sensors JSON endpoint
  - `crt_IndexHtml.h` - embedded HTML dashboard (adapted from original, MAX_VALUE changed to 1023)
  - `collector_phase1_ino.h` / `collector_phase1.ino` - Arduino wrapper
- **`apps/sensor_v1/src/`** - sensor app (WiFi STA + ESP-NOW sender)
  - `crt_SensorNode.h` - main class: WiFi STA, ESP-NOW broadcast, sends incrementing simulated values (counter += 10*sensorId, mod 1024)
  - `sensor_v1_ino.h` / `sensor_v1.ino` - Arduino wrapper with configurable SENSOR_ID

### Modified files
- **`main/main.cpp`** - added NVS init (uncommented), added includes for collector_phase1.ino and sensor_v1.ino
- **`main/CMakeLists.txt`** - added include directories for sensorgrid_common, collector_phase1/src, sensor_v1/src

### Key design decisions
- HTML embedded as C++ raw string literal (avoids LittleFS image creation)
- ESP-NOW uses broadcast address (FF:FF:FF:FF:FF:FF) instead of hardcoded collector MAC
- No deep sleep - sensors send continuously every 100ms
- ESP_LOGI used for immediate serial monitor output

### Flash status
- Server flashed to /dev/ttyACM0
- Sensor 1 (SENSOR_ID=1) flashed to /dev/ttyACM1
- Sensor 2 (SENSOR_ID=2) flashed to /dev/ttyACM2
- All three devices verified working: server receives data from both sensors

### Refactoring: folder restructuring and collector-to-server rename

- Created parent app folder `apps/sensorgrid_v1/` per updated guidelines
- Moved `sensor_v1`, `sensorgrid_common`, and `collector_phase1` (renamed to `server_v1`) into it
- Renamed class `CollectorNode` to `ServerNode`, updated all file names and references
- Updated `main/CMakeLists.txt` include paths and `main/main.cpp` include references
- Added `doc/` folders with `img/`, `mermaid/` subfolders and `.md` documentation for both apps
- Created mermaid object models for sensor_v1 and server_v1

### Phase 1c: Test client and mermaid SVG generation

#### client_v1 app
- Created `apps/sensorgrid_v1/client_v1/` - automated test client for the server's web endpoints
- `crt_ClientNode.h` - connects to WiFi AP "SCOLIOSE" as STA, runs 5 HTTP tests against server_v1:
  1. GET / (dashboard) - validates HTML structure (title, heading, script, api reference)
  2. GET /api/sensors (structure) - validates JSON fields (now, sensors[], id, seen, value)
  3. Sensor data present - checks at least one sensor has seen:true
  4. Sensor values updating - polls twice with 500ms gap, verifies data changes
  5. GET /nonexistent (404) - verifies 404 response
- All 5 tests passed on first run (after fixing initial WiFiClient crash by making it a class member)
- Flashed to /dev/ttyACM3
- Added `doc/` with `client_v1.md`, `test.md` (with actual results), and mermaid object model

#### Download button test and SVG fixes
- Added test 6: "Download button (CSV export)" - validates button element, text/csv MIME type, CSV headers, and filename in served HTML
- Fixed mermaid syntax: changed `<<control>>\n` to `&lt;&lt;control&gt;&gt;<br/>` (HTML entities + br tag) for correct stereotype rendering
- Fixed SVG edge label backgrounds: post-process replaces gray `rgba(232,232,232, 0.8)` with white `rgba(255,255,255, 0.8)`
- Added call trees to all 3 app docs (sensor_v1.md, server_v1.md, client_v1.md)
- Added `neopixelWrite(RGB_BUILTIN, 0, 0, 0)` to all 3 apps to turn off onboard RGB LED at startup

#### Mermaid to SVG generation
- Created `tools/mermaid_to_svg.py` - converts .mmd files to .svg via the Kroki.io online service
- Uses HTTP POST to `https://kroki.io/mermaid/svg` (requires User-Agent header to avoid 403)
- Supports `--all` flag to convert all .mmd files found in sensorgrid_v1 app doc folders
- Generated SVG files for all 3 apps: sensor_v1, server_v1, client_v1
- No local tools needed to be installed (uses only Python stdlib `urllib`)

### Phase 1d: System-level documentation

- Created `apps/sensorgrid_v1/doc/` with `img/`, `mermaid/` subfolders
- Created `sensorgrid_v1.md` - system-level summary describing the role of each app and their interactions
- Created mermaid object model showing inter-app communication (ESP-NOW, WiFi, HTTP)
- Generated system-level SVG via `mermaid_to_svg.py`

## Phase 2

### Summary
Implemented sensorgrid_v2 with a polling-based ESP-NOW protocol. Unlike v1 where sensors broadcast freely, in v2 the server controls all communication: it discovers and registers sensors, then polls each one in round-robin order for data. This eliminates collision risk when using many sensors or large data packets.

### Phase 2a: Protocol design
- Designed DISCOVER/REGISTER/POLL/DATA protocol
- DISCOVER: server broadcasts to find sensors
- REGISTER: sensor responds with its ID
- POLL: server unicasts to a specific sensor requesting data
- DATA: sensor responds with current value (supports multi-packet via packetIndex/totalPackets)
- Recovery: 5 retries on POLL timeout, then mark unregistered and broadcast DISCOVER
- LED indicator: flashes red ~1Hz when any sensor is missing

### Phase 2b: Implementation

#### Created files
- **`apps/sensorgrid_v2/sensorgrid_common/crt_SensorGridPacket.h`** - packet definitions (MessageType enum, DiscoverPacket, RegisterPacket, PollPacket, DataPacket with 200-byte payload)
- **`apps/sensorgrid_v2/sensor_v2/src/`** - reactive sensor node
  - `crt_SensorNode.h` - responds to DISCOVER with REGISTER, to POLL with DATA. Auto-adds server MAC as peer from receive callback. Static instance pointer for ESP-NOW callbacks.
  - `sensor_v2_ino.h` / `sensor_v2.ino` - Arduino wrapper
- **`apps/sensorgrid_v2/server_v2/src/`** - polling server node
  - `crt_ServerNode.h` - state machine (DISCOVERING/POLLING/WAITING_DATA). Volatile flags for callback-to-update communication. LED flashing. Sensor recovery without blocking healthy sensors.
  - `crt_IndexHtml.h` - embedded HTML dashboard (identical to v1)
  - `server_v2_ino.h` / `server_v2.ino` - Arduino wrapper with EXPECTED_SENSOR_COUNT=2
- **`apps/sensorgrid_v2/client_v2/src/`** - automated test client
  - `crt_ClientNode.h` - identical tests to v1, updated log strings to "v2"
  - `client_v2_ino.h` / `client_v2.ino` - Arduino wrapper
- **Documentation** - docs, mermaid diagrams, and SVGs for all 3 apps + system level

#### Modified files
- **`main/main.cpp`** - added v2 includes (commented out)
- **`main/CMakeLists.txt`** - added v2 include paths
- **`tools/mermaid_to_svg.py`** - updated `--all` to scan all `sensorgrid_*` folders and system-level doc folders

#### Test results
- Server flashed to /dev/ttyACM0 - DISCOVER/REGISTER/POLL/DATA cycle working
- Sensor 1 (ID=1) flashed to /dev/ttyACM1 - responds to POLL with incrementing values
- Sensor 2 (ID=2) flashed to /dev/ttyACM2 - responds to POLL with incrementing values
- Client flashed to /dev/ttyACM3 - all 6/6 HTTP tests passed
- Dashboard verified working at http://192.168.4.1

## Phase 3

### Summary
Created sensorgrid_v3 by fully copying sensorgrid_v2 and renaming all sub-apps from v2 to v3. This provides a clean starting point for further v3 functionality.

### What was done
- Copied `apps/sensorgrid_v2/` to `apps/sensorgrid_v3/`
- Renamed directories: `sensor_v2/` → `sensor_v3/`, `server_v2/` → `server_v3/`, `client_v2/` → `client_v3/`
- Renamed all source files (`*_v2*` → `*_v3*`)
- Updated all includes, log strings, and doc references from "v2" to "v3"
- Renamed and updated all documentation and mermaid diagram files
- Regenerated SVGs: 12/12 successful (4 v1 + 4 v2 + 4 v3)
- Added v3 include paths to `main/CMakeLists.txt`
- Added v3 includes to `main/main.cpp` (commented out)

### Test results
- Server_v3 flashed to /dev/ttyACM0 - working
- Sensor_v3 (ID=1) flashed to /dev/ttyACM1 - responding to POLL
- Sensor_v3 (ID=2) flashed to /dev/ttyACM2 - responding to POLL
- Client_v3 flashed to /dev/ttyACM3 - all 6/6 HTTP tests passed (with both sensors active)

