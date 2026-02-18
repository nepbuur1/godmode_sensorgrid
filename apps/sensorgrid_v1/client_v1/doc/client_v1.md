# client_v1

## Summary
Automated test client for the sensorgrid server. Connects to the server's WiFi access point and validates all web endpoints by making HTTP requests, similar to how a human would test via a browser. Runs a sequence of tests on boot and reports PASS/FAIL results via the serial log.

## Object Model

![client_v1 object model](img/client_v1_object_model.svg)

### Object List

| Object | Stereotype | Responsibility |
|--------|-----------|---------------|
| **ClientNode** | control | Orchestrates the test sequence: connects to WiFi, executes HTTP tests against the server's endpoints, validates responses, and logs results. |
| **WiFi** | boundary | Represents the ESP32-S3 WiFi hardware in station mode. Connects to the server's access point. |
| **HttpClient** | boundary | Represents the HTTP protocol layer. Makes GET requests to the server and returns the response code and body. |

## Call Trees

### init()
- ! init()
  - ! WiFi.mode(WIFI_STA)
  - ! WiFi.begin(ssid, pass)

### update()
- ! update()
  - ? testDashboardPage()
    - ! httpGet("/")
    - ! logResult()
  - ? testApiSensorsStructure()
    - ! httpGet("/api/sensors")
    - ! logResult()
  - ? testSensorDataPresent()
    - ! httpGet("/api/sensors")
    - ! logResult()
  - ? testSensorValuesUpdating()
    - ! httpGet("/api/sensors")
    - ! httpGet("/api/sensors")
    - ! logResult()
  - ? testDownloadButton()
    - ! httpGet("/")
    - ! logResult()
  - ? testNotFound()
    - ! logResult()
