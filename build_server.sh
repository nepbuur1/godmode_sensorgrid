#!/usr/bin/env bash
# build_server.sh - bouwt en flasht server_v7 voor de ESP32-S3 met 16 MB flash (bash/WSL-variant
# van build_server.bat; eigen build-map build_server, sdkconfig.server, partitions_server_16mb.csv).
#
# gebruik:  ./build_server.sh [actie] [poort]
#   actie: build (standaard) | flash | monitor | flash-monitor | size | menuconfig | fullclean
#   poort: seriële poort van de server-devkit, standaard /dev/ttyACM0
#          (in WSL eerst het USB-apparaat koppelen: usbipd attach --wsl --busid <BUSID>)
#
# De ESP-IDF-omgeving wordt geladen uit $IDF_EXPORT (standaard ~/esp54/esp-idf/export.sh).
set -euo pipefail

ACTION="${1:-build}"
PORT="${2:-/dev/ttyACM0}"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IDF_EXPORT="${IDF_EXPORT:-$HOME/esp54/esp-idf/export.sh}"

if [ ! -f "$IDF_EXPORT" ]; then
  echo "[build_server] ESP-IDF niet gevonden: $IDF_EXPORT" >&2
  exit 1
fi
# shellcheck disable=SC1090
. "$IDF_EXPORT" >/dev/null

cd "$HERE"
COMMON=(-B "$HERE/build_server" -D "SDKCONFIG=$HERE/sdkconfig.server"
        -D "SDKCONFIG_DEFAULTS=$HERE/sdkconfig.defaults;$HERE/sdkconfig.server.defaults"
        -D SENSORGRID_APP=server_v7)

if [ ! -f "$HERE/sdkconfig.server" ]; then
  echo "[build_server] eerste keer: set-target esp32s3 met sdkconfig.defaults + sdkconfig.server.defaults"
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
  *) echo "[build_server] onbekende actie: $ACTION" >&2; exit 2 ;;
esac
