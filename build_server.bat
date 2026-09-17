@echo off
setlocal
rem build_server.bat - bouwt en flasht server_v7 voor de ESP32-S3 met 16 MB flash
rem (eigen build-map build_server, eigen sdkconfig.server en partitions_server_16mb.csv).
rem
rem gebruik:  build_server.bat [actie] [poort]
rem   actie: build (standaard) | flash | monitor | flash-monitor | size | menuconfig | fullclean
rem   poort: COM-poort van de server-devkit (USB JTAG/serial debug unit), standaard COM5
rem
rem voorbeelden:  build_server.bat
rem               build_server.bat flash-monitor COM5
rem
rem De ESP-IDF-omgeving (esp54) wordt hier zelf geladen; de app wordt gekozen via
rem -D SENSORGRID_APP=server_v7 (zie main/CMakeLists.txt en main/main.cpp).

set ACTION=%~1
if "%ACTION%"=="" set ACTION=build
set PORT=%~2
if "%PORT%"=="" set PORT=COM5

set HERE=%~dp0
set HERE=%HERE:\=/%
set BUILD_DIR=%HERE%build_server
set SDKCONFIG=%HERE%sdkconfig.server
set DEFAULTS=%HERE%sdkconfig.defaults;%HERE%sdkconfig.server.defaults
set IDF_EXPORT=%USERPROFILE%\esp54\esp-idf\export.bat

cd /d "%~dp0"
if not exist "%IDF_EXPORT%" (
  echo [build_server] ESP-IDF niet gevonden: %IDF_EXPORT%
  exit /b 1
)
call "%IDF_EXPORT%" >nul 2>&1
if errorlevel 1 (
  echo [build_server] export.bat mislukt
  exit /b 1
)

set COMMON=-B "%BUILD_DIR%" -D SDKCONFIG="%SDKCONFIG%" -D SDKCONFIG_DEFAULTS="%DEFAULTS%" -D SENSORGRID_APP=server_v7

if not exist "%HERE%sdkconfig.server" (
  echo [build_server] eerste keer: set-target esp32s3 met %DEFAULTS%
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
echo [build_server] onbekende actie: %ACTION%
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
