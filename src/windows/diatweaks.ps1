#Requires -Version 5.1
$ErrorActionPreference = 'Stop'

$Keys = @('edgeToEdge', 'squareSidebarCorners', 'hideToolbar', 'hideSidebarButton', 'hideChatButton')

function Format-Bool([bool]$Value) {
    if ($Value) { 'true' } else { 'false' }
}

function Get-Wrap {
    foreach ($dir in @($PSScriptRoot, (Join-Path $PSScriptRoot 'bin'), (Join-Path $env:LOCALAPPDATA 'DiaPatches'))) {
        $exe = Join-Path $dir 'diawrap.exe'
        if (Test-Path -LiteralPath $exe) { return $exe }
    }
    return $null
}

function Get-TweaksPath {
    $dir = Join-Path $env:LOCALAPPDATA 'DiaPatches'
    if (-not (Test-Path -LiteralPath $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }
    return (Join-Path $dir 'tweaks.json')
}

function Read-State {
    $state = @{
        edgeToEdge = $false
        squareSidebarCorners = $false
        hideToolbar = $false
        hideSidebarButton = $false
        hideChatButton = $false
        dry = $false
        inset = 6.0
    }
    $path = Get-TweaksPath
    if (-not (Test-Path -LiteralPath $path)) { return $state }
    $raw = Get-Content -LiteralPath $path -Raw
    if ([string]::IsNullOrWhiteSpace($raw)) { return $state }
    $parsed = $raw | ConvertFrom-Json
    foreach ($name in $Keys) {
        $value = $parsed.$name
        if ($null -ne $value) { $state[$name] = [bool]$value }
    }
    if ($null -ne $parsed.dry) { $state.dry = [bool]$parsed.dry }
    if ($null -ne $parsed.inset) { $state.inset = [double]$parsed.inset }
    return $state
}

function Write-State($State) {
    $path = Get-TweaksPath
    $inv = [System.Globalization.CultureInfo]::InvariantCulture
    $inset = ([double]$State.inset).ToString($inv)
    $text = @"
{
  "edgeToEdge": $(Format-Bool $State.edgeToEdge),
  "squareSidebarCorners": $(Format-Bool $State.squareSidebarCorners),
  "hideToolbar": $(Format-Bool $State.hideToolbar),
  "hideSidebarButton": $(Format-Bool $State.hideSidebarButton),
  "hideChatButton": $(Format-Bool $State.hideChatButton),
  "dry": $(Format-Bool $State.dry),
  "inset": $inset
}

"@
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($path, $text.Replace("`r`n", "`n"), $utf8)
}

function Show-LastRun {
    $last = Join-Path $env:LOCALAPPDATA 'DiaPatches\last-run.txt'
    if (Test-Path -LiteralPath $last) {
        Write-Output (Get-Content -LiteralPath $last -Raw).TrimEnd()
    }
}

function Invoke-Wrap([string[]]$WrapArgs) {
    $exe = Get-Wrap
    if (-not $exe) {
        Write-Error 'diawrap.exe was not found. Run src\windows\install.ps1.'
        exit 1
    }
    $p = Start-Process -FilePath $exe -ArgumentList (@($WrapArgs) + @('--from-ps')) -Wait -PassThru -WindowStyle Hidden
    Show-LastRun
    return $p.ExitCode
}

function Show-Help {
    Write-Output @'
DiaTweaks toggles. Settings live in %LOCALAPPDATA%\DiaPatches\tweaks.json.

  diatweaks.ps1 status
  diatweaks.ps1 on <key>
  diatweaks.ps1 off <key>
  diatweaks.ps1 dry on
  diatweaks.ps1 dry off
  diatweaks.ps1 launch

Keys: edgeToEdge, squareSidebarCorners, hideToolbar, hideSidebarButton, hideChatButton.
Square corners apply only while edgeToEdge is on. Edit "inset" in the json file to change the padding that edge-to-edge removes (default 6).
'@
}

$cmd = if ($args.Count -gt 0) { $args[0] } else { 'help' }

switch ($cmd) {
    'help' { Show-Help }
    'status' {
        $state = Read-State
        foreach ($name in $Keys) {
            Write-Output ('{0}: {1}' -f $name, (Format-Bool $state[$name]))
        }
        Write-Output ('dry: {0}' -f (Format-Bool $state.dry))
        Write-Output ('inset: {0}' -f $state.inset)
        $code = Invoke-Wrap @('status')
        exit $code
    }
    'launch' {
        $code = Invoke-Wrap @('launch')
        exit $code
    }
    'dry' {
        if ($args.Count -lt 2 -or ($args[1] -ne 'on' -and $args[1] -ne 'off')) {
            Write-Error 'usage: diatweaks.ps1 dry on|off'
            exit 2
        }
        $state = Read-State
        $state.dry = $args[1] -eq 'on'
        Write-State $state
        Write-Output ('dry: {0}' -f (Format-Bool $state.dry))
        if ($state.dry) {
            Write-Output 'The next Dia (Tweaks) launch logs decisions and does not change the window.'
        }
    }
    { $_ -eq 'on' -or $_ -eq 'off' } {
        if ($args.Count -lt 2 -or $Keys -notcontains $args[1]) {
            Write-Error ('usage: diatweaks.ps1 {0} <{1}>' -f $cmd, ($Keys -join '|'))
            exit 2
        }
        $state = Read-State
        $state[$args[1]] = $cmd -eq 'on'
        Write-State $state
        Write-Output ('{0}: {1}' -f $args[1], (Format-Bool $state[$args[1]]))
        $exe = Get-Wrap
        if (-not $exe) {
            Write-Output 'Saved. Install DiaTweaks, then open Dia (Tweaks).'
            exit 0
        }
        $code = Invoke-Wrap @('inject')
        if ($code -ne 0) {
            Write-Output 'Saved. Open Dia (Tweaks) to apply it. The normal Dia icon does not load DiaTweaks.'
        }
        exit 0
    }
    default {
        Show-Help
        exit 2
    }
}
