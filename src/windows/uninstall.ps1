#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

if ($env:OS -ne 'Windows_NT') {
    Write-Error 'Uninstall the Windows launcher on Windows. macOS keeps using diapatch uninstall.'
    exit 1
}

$dest = Join-Path $env:LOCALAPPDATA 'DiaPatches'
$lnk = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs\Dia (Tweaks).lnk'
if (Test-Path -LiteralPath $lnk) { Remove-Item -LiteralPath $lnk -Force }

if (Test-Path -LiteralPath $dest) {
    try {
        Remove-Item -LiteralPath $dest -Recurse -Force -ErrorAction Stop
    } catch {
        Write-Error "Could not remove $dest. Quit Dia so diatweaks.dll is no longer loaded, then run uninstall again."
        exit 1
    }
}

Write-Output 'DiaTweaks removed. The installed Dia package was not changed.'
