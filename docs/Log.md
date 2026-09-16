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

### Phase 3b: Multi-measurement sensor data

#### Summary
Changed sensors from sending a single value to sending 50 cached uint16_t measurements per POLL response. Added double buffering to handle POLL arriving during measurement (future I2C scenario). Added multi-packet send/reassembly to support payloads exceeding ESP-NOW's 250-byte frame limit.

#### Changes
- **`crt_SensorGridPacket.h`**: Added `MEASUREMENT_COUNT = 50`, increased `DATA_PAYLOAD_MAX_SIZE` from 200 to 245 (ESP-NOW max minus 5-byte header)
- **`crt_SensorNode.h`**: Double-buffered `measurements[2][50]` array. `update()` simulates 20ms I2C delay, fills write buffer, atomically swaps. `handlePoll()` sends from ready buffer with multi-packet chunking.
- **`crt_ServerNode.h`**: Multi-packet reassembly in `onDataRecv()` callback using `reassemblyBuffer[500]`. `SensorState` stores `measurements[50]` + `measurementCount`. JSON API uses `measurements[0]` for dashboard value.
- **`main/CMakeLists.txt`**: Reordered include paths (v3 first) to resolve v2/v3 `crt_SensorGridPacket.h` name clash
- Updated docs, mermaid diagrams, and regenerated SVGs (12/12)

#### Test results
- Server logs: `Sensor 1 -> 50 measurements, first=258`, `pkt 1/1 (100 bytes)` — correct single-packet delivery of 50 × uint16_t
- Both sensors active and values incrementing
- Client_v3: all 6/6 HTTP tests passed

## Phase 4

### Summary
Created sensorgrid_v4 by copying sensorgrid_v3 and renaming all sub-apps from v3 to v4. Then upgraded the web interface with a navigation bar and a new grid visualization page.

### Phase 4: Copy v3 → v4
- Copied `apps/sensorgrid_v3/` to `apps/sensorgrid_v4/`
- Renamed directories, source files, includes, log strings, and documentation from v3 to v4
- Added v4 include paths to `main/CMakeLists.txt` (listed first before v3)
- Added v4 includes to `main/main.cpp` (commented out)

### Phase 4a: Web interface upgrade

#### Changes
- **`crt_IndexHtml.h`**: Added navigation bar with links to Home (`/`) and Grid View (`/grid`)
- **`crt_GridHtml.h`** (new): Grid visualization page showing sensor 1's measurements as gray-scale rectangles (5 columns). Gray intensity proportional to value (0=white, 1023=black). Polls `/api/measurements/1` every 500ms.
- **`crt_ServerNode.h`**: Added `/grid` route serving GRID_HTML, added `/api/measurements/1` endpoint returning `{id, count, values[]}` JSON, added `#include "crt_GridHtml.h"`
- **`crt_ClientNode.h`**: Added `testGridPage()` and `testApiMeasurements()` tests, updated `testDashboardPage()` to check for nav bar. Now 8 tests total.
- Updated docs, mermaid diagrams, and regenerated SVGs (16/16: 4×v1 + 4×v2 + 4×v3 + 4×v4)

#### Test results
- Server_v4 flashed to /dev/ttyACM0 - working with nav bar and grid page
- Sensor_v4 (ID=1) flashed to /dev/ttyACM1 - responding to POLL
- Sensor_v4 (ID=2) flashed to /dev/ttyACM2 - responding to POLL
- Client_v4 flashed to /dev/ttyACM3 - all 8/8 HTTP tests passed

### Phase 4b: Grid view diamond layout

#### Changes
- **`crt_GridHtml.h`**: Changed rectangles to circles (`border-radius: 50%`). Changed rectangular grid layout to diamond pattern using flexbox rows. Row sizes computed as: ascending 1, 2, ..., W (where W = ceil(sqrt(N))), then descending W-1, W-2, ..., with the last row potentially partial. For 50 measurements: 1, 2, 3, 4, 5, 6, 7, 8, 7, 6, 1.
- Updated sensorgrid_v4.md grid view description

#### Test results
- All 4 devices re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4c: Histogram

#### Changes
- **`crt_GridHtml.h`**: Added a histogram below the diamond showing the distribution of measurement values across 50 bins (0-1023). Bars are proportional to max bin count, with axis labels below.
- **`crt_ClientNode.h`**: Added `hasHistogram` check to `testGridPage()` (checks for "histogram" in HTML)
- Updated sensorgrid_v4.md and test.md

#### Test results
- All 4 devices re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4d: Statistics table

#### Changes
- **`crt_GridHtml.h`**: Added a statistics table below the histogram with columns "max", "average", "sqrt(var)". Computed in JS on each poll: maximum value, arithmetic mean, and population standard deviation.
- **`crt_ClientNode.h`**: Added `hasStatsTable` check to `testGridPage()`
- Updated sensorgrid_v4.md and test.md

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4e: Normalize and Colorize buttons

#### Changes
- **`crt_GridHtml.h`**: Added two toggle buttons above the diamond:
  - **Normalize**: maps gray/color range to current min-max instead of 0-1023
  - **Colorize**: switches from gray-scale to color gradient (black → blue → green → yellow → red)
  - Both can be active simultaneously (full color range mapped to current data range)
  - Buttons toggle visually (dark background when active)
  - `recolor()` function allows instant re-rendering on toggle without waiting for next poll
- **`crt_ClientNode.h`**: Added `hasNormalize` and `hasColorize` checks to `testGridPage()`
- Updated sensorgrid_v4.md and test.md

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4f: Multi-sensor grid view (2×2 layout)

#### Changes
- **`crt_GridHtml.h`**: Complete rewrite for 2×2 sensor layout:
  - Four sensor widgets (Sensor 1-4), each with its own diamond grid, histogram, and statistics table
  - `.sensor-layout` CSS grid with `grid-template-columns: 1fr 1fr`
  - Reduced circle size from 48px to 30px, histogram height from 120px to 60px to fit side-by-side
  - JS refactored: per-sensor state objects, `SENSOR_IDS = [1, 2, 3, 4]`, `fetchAll()` fetches all 4 sensors in parallel via `Promise.all()`
  - Normalize and Colorize buttons apply to all four sensors simultaneously
- **`crt_ServerNode.h`**: Added `/api/measurements/2`, `/api/measurements/3`, `/api/measurements/4` routes
- **`crt_ClientNode.h`**: Updated `testGridPage()` to check for `Sensor 1`/`Sensor 4` headings and `sensor-layout` class instead of literal API URLs
- Updated sensorgrid_v4.md, server_v4.md, test.md, mermaid diagrams, regenerated 16/16 SVGs

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4g: Single-row layout and 64 measurements

#### Changes
- **`crt_SensorGridPacket.h`**: Changed `MEASUREMENT_COUNT` from 50 to 64 (128 bytes, still fits in single ESP-NOW frame)
- **`crt_GridHtml.h`**: Changed layout from 2×2 to single-row (4 columns) for landscape viewing:
  - `grid-template-columns: 1fr 1fr 1fr 1fr`, page max-width increased to 1200px
  - Circle size reduced from 30px to 22px, gap from 3px to 2px
  - Histogram height reduced from 60px to 40px
  - Widget padding and font sizes reduced to fit 4 side-by-side
- Updated sensorgrid_v4.md, server_v4.md, sensor_v4.md (50→64 measurements, 2×2→single-row)
- Updated mermaid diagrams (DataPacket label now shows 64×uint16_t), regenerated 16/16 SVGs

#### Test results
- All four devices re-flashed (server, sensor 1, sensor 2, client)
- Client_v4: all 8/8 HTTP tests passed

### Phase 4h: Hex-packed circle grid

#### Changes
- **`crt_GridHtml.h`**: Adjusted vertical spacing for hex packing — circle centers are equidistant in all 6 directions:
  - Removed vertical gap from `.grid-container` (set to 0)
  - Added `margin-top: -1px` on rows (first row excluded) to achieve the `sqrt(3)/2` ratio
  - With 22px circles and 2px horizontal gap: horizontal center-to-center = 24px, vertical center-to-center ≈ 21px (close to ideal 20.8px)
  - The diamond's centered rows with differing counts naturally provide the half-cell horizontal offset for hex packing
- Updated sensorgrid_v4.md (noted hex packing in grid view description)

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 8/8 HTTP tests passed

### Phase 4i: Single-endpoint measurement fetch

#### Changes
- **`crt_ServerNode.h`**: Added `handleApiAllMeasurements()` method and `/api/allmeasurements` route — returns all 4 sensors' measurement arrays in a single JSON response
- **`crt_GridHtml.h`**: Replaced 4 parallel `fetch("/api/measurements/" + id)` calls with a single `fetch("/api/allmeasurements")`. JS now parses the combined response and updates all 4 sensor widgets from it. This reduces the web server load from 4 sequential HTTP request/response cycles to 1 per update interval.
- **`crt_ClientNode.h`**: Added `testApiAllMeasurements()` test (checks for sensors[], id:1, id:2, count, values[]). Updated `testGridPage()` to verify `allmeasurements` reference in HTML. Test count increased from 8 to 9.
- Updated sensorgrid_v4.md (added `/api/allmeasurements` endpoint docs and JSON example), server_v4.md, test.md, mermaid diagrams, regenerated 16/16 SVGs
- Added Phase 4i to Instructions document

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 9/9 HTTP tests passed

### Phase 4j: Faster polling (100ms cycle, setTimeout-based loop)

#### Changes
- **FreeRTOS tick rate**: Already at 1000Hz (`CONFIG_FREERTOS_HZ=1000` in sdkconfig), so `vTaskDelay(1)` = 1ms. No change needed.
- **`crt_GridHtml.h`**: Lowered `POLL_MS` from 200 to 100. Replaced `setInterval(fetchAll, POLL_MS)` with a `setTimeout`-based `pollLoop()` that measures `fetchAll()` duration and waits only the remaining time to reach 100ms total cycle. This prevents request pileup if a response occasionally takes longer than 100ms.
- Updated sensorgrid_v4.md (grid view polling interval updated to 100ms)
- Added Phase 4j to Instructions document

#### Test results
- Server and client re-flashed and tested
- Client_v4: all 9/9 HTTP tests passed

## Phase 5

### Summary
Created sensorgrid_v5 by copying sensorgrid_v4 and renaming all sub-apps from v4 to v5. The system now uses 5 devices (up from 4) with 3 physical sensors (up from 2). Device assignments changed: server moved to ACM3, client to ACM2, and a third sensor connects via a CP210x USB-to-UART bridge on /dev/ttyUSB0.

### What was done
- Copied `apps/sensorgrid_v4/` to `apps/sensorgrid_v5/`
- Renamed directories: `sensor_v4/` → `sensor_v5/`, `server_v4/` → `server_v5/`, `client_v4/` → `client_v5/`
- Renamed all source files (`*_v4*` → `*_v5*`)
- Updated all includes, log strings, and doc references from "v4" to "v5"
- Changed `EXPECTED_SENSOR_COUNT` from 2 to 3 in `server_v5_ino.h`
- Added v5 include paths to `main/CMakeLists.txt` (listed first before v4)
- Added v5 includes to `main/main.cpp` (commented out)
- Updated documentation with new device assignments (5 devices, 3 sensors)
- Updated system-level mermaid diagram (sensor count x2 → x3)
- Regenerated SVGs: 20/20 successful (4×v1 + 4×v2 + 4×v3 + 4×v4 + 4×v5)

### Device assignments
| Port | USB Device | Role |
|------|-----------|------|
| /dev/ttyACM3 | Espressif USB JTAG (Bus Dev 006) | server_v5 |
| /dev/ttyACM2 | Espressif USB JTAG (Bus Dev 005) | client_v5 |
| /dev/ttyACM1 | Espressif USB JTAG (Bus Dev 004) | sensor_v5 (ID=1) |
| /dev/ttyACM0 | Espressif USB JTAG (Bus Dev 003) | sensor_v5 (ID=2) |
| /dev/ttyUSB0 | CP210x UART Bridge (Bus Dev 002) | sensor_v5 (ID=3) |

### Test results
- Server_v5 flashed to /dev/ttyACM3 — all 3 sensors discovered, registered, and polled successfully
- Sensor_v5 (ID=1) flashed to /dev/ttyACM1 — responding to POLL with 64 measurements
- Sensor_v5 (ID=2) flashed to /dev/ttyACM0 — responding to POLL with 64 measurements
- Sensor_v5 (ID=3) flashed to /dev/ttyUSB0 — responding to POLL with 64 measurements
- Client_v5 flashed to /dev/ttyACM2 — all 9/9 HTTP tests passed

### Phase 5a: Rectangular hex-packed circle grid

#### Changes
- **`crt_GridHtml.h`**: Changed circle grid layout from diamond pattern to rectangular hex-packed grid:
  - `computeRowSizes()` now returns rows of 8 circles (configurable via `COLS` constant) instead of the diamond 1, 2, ..., W, ..., 2, 1 pattern
  - Odd rows are offset by 12px (`marginLeft`) to create hex packing — circle centers remain equidistant in all 6 directions
  - `.grid-container` changed from `align-items: center` to `align-items: flex-start` with `width: fit-content` for proper left-aligned layout
  - `.row` removed `justify-content: center` since all rows have equal width
  - For 64 measurements: 8 rows of 8 circles each
- Updated sensorgrid_v5.md (grid view description updated from diamond to rectangular hex-packed)

#### Test results
- Server_v5 re-flashed to /dev/ttyACM3
- Client_v5 re-flashed to /dev/ttyACM2 — all 9/9 HTTP tests passed

### Phase 5b: Real MCP23017 sensor measurements

#### Summary
Added real hardware measurement support using MCP23017 GPIO expander, ADG706 column multiplexer, and 8x ADS1220 24-bit ADCs. When an MCP23017 is detected on I2C at startup, the sensor reads a physical 8×16 pressure sensor grid. If not detected, stub mode continues as before with simulated data. Measurement count increased from 64 to 128 (8×16 grid), requiring 2 ESP-NOW packets per sensor. MAX_VALUE updated from 1023 to 65535 for full 16-bit ADC range.

#### Created files (in `apps/sensorgrid_v5/sensor_v5/src/`)
- **`crt_IMeasurementProvider.h`** — abstract interface with `init()` and `measure(uint16_t* buffer)` methods
- **`crt_StubMeasurement.h`** — stub provider: generates 128 simulated values with 20ms delay, extracted from previous inline SensorNode code
- **`crt_MCP23017.h`** — header-only MCP23017 I2C GPIO expander driver with `McpPin` enum, register constants, and static `detect()` method for probing
- **`crt_ADG706.h`** — header-only ADG706 16-channel analog mux driver, controlled via MCP23017 pins
- **`crt_ADS1220.h`** — header-only ADS1220 24-bit ADC driver with turbo mode options table and `rawToUint16()` conversion
- **`crt_RealMeasurement.h`** — real measurement provider: full 16-column scan loop with batch ADC operations, EMA filter (factor 0.5), and row/column transposition for server display

#### Modified files
- **`crt_SensorGridPacket.h`** — `MEASUREMENT_COUNT`: 64 → 128 (256 bytes, 2 ESP-NOW packets)
- **`crt_SensorNode.h`** — refactored with `IMeasurementProvider` pattern: probes MCP23017 at init, selects real or stub provider, delegates `measure()` calls. Fixed deprecated `neopixelWrite` → `rgbLedWrite`.
- **`crt_GridHtml.h`** — `MAX_VALUE`: 1023 → 65535, histogram axis labels: 0/32768/65535
- **`crt_IndexHtml.h`** — `MAX_VALUE`: 1023 → 65535

#### Key design decisions
- Both `StubMeasurement` and `RealMeasurement` are always constructed as members (~200 bytes extra RAM), avoiding heap allocation. Only the active provider's `init()` is called.
- Data transposition in `measure()`: `buffer[col*8 + row] = sensorValues[row][col]` — server grid row = physical column, resulting in 16 rows × 8 columns display.
- Hardware pin assignments from ScoliosePCB2: I2C SDA=4, SCL=5; SPI SCLK=12, MISO=13, MOSI=11, CS=10; MCP reset=14, addr=0x24

#### Test results
- Server_v5 re-flashed to /dev/ttyACM3 — all 3 sensors discovered and polled
- Sensor_v5 (ID=1) flashed to /dev/ttyACM1 — stub mode, 128 measurements in 2 packets
- Sensor_v5 (ID=2) flashed to /dev/ttyACM0 — stub mode, 128 measurements in 2 packets
- Sensor_v5 (ID=3) flashed to /dev/ttyUSB0 — stub mode, 128 measurements in 2 packets
- Client_v5 flashed to /dev/ttyACM2 — all 9/9 HTTP tests passed
- Server serial confirmed: `pkt 1/2 (245 bytes)`, `pkt 2/2 (11 bytes)`, `128 measurements` per sensor

### Phase 5c: Remove-offset button + inverted sensor sequence

#### Summary
Added a "Remove offset" button to each sensor panel in the grid view. When pressed, current measurement values are captured as baseline offsets; subsequent readings display the difference (value minus offset), clamped to zero. Also inverted the sensor value output order in both RealMeasurement and StubMeasurement to mirror the grid display in both X and Y, matching the physical pressure-sensor geometry.

#### Modified files
- **`crt_GridHtml.h`** — added per-sensor `offsets` state, "Remove offset" button in each sensor widget, `removeOffset()` JS handler, offset subtraction in `updateSensor()` and `colorCells()`
- **`crt_RealMeasurement.h`** — reversed transpose loop order (col high→low, row high→low) to invert value sequence
- **`crt_StubMeasurement.h`** — reversed buffer fill order for consistency with real mode

#### Test results
- All 5 devices re-flashed, client_v5 9/9 HTTP tests passed

### Phase 5d: Max 2500 normalization button + stub value range

#### Summary
Added a "Max 2500" toggle button to the grid view, next to Normalize and Colorize. When active, the color scale normalizes to a max value of 2500 (matching the sensor output range). Max 2500 and Normalize are mutually exclusive — toggling one turns the other off. Stub measurement values now range 0–2500 instead of 0–65535.

#### Modified files
- **`crt_GridHtml.h`** — added "Max 2500" button, `max2500` state, `toggleMax2500()` with mutual exclusion against Normalize, updated `colorForValue()` to use 2500 as hi when `max2500` is active
- **`crt_StubMeasurement.h`** — counter wraps at 2501, per-index values computed as `(counter + i*19) % 2501` keeping all values in 0–2500 range

#### Test results
- All 5 devices re-flashed, client_v5 9/9 HTTP tests passed

### Phase 5e: Capture, calibration, and value normalization modes

#### Summary
Major overhaul of the grid view control panel. Renamed buttons ("Normalize" → "Norm Display", "Max 2500" → "MaxFixed Display", "Colorize" → "Color Display"). Made the fixed max value configurable via a "Fixed Max Display" input field (default 2500). Added a "Capture" button that identifies the sensor grid with the highest individual measurement, storing its max value (`maxCaptured`) and sum of all values (`maxSumCaptured`). Added "Calibrate Grams" input field (default 1000). Added two new value normalization modes: "Norm MaxCap" transforms displayed values to `calibrateGrams * (value - offset) / maxCaptured`, and "Norm SumCap" transforms to `calibrateGrams / maxSumCaptured * (value - offset) / maxCaptured`. These value normalization modes are separate from the display normalization (Norm Display, MaxFixed Display, Color Display) which controls color/gray mapping.

#### Modified files
- **`crt_GridHtml.h`** — renamed buttons, restructured controls into buttons + fields layout, added Capture/NormMaxCap/NormSumCap buttons, added maxCaptured/maxSumCaptured/calibrateGrams/fixedMax input fields, added `getDisplayValues()` function for value normalization pipeline, updated `colorForValue()` to use configurable fixedMax
- **`crt_ClientNode.h`** — updated test assertions from "Normalize"/"Colorize" to "Norm Display"/"Color Display"

#### Test results
- All 5 devices re-flashed, client_v5 9/9 HTTP tests passed

### Phase 5i: HX711 loadcell support

#### Summary
Added optional HX711 loadcell support to each sensor device. On startup, each sensor probes for an HX711 on IO6 (power), IO9 (ground), IO7 (SCK), IO8 (DOUT). If detected, raw loadcell readings are appended to the measurement data sent to the server via a 5-byte `LoadcellAppendix` (1 byte flag + 4 bytes int32_t raw value). The server parses this appendix, stores it per sensor, and exposes `hasLoadcell` and `loadcellRaw` in the `/api/allmeasurements` JSON endpoint.

The Grid View web page shows a per-sensor "Loadcell" section (hidden unless HX711 is detected) with:
- Current weight in grams
- Tare button (stores current raw value as zero offset)
- Known weight input field + Calibrate button (computes scale factor from known weight)
- Calibration parameters (tare offset, scale factor) stored in browser cookies for persistence

Calibration is done entirely in JavaScript using raw values from the sensor. Formula: `weight = (raw - tareOffset) / scaleFactor`.

#### New files
- **`sensor_v5/src/crt_HX711Measurement.h`** — HX711 wrapper class: powers HX711 via GPIO, detects presence via `wait_ready_timeout()`, non-blocking reads via `is_ready()` + `read()`

#### Modified files
- **`sensorgrid_common/crt_SensorGridPacket.h`** — added `LoadcellAppendix` packed struct
- **`sensor_v5/src/crt_SensorNode.h`** — integrated HX711Measurement: init/detect in `init()`, read in `update()`, append loadcell data in `handlePoll()`
- **`server_v5/src/crt_ServerNode.h`** — added `hasLoadcell`/`loadcellRaw` to `SensorState`, parse `LoadcellAppendix` from reassembly buffer, include in `/api/allmeasurements` JSON
- **`server_v5/src/crt_GridHtml.h`** — added loadcell CSS, HTML section per sensor panel (weight display, tare, calibrate), JavaScript for calibration/cookie storage
- **`client_v5/src/crt_ClientNode.h`** — added test assertions for `hasLoadcell`/`loadcellRaw` in API and loadcell UI elements in grid page
- **`main/main.cpp`** — added `crt::criticalSectionMutex` (required by HX711 lib's `TaskCriticalSection`)
- **`server_v5/src/server_v5_ino.h`** — temporarily changed `EXPECTED_SENSOR_COUNT` to 2 (only 2 ESP32-S3 sensors available this session)

#### Test results
- Server (ACM0), sensor id=1 (ACM2), sensor id=2 (ACM3), client (ACM1) flashed and running
- Server logs confirm: `lc=1 raw=0` parsed correctly for both sensors (stub sensors, no physical HX711 attached)
- Client_v5 9/9 HTTP tests passed, including new loadcell field verification

### Phase 5f: Sum statistic and filtered stats (EMA)

#### Summary
Added "sum" column to the statistics table (sum of all circle values). All stats (sum, max, average, sqrt(var)) are now updated using an Exponential Moving Average (EMA) filter: `displayed = displayed * filterFactor + (1 - filterFactor) * new`. Default filter factor is 0.9. Added editable "Stats Filter" text field below the other control fields.

#### Modified files
- **`crt_GridHtml.h`** — added `filteredSum/filteredMax/filteredAvg/filteredStd` per-sensor state, `getStatsFilter()` function, EMA update in `updateStats()`, "Stats Filter" input field

### Phase 5g: Running plot

#### Summary
Added a per-sensor running plot below the stats table. Plots the 3 largest circle values over the 20 most recent time slots (red = largest, green = 2nd, blue = 3rd). Vertical axis corresponds to the current display range (e.g. Fixed Max Display value when MaxFixed is active).

#### Modified files
- **`crt_GridHtml.h`** — added `<canvas>` running-plot elements, `plotHistory[]` per-sensor state, `updatePlot()` function, `getPlotMax()` helper

### Phase 5h: 3D surface view (WebGL)

#### Summary
Added a per-panel "3D Surface" toggle button that switches between the 2D circle grid and a 3D height-mapped mesh using custom WebGL (no external libraries). Vertex heights correspond to circle values, vertex colors use the same color/gray mapping as 2D view. Lighting with diffuse + specular shading. Histogram, stats, and running plot remain visible in both modes.

#### Modified files
- **`crt_GridHtml.h`** — added WebGL shaders (GL_VS, GL_FS), matrix math functions (m4Mul, m4Pers, m4LookAt), `initGL()`, `renderSurface()`, `toggle3D()`, per-sensor GL state, surface canvas elements

### Phase 5j: avResidualNoiseSum and negative circle values

#### Summary
Added "avResidualNoiseSum" calculation — captures average grid-sum noise over 1 second after removing offset. Updated Norm SumCap formula to use avResidualNoiseSum: `calibrateGrams / (maxSumCaptured - avResidualNoiseSum) * (value - offset)`. Circle values now support negative numbers (after offset subtraction).

#### Modified files
- **`crt_GridHtml.h`** — added residual noise collection state, 1-second sampling after `removeOffset()`, avResidualNoiseSum field, updated `getDisplayValues()` formula

### Phase 5k: Cookie persistence for grid view settings

#### Summary
Added browser cookie persistence for all grid view settings. On page load, all toggle button states, field values, and calibration parameters are restored from cookies.

#### Modified files
- **`crt_GridHtml.h`** — added `setCookie()`/`getCookie()` functions, `restoreSettings()` on page load, `onchange` handlers saving to cookies for all input fields, `saveToggleStates()` for button states

### Phase 5l: Per-circle value filter

#### Summary
Added per-circle EMA filtering with a separate filter constant. Added "Value Filter" text field (default 0.5) below "Stats Filter". Each circle value is smoothed independently: `filteredValues[i] = filteredValues[i] * vf + displayValues[i] * (1 - vf)`. Setting persisted in cookies.

#### Modified files
- **`crt_GridHtml.h`** — added `filteredValues` per-sensor state, `getValueFilter()` function, per-circle EMA in `updateSensor()`, "Value Filter" input field with cookie persistence

### Phase 5m: Removed Norm MaxCap, renamed Capture to Capture Sum

#### Summary
Removed the "Norm MaxCap" button and "maxCaptured" field from the grid view (no longer needed). Renamed "Capture" button to "Capture Sum". Cleaned up all related JS state and cookie handling.

#### Modified files
- **`crt_GridHtml.h`** — removed `normMaxCap` state, `toggleNormMaxCap()`, `maxCaptured` variable, associated cookie logic, simplified `doCapture()` to only track sum

### Phase 5n: Selected sensor info panel

#### Summary
Restructured the grid view: maxSumCaptured, avResidualNoiseSum, and Calibrate Grams fields moved from the main control panel into a "selected sensor info panel" (green border, hidden until a sensor panel is clicked). Clicking a sensor widget selects it (green border). These values are now per-sensor with per-sensor cookie persistence. Capture Sum operates on the selected sensor only.

#### Modified files
- **`crt_GridHtml.h`** — added `.selected-info-panel` CSS, `selectedSensorId` state, `selectSensor()`, `onCalGramsChange()`, per-sensor `maxSumCaptured`/`avResidualNoiseSum`/`calGrams`, updated `doCapture()` and `getDisplayValues()` for per-sensor state

### Phase 5o: avResidualNoise only on Remove Offset

#### Summary
Changed avResidualNoiseSum to only be updated when "Remove offset" is pressed, not when "Capture Sum" is pressed.

#### Modified files
- **`crt_GridHtml.h`** — removed residual noise collection from `doCapture()`

### Phase 5p: Norm IndivCap and per-circle capture tuples

#### Summary
Added "Norm IndivCap" toggle button (mutually exclusive with "Norm SumCap"). Renamed "Calibrate Grams" to "Calibrate Sum Grams". Each circle can store 3 capture tuples (captureValue, calibratedIndivGrams). Clicking a circle selects it (blue border), revealing a red-bordered "selected circle info panel" with 3 rows for capturing individual calibration points. Circles with all 3 tuples captured get a black border. Added "Reset Indiv Caps" button to reset all tuples for the selected sensor.

#### Modified files
- **`crt_GridHtml.h`** — added `.cell-selected`/`.cell-captured` CSS, `.circle-info-panel` UI, `indivCaps[]` per-sensor state, `selectCircle()`, `captureIndiv()`, `resetIndivCaps()`, `ensureIndivCaps()`, `updateCircleBorders()`, circle click handlers in `createGrid()`

### Phase 5q: localStorage persistence for indivCaps

#### Summary
Switched indivCaps storage from cookies to localStorage. With 128 circles × 3 tuples, the JSON data (~12KB) exceeded the 4KB cookie size limit. localStorage supports 5MB+.

#### Modified files
- **`crt_GridHtml.h`** — changed `setCookie`/`getCookie` calls for indivCaps to `localStorage.setItem`/`getItem`, fixed `ensureIndivCaps()` to preserve existing data when resizing and skip count=0

### Phase 5r: Remove Offset moved to info panel

#### Summary
Removed per-sensor "Remove offset" buttons from each sensor widget. Added a single "Remove Offset" button in the selected sensor info panel that operates on the currently selected sensor.

#### Modified files
- **`crt_GridHtml.h`** — removed 4 per-sensor offset buttons, added `doRemoveOffset()` function and "Remove Offset" button in selected info panel

### Phase 5s: Piecewise linear interpolation for Norm IndivCap

#### Summary
Implemented the Norm IndivCap display mode. Each circle's 3 capture tuples define a piecewise linear lookup curve from (0,0) through the captured points. Raw offset-compensated values are mapped through this curve to produce calibrated display values. Linear extrapolation is used beyond the last captured point.

#### Modified files
- **`crt_GridHtml.h`** — added `indivLookup()` function (piecewise linear interpolation/extrapolation), updated `getDisplayValues()` to use it when `normIndivCap` is active

### Phase 5t: Zero value for uncaptured circles in Norm IndivCap

#### Summary
In Norm IndivCap mode, circles with all tuples at zero now display 0 instead of the raw input value.

#### Modified files
- **`crt_GridHtml.h`** — changed `indivLookup()` to return 0 when no tuples are set

## Phase 6

### Summary
Created sensorgrid_v6 by fully copying sensorgrid_v5 and renaming all sub-apps from v5 to v6 (server_v6, sensor_v6, client_v6). No code changes, providing a clean baseline for v6 features.

### What was done
- Copied `apps/sensorgrid_v5/` to `apps/sensorgrid_v6/`
- Renamed directories, source files, includes, log strings, and documentation from v5 to v6
- Added v6 include paths to `main/CMakeLists.txt`
- Added v6 includes to `main/main.cpp`

### Phase 6a: Dynamic sensor discovery (time-based)

#### Summary
Changed the server from waiting for a fixed number of sensors to using a 10-second discovery window. After the window, the server transitions to operational mode if at least one sensor has registered. Green LED flashes during discovery window; red after window expires with no sensors. Old fixed-count behavior preserved via `USE_TIMED_DISCOVERY` define.

#### Modified files
- **`server_v6_ino.h`** — added `USE_TIMED_DISCOVERY` define, `DISCOVER_WINDOW_MS` constant
- **`crt_ServerNode.h`** — added `initMs` member, time-based discovery logic, conditional green/red LED flashing

### Phase 6b: Snapshot upload

#### Summary
Added an Upload button to the Snapshots panel, allowing previously downloaded snapshot files to be uploaded and replayed.

#### Modified files
- **`crt_GridHtml.h`** — added `btnSnapUpload` button and `snapFileInput` file input element
- **`crt_GridSnapshot.h`** (later split) — added `snapUpload(event)` function to parse and load JSON snapshot files

### Phase 6c: Record upload

#### Summary
Added an Upload button to the Record panel, allowing previously downloaded recording files to be uploaded and played back.

#### Modified files
- **`crt_GridHtml.h`** — added `btnRecUpload` button and `recFileInput` file input element
- **`crt_GridRecPlay.h`** (later split) — added `recUpload(event)` function to parse and load JSON recording files

### Phase 6d: Snapshot slider

#### Summary
Added a horizontal slider to the Snapshots panel for quick navigation between snapshots. The slider is only visible when more than 3 snapshots are present.

#### Modified files
- **`crt_GridHtml.h`** — added `snapSliderRow` element (initially hidden)
- **`crt_GridSnapshot.h`** (later split) — added `snapSliderChange(val)` function, slider sync with snapshot index

### Phase 6e: Play pause button and button group spacing

#### Summary
Added a Pause button for playback in the Record panel (separate from the recording pause). Added spacing between button groups to clarify grouping: Record/Pause/Stop | Play/Pause | Download/Upload.

#### Modified files
- **`crt_GridHtml.h`** — added `btnPlayPause` button, margin-left spacing between button groups
- **`crt_GridRecPlay.h`** (later split) — added `recPlayPause()` function, "playpaused" state handling
- **`crt_GridCss.h`** (later split) — added `.pause-active` CSS class (orange background)

### Phase 6f: Record slider

#### Summary
Added a horizontal slider to the Record panel, visible during playback. The slider syncs bidirectionally: dragging it sets the playback position, and during playback the slider automatically updates to reflect the current frame.

#### Modified files
- **`crt_GridHtml.h`** — added `recSliderRow` element (initially hidden)
- **`crt_GridRecPlay.h`** (later split) — added `recSliderChange(val)` function, slider range/position sync

### Phase 6g: CSS separation

#### Summary
Moved all CSS from inline `<style>` in the grid HTML to a separate `crt_GridCss.h` file, served via `/grid.css`.

#### Created files
- **`crt_GridCss.h`** — all CSS rules for the grid page

#### Modified files
- **`crt_GridHtml.h`** — removed inline CSS, added `<link>` to `/grid.css`
- **`crt_ServerNode.h`** — added `/grid.css` HTTP route

### Phase 6h: 3D surface separation

#### Summary
Moved the WebGL 3D surface rendering code to a separate `crt_Grid3D.h` file, served via `/grid3d.js`.

#### Created files
- **`crt_Grid3D.h`** — WebGL shaders, matrix math, surface rendering, 3D toggle function

#### Modified files
- **`crt_GridHtml.h`** — removed inline 3D code, added `<script src="/grid3d.js">`
- **`crt_ServerNode.h`** — added `/grid3d.js` HTTP route

### Phase 6i: JavaScript module separation

#### Summary
Split the remaining JavaScript from the monolithic grid HTML into six separate files, each served as a standalone `.js` resource. This significantly reduces the main HTML payload and improves maintainability.

#### Created files
- **`crt_GridCircle.h`** — grid/circle rendering (`createGrid`, `colorForValue`, `getDisplayValues`, `indivLookup`, etc.), served via `/gridcircle.js`
- **`crt_GridRecPlay.h`** — record/play functions (`recRecord`, `recPause`, `recStop`, `recPlay`, `recPlayPause`, `recDownload`, `recUpload`, `recSliderChange`), served via `/gridrecplay.js`
- **`crt_GridSnapshot.h`** — snapshot functions (`snapTake`, `snapClear`, `snapDownload`, `snapUpload`, `snapReplayStart`, `snapStepBack/Forward`, `snapSliderChange`), served via `/gridsnapshot.js`
- **`crt_GridLoadcell.h`** — loadcell support (`lcTare`, `lcCalibrate`, `updateLoadcell`), served via `/gridloadcell.js`
- **`crt_GridHistogram.h`** — histogram drawing and statistics (`createHistogram`, `updateHistogram`, `updateStats`), served via `/gridhistogram.js`
- **`crt_GridPlot.h`** — running plot of top 3 values (`updatePlot`, `getPlotMax`), served via `/gridplot.js`

#### Modified files
- **`crt_GridHtml.h`** — removed all extracted JS, added `<script>` tags for each new file
- **`crt_ServerNode.h`** — added 6 new HTTP routes

### Phase 6j: Capture error messages when normalization active

#### Summary
When Capture Sum or Capture Indiv buttons are pressed while Norm SumCap or Norm IndivCap is active, the capture is blocked and an error message is shown: "Please first disable NormSumCap mode and wait a few seconds." The error disappears once the normalization mode is disabled.

#### Modified files
- **`crt_GridHtml.h`** — added `captureError` and `captureIndivError` span elements, error styling (red, bold), `clearCaptureErrors()` function, guard logic in `doCapture()` and `captureIndiv()`

### Phase 6k: Button spacing

#### Summary
Added additional spacing between button groups for clearer visual grouping: between Color Display and Norm SumCap, between Stop and Play, between Pause and Download.

#### Modified files
- **`crt_GridHtml.h`** — added `margin-left: 1rem` to Norm SumCap, Play, and Download buttons

### Phase 6l: Remove average and sqrt(var) from statistics

#### Summary
Removed the "average" and "sqrt(var)" columns from the statistics tables, along with all related calculations. Only "sum" and "max" remain.

#### Modified files
- **`crt_GridHtml.h`** — removed `avg` and `std` table headers and cells from all 4 sensor stats tables
- **`crt_GridHistogram.h`** — simplified `updateStats()` to only compute sum and max (removed mean, variance, and standard deviation calculations)

### Flash status
- Server_v6 flashed to /dev/ttyACM0

## Phase 7

### Summary
Created sensorgrid_v7 by fully copying sensorgrid_v6 and renaming all sub-apps from v6 to v7 (server_v7, sensor_v7, client_v7). No code changes, providing a clean baseline for v7 features. As the code is functionally identical, no reprogramming of the devices was needed.

### What was done
- Copied `apps/sensorgrid_v6/` to `apps/sensorgrid_v7/`
- Renamed directories, source files, includes, log strings, and documentation from v6 to v7 (including the `doc/mermaid/*.mmd` and `doc/img/*.svg` object model files)
- Added v7 include paths to `main/CMakeLists.txt`, listed above v6 so the latest `crt_SensorGridPacket.h` is picked up
- Added v7 includes to `main/main.cpp`, with `server_v7.ino` active (the v6 includes were commented out but retained)

### Verification
- Built successfully with each of `server_v7.ino`, `sensor_v7.ino` and `client_v7.ino` active in turn; `server_v7.ino` left as the active app
- No devices were connected, so no on-target test was run. Not needed for this phase: v7 is byte-for-byte equivalent in behaviour to the already verified v6.

### Note
- The app binary is 0xF9A40 bytes against an app partition of 0x100000 — only 2% free. This is not a v7 regression (v6 was the same size), but adding v7 functionality will require attention to the partition layout.

### Phase 7a: microSD interconnection plan

#### Summary
Design-only phase, no code changes. Produced a wiring scheme for connecting a microSD card module (labelled "simcard" in the assignment; the module is in fact a microSD adapter — a SIM holder has no SPI) to the server's ESP32-S3 devkit. The goal it serves: move the static files served by server_v7 off the flash to relieve the 2% free app partition noted above.

#### Hardware
- Module: microSD adapter with a 74LVC125A quad buffer **and** an AMS1117 regulator -> **VCC = 5V**. (Without the regulator it would have been 3.3V; feeding 5V to a regulator-less board destroys the card, so the variant had to be established first.)
- Card: 8GB microSD = SDHC, to be formatted FAT32.
- The server devkit had no pins connected to anything yet, so the whole pin space was available.

#### Wiring scheme
| SD module | ESP32-S3 | IO_MUX function |
|-----------|----------|-----------------|
| `GND`  | `GND`     | |
| `VCC`  | `5V`      | AMS1117 regulates to 3.3V |
| `CS`   | `GPIO10`  | FSPICS0 |
| `MOSI` | `GPIO11`  | FSPID |
| `SCK`  | `GPIO12`  | FSPICLK |
| `MISO` | `GPIO13`  | FSPIQ |

Plus: 10k pull-up from `GPIO10` (CS) to 3.3V (the pin floats during reset), and a 10-47uF capacitor at the module's VCC (SD cards draw 100mA+ current spikes).

#### Key design decisions
- **SPI2 (FSPI) IO_MUX pins instead of the right-side header.** IO_MUX allows up to 80MHz where the GPIO matrix caps at 40MHz — and SD-SPI runs up to 40MHz, exactly at that limit. Costs nothing here since the devkit was empty. Note that this is the same pin set `crt_RealMeasurement.h` uses for the ADS1220 on the *sensor* devices; different physical devices, so no conflict, and it keeps the project convention consistent.
- **IO_MUX does not reduce CPU load** — that is a common misconception. It affects maximum clock and signal delay only. CPU relief comes from **DMA**, which works equally on both routings. For Phase 7b this means: use the ESP-IDF `sdspi` driver (`SDSPI_HOST_DEFAULT()` + `SPI_DMA_CH_AUTO`), not the Arduino `SD.h` library, which does byte-wise SPI without DMA.
- **SDMMC (the dedicated, genuinely CPU-cheaper peripheral) is not possible with this module.** Wiring-wise it would fit — on the card `CS`=DAT3, `MOSI`=CMD, `SCK`=CLK, `MISO`=DAT0, so 1-bit SDMMC is the same four wires. But SDMMC needs CMD and DAT0 to be bidirectional, and the 74LVC125A buffers in the path are unidirectional. SPI mode it is. A different, unbuffered adapter would be needed to change this.

#### Pins deliberately avoided
| Pin | Reason |
|-----|--------|
| `GPIO0`, `GPIO45` | strapping pins (BOOT / VDD_SPI select); a pull-up on the module can break booting |
| `GPIO48` | in use as the RGB status LED (`crt_ServerNode.h` -> `neopixelWrite(RGB_BUILTIN, ...)`) |
| `GPIO38` | is the RGB LED instead of 48 on some DevKitC-1 revisions |
| `GPIO19`, `GPIO20` | native USB D-/D+, used to flash and monitor the server |
| `GPIO35/36/37` | physically committed on modules with octal PSRAM. PSRAM is not enabled in `sdkconfig` and `crt_SensorNode.h` already uses 35 as `SNAPSHOT_PIN`, so they are probably free — but there was no need to take the risk |

Note: `GPIO12` is a strapping pin (MTDI) on the classic ESP32, but **not** on the ESP32-S3, where the strapping pins are only 0, 3, 45 and 46.

#### Status
- Wiring scheme delivered; nothing connected or built yet. Phase 7b (the `hello_simcard` test app) follows.

### Phase 7b: hello_simcard test app

#### Summary
Built and verified the standalone `hello_simcard` app on the real hardware. The microSD module works:
both cards tested mount, the read-back matches what was written, and the throughput is roughly 0.5-0.7 MB/s.
The card is proven. Along the way, the assumption that 40 MHz depends on wiring quality was measured and
disproven — the 20 MHz ceiling turns out to be a protocol matter, not an electrical one.

#### Hardware as finally connected
- Module fed from an **external** 5V supply, GND common with the devkit, **47uF** buffer capacitor.
  A larger capacitor was tried first and crashed the PC twice: its inrush charging current browned out
  the host USB 5V. 47uF is the working compromise.
- Wiring as per phase 7a: CS=GPIO10, MOSI=GPIO11, SCK=GPIO12, MISO=GPIO13, 10k pull-up on CS.
- Short custom-cut breadboard leads, not dupont wires.

#### Created files
- **`apps/hello_simcard/src/`**
  - `crt_SdCard.h` - owns the SPI bus, the card handle and the FAT mount; offers file/directory access and card properties
  - `crt_SdCardTest.h` - control object: runs the five-step sequence once at boot, reports the verdict on the RGB LED
  - `hello_simcard.ino` / `hello_simcard_ino.h` - Arduino wrapper holding the pin and frequency configuration
- **`apps/hello_simcard/doc/`** - `hello_simcard.md` (summary, object model, object list, call trees, full speed analysis), `test.md` (measured results), plus `mermaid/` and `img/`
- **`apps/hello_simcard/tests/`** - empty, matching the other apps (the app itself is the test)

#### Modified files
- **`main/main.cpp`** - added the `hello_simcard.ino` include (active; the v7 includes are commented out)
- **`main/CMakeLists.txt`** - added the `apps/hello_simcard/src` include path

#### Measured results (20 MHz, both cards pass 5/5)
| Card (CID name) | Type | Capacity | Write | Read |
|-----------------|------|----------|-------|------|
| `SD16G` | SDHC | 7684 MB | 664-672 kB/s | 478-483 kB/s |
| `CBADS` | SDHC | 15115 MB | 630 kB/s | 675 kB/s |

Despite similar packaging these are demonstrably different cards: different CID name, different capacity,
and an opposite read/write balance.

#### Key finding: what limits the speed to 20 MHz
Anything above 20 MHz fails identically on **both** cards, always with
`sdmmc_enable_hs_mode_and_check: send_csd returned 0x108`.

- `sdmmc_enable_hs_mode_and_check()` (IDF `components/sdmmc/sdmmc_sd.c`) switches the card into SDR25
  high-speed mode via CMD6 as soon as `host.max_freq_khz > SDMMC_FREQ_DEFAULT` (20000), then re-reads
  the CSD. That re-read fails.
- That check is init step 140 in `sdmmc_init.c`; the host clock is only raised at step **160**. The
  failure therefore happens while the bus is still at its 400 kHz probing speed.
- **Decisive experiment:** requesting 20001 kHz also fails, while the SPI divider makes its actual bus
  clock identical to the passing 20000 kHz case (80/4 = 20 MHz; the next step up, 80/3 = 26.7 MHz,
  exceeds the request). Same clock, opposite outcome — so the mode switch is the variable, not the speed.

This rules out the clock rate, signal integrity, the wiring and the 74LVC125A buffer. That buffer is
stateless and combinational, transparent at 400 kHz, and demonstrably works at 400 kHz during every
successful mount. Two candidates remain and could not be separated with the cards at hand: either both
cards share a controller with a broken CMD6 in SPI mode, or the high-speed switch over SPI does not work
on this IDF/host combination at all. A third card from a genuinely different manufacturer would decide it.

Whether the module and wiring could carry 40 MHz *electrically* remains untested — the card refuses
before the clock is ever raised.

#### Is 40 MHz worth chasing?
Probably not. At 20 MHz a single SPI data line offers 2.5 MB/s; we measure 0.48-0.68 MB/s, i.e. 19-27%
of that. The clock is not the bottleneck — FAT bookkeeping, per-block CRC and the card's internal timing
are. The real lever for throughput would be SD 4-bit mode (four data lines at the same clock), which needs
a module without the unidirectional 74LVC125A buffer: either a plain bufferless microSD socket breakout
(the ESP32-S3 is a 3.3V host, so no level shifting is needed at all) or a purpose-built bidirectional SD
transceiver such as the **TI TXS0206A** (auto-direction sensing, up to 60 MHz / 60 Mbps per channel) or
the **TI TXS02612** (SDIO port expander with level translation). Note that 4-bit mode does not depend on
the CMD6 switch our cards reject.

#### Defects found and fixed
- **Stack overflow at test 5/5**: `DirEntry entries[32]` (~2.3 kB) sat on the 3584 byte main task stack.
  Moved to the heap.
- **Misleading mount diagnostic**: it blamed power and wiring unconditionally, which at >20 MHz points
  exactly the wrong way. Now a separate message when a high-speed rate was requested, recommending the
  20 MHz fallback.

#### Known open issue
The MISO probe assumes a powered card holds MISO high at idle. `SD16G` does, `CBADS` does not — so the
probe warns about missing power or a missing MISO connection while everything works. Card-dependent and
unreliable; it should be softened to a neutral observation.

#### Relevance to the original goal
Phase 7a introduced the card to relieve the app partition, which was 98% full. That premise has since
weakened: the devkit has 16 MB of flash while the image header and partition table only use 2 MB
(`W spi_flash: Detected size(16384k) larger than the size in the binary image header(2048k)`). Re-partitioning
is likely the cheaper route for server_v7's static files, which makes `hello_simcard` primarily a piece of
proven hardware rather than a stepping stone. That decision is for a following phase.

#### Status
- Card proven, 5/5 on both cards at 20 MHz, LED green. The device is left with the working 20 MHz build.

