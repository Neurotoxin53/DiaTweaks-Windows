#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

if ($env:OS -ne 'Windows_NT') {
    Write-Error 'Install DiaTweaks for Windows on Windows. macOS keeps using ./install.sh.'
    exit 1
}

$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$dll = Join-Path $here 'bin\diatweaks.dll'
$exe = Join-Path $here 'bin\diawrap.exe'
$cli = Join-Path $here 'diatweaks.ps1'
foreach ($file in @($dll, $exe, $cli)) {
    if (-not (Test-Path -LiteralPath $file)) {
        Write-Error "Missing $file. From a build machine run: make -C src/windows"
        exit 1
    }
}

$dest = Join-Path $env:LOCALAPPDATA 'DiaPatches'
New-Item -ItemType Directory -Force -Path $dest | Out-Null
Copy-Item -LiteralPath $dll -Destination (Join-Path $dest 'diatweaks.dll') -Force
Copy-Item -LiteralPath $exe -Destination (Join-Path $dest 'diawrap.exe') -Force
Copy-Item -LiteralPath $cli -Destination (Join-Path $dest 'diatweaks.ps1') -Force

$json = Join-Path $dest 'tweaks.json'
if (-not (Test-Path -LiteralPath $json)) {
    $text = @"
{
  "edgeToEdge": false,
  "squareSidebarCorners": false,
  "hideToolbar": false,
  "hideSidebarButton": false,
  "hideChatButton": false,
  "dry": false,
  "inset": 6
}

"@
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($json, $text.Replace("`r`n", "`n"), $utf8)
}

$programs = Join-Path $env:APPDATA 'Microsoft\Windows\Start Menu\Programs'
$lnkPath = Join-Path $programs 'Dia (Tweaks).lnk'
$powershell = Join-Path $env:SystemRoot 'System32\WindowsPowerShell\v1.0\powershell.exe'
$shell = New-Object -ComObject WScript.Shell
$lnk = $shell.CreateShortcut($lnkPath)
$lnk.TargetPath = $powershell
$script = Join-Path $dest 'diatweaks.ps1'
$lnk.Arguments = "-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File `"$script`" launch"
$lnk.WorkingDirectory = $dest
$lnk.Description = 'Dia with DiaTweaks'
$lnk.Save()

Write-Output "Installed to $dest"
Write-Output "Shortcut: $lnkPath"
Write-Output 'The Dia package under WindowsApps was not changed.'

if ($args -contains '-NoLaunch') { exit 0 }

& $script launch
exit $LASTEXITCODE
