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

### Phase 7c: unlocking the flash beyond the 1MB app partition

#### Summary
The app partition went from **2% free to 76% free** without a single line of application code changing —
it was purely configuration. The phase was asked as "use the full 16MB of the N16R8", but ended at an
**8MB** layout on purpose, because at that time the project built every app from one shared `sdkconfig`
and one partition table, and the sensor boards only have 8MB. That constraint has since been removed: the
project now has per-board builds (see "Superseded by per-board builds" at the end of this chapter), so the
server does run a 16MB layout today. The measurements and the two root causes below are what still matter.

#### Why the flash was unused
Two independent causes, both configuration:
- `CONFIG_ESPTOOLPY_FLASHSIZE_2MB=y` — the binary image header claimed 2MB, which is what produced the
  boot warning `Detected size(16384k) larger than the size in the binary image header(2048k)`. The chip
  itself was correctly detected all along.
- `CONFIG_PARTITION_TABLE_SINGLE_APP=y` — the build used the **built-in** `partitions_singleapp.csv`
  (nvs + phy_init + a 1MB factory app), not the `partitions.csv` in the project root. That file was
  configured as `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` but never *selected*, so it had no effect at all.
  It describes a 4MB OTA layout (app0/app1/spiffs) that does not match this project's factory-only setup.

The second cause is the nastier one: a custom filename that is set but not selected looks configured and
does nothing. Both `sdkconfig.server.defaults` and `sdkconfig.sensor.defaults` therefore set
`CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` *and* rely on `CONFIG_PARTITION_TABLE_CUSTOM=y` from the shared
`sdkconfig.defaults`.

#### Key finding: the boards do not have the same flash
Measured with `esptool.py flash_id` (read-only, flashes nothing):

| Board | Port | Module | Flash | PSRAM |
|-------|------|--------|-------|-------|
| server devkit | ACM0 | ESP32-S3-N16R8 | 16MB external | 8MB embedded |
| sensor 3 | USB0 | ESP32-S3-MINI-1-N8 | **8MB embedded (XMC)** | none |

This is a durable hardware fact and it is why a single flash-size setting cannot be right for both. The
old 2MB/1MB setup was accidentally safe: a 2MB header with a 1MB app fits any board of 2MB or more. A
16MB table on a shared config would *not* have been safe, and it would have failed **silently** — the
sensor's app is only ~1MB and sits at 0x10000, so it boots happily while the table claims flash above
0x800000 that does not physically exist. That only surfaces once something actually touches the data
partition.

Hence the choice at the time: one 8MB table, the common denominator. `factory` became 4MB, which was what
the phase was really after, and the server's upper 8MB was simply left unused.

#### Verification (phase 7c, on the then-current 8MB layout)
- Build: `app-template.bin 0xf9a40 bytes. Smallest app partition is 0x400000 bytes. 0x3065c0 bytes (76%) free.`
  (was 0x100000 and 2% free)
- Boot log reported `SPI Flash Size : 8MB` with `factory 00010000 00400000` and `spiffs 00410000 003f0000`
- `W spi_flash: Detected size(16384k) larger than the size in the binary image header(8192k)` reappeared on
  the server — now correct rather than a symptom: the chip is 16MB and we deliberately used 8. On the
  sensor board the 8MB header matched the chip exactly, so no warning there.
- server_v7 ran: AP `SCOLIOSE` on 192.168.4.1, WebServer on port 80
- End-to-end over ESP-NOW with a sensor_v6 board (identical to sensor_v7 at that point):
  `REGISTER from sensor 3`, `DATA from sensor 3, pkt 1/2 (245 bytes)` + `pkt 2/2 (17 bytes)`,
  `Sensor 3 -> 128 measurements`. The repeated `Broadcast DISCOVER (1/3 registered)` is designed
  behaviour: `EXPECTED_SENSOR_COUNT` is 3 and only one sensor was powered.
- Also verified on the real 8MB sensor board (USB0): `Sensor node v7 starting, id=3`,
  `MCP23017 detected, using real measurement`, `ESP-NOW ready`,
  `Received POLL, sent 2 pkt(s) id=3 val=6 lc=1 (128 measurements)`

#### `nvs` and `phy_init` must keep their offsets
Both tables written in this phase kept `nvs` at 0x9000 (24K) and `phy_init` at 0xf000 (4K) — the exact
offsets and sizes of the built-in table they replace — so existing NVS contents survive a repartition.
Confirmed on target: WiFi came up with `config NVS flash: enabled` and the phy calibration loaded
normally. The current `partitions_server_16mb.csv` and `partitions_sensor_8mb.csv` keep the same two
entries, so that property still holds.

#### Two settings were silently living on one machine only
`sdkconfig` is in `.gitignore` (line 13), so every setting in it existed only locally. A partition table
in git does not help if nothing selects it: a fresh clone would silently revert to a 2MB header and the
built-in 1MB app partition, undoing this phase without a trace. That is why this phase introduced an
`sdkconfig.defaults` — and generating a config from scratch in an isolated build directory to test it
turned up two settings that were being lost beyond the two the phase was about:

| Setting | Why it matters |
|---------|----------------|
| `CONFIG_IDF_TARGET="esp32s3"` | Without it a fresh checkout builds for the **plain ESP32** — the IDF default — and every `SOC_*` setting comes out wrong. |
| `CONFIG_FREERTOS_HZ=1000` | CleanRTOS requires 1 tick == 1ms; the IDF default is 100Hz, i.e. 10ms per tick. `libs/CleanRTOS/src/crt_Handler.h` names this exact setting in a comment: *"This Handler class assumes that 1 tick equals 1ms ... update in sdkconfig: CONFIG_FREERTOS_HZ=1000"*. Losing it does not fail the build — it silently makes every delay and timer ten times coarser. |

Both now live in the shared `sdkconfig.defaults`, so this finding survived the rework. Note that
`sdkconfig.defaults` is a *seed*, not a mirror: settings changed later via `menuconfig` do not flow back
into it, and an existing `sdkconfig` takes precedence over it. Only deliberate choices belong there.

#### Consequence for phases 7a/7b
The premise of 7a — a 98% full app partition — is gone: the same binary leaves 76% free, with a multi-MB
data partition on top of that. Moving server_v7's static files to the microSD card is therefore no longer
necessary to relieve memory. The `hello_simcard` app remains valid as proven hardware, and the card stays
an option for data that should outlive a reflash or be swapped between machines, but it is no longer on
the critical path.

#### PSRAM deliberately left disabled
The server's N16R8 carries 8MB of embedded PSRAM (`flash_id` reports `Features: WiFi, BLE, Embedded PSRAM
8MB`), which is separate from the flash this phase is about, and nothing asks for it. The sensor's
MINI-1-N8 has none. Beyond that, octal PSRAM commits GPIO35/36/37, and `crt_SensorNode.h:53` uses GPIO35
as `SNAPSHOT_PIN`, which the sensor boards really do use (`Snapshot button configured on GPIO35`). So
enabling PSRAM is a server-only setting with a pin clash on the sensors — now cheap to do per board, but
still unmotivated.

#### Superseded by per-board builds
This chapter's one-config-for-every-board premise no longer holds, and the "revisit with per-board configs
if server storage ever exceeds ~4MB" note it ended on is exactly what happened: the autocalibration work
grew server_v7's web content. The project now builds per board type:

| | server | sensor |
|---|---|---|
| script | `build_server.sh` / `.bat` | `build_sensor.sh` / `.bat` |
| build dir | `build_server` | `build_sensor` |
| config | `sdkconfig.server` | `sdkconfig.sensor` |
| seeds | `sdkconfig.defaults` + `sdkconfig.server.defaults` | `sdkconfig.defaults` + `sdkconfig.sensor.defaults` |
| flash size | 16MB | 8MB |
| table | `partitions_server_16mb.csv` (app 6M) | `partitions_sensor_8mb.csv` (app 3M) |

App selection and sensor id moved out of the sources too: `idf.py -D SENSORGRID_APP=server_v7|sensor_v7|client_v7`
and `-D SENSORGRID_SENSOR_ID=<n>` become `#define`s via `main/CMakeLists.txt`, with the manual `#include`
in `main.cpp` and the `SENSOR_ID` in `sensor_v7_ino.h` as fallback. So the server now gets its 16MB after
all — the literal request of this phase — and the silent-failure risk that forced 8MB is gone, because the
sensors no longer share the server's table.

The files this phase created (`partitions_8MB.csv`, and its single-table `sdkconfig.defaults`) were dropped
in favour of that setup and no longer exist.

#### Status
- Done and verified at the time on both board types, from one shared 8MB layout. Since replaced by the
  per-board builds above; this chapter is kept for the two root causes, the `flash_id` measurements, and
  the two settings that were being lost.

### Per-board builds: server 16MB, sensor 8MB (post-7c rework)

*This chapter is reconstructed from the committed files (commits `715e5ef`..`df3515d`), not from a
working session — the rework was done on another machine. The decisions below are read out of the code
and the comments in it; the verification at the end is the one thing measured here.*

#### Why
Phase 7c ended on an explicit escape hatch: *"If server-side storage ever genuinely exceeds ~4MB,
revisit with per-board configs then."* The autocalibration work (`crt_GridAutoCalib.h`,
`crt_GridAutoCalibUi.h`) and the recording views grew server_v7's embedded web content, and the account
system (`crt_AccountHtml.h` alone is ~500 lines of embedded HTML/CSS) added more. The shared-config
premise — one `sdkconfig`, one partition table, sized for the smallest board — was what kept the server
at a 4MB app partition on a 16MB chip. So the hatch was taken.

#### What replaced it
Two parallel builds that share nothing but the sources and the common seed. `README.md` documents the
usage; this is the shape and the reasoning:

| | server | sensor |
|---|---|---|
| script | `build_server.sh` / `build_server.bat` | `build_sensor.sh` / `build_sensor.bat` |
| build dir | `build_server/` | `build_sensor/` |
| generated config | `sdkconfig.server` | `sdkconfig.sensor` |
| seeds | `sdkconfig.defaults` + `sdkconfig.server.defaults` | `sdkconfig.defaults` + `sdkconfig.sensor.defaults` |
| flash size | 16MB | 8MB |
| table | `partitions_server_16mb.csv` | `partitions_sensor_8mb.csv` |
| factory app | 6M at 0x10000 | 3M at 0x10000 |
| spiffs | 0x610000, 0x9F0000 (~9.9M) | 0x310000, 0x4F0000 (~5M) |
| default port | `/dev/ttyACM0` (COM5) | `/dev/ttyACM1` (COM6) |

Both tables total their chip exactly: 0x610000 + 0x9F0000 = 0x1000000 (16MB), and
0x310000 + 0x4F0000 = 0x800000 (8MB). Separate build directories and separate `sdkconfig`s mean the two
never overwrite each other's configuration, which is what made the shared setup fragile — switching board
type previously meant a reconfigure plus a full rebuild.

The server's app partition goes 1M (pre-7c) -> 4M (7c, shared) -> **6M**. The sensor goes 1M -> 4M -> 3M;
that is a reduction on paper but irrelevant in practice, since sensor_v7's binary is around 1MB and the
space it gives back goes to its spiffs partition.

#### What carried over from 7c
- **`nvs` at 0x9000 (24K) and `phy_init` at 0xf000 (4K)** in both tables — the offsets of the built-in
  table they replace, so existing NVS contents (WiFi calibration among others) survive a repartition.
- **`CONFIG_IDF_TARGET="esp32s3"` and `CONFIG_FREERTOS_HZ=1000`** are in the shared `sdkconfig.defaults`.
  These are the two settings 7c found were living on one machine only; both are silent failures when
  lost (wrong target, or every delay ten times coarser).
- **`CONFIG_PARTITION_TABLE_CUSTOM=y`** sits in the shared seed, and each per-board seed sets
  `CONFIG_PARTITION_TABLE_CUSTOM_FILENAME` *and* `CONFIG_PARTITION_TABLE_FILENAME`. That belt-and-braces
  is the direct lesson of 7c, where a custom filename was set while `PARTITION_TABLE_SINGLE_APP` was
  still selected — so the setting looked configured and did nothing.

The shared seed also carries what the Arduino-as-component setup needs: `CONFIG_AUTOSTART_ARDUINO` off
(`setup()`/`loop()` are called from `app_main`), the five mbedTLS PSK options, `CONFIG_COMPILER_CXX_EXCEPTIONS=y`,
and `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y` so `idf.py monitor` shows `ESP_LOG` output over the built-in
USB port the devkits hang on.

#### App selection moved out of the sources
Previously the active app was chosen by commenting an `#include` in `main.cpp`, and the sensor id by
editing `SENSOR_ID` in `sensor_v7_ino.h` — both source edits, both easy to forget before a flash, and
`main.cpp` is shared with every older app generation.

Now `main/CMakeLists.txt` turns two CMake cache variables into defines:
- `idf.py -D SENSORGRID_APP=server_v7|sensor_v7|client_v7` -> `#define SENSORGRID_APP_<UPPER> 1`
- `idf.py -D SENSORGRID_SENSOR_ID=<n>` -> `#define SENSORGRID_SENSOR_ID <n>`

`main.cpp` selects on `SENSORGRID_APP_SERVER_V7` / `_SENSOR_V7` / `_CLIENT_V7`, and `sensor_v7_ino.h`
guards its id with `#ifndef SENSORGRID_SENSOR_ID`. Both keep the old manual choice in the `#else` /
`#ifndef` branch, so editing `main.cpp` by hand still works and nothing in the older app generations
changed behaviour. The build scripts set `SENSORGRID_APP` always and `SENSORGRID_SENSOR_ID` only when the
third argument is given.

#### Also gitignored
`sdkconfig.server`, `sdkconfig.sensor` and their `.old` siblings, plus `build_*.log` and `flash_*.log`.
The two build directories were already covered by the existing `build_*/` pattern. So the committed
truth is the five defaults files and the two tables; everything generated stays out.

#### Finding: the .sh scripts were not executable
Both `build_server.sh` and `build_sensor.sh` were committed with mode `100644`, while `README.md`
prescribes `./build_server.sh [actie] [poort]`. On a fresh clone (and in WSL) that fails with
`/bin/bash: ./build_server.sh: Permission denied`, exit 126 — the `.bat` variants are unaffected since
Windows does not use a mode bit. Fixed by setting the bit (mode `100755`); `bash build_server.sh ...`
was the workaround.

#### Verification of the server build (WSL, 2026-10-12)
`./build_server.sh build` from a clean state — no `build_server/`, no `sdkconfig.server` — so the script
took its first-run path (`idf.py set-target esp32s3` seeded from `sdkconfig.defaults` +
`sdkconfig.server.defaults`) and then built all 1839 targets. Exit 0.

- `app-template.bin binary size 0x11f460 bytes. Smallest app partition is 0x600000 bytes.
  0x4e0ba0 bytes (81%) free.`
- The generated partition table matches the csv exactly: `nvs` 24K at 0x9000, `phy_init` 4K at 0xf000,
  `factory` 6M at 0x10000, `spiffs` 10176K at 0x610000.
- `sdkconfig.server` came out with everything the two seeds prescribe:
  `CONFIG_IDF_TARGET="esp32s3"`, `CONFIG_ESPTOOLPY_FLASHSIZE_16MB=y` / `="16MB"`,
  `CONFIG_PARTITION_TABLE_CUSTOM=y` with both filename settings on `partitions_server_16mb.csv`,
  `CONFIG_FREERTOS_HZ=1000`, `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y`,
  `CONFIG_COMPILER_CXX_EXCEPTIONS=y`, and `# CONFIG_AUTOSTART_ARDUINO is not set`.
- The flash command the build prints carries `--flash_size 16MB`, i.e. the image header now matches the
  chip — the mismatch warning 7c lived with is gone on the server.
- `git status` stays clean apart from intended edits: `build_server/` and `sdkconfig.server` are both
  ignored as designed.

Only pre-existing warnings, all from `libs/TFT_eSPI` and unrelated to sensorgrid: the
`DMA is not supported on the ESP32 S3` `#warning`, and `-Wmissing-field-initializers` on a few
`spi_bus_config_t` / `spi_device_interface_config_t` initialisers.

Not flashed — this was a build check only.

**Size in perspective.** server_v7 is now 0x11f460, up from 0xf9a40 measured in 7c: about 154KB (+15%)
of autocalibration, recording views and account pages. That still fits 7c's 4M partition at 71% free, so
the 6M is headroom rather than a binary that had outgrown the previous layout. The partition that was
genuinely too small is the original built-in 1M one, which this binary exceeds outright.


#### Flashed and verified on target (ACM0, 2026-10-12)
`./build_server.sh flash /dev/ttyACM0` — exit 0, all three images written with `Hash of data verified`
(bootloader at 0x0, partition table 3072 bytes at 0x8000, app 1176672 bytes at 0x10000, 9.8s at
~957 kbit/s), then `Hard resetting via RTS pin`.

The boot log confirms the repartition took effect:
```
I (28) boot.esp32s3: SPI Flash Size : 16MB
I (28) boot: Partition Table:
I (28) boot: ## Label            Usage          Type ST Offset   Length
I (29) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (29) boot:  1 phy_init         RF data          01 01 0000f000 00001000
I (29) boot:  2 factory          factory app      00 00 00010000 00600000
I (30) boot:  3 spiffs           Unknown data     01 82 00610000 009f0000
I (254) boot: Loaded app from partition at offset 0x10000
```
- **No `Detected size(16384k) larger than the size in the binary image header` warning.** 7c had to live
  with it (8MB header on a 16MB chip); the server's header now matches its flash exactly.
- **NVS survived the repartition**, as the kept offsets intended: `wifi:config NVS flash: enabled` and
  `phy_init: phy_version 701` both load normally.
- server_v7 runs: `AP SSID: SCOLIOSE`, `AP IP: 192.168.4.1`, `WebServer started on port 80`,
  `ESP-NOW init OK`, STA MAC `10:20:BA:4B:DF:94` / AP MAC `10:20:BA:4B:DF:95`, `Expecting 3 sensors`.
- `Broadcast DISCOVER (0/3 registered)` repeating is the designed behaviour with no sensors powered —
  none were attached to WSL for this check.

So the server half of the per-board setup is proven end to end: seeds -> config -> table -> flash -> boot.
The sensor half is still only proven by inspection of its files.

#### Status
- **Server: proven end to end.** Built from scratch, flashed to ACM0, boots on the 16MB header with the
  6M factory partition, NVS intact, server_v7 serving on 192.168.4.1.
- **Sensor: not exercised.** `./build_sensor.sh` needs a sensor module on a port, and only
  `/dev/ttyACM0` (the server devkit) was attached to WSL. The sensor half rests on file inspection only.
- The `.sh` executable-bit fix is in the working tree, not yet committed.

### Grid View: display controls as radio groups
The toggle buttons above the grid became three framed radio groups:
- **Sensor Value**: Raw / Norm SumCap / Norm IndivCap / Norm AutoCal, now stacked vertically.
- **Kleurnormalisatie**: Normalised / MaxFixed. These replace Norm Display and MaxFixed Display. Under
  MaxFixed there is a slider with the Fixed Max field beneath it; both are grayed out under Normalised.
- **Display**: Grays / Colors, replacing Color Display.

The fixed maximum now exists twice: `fixedMaxRaw` (1..65535, used while Sensor Value is Raw) and
`fixedMaxGrams` (1..3500, used in the three gram modes). Slider and field show the one belonging to the
active mode, and `getFixedMax()` returns it, so circles, 3D surface and plot follow without other changes.
Out-of-range input is clamped; unparseable input restores the previous value.

The radio buttons have no "neither" state, while the old buttons did (range 0..65535). Old cookies and old
meta files map onto the new model: "neither" becomes MaxFixed with raw 65535 (identical view), and old
MaxFixed with value X becomes raw X and grams min(X, 3500). New cookies: `gvColorNorm`, `gvFixedMaxRaw`,
`gvFixedMaxGrams`; meta gains `fixedMaxRaw`/`fixedMaxGrams`.

Hovering 1.5 s over a radio button or a group title shows a short explanation. This uses a custom tooltip
(`data-tip`, `#tipBox`), because the delay of the browser's own `title` tooltip cannot be set. After a
recording or snapshot download, a "download completed" popup shows under the button for 1 s. It appears
when the download is handed to the browser; a page cannot observe when the file has actually been written.

client_v7's grid-page check now looks for "Kleurnormalisatie" and the `colorized` radio group instead of
the old button labels.

Verification: the page was run in headless Chrome with an injected test script (25 checks covering
ranges, clamping, raw/gram switching, graying out, cookies, old and new meta, tooltip timing, popup
lifetime), and screenshots confirmed the layout. server_v7 was built and flashed to ACM0. client_v7 was
not rebuilt.

Follow-up in the recording player:
- `sensor_val` and `loadcell_val` sit stacked in a small framed box. `sensor_val` carries `(raw)` under
  Raw and `g` otherwise. `loadcell_val` is always `g`, because the loadcell curve is grams in every mode
  (`recLcGramOfFrame`).
- The single y-axis checkbox, and the y-clip fields that come with it, are hidden under Raw. Comparing
  raw counts with grams on one axis means nothing, so `singleY` is also ignored there while the checkbox
  keeps its state for the gram modes.

Verified headless with a synthetic recording (10 checks, plus screenshots in Raw and SumCap), then built
and flashed to ACM0.

### Grid View: calibration controls only for Admin
For roles other than Admin (Gebruiker, Onderzoeker), the grid page hides:
- Stats Filter and Value Filter. Their getters return the defaults 0.9 and 0.5 regardless of the field,
  and the fields are reset to those values. Cookies are left alone, since they hold what an Admin set in
  this browser.
- In the green sensor panel: Capture Sum, maxSumCaptured, avResidualNoiseSum and Calibrate Sum Grams.
- In the red circle panel: the three Capture Indiv rows (button, cap, grams, Calibrate Grams Indiv).
- Both capture-error spans; they only report capture errors.

The Auto-PWL line stays visible, now capitalised. Without knots, a non-admin sees "Nog geen Auto-PWL
knopen beschikbaar. (Her-)calibratie is nodig!"; an Admin keeps "Auto-PWL: geen (<reason>)".

Mechanism: the grid page defines `window.accPage`, which account.js already calls with the `/api/me`
result. It sets `isAdmin` and the body class `is-admin`; CSS hides `.admin-only` without it. Until the
role is known the page behaves as non-admin, so nothing flashes into view. This is presentation only: the
server does not enforce it. Calibration lives in the browser (cookies/localStorage) and can still arrive
through Upload Meta.

Verified headless against a mock `/api/me` returning role 2 and role 0 (9 checks each, including a
pre-set filter cookie of 0.7 that an Admin sees and a non-admin does not). Built and flashed to ACM0.

### Grid View: Sensor Value options per role, AutoCal only with calibration data
- **Remove Offset** is hidden while Sensor Value is Norm AutoCal. That mode maps the absolute raw value
  through the knots and ignores the offset. Mechanism: `syncDisplayControls()` toggles the body class
  `cal-autocal`, and CSS hides `.hide-in-autocal`.
- **Reset Indiv Caps** is `admin-only` now, like the rest of the manual calibration controls.
- **Norm SumCap and Norm IndivCap** are `admin-only`. For other roles only Raw and Norm AutoCal remain.
  `calModeAllowed()` guards `setCalMode()` and `applyMetaState()`, so a cookie or an uploaded meta or
  recording cannot select them either. A mode restored from the cookie before the role is known falls
  back to Raw in `accPage` once the role turns out not to be Admin. The cookie itself is left alone.
- **Norm AutoCal without knots**: as long as no circle of any sensor has at least 2 Auto-PWL knots,
  clicking AutoCal keeps the previous mode and shows a popup (`notePopup`, under the radio button). It
  says a (re-)calibration of the pressure sensors is needed first, or calibration data from an earlier
  recording or a meta upload. The popup closes at the next mouse click, or after 8 s. A meta or
  recording upload may still set AutoCal without knots: the check is only for the click.

Verified in headless Edge (playwright-core) against a mock `/api/me` for roles 0, 1 and 2 and mock
measurements: 46 checks (visibility per role, refusal plus popup without knots, selection with knots,
Remove Offset hidden and back, role downgrade, meta with a forbidden mode), no page errors. Built and
flashed to COM5 (ESP32-S3, USB-Serial/JTAG 303A:1001).

### Snapshots: loadcell as raw only, like recordings
A snapshot frame no longer stores `loadcellGram`, only `loadcellRaw`. The gram follows on replay from the
loadcell parameters (`lcTareOffset`, `lcScale`) that already travel in the meta of a snapshot download.

- `loadcellRaw` is the raw value belonging to the filtered gram shown at the moment of the snapshot:
  `round(filteredWeight * lcScale + lcTareOffset)`. The circle values in a snapshot are the filtered
  `emaRawValues` too, so circles and loadcell stay consistent and a replay shows the same gram (within
  rounding, ~0.005 g). Without a filtered weight yet, the last raw sample is stored.
- Replay goes through `snapApplyFrame()`, which passes `lcGramFromRaw(s, loadcellRaw)` to
  `updateLoadcell` like `recApplyFrame` does for recordings. Older snapshot files still carry
  `loadcellGram`; that value is shown as is, so they look as before.

Verified in headless Edge (9 checks: no loadcellGram stored, raw/gram round trip, label, round trip
through download + upload with other loadcell parameters on the page, old file, fallback, disabled
sensor), plus the earlier Sensor Value checks again without failures. Built and flashed to COM5.


### Grid View: download dialogs with patient-id/note, auto-select on Play, zoom on the right
- **Zoom slider** moved to the far right of the record row, right of the single y-axis checkbox and y-clip.
- **Play** (and resume from Pause): with no sensor selected, the first enabled sensor with data in the
  recording is selected; with no circle selected, circle 0 of the selected sensor (`recAutoSelect`, same
  effect as a mouse click). If the grid has no cells yet, it is built from the recording's count first.
- **Download dialogs** in a new `crt_GridMeasInfo.h` (`/gridmeasinfo.js`), so the 67 KB limit of the grid
  HTML literal is not touched. After Stop and on Download: "Wat voor recording is dit?" with patient
  meting, kalibratiemeting (met de 3D printer) and Afbreken. The patient dialog has a required patient-id
  (`[A-Za-z0-9_-]+`) and an optional note. File names: `recording_<id>_<datetime>.json` and
  `recording_kalibratie_<datetime>.json`. Snapshots ask "Is dit een patient-meting?" (Ja/Nee/Afbreken);
  their names are `snapshots_<id>_<datetime>.json` or `snapshots_<datetime>.json`.
- Kind, patient-id and note go into the meta of the file as `measurement` (third argument of
  `metaAppendForExport`), not into Download Meta. An upload restores them. **Info** (record panel, second
  row under Record) and **Snapshots Info** show them.
- **Download again** of content already downloaded this session first asks for confirmation, then repeats
  the previous handling, prefilled. New frames, a new snapshot and a successful recalibration reset that.
  An uploaded file counts as not downloaded.
- The record panel now has two rows (`.rec-play-row`), left-aligned so that Info sits under Record. The
  first row used to be centred.

Verified in headless Edge: 58 checks (layout order, auto-select incl. disabled sensor and existing
selection, all dialog paths, file names, meta contents, prefill, re-download warning, pause/resume, upload
restore, Info contents, Escape), plus the earlier role (46) and snapshot (9) checks without failures.
Built and flashed to COM5 (ESP32-S3, USB-Serial/JTAG 303A:1001).

### Uploaded recordings and snapshots count as downloaded, versioned file names
- An upload now sets `downloaded` and remembers the file name (`source`). The first Download of uploaded
  content therefore asks "U heeft deze recording/snapshots al eens (ooit) gedownload, weet u het zeker?".
  The question now has "(ooit)" in all cases. After Ja, the previous handling follows (prefilled patient
  dialog, or a calibration download at once). An older file without `measurement` gets the kind question.
- Content from an upload is downloaded under the upload's name plus `_vN` (`miNextVersion`): `x.json` gives
  `x_v1.json`, `x_v1.json` gives `x_v2.json`, and each next download in the session counts up. A browser
  suffix " (1)" is dropped first. The browser cannot check the download folder itself. Record and Clear
  Snapshots end this and give normal names again; new frames or snapshots keep the versioned name.

Verified in headless Edge: 72 checks (the 58 earlier ones adapted, plus upload naming `_v1`/`_v2`/`_v3`,
" (1)" handling, old upload without measurement, normal names after Record/Clear), plus 46 role checks and
9 snapshot checks. Built and flashed to COM5 (ESP32-S3).

### Upload names: no version suffix after all
The `_vN` suffix is dropped again (`miNextVersion` replaced by `miUploadName`). Content from an upload is
downloaded under the same name as the upload; the browser itself numbers a name that already exists in the
download folder ("x (1).json"). A " (1)" in the uploaded name is dropped, so the browser numbers from the
original name. Tests adapted (72 checks pass, plus 46 role and 9 snapshot checks). Built and flashed to COM5.

### First download of fresh content gets _org
Only the first download of a fresh recording (after Record) or snapshot series (after Clear Snapshots or
page load) gets `_org` (original) before `.json`, via the `orgDone` flag in `MI_REC` / `MI_SNAP`. Later
downloads of the same content and downloads of uploaded content do not. An uploaded `x_org.json` (also with
a browser suffix like " (2)") is downloaded again as `x.json`; the browser numbers it if the name exists.
Verified in headless Edge (73 checks, plus 46 role and 9 snapshot checks). Built and flashed to COM5.

### Loadcell: "?" help and button style
- The loadcell section of each sensor gets a **?** button on the Tare row, right-aligned above the right edge
  of Calibrate. The two rows sit in a shrink-to-fit `.lc-ctrl` block, so its width is that of the
  Calibrate row. The "?" opens a dialog (`lcHelp`, using the dialog helpers of `gridmeasinfo.js`) with the
  calibration steps, when Tare alone suffices, the link with "Herkalibreer druksensoren", and where tare and
  scale are kept.
- Tare/Calibrate/? now have the same style as the other small buttons (like "3D Surface"): 2px grey border,
  rounded, white, bold grey text.

Grid HTML literal now 59.2 KB. Verified by screenshot (alignment: right edges of "?" and Calibrate equal)
and the 73 + 46 + 9 headless checks. Built and flashed to COM5.

### Upload of a recording removes the snapshots, and vice versa (after a warning)
- Both Upload buttons go through `miUploadClick`. If the other series exists (snapshots for a recording
  upload, a recording for a snapshots upload), a dialog "Weet u het zeker?" explains that it will be removed,
  adds "Die is nog niet gedownload." if that applies, and offers Ja / Afbreken. Ja opens the file dialog.
- The removal happens in `recUpload` / `snapUpload` after a successful parse: `snapClear()`, or the new
  `recClear()`, which brings the record panel back to Ready (live data, buttons and Info reset, Herkalibreer
  disabled). Cancelling the file dialog therefore removes nothing.

Verified in headless Edge (87 checks, plus 46 role and 9 snapshot checks). Built and flashed to COM5.

### Upload checks the file type (recording / snapshots / meta / other)
- Recording and snapshot downloads now carry `meta.fileType` ('recording' / 'snapshots'), as a fourth
  argument of `metaAppendForExport`.
- After the file choice, `recUpload` / `snapUpload` first call `miCheckUpload`. It determines the type with
  `miFileKind`:
  - `fileType` when present;
  - otherwise a `recording` block in the meta means recording (since 2026-09-17);
  - meta without that block probably means snapshots (not certain);
  - no meta means unknown;
  - a meta object (global + sensors) means meta;
  - anything else is other.
- A certain mismatch gives a dialog "Dit is geen recording" / "Dit is geen snapshot-serie", naming what the
  file is and which upload to use. Nothing is loaded and the other series is not removed. Uncertain (meta
  without recording block, as a recording) asks "Is dit een recording?" (Ja / Afbreken). Unknown is accepted.
- JSON parse errors now also end in the "other" message instead of an alert.

Verified in headless Edge (104 checks incl. 17 new ones for all type combinations, old files and files
without meta, plus 46 role and 9 snapshot checks). Built and flashed to COM5.

### Login: focus on the user name field
The user name field of /login has `autofocus`, so you can type at once. When the forced password change
appears (first login of the root admin), the focus moves to the new-password field. Verified in headless
Edge (focus on load, typing lands in the name field, focus after showForce). Built and flashed to COM5.

### Home page per role, user modes for Gebruiker, test page at /test, Clear Snapshots grayed
- **`/`** is now a Home page (`crt_HomeHtml.h`) that requires a login. The former home page "Sensormetingen"
  moved to **`/test`** (open, not linked from any navigation bar). client_v7 now fetches `/test` for its
  dashboard tests; the client has to be rebuilt for that.
- **After login** (`accLandingPage` in account.js, `landingPage` in AccountWeb; `/api/login` now returns the
  role): Admin and Onderzoeker go to `/grid`, a Gebruiker goes to `/`. `/login` redirects a logged-in user
  the same way, and so does a page that needs a higher role.
- **Home**: Admin and Onderzoeker see "U bent ingelogd als admin." (or onderzoeker) and an "Open Grid View"
  button. A Gebruiker sees "Wat wilt u doen?" with Patientmeting maken / Patientmeting bekijken / Snapshots
  bekijken, which open `/grid?mode=meten|bekijken|snapshots`.
- **Navigation per role** (`navRender`, `<span id="navLinks">` on Home, Grid View and the account pages):
  Home + Grid View, or for a Gebruiker Home + Patient Meten / Meting Bekijken / Snapshots Bekijken. The
  current page is marked via `<body data-page>`.
- **User modes** in the Grid View for a Gebruiker: body class `um-<mode>`, elements marked `hide-meten`,
  `hide-bekijken` or `hide-snapshots`. The mode comes from `?mode=` or the last choice
  (localStorage `sgUserMode`); without one the page goes back to `/`. Admin and Onderzoeker ignore the mode.
- **Clear Snapshots** is disabled while there are no snapshots.

Verified in headless Edge: 126 checks (visibility of all listed elements per mode and for Onderzoeker/Admin,
navigation incl. active link, remembered choice, redirect without choice, Home page per role, Clear
Snapshots state), plus 104 + 46 + 9 earlier checks. Built and flashed to COM5.

### User modes refined: Meting Bekijken, leave warnings, Snapshots bekijken
- Home buttons renamed: **Patient Meten** and **Meting Bekijken** (Snapshots bekijken unchanged).
- **Meting Bekijken**:
  - No live data: `pollLoop` only calls `fetchAll` when `pageReady` (role known, set in `accPage`) and the
    mode is not `bekijken`.
  - `enforceUserMode`: all sensors enabled and single y-axis on, without touching the cookies. It is also
    called at the end of `applyMetaState`.
  - Hidden (class `hide-bekijken`): Remove Offset, Enable Sensor, the red circle panel, the loadcell
    controls (`.lc-ctrl`) and the single y-axis checkbox.
- **Leave warning** (Patient Meten, Meting Bekijken): a capturing click handler on `<nav>` checks
  `unsavedText()` (not downloaded: recording in Patient Meten, snapshots in both modes). It asks "... Weet u
  zeker dat u de pagina wilt verlaten?" with Ja / Nee, afbreken; Ja clicks the same link again. A
  `beforeunload` guard covers refresh and closing.
- **Active nav link** does nothing (`navRender` sets a click handler that cancels it), on every page.
- **Snapshots bekijken**: Snapshot Replay and Stop Snapshot Replay are hidden; a snapshots upload starts
  the replay.

Verified in headless Edge: 36 new checks (no live data in Meting Bekijken, enforced settings also after a
recording with `enabled:false` / `recSingleY:false`, hidden controls, unsaved texts for all combinations,
active link, warning + Nee/Ja, user menu guarded, no question when saved, auto replay, admin unaffected),
plus 126 + 104 + 46 + 9 earlier checks (expectations adapted to the new Snapshots bekijken rule). Built and
flashed to COM5.

### Leave warning also for Admin and Onderzoeker
`unsavedText()` now also applies when the role (`userRole`, set in `accPage`) is Onderzoeker or Admin, with
the same texts as Patient Meten (recording and/or snapshots not downloaded). Without a login (test client)
there is no warning. Verified in headless Edge (48 checks incl. Onderzoeker and Admin runs: texts, warning on
Home, Nee keeps the page, active Grid View link does nothing), plus 126 + 104 + 46 + 9. Built and flashed to
COM5.

### Auto-play after upload, mode titles, fewer controls in the viewing modes
- **Recording upload** starts playback at once (all roles): `recUpload` calls `recPlay()` after loading.
- **Meting Bekijken**: the Snapshot button is grayed until a recording is loaded
  (`snapUpdateSnapshotButton`, called from `recUpdateInfo` and `applyUserMode`; tooltip "Upload eerst een
  recording.").
- **Meting Bekijken and Snapshots Bekijken**: Upload Meta, Download Meta (`.ctrl-meta`) and the
  "Laatste update" status line (`#status`) are hidden.
- **Page title** in a user mode: the heading (`#pageTitle`) and the browser tab show the choice (Patient
  Meten, Meting Bekijken, Snapshots Bekijken) instead of "Grid View".

Verified in headless Edge: 64 mode checks (titles per mode, hidden or visible meta buttons and status,
Snapshot grayed and enabled after an upload, auto-play; Admin/Onderzoeker unchanged), 105 dialog checks
(incl. auto-play after upload), plus 126 + 46 + 9. Built and flashed to COM5.

### Meting Bekijken and Snapshots Bekijken: fixed display, Snapshots Bekijken without live data
- **Both modes**: Sensor Value is forced to Norm AutoCal and Kleurnormalisatie to MaxFixed
  (`enforceUserMode`, not stored in the cookies, again after a file's meta is applied); both fieldsets
  are hidden (`hide-bekijken hide-snapshots`).
- **Snapshots Bekijken** now gets the Meting Bekijken rules: no live data (`pollLoop` skips `fetchAll`),
  all sensors enabled, and hidden Remove Offset, Enable Sensor, the red circle panel and the loadcell
  controls (Known weight, Tare, Calibrate, ?).

Verified in headless Edge: 83 mode checks (forced AutoCal/MaxFixed also after a meta with Raw/Normalised,
hidden fieldsets, no live data and all sensors enabled in Snapshots Bekijken, Patient Meten and
Admin/Onderzoeker unchanged), 126 + 105 dialog checks + 46 role checks (now run in mode meten, since
Snapshots Bekijken forces AutoCal) + 9. Built and flashed to COM5.

### Download dialogs: title on the kind question, note dialog for a calibration recording
- The kind question after Stop (or Download) now has the title "Recording downloaden", with "Wat voor
  recording is dit?" as its question; the snapshots question likewise has the title "Snapshots downloaden".
- **Kalibratiemeting** no longer downloads at once: it opens the "Recording downloaden" dialog with only the
  optional note (no patient-id). Downloading it again (after "Ja, downloaden") shows that dialog prefilled.
  `miDownloadDialog(m, type)` now takes the kind ('patient' | 'calibration' | 'other').

Verified in headless Edge: 108 dialog checks per role (calibration note dialog, nothing downloaded before
Downloaden, note in meta.measurement, prefilled on re-download), plus 83 + 126 + 46 + 9. Built and flashed
to COM5.

### hello_simcard: 4 MB write, read and delete times; app selectable via SENSORGRID_APP
- Three new steps (5-7 of now 8): write a 4 MB file, read it back, delete it, each timed
  (`testWriteBigFile`, `testReadBigFile`, `testRemoveBigFile`; `SdCard::fileExists` for the delete check).
- Card `CBADS` at 20 MHz, three runs: write 6200-6223 ms (658-660 kB/s), read 6019-6024 ms (679-680 kB/s),
  delete 9.3 ms. Results in Appendix A of `apps/hello_simcard/doc/hello_simcard.md`.
- `main.cpp`: `#elif defined(SENSORGRID_APP_HELLO_SIMCARD)`, so `idf.py -D SENSORGRID_APP=hello_simcard`
  selects the app (built in its own `build_simcard` with `sdkconfig.server`).

### sensor_v7: no false HX711 detection without a loadcell
- Without an HX711, the floating DOUT pin (GPIO8, library sets plain INPUT) often read low, so the sensor
  reported `lc=1` and sent junk raw values (0 and 0x7FFFFF), visible as a jumping loadcell weight.
- `HX711Measurement::init()` now enables the pull-up on DOUT before detection. Verified on the bare devkit
  (COM17, id 2): "HX711 not detected", stub measurement still active.

### Button pins: snapshot GPIO35 -> GPIO40, logger dump GPIO22 -> GPIO41
- Every app logged `gpio_set_direction(308): GPIO number error` (and the same for `gpio_set_pull_mode`) just
  before `app_main`: the logger in `main.cpp` used GPIO22 as dump button, which does not exist on the ESP32-S3.
  Now GPIO41 (button to GND, internal pull-up; the logger dumps on release, no dump at boot).
- Snapshot button (sensor_v7) moved from GPIO35 to GPIO40. The devkits are N16R8 (esptool: "Embedded PSRAM 8MB
  (AP_3v3)"), whose octal PSRAM is wired to GPIO35-37; fine while PSRAM is off, but not reserved for us.
- Not chosen: GPIO19/20 (native USB D-/D+, carries the console, COM port and JTAG). GPIO40/41 are only JTAG pins
  after burning an eFuse; by default they are ordinary GPIOs and no app uses them.
- Verified on the bare sensor devkit (COM17, id 2): no GPIO errors, "Snapshot button configured on GPIO40".
  The buttons themselves were not pressed (no hardware attached).
