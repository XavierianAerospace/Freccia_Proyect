@echo off
setlocal
REM Script para ejecutar TileServer-GL
REM Sirve las tiles vectoriales de MBTiles como PNG raster

set "BASE_DIR=%~dp0"
set "MBTILES=%BASE_DIR%maps\maptiler-osm-2020-02-10-v3.11-planet.mbtiles"
set "TILESERVER_BIN=%APPDATA%\npm\tileserver-gl.cmd"

if not exist "%MBTILES%" (
    set "MBTILES="
    for %%F in ("%BASE_DIR%maps\*.mbtiles") do (
        set "MBTILES=%%~fF"
        goto :mbtiles_found
    )
)

:mbtiles_found
if not defined MBTILES (
    echo.
    echo ERROR: No se encontro ningun archivo .mbtiles en:
    echo   %BASE_DIR%maps
    echo.
    pause
    exit /b 1
)

if not exist "%TILESERVER_BIN%" (
    set "TILESERVER_BIN=tileserver-gl"
)

echo.
echo ============================================================
echo INICIANDO TILESERVER-GL
echo ============================================================
echo.
echo Endpoint: http://127.0.0.1:8080
echo.
echo Tiles disponibles en:
echo   http://127.0.0.1:8080/styles/basic-preview/{z}/{x}/{y}.png
echo.
echo Abre en navegador:
echo   http://127.0.0.1:8080
echo.
echo Para detener: CTRL+C
echo.
echo ============================================================
echo.

cd /d "%BASE_DIR%"
"%TILESERVER_BIN%" "%MBTILES%"

pause
