# Instructions for claude, google-assist and cgpt-codex

## Attached devices
Three ESP32-S3 devices are attached. They can be programmed using:
- idf.py -p /dev/ttyACM0 flash monitor
- idf.py -p /dev/ttyACM1 flash monitor
- idf.py -p /dev/ttyACM2 flash monitor

The first one will be referred to as the "collector". The second one as "sensor_1".
The third one as "sensor_2".

## Development phases
Please, follow the development phases in this document.
Do that, while adhering the software development guidelines as specified in Software_dev_guidelines.md in this folder.

### Phase 1
In the folder _not_part_of_this_project_reference_for_inspiration,
you can find a previous project that I created for esp32 (not esp32-s3).

In this phase, I'd like this project to be converted to esp32-s3:
- in the apps folder an app called collector_phase1 a corresponding app called sensor_v1.
- de code of sensor_v1 is used to program both the sensor_1 and the sensor_2 device.
  (after changing a #define for unique id).
- the original "sensor_node" was reading adc input. for now, change that, such that a number is sent which is modulo 1024, but incremented by 10*sensor_index for every message.(something like this for sensor_1: n+=10; send(n%1024), and for sensor_2:
n+=20; send(n%1024)).
- after you have succesfully created the apps and programmed the attached devices with it, notify that everything is ready.
- provide the instruction of how I can login on the hotspot to see the webpage.
- provide the instruction of how I can view the monitor output of the collector, via which I should be able to verify via the logs that it's working properly.
- of course, I also should be able to see it by logging in on the hotspot.

#### Phase 1b (already completed)
I've made some updates to guidelines for coding and documenting. I'd like you to take into account those updates for our current sensor/collector project. 
- You'll see I request a "parent app" folder within the apps folder. Please call it "sensorgrid_v1". 
- Then put the current sensor_v1 folder within it, and also the
  collector_phase1 folder, but rename it server_v1 (from hereon, I'd rather talk about the server than the collector.
- Please update any files to synchronize that). When you're ready, please don't forget to test if everything still builds and properly executes. (everything mentioned above can still be seen as part of phase 1 - a slight refactoring, and adding docs, without altering functionality)

#### Phase 1c
- Notice that I have added a fourth esp32-s3 device, which can be programmed using:
`idf.py -p /dev/ttyACM3 flash monitor`.
- Create an app for it within sensorgrid_v1, called "client_v1". The client_v1 device, app, should do following:
- Log in on the wifi hotspot of server_v1. Test the webpage(s) that it serves (with all functionalities), in a similar way as a human would test it via a browser.
- in its docs, so in sensorgrid_v1/client_v1/docs, add a test.md document that summarizes the tests that were done and their results.
- within the apps of sensorgrid_v1, in the docs, there are nice refers to .svg files in img folder. but the .svg files in question (which should be converted versions of the corresponding files in the mermaid folder) are missing. So I think a mermaid to svg tool should be downloaded, installed and used to fix that. Please also summarize how you did that in Log.md.

#### Phase 1d
Perhaps you can add a docs folder (with subfolders img and mermaid again) to sensorgrid_v1 as well, and add sensorgrid_v1.doc that summarises the complete sensorgrid_v1 - the role of it's apps, how they interact. This time including an "object model" where each of the apps is represented as an object, and along the arrows, the communications between them. (this is not a real object model - the stereo types within each object can be omitted, in this case).

### Phase 2
In phase 1, we created the project sensorgrid_v1, which consists of the cooperating apps client_v1, sensor_v1 and server_v1.
In this phase we start a new project, named sensorgrid_v2, which will have a folder structure that is similar to that of v1, but with client_v2, sensor_v2 and server_v2, in this case. sensorgrid_v1 can be looked at for ideas, but sensorgrid_v2 will be different in the way that server and sensors setup their communications and communicate. in sensorgrid_v1, sensors tried to send their message whenever they wanted. that'll probably go wrong when using a lot of sensors, and when the sensors send large datapackets (at the same moment or overlapping moment).
#### Phase 2a
Planning. Propose a planning of how sensorgrid_v2 could be made without the limitation stated above. Perhaps somehow, the server device could handshake with the client devices via espnow. It "says" to sensor1 metaphorically: go ahead, send what you've got. only then, sensor1 sends its data to the server. if the data is too much for a single packet, it may be distributed over multiple packets. when the server has received all datapackets from sensor1 (or it time-outs), it repeats the above for sensor2, etc. In advance, the server knows how much sensors there are, so it does not need to wait for non-existing sensor ids.
Is this idea viable? Or do you have suggestions for improvement of this plan?
#### Phase 2b
Implement sensorgrid_v2 with the following polling-based ESP-NOW protocol.

##### Protocol overview
The server operates in two phases:

**DISCOVER/REGISTER phase (initial startup only):**
- The server knows the expected number of sensors in advance (configured constant).
- The server broadcasts a `DISCOVER` message periodically.
- Each sensor that receives it replies with a `REGISTER` message containing its sensor ID. The server records the sensor ID and the sender's MAC address (available for free in the ESP-NOW receive callback metadata).
- The server stays in this phase until all expected sensors have registered. There is no timeout — it waits indefinitely.
- Once all sensors are registered, the server transitions to the POLL/DATA phase.

**POLL/DATA phase (normal operation):**
- The server polls each registered sensor in round-robin order.
- For each sensor, the server sends a unicast `POLL` message to that sensor's MAC address.
- The sensor responds with one or more `DATA` packets. Each DATA packet contains: sensor ID, packet index, total packet count, and payload.
- When a sensor receives a POLL, it extracts the server's MAC from the receive callback and auto-adds it as a peer if not already present. This allows sensors to recover transparently from a power cycle without needing re-registration in the normal case.
- After receiving all DATA packets from a sensor, the server moves to the next one.

**Sensor unresponsive — recovery without blocking healthy sensors:**
- If a sensor does not respond to a POLL, the server retries the POLL up to 5 more times.
- If the sensor still does not respond after 5 retries, the server marks it as "unregistered" but stays in the POLL/DATA phase.
- The server continues polling all remaining healthy sensors. Data from working sensors keeps flowing.
- Between poll cycles (after polling all healthy sensors), the server broadcasts a single `DISCOVER` to attempt re-registration of the missing sensor.
- Once the missing sensor responds with `REGISTER`, it is included in the next poll cycle.

**Onboard RGB LED status indicator:**
- LED off: all expected sensors are registered and responding. A healthy system shows no visible LED activity.
- LED flashing (~1 Hz): one or more sensors are not yet registered or have become unresponsive. This applies both during initial startup and during normal operation.

##### Packet types

| Type | Direction | Contents |
|------|-----------|----------|
| `DISCOVER` | server -> broadcast | message type |
| `REGISTER` | sensor -> server | message type, sensor ID |
| `POLL` | server -> sensor (unicast) | message type, sensor ID |
| `DATA` | sensor -> server | message type, sensor ID, packet index, total packets, payload |

##### What carries over from v1
- WiFi AP+STA mode on server (for web dashboard + ESP-NOW)
- WiFi STA mode on sensors (for ESP-NOW)
- Web dashboard with real-time bar charts and `/api/sensors` JSON API
- Download button (CSV export)
- client_v2 test app adapted for v2 endpoints
- Simulated sensor values (same incrementing pattern as v1)
- `neopixelWrite(RGB_BUILTIN, 0, 0, 0)` to turn off LED at startup (LED is then used only for status flashing)

##### Folder structure
```
apps/sensorgrid_v2/
    sensorgrid_common/    (shared packet definitions)
    sensor_v2/src/        (sensor app)
    server_v2/src/        (server app)
    client_v2/src/        (test client app)
    doc/                  (system-level docs, object models, mermaid, SVGs)
```
Each app gets its own `doc/` folder with object model, call trees, and (for client_v2) test results.
