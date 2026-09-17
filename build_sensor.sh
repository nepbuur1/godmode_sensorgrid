#!/usr/bin/env bash
# build_sensor.sh - bouwt en flasht sensor_v7 voor de ESP32-S3-sensormodules met 8 MB flash
# (bash/WSL-variant van build_sensor.bat; eigen build-map build_sensor, sdkconfig.sensor,
# partitions_sensor_8mb.csv).
#
# gebruik:  ./build_sensor.sh [actie] [poort] [sensor-id]
#   actie:     build (standaard) | flash | monitor | flash-monitor | size | menuconfig | fullclean
#   poort:     seriële poort van de sensormodule, standaard /dev/ttyACM1
#   sensor-id: 1, 2, 3 ... (SENSOR_ID in sensor_v7_ino.h); zonder opgave geldt de waarde in dat bestand
#
# De ESP-IDF-omgeving wordt geladen uit $IDF_EXPORT (standaard ~/esp54/esp-idf/export.sh).
set -euo pipefail

ACTION="${1:-build}"
PORT="${2:-/dev/ttyACM1}"
SENSOR_ID="${3:-}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IDF_EXPORT="${IDF_EXPORT:-$HOME/esp54/esp-idf/export.sh}"

if [ ! -f "$IDF_EXPORT" ]; then
  echo "[build_sensor] ESP-IDF niet gevonden: $IDF_EXPORT" >&2
  exit 1
fi
# shellcheck disable=SC1090
. "$IDF_EXPORT" >/dev/null

cd "$HERE"
COMMON=(-B "$HERE/build_sensor" -D "SDKCONFIG=$HERE/sdkconfig.sensor"
        -D "SDKCONFIG_DEFAULTS=$HERE/sdkconfig.defaults;$HERE/sdkconfig.sensor.defaults"
        -D SENSORGRID_APP=sensor_v7)
if [ -n "$SENSOR_ID" ]; then
  COMMON+=(-D "SENSORGRID_SENSOR_ID=$SENSOR_ID")
fi

if [ ! -f "$HERE/sdkconfig.sensor" ]; then
  echo "[build_sensor] eerste keer: set-target esp32s3 met sdkconfig.defaults + sdkconfig.sensor.defaults"
  idf.py "${COMMON[@]}" set-target esp32s3
fi

case "$ACTION" in
  build)         idf.py "${COMMON[@]}" build ;;
  flash)         idf.py "${COMMON[@]}" -p "$PORT" flash ;;
  monitor)       idf.py "${COMMON[@]}" -p "$PORT" monitor ;;
  flash-monitor) idf.py "${COMMON[@]}" -p "$PORT" flash monitor ;;
  size)          idf.py "${COMMON[@]}" size ;;
  menuconfig)    idf.py "${COMMON[@]}" menuconfig ;;
  fullclean)     idf.py "${COMMON[@]}" fullclean ;;
  *) echo "[build_sensor] onbekende actie: $ACTION" >&2; exit 2 ;;
esac
