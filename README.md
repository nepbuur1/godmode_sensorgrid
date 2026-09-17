# godmode_sensorgrid
sensorgrid

## Bouwen en flashen (sensorgrid_v7)

Er zijn twee soorten borden, elk met een eigen build-map, eigen `sdkconfig` en eigen
partitietabel, zodat de ene of de andere build met één commando te starten is:

| Bord | Script | Build-map | sdkconfig | Partitietabel | Flash |
|---|---|---|---|---|---|
| server (ESP32-S3 N16R8) | `build_server.bat` / `build_server.sh` | `build_server/` | `sdkconfig.server` | `partitions_server_16mb.csv` (factory-app 6 MB) | 16 MB |
| sensormodule (ESP32-S3, 8 MB) | `build_sensor.bat` / `build_sensor.sh` | `build_sensor/` | `sdkconfig.sensor` | `partitions_sensor_8mb.csv` (factory-app 3 MB) | 8 MB |

```
build_server.bat [actie] [poort]              (Windows; laadt zelf %USERPROFILE%\esp54\esp-idf\export.bat)
./build_server.sh [actie] [poort]             (bash/WSL; laadt $IDF_EXPORT of ~/esp54/esp-idf/export.sh)
build_sensor.bat [actie] [poort] [sensor-id]
./build_sensor.sh [actie] [poort] [sensor-id]

actie: build (standaard) | flash | monitor | flash-monitor | size | menuconfig | fullclean
poort: standaard COM5 (server) / COM6 (sensor) op Windows, /dev/ttyACM0 / /dev/ttyACM1 in bash
```

Voorbeelden: `build_server.bat flash-monitor COM5`, `build_sensor.bat flash COM7 1` (sensor-id 1).

Hoe het werkt:

- De eerste keer (geen `sdkconfig.server` resp. `sdkconfig.sensor`) doet het script
  `idf.py set-target esp32s3` met `sdkconfig.defaults` (gemeenschappelijk: Arduino als component,
  FreeRTOS 1000 Hz, console op USB-JTAG, eigen partitietabel) plus `sdkconfig.server.defaults`
  of `sdkconfig.sensor.defaults` (flash-grootte en partitietabel). Daarna is de gegenereerde
  `sdkconfig.<type>` leidend (`menuconfig` bewerkt die); de defaults-bestanden staan in git, de
  gegenereerde sdkconfigs en build-mappen niet.
- De app wordt gekozen met de CMake-variabele `SENSORGRID_APP` (`server_v7` / `sensor_v7` /
  `client_v7`), die `main/CMakeLists.txt` omzet in `#define SENSORGRID_APP_<NAAM>`; `main/main.cpp`
  kiest daarmee het juiste `.ino`. Zonder die variabele geldt de handmatige selectie in `main.cpp`
  zoals vroeger.
- Het sensor-id komt via `SENSORGRID_SENSOR_ID` (derde argument) in `sensor_v7_ino.h`; zonder
  argument geldt de waarde in dat bestand.
- De componenten (o.a. `espressif/arduino-esp32`) worden bij de eerste build door de
  component-manager gedownload naar `managed_components/` (vergt internet).

De webinhoud van server_v7 (`crt_Grid*.h`) wordt ontwikkeld en getest in
`../TmpWindowsWebsiteTest/` (zie de README daar) en met `sync_headers.py inject` in de headers gezet.
