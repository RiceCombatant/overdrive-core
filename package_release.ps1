$ErrorActionPreference = "Stop"

$distRoot = "OverdriveCore_v1.1.0_Win64"
$pkgDir = Join-Path $distRoot "OverdriveCore"

if (Test-Path $distRoot) {
    Remove-Item -Recurse -Force $distRoot
}
New-Item -ItemType Directory -Path $pkgDir -Force | Out-Null

Copy-Item "build_release\OverdriveCore.exe" -Destination $pkgDir -Force
Copy-Item "build_release\SDL3.dll" -Destination $pkgDir -Force
Copy-Item -Recurse "assets" -Destination $pkgDir -Force
Copy-Item "loadout.ini" -Destination $pkgDir -Force
Copy-Item "network.ini" -Destination $pkgDir -Force
if (Test-Path "OverdriveCore_v1.0.0_Win64\OverdriveCore\MULTIPLAYER_GUIDE.md") {
    Copy-Item "OverdriveCore_v1.0.0_Win64\OverdriveCore\MULTIPLAYER_GUIDE.md" -Destination $pkgDir -Force
}

$sjis = [System.Text.Encoding]::GetEncoding(932)

$clientBat = @"
@echo off
set /p HOST_IP=接続先IPアドレスを入力してください (未入力で127.0.0.1): 
if "%HOST_IP%"=="" set HOST_IP=127.0.0.1
start "" "%~dp0OverdriveCore.exe" --connect %HOST_IP%
"@
[System.IO.File]::WriteAllText((Join-Path $pkgDir "クライアントとして接続.bat"), $clientBat, $sjis)

$gameBat = @"
@echo off
start "" "%~dp0OverdriveCore.exe"
"@
[System.IO.File]::WriteAllText((Join-Path $pkgDir "ゲーム起動.bat"), $gameBat, $sjis)

$hostBat = @"
@echo off
start "" "%~dp0OverdriveCore.exe" --host
"@
[System.IO.File]::WriteAllText((Join-Path $pkgDir "ホストとして起動.bat"), $hostBat, $sjis)

$zipPath = "OverdriveCore_v1.1.0_Win64.zip"
if (Test-Path $zipPath) {
    Remove-Item -Force $zipPath
}
Compress-Archive -Path "$distRoot\*" -DestinationPath $zipPath -Force

Write-Host "Package created successfully: $zipPath"
Get-Item $zipPath | Select-Object Name, Length
