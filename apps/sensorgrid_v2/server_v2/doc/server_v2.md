# server_v2

## Summary
Server node app for the sensorgrid. Runs a WiFi access point and actively polls sensor nodes for data using ESP-NOW. Operates a state machine: first discovers and registers all expected sensors, then polls them in round-robin order. Serves a web dashboard showing real-time bar charts for up to 8 sensors, and provides a JSON API endpoint for programmatic access. Flashes the onboard LED when any sensor is missing.

## Object Model

![server_v2 object model](img/server_v2_object_model.svg)

### Object List

| Object | Stereotype | Responsibility |
|--------|-----------|---------------|
| **ServerNode** | control | Orchestrates the server: runs the DISCOVERING/POLLING/WAITING_DATA state machine, manages sensor registration, sends POLL requests, processes DATA responses, handles sensor recovery, controls the LED, and serves the web dashboard. |
| **WiFi** | boundary | Represents the ESP32-S3 WiFi hardware in AP+STA mode. Provides the access point that web clients connect to and the channel for ESP-NOW communication. |
| **EspNow** | boundary | Represents the ESP-NOW protocol layer. Broadcasts DISCOVER, sends unicast POLL to sensors, and receives REGISTER and DATA messages via callback. |
| **WebServer** | boundary | Represents the HTTP server. Serves the HTML dashboard on `/` and the sensor data JSON API on `/api/sensors`. |

## Call Trees

### init()
- ! init()
  - ! neopixelWrite(RGB_BUILTIN, 0, 0, 0)
  - ! WiFi.mode(WIFI_AP_STA)
  - ! WiFi.softAP(ssid, pass, channel)
  - ! server.on("/", handleRoot)
  - ! server.on("/api/sensors", handleApiSensors)
  - ! server.onNotFound(handleNotFound)
  - ! server.begin()
  - ! esp_now_init()
  - ! esp_now_register_recv_cb(onDataRecv)
  - ! esp_now_register_send_cb(onDataSent)
  - ! esp_now_add_peer(broadcastPeer)

### update()
- ! update()
  - ! server.handleClient()
    - ? server.send(INDEX_HTML)
    - ? handleApiSensors()
      - ! server.send(json)
    - ? server.send(404, "Not found")
  - ! updateLed()
    - ? neopixelWrite(red/off)
  - ? handleDiscovering()
    - ! processRegister()
    - ? broadcastDiscover()
  - ? handlePolling()
    - ! processRegister()
    - ? broadcastDiscover()
    - ! ensureSensorPeer(id)
    - ! esp_now_send(PollPacket)
  - ? handleWaitingData()
    - ! processRegister()
    - ? updateSensorState(id, value)
    - ? retryPoll(id)
    - ? markUnregistered(id)

### onDataRecv() (ESP-NOW callback)
- ! onDataRecv(info, data, len)
  - ? set newRegisterReceived + register data
  - ? set newDataReceived + DataPacket buffer
