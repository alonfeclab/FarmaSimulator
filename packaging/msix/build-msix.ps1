<#
    Genera el instalador MSIX de FarmaciaSim a partir de un build Release
    ya compilado (con windeployqt ya ejecutado como paso POST_BUILD).

    Uso:
        .\packaging\msix\build-msix.ps1
        .\packaging\msix\build-msix.ps1 -BuildDir ..\build-release-msix -Version 1.0.0.0

    Requiere el Windows SDK (makeappx.exe, signtool.exe) y PowerShell con
    permisos para crear un certificado autofirmado en Cert:\CurrentUser\My
    (primera ejecución) o reutilizarlo (siguientes ejecuciones).
#>
param(
    [string]$BuildDir = "$PSScriptRoot\..\..\build-release-msix",
    [string]$Version = "1.0.0.2",
    [string]$CertSubject = "CN=FarmaciaSim"
)

$ErrorActionPreference = "Stop"

$RepoRoot   = Resolve-Path "$PSScriptRoot\..\.."
$MsixDir    = "$PSScriptRoot"
$Staging    = "$MsixDir\staging"
$OutDir     = "$MsixDir\out"
$AssetsSrc  = "$RepoRoot\app_icon.ico"

# Ruta al Windows SDK: se usa la versión más reciente instalada.
$sdkRoot = "C:\Program Files (x86)\Windows Kits\10\bin"
$sdkVersion = Get-ChildItem $sdkRoot -Directory |
    Where-Object { $_.Name -match '^\d+\.\d+\.\d+\.\d+$' } |
    Sort-Object { [Version]$_.Name } -Descending |
    Select-Object -First 1
if (-not $sdkVersion) { throw "No se encontro el Windows SDK en $sdkRoot" }
$sdkBin = "$sdkRoot\$($sdkVersion.Name)\x64"
$makeappx = "$sdkBin\makeappx.exe"
$signtool = "$sdkBin\signtool.exe"
if (-not (Test-Path $makeappx)) { throw "No se encontro makeappx.exe en $sdkBin" }
if (-not (Test-Path $signtool)) { throw "No se encontro signtool.exe en $sdkBin" }

if (-not (Test-Path "$BuildDir\FarmaciaSim.exe")) {
    throw "No se encontro $BuildDir\FarmaciaSim.exe. Compila primero en Release (ver README-msix)."
}

Write-Host "== Preparando carpeta de staging ==" -ForegroundColor Black
if (Test-Path $Staging) { Remove-Item $Staging -Recurse -Force }
New-Item -ItemType Directory -Path $Staging | Out-Null
New-Item -ItemType Directory -Path $OutDir -Force | Out-Null

# --- Copiar el runtime desplegado por windeployqt (sin artefactos de build) ---
Copy-Item "$BuildDir\FarmaciaSim.exe" $Staging
Copy-Item "$BuildDir\*.dll" $Staging
foreach ($dir in @("platforms", "imageformats", "iconengines", "networkinformation", "tls", "generic", "qml")) {
    if (Test-Path "$BuildDir\$dir") {
        Copy-Item "$BuildDir\$dir" "$Staging\$dir" -Recurse
    }
}

# --- Generar los iconos PNG requeridos por el manifest a partir del .ico ---
Write-Host "== Generando iconos (Assets) ==" -ForegroundColor Black
Add-Type -AssemblyName System.Drawing
$assetsDir = "$Staging\Assets"
New-Item -ItemType Directory -Path $assetsDir | Out-Null

function New-IconAsset {
    param(
        [string]$IcoPath,
        [string]$OutPath,
        [int]$CanvasW,
        [int]$CanvasH,
        [int]$IconSize,
        [string]$BackgroundColor = "Transparent"
    )
    $srcIcon = New-Object System.Drawing.Icon($IcoPath, 256, 256)
    $srcBmp = $srcIcon.ToBitmap()

    $canvas = New-Object System.Drawing.Bitmap($CanvasW, $CanvasH)
    $g = [System.Drawing.Graphics]::FromImage($canvas)
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    if ($BackgroundColor -ne "Transparent") {
        $brush = New-Object System.Drawing.SolidBrush([System.Drawing.ColorTranslator]::FromHtml($BackgroundColor))
        $g.FillRectangle($brush, 0, 0, $CanvasW, $CanvasH)
        $brush.Dispose()
    } else {
        $g.Clear([System.Drawing.Color]::Transparent)
    }
    $x = [int](($CanvasW - $IconSize) / 2)
    $y = [int](($CanvasH - $IconSize) / 2)
    $g.DrawImage($srcBmp, $x, $y, $IconSize, $IconSize)
    $g.Dispose()
    $canvas.Save($OutPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $canvas.Dispose()
    $srcBmp.Dispose()
    $srcIcon.Dispose()
}

$brand = "#123f31"
# Fondo horneado en el propio PNG (no transparente): algunas superficies de
# Windows, como la tarjeta de vista previa de App Installer, no respetan el
# BackgroundColor del manifest y pintan su propio color de acento genérico
# detrás de un logo transparente (se ve cian en vez del verde de marca).
New-IconAsset -IcoPath $AssetsSrc -OutPath "$assetsDir\Square44x44Logo.png"   -CanvasW 44  -CanvasH 44  -IconSize 40  -BackgroundColor $brand
New-IconAsset -IcoPath $AssetsSrc -OutPath "$assetsDir\Square150x150Logo.png" -CanvasW 150 -CanvasH 150 -IconSize 130 -BackgroundColor $brand
New-IconAsset -IcoPath $AssetsSrc -OutPath "$assetsDir\StoreLogo.png"         -CanvasW 50  -CanvasH 50  -IconSize 44  -BackgroundColor $brand
New-IconAsset -IcoPath $AssetsSrc -OutPath "$assetsDir\Wide310x150Logo.png"   -CanvasW 310 -CanvasH 150 -IconSize 130 -BackgroundColor $brand
New-IconAsset -IcoPath $AssetsSrc -OutPath "$assetsDir\SplashScreen.png"      -CanvasW 620 -CanvasH 300 -IconSize 200 -BackgroundColor $brand

# --- Manifest (sustitucion literal de marcadores, sin regex, para no tocar
#     el "version" de la propia declaracion <?xml ... ?>) ---
$manifestContent = Get-Content "$MsixDir\AppxManifest.xml" -Raw
$manifestContent = $manifestContent.Replace('__VERSION__', $Version).Replace('__PUBLISHER__', $CertSubject)
$utf8NoBom = New-Object System.Text.UTF8Encoding($false)
[System.IO.File]::WriteAllText("$Staging\AppxManifest.xml", $manifestContent, $utf8NoBom)

# --- Indice de recursos (resources.pri): necesario para que Windows valide
#     y sirva las imagenes declaradas en el manifest (Square44x44Logo, etc.)
#     por su nombre; sin el, algunas superficies del shell no confian en las
#     imagenes tal cual y sustituyen su fondo por uno generico propio. ---
Write-Host "== Generando resources.pri ==" -ForegroundColor Cyan
$makepri = "$sdkBin\makepri.exe"
if (-not (Test-Path $makepri)) { throw "No se encontro makepri.exe en $sdkBin" }
$priConfig = "$Staging\priconfig.xml"
& $makepri createconfig /cf $priConfig /dq en-US /o | Out-Null
& $makepri new /pr $Staging /cf $priConfig /of "$Staging\resources.pri" /o | Out-Null
if ($LASTEXITCODE -ne 0) { throw "makepri fallo (codigo $LASTEXITCODE)" }
Remove-Item $priConfig -Force

# --- Certificado autofirmado (se reutiliza si ya existe uno con el mismo Subject) ---
Write-Host "== Certificado de firma ($CertSubject) ==" -ForegroundColor Cyan
$cert = Get-ChildItem Cert:\CurrentUser\My -CodeSigningCert |
    Where-Object { $_.Subject -eq $CertSubject } |
    Sort-Object NotAfter -Descending | Select-Object -First 1

if (-not $cert) {
    Write-Host "No existe certificado previo; generando uno nuevo..." -ForegroundColor Yellow
    $cert = New-SelfSignedCertificate `
        -Type Custom `
        -Subject $CertSubject `
        -KeyUsage DigitalSignature `
        -FriendlyName "FarmaciaSim (MSIX, autofirmado)" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3", "2.5.29.19={text}") `
        -NotAfter (Get-Date).AddYears(5)
}

$cerPath = "$OutDir\FarmaciaSim.cer"
Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null

# --- Empaquetar ---
Write-Host "== Empaquetando con makeappx ==" -ForegroundColor Cyan
$msixPath = "$OutDir\FarmaciaSim.msix"
if (Test-Path $msixPath) { Remove-Item $msixPath -Force }
& $makeappx pack /d $Staging /p $msixPath /overwrite
if ($LASTEXITCODE -ne 0) { throw "makeappx fallo (codigo $LASTEXITCODE)" }

# --- Firmar ---
Write-Host "== Firmando el paquete ==" -ForegroundColor Cyan
& $signtool sign /fd SHA256 /sha1 $cert.Thumbprint $msixPath
if ($LASTEXITCODE -ne 0) { throw "signtool fallo (codigo $LASTEXITCODE)" }

Write-Host ""
Write-Host "Listo:" -ForegroundColor Green
Write-Host "  Paquete:     $msixPath"
Write-Host "  Certificado: $cerPath  (instalar como de confianza antes de instalar el paquete)"
Write-Host ""
Write-Host "Para instalar en este PC u otro:" -ForegroundColor Cyan
Write-Host "  1) Import-Certificate -FilePath `"$cerPath`" -CertStoreLocation Cert:\LocalMachine\TrustedPeople  (requiere PowerShell como administrador)"
Write-Host "  2) Add-AppxPackage -Path `"$msixPath`""
