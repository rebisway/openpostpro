# Builds the Windows release: the Release build, the zip and the installer (MSI) of the version of src\common\version.h.
# The files are written in the folder which contains the repository (OpenPostPro-v<version>-win64.zip and .msi).
# Usage: tools\build-release.ps1 [-NoBuild] [-NoMsi]
param([switch]$NoBuild, [switch]$NoMsi)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$proj = Join-Path $repo 'openpostpro'
$release = Join-Path $proj 'x64\Release'

# the version is the one of the program
$header = Get-Content (Join-Path $proj 'src\common\version.h') -Raw
$parts = 'MAJOR', 'MINOR', 'PATCH' | ForEach-Object { [regex]::Match($header, "APP_VERSION_$_\s+(\d+)").Groups[1].Value }
$version = $parts -join '.'
$name = "OpenPostPro-v$version-win64"
$out = Split-Path $repo -Parent
if (Get-Process openpostpro -ErrorAction SilentlyContinue) { throw 'openpostpro is running: quit it first.' }

if (-not $NoBuild) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    $msbuild = & $vswhere -latest -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
    & $msbuild (Join-Path $proj 'openpostpro.vcxproj') /p:Configuration=Release /p:Platform=x64 /m /v:m /nologo
    if ($LASTEXITCODE -ne 0) { throw 'build failed' }
}

# the folder of the release: the program, its resources, the documentation with the examples, the licenses
$stage = Join-Path $env:TEMP "opp_release\$name"
if (Test-Path (Split-Path $stage)) { Remove-Item -Recurse -Force (Split-Path $stage) }
New-Item -ItemType Directory -Force $stage | Out-Null
Copy-Item (Join-Path $release 'openpostpro.exe'), (Join-Path $release 'freetype.dll') $stage
# the C++ runtime beside the program (the three libraries it imports): nothing to install, no administrator rights, the same in the zip
$vsroot = & (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe') -latest -property installationPath
$crt = Get-ChildItem (Join-Path $vsroot 'VC\Redist\MSVC\*\x64\Microsoft.VC*.CRT') -Directory | Sort-Object FullName | Select-Object -Last 1
if ($null -eq $crt) { throw 'the redistributable C++ runtime of Visual Studio was not found' }
foreach ($dll in 'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll') { Copy-Item (Join-Path $crt.FullName $dll) $stage }
$resources = @{ 'lang' = 'lang'; 'fonts' = 'fonts'; 'src\shaders' = 'shaders'; 'textures' = 'textures'; 'postpro' = 'postpro'; 'script' = 'script'; 'simulator' = 'simulator' }
foreach ($source in $resources.Keys) {
    $target = Join-Path $stage $resources[$source]
    New-Item -ItemType Directory -Force $target | Out-Null
    Get-ChildItem (Join-Path $proj $source) -File | Where-Object { $_.Extension -ne '.pyc' } | Copy-Item -Destination $target
}
Copy-Item -Recurse (Join-Path $repo 'docs') (Join-Path $stage 'docs')
Copy-Item (Join-Path $repo 'LICENSE'), (Join-Path $repo 'THIRD_PARTY_LICENSES.md'), (Join-Path $repo 'README.md'), (Join-Path $repo 'README.fr.md') $stage

$zip = Join-Path $out "$name.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path $stage -DestinationPath $zip -CompressionLevel Optimal
"{0} ({1:N1} MB)" -f $zip, ((Get-Item $zip).Length / 1MB)

if (-not $NoMsi) {
    # WiX 5 (free) is installed once, in a folder of its own
    $wixDir = Join-Path $env:LOCALAPPDATA 'OpenPostProBuild\wix'
    if (-not (Test-Path (Join-Path $wixDir 'wix.exe'))) {
        $shared = Join-Path $env:LOCALAPPDATA 'FreeSpeechBuild\wix'
        if (Test-Path (Join-Path $shared 'wix.exe')) { $wixDir = $shared } else { dotnet tool install --tool-path $wixDir wix --version 5.0.2 }
    }
    $msi = Join-Path $out "$name.msi"
    & (Join-Path $wixDir 'wix.exe') build (Join-Path $PSScriptRoot 'installer\Package.wxs') -arch x64 -d "Version=$version" -d "StageDir=$stage" -d "IconFile=$(Join-Path $proj 'textures\openpostpro.ico')" -o $msi
    if ($LASTEXITCODE -ne 0) { throw 'wix build failed' }
    "{0} ({1:N1} MB)" -f $msi, ((Get-Item $msi).Length / 1MB)
}
