@echo off
setlocal
rem build_sensor.bat - bouwt en flasht sensor_v7 voor de ESP32-S3-sensormodules met 8 MB flash
rem (eigen build-map build_sensor, eigen sdkconfig.sensor en partitions_sensor_8mb.csv).
rem
rem gebruik:  build_sensor.bat [actie] [poort] [sensor-id]
rem   actie:     build (standaard) | flash | monitor | flash-monitor | size | menuconfig | fullclean
rem   poort:     COM-poort van de sensormodule, standaard COM6
rem   sensor-id: 1, 2, 3 ... (SENSOR_ID in sensor_v7_ino.h); zonder opgave geldt de waarde in dat bestand
rem
rem voorbeelden:  build_sensor.bat
rem               build_sensor.bat flash-monitor COM6 1
rem
rem De ESP-IDF-omgeving (esp54) wordt hier zelf geladen; de app wordt gekozen via
rem -D SENSORGRID_APP=sensor_v7 en het id via -D SENSORGRID_SENSOR_ID (zie main/CMakeLists.txt).

set ACTION=%~1
if "%ACTION%"=="" set ACTION=build
set PORT=%~2
if "%PORT%"=="" set PORT=COM6
set SENSOR_ID=%~3

set HERE=%~dp0
set HERE=%HERE:\=/%
set BUILD_DIR=%HERE%build_sensor
set SDKCONFIG=%HERE%sdkconfig.sensor
set DEFAULTS=%HERE%sdkconfig.defaults;%HERE%sdkconfig.sensor.defaults
set IDF_EXPORT=%USERPROFILE%\esp54\esp-idf\export.bat

cd /d "%~dp0"
if not exist "%IDF_EXPORT%" (
  echo [build_sensor] ESP-IDF niet gevonden: %IDF_EXPORT%
  exit /b 1
)
call "%IDF_EXPORT%" >nul 2>&1
if errorlevel 1 (
  echo [build_sensor] export.bat mislukt
  exit /b 1
)

set COMMON=-B "%BUILD_DIR%" -D SDKCONFIG="%SDKCONFIG%" -D SDKCONFIG_DEFAULTS="%DEFAULTS%" -D SENSORGRID_APP=sensor_v7
if not "%SENSOR_ID%"=="" set COMMON=%COMMON% -D SENSORGRID_SENSOR_ID=%SENSOR_ID%

if not exist "%HERE%sdkconfig.sensor" (
  echo [build_sensor] eerste keer: set-target esp32s3 met %DEFAULTS%
  idf.py %COMMON% set-target esp32s3
  if errorlevel 1 exit /b 1
)

if "%ACTION%"=="build"         goto build
if "%ACTION%"=="flash"         goto flash
if "%ACTION%"=="monitor"       goto monitor
if "%ACTION%"=="flash-monitor" goto flashmonitor
if "%ACTION%"=="size"          goto size
if "%ACTION%"=="menuconfig"    goto menuconfig
if "%ACTION%"=="fullclean"     goto fullclean
echo [build_sensor] onbekende actie: %ACTION%
exit /b 2

:build
idf.py %COMMON% build
exit /b %errorlevel%
:flash
idf.py %COMMON% -p %PORT% flash
exit /b %errorlevel%
:monitor
idf.py %COMMON% -p %PORT% monitor
exit /b %errorlevel%
:flashmonitor
idf.py %COMMON% -p %PORT% flash monitor
exit /b %errorlevel%
:size
idf.py %COMMON% size
exit /b %errorlevel%
:menuconfig
idf.py %COMMON% menuconfig
exit /b %errorlevel%
:fullclean
idf.py %COMMON% fullclean
exit /b %errorlevel%
