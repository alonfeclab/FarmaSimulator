@echo off
rem ============================================================
rem  Crea una carpeta portable de FarmaciaSim con esta pinta:
rem     FarmaciaSim-portable\
rem        FarmaciaSim.exe      (lanzador, parece que esta solo)
rem        bin\                 (OCULTA: programa real + DLLs + datos)
rem  Requisito: haber compilado en modo Release desde Qt Creator.
rem ============================================================

set QT=C:\Qt6\6.11.1\mingw_64
set PROYECTO=%~dp0
set BUILD=%PROYECTO%..\build-FarmaciaSim-Desktop_Qt_6_11_1_MinGW_64_bit-Release
set DESTINO=C:\dev\FarmaciaSim-portable

if not exist "%BUILD%\FarmaciaSim.exe" (
    echo No encuentro %BUILD%\FarmaciaSim.exe
    echo Compila primero en Release, o edita la variable BUILD de este script.
    pause
    exit /b 1
)

rem Limpiar despliegue anterior
if exist "%DESTINO%" rmdir /s /q "%DESTINO%"
mkdir "%DESTINO%\bin"

rem ---- Programa real + DLLs de Qt en bin\
copy /y "%BUILD%\FarmaciaSim.exe" "%DESTINO%\bin\" >nul
"%QT%\bin\windeployqt.exe" --release --compiler-runtime ^
    --qmldir "%PROYECTO%qml" "%DESTINO%\bin\FarmaciaSim.exe"

rem ---- Lanzador visible (ya compilado; si falta, se intenta compilar)
if exist "%PROYECTO%launcher\FarmaciaSim-launcher.exe" (
    copy /y "%PROYECTO%launcher\FarmaciaSim-launcher.exe" "%DESTINO%\FarmaciaSim.exe" >nul
) else (
    for /d %%M in (C:\Qt6\Tools\mingw*_64) do set MINGW=%%M
    if defined MINGW (
        "%MINGW%\bin\g++.exe" -O2 -static -municode -mwindows ^
            "%PROYECTO%launcher\launcher.cpp" -o "%DESTINO%\FarmaciaSim.exe"
    ) else (
        echo AVISO: no hay lanzador; la app esta en bin\FarmaciaSim.exe
    )
)

rem ---- Ocultar la carpeta bin
attrib +h "%DESTINO%\bin"

echo.
echo Portable creado en: %DESTINO%
echo Veras solo FarmaciaSim.exe; la carpeta 'bin' esta oculta.
echo (Para verla: Explorador ^> Vista ^> Elementos ocultos)
pause
