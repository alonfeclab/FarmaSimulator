# Instalador MSIX de FarmaciaSim

Genera `FarmaciaSim.msix`, firmado con un certificado autofirmado (`CN=FarmaciaSim`),
a partir de un build Release desplegado con `windeployqt`.

## Requisitos

- Qt 6.11 (MinGW 64) en `C:\Qt6\6.11.1\mingw_64`
- Windows SDK (aporta `makeappx.exe` y `signtool.exe`; ya viene con Visual Studio
  o se puede instalar suelto)
- PowerShell (Windows PowerShell 5.1 o superior)

## 1. Compilar en Release

```powershell
& "C:\Qt6\Tools\CMake_64\bin\cmake.exe" -S . -B build-release-msix -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH=C:\Qt6\6.11.1\mingw_64 `
    -DCMAKE_C_COMPILER=C:\Qt6\Tools\mingw1310_64\bin\gcc.exe `
    -DCMAKE_CXX_COMPILER=C:\Qt6\Tools\mingw1310_64\bin\g++.exe `
    -DBUILD_TESTING=OFF

& "C:\Qt6\Tools\CMake_64\bin\cmake.exe" --build build-release-msix --target FarmaciaSim
```

(Se usa el CMake de `C:\Qt6\Tools\CMake_64`, no el del sistema, porque los
módulos de Qt 6.11 requieren CMake ≥ 3.22.)

El paso `POST_BUILD` ya ejecuta `windeployqt` automáticamente (ver
`CMakeLists.txt`), así que `build-release-msix\` queda con el `.exe` y todas
las DLLs/plugins de Qt necesarios.

## 2. Empaquetar y firmar

```powershell
.\packaging\msix\build-msix.ps1
```

La primera vez crea un certificado autofirmado (`CN=FarmaciaSim`, válido 5
años) en `Cert:\CurrentUser\My` y lo reutiliza en las siguientes ejecuciones.
Genera:

- `packaging\msix\out\FarmaciaSim.msix` — el instalador, ya firmado
- `packaging\msix\out\FarmaciaSim.cer` — la parte pública del certificado,
  para marcar como confiable en cada equipo donde se instale

Para cambiar la versión del paquete: `.\packaging\msix\build-msix.ps1 -Version 1.1.0.0`

## 3. Instalar

Hace falta confiar el certificado **una vez por equipo** (requiere PowerShell
como administrador, porque toca el almacén de certificados de la máquina):

```powershell
Import-Certificate -FilePath ".\packaging\msix\out\FarmaciaSim.cer" `
    -CertStoreLocation Cert:\LocalMachine\TrustedPeople
```

Y luego instalar (esto no necesita administrador):

```powershell
Add-AppxPackage -Path ".\packaging\msix\out\FarmaciaSim.msix"
```

Alternativa sin PowerShell: doble clic en `FarmaciaSim.cer` → *Instalar
certificado* → *Equipo local* → *Colocar todos los certificados en el
siguiente almacén* → *Personas de confianza*. Luego doble clic en
`FarmaciaSim.msix` para instalar con el instalador gráfico de Windows (App
Installer).

## Notas

- Al no ser un certificado emitido por una entidad reconocida, solo instalará
  sin avisos en los equipos donde se haya importado `FarmaciaSim.cer` como
  "de confianza". En cualquier otro PC, Windows rechazará el paquete.
- Para desinstalar: `Get-AppxPackage FarmaciaSim | Remove-AppxPackage`, o
  desde *Configuración → Aplicaciones*.
- Para publicar de verdad (Microsoft Store o distribución externa sin este
  aviso de confianza), hace falta un certificado de firma de código emitido
  por una CA reconocida (p. ej. DigiCert, Sectigo) o publicar a través de la
  Store, que firma el paquete automáticamente.
