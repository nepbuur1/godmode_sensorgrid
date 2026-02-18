# sensor_v2

## Summary
Sensor node app for the sensorgrid. Purely reactive: responds to DISCOVER messages from the server with a REGISTER reply, and responds to POLL messages with DATA containing the current sensor value. Configurable sensor ID allows the same codebase to be flashed to multiple sensor devices, each with a unique identity.

Currently sends incrementing simulated values: `counter += 10 * sensorId; send(counter % 1024)`.

## Object Model

![sensor_v2 object model](img/sensor_v2_object_model.svg)

### Object List

| Object | Stereotype | Responsibility |
|--------|-----------|---------------|
| **SensorNode** | control | Responds to server messages: sends REGISTER on DISCOVER, sends DATA on POLL. Independently advances simulated sensor value. Manages WiFi STA mode and channel configuration. |
| **WiFi** | boundary | Represents the ESP32-S3 WiFi hardware in station mode. Provides channel selection for ESP-NOW communication. |
| **EspNow** | boundary | Represents the ESP-NOW protocol layer. Receives DISCOVER and POLL from the server, sends REGISTER and DATA back via unicast. |

## Call Trees

### init()
- ! init()
  - ! neopixelWrite(RGB_BUILTIN, 0, 0, 0)
  - ! WiFi.mode(WIFI_STA)
  - ! esp_wifi_set_channel(channel)
  - ! esp_now_init()
  - ! esp_now_register_recv_cb(onDataRecv)
  - ! esp_now_register_send_cb(onDataSent)

### update()
- ! update()
  - ? counter += 10 * sensorId
  - ? currentValue = counter % 1024

### onDataRecv() (ESP-NOW callback)
- ! onDataRecv(info, data, len)
  - ? handleDiscover(src_addr)
    - ! ensureServerPeer(mac)
    - ! esp_now_send(RegisterPacket)
  - ? handlePoll(src_addr)
    - ! ensureServerPeer(mac)
    - ! esp_now_send(DataPacket)
