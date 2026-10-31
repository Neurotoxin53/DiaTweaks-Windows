# DiaTweaks

Small tweaks for the [Dia](https://www.diabrowser.com) browser. Unofficial and not affiliated with The Browser Company.

- Edge-to-edge web content
- Square corners by the sidebar
- Hide toolbar
- Hide sidebar button
- Hide Chat button

macOS and Windows are separate. The macOS patch does not run on Windows, and the Windows launcher does not change the macOS build.

## macOS

Apple Silicon, macOS 14 or later. Turn the tweaks on and off in **Dia → Settings → Patches**. They stay in place when Dia updates.

### Install

1. Install Dia into Applications, if it isn't there already.
2. Install Apple's developer tools: `xcode-select --install`
3. Run:

   ```sh
   git clone https://github.com/JaceThings/DiaTweaks.git
   cd DiaTweaks
   ./install.sh
   ```

Dia quits and reopens. When macOS asks to access "Dia Safe Storage", enter your password and click **Always Allow**.

### How it works

- Everything is built on your Mac from this source. The only download is Dia itself, from The Browser Company's own update feed, and it's used only if it's signed by The Browser Company and passes Gatekeeper.
- An untouched copy of the official Dia is kept in `~/Library/Application Support/DiaPatches/pristine`. Restoring puts it back.
- The patched Dia gets one extra line that loads the add-on, its built-in updater is switched off (DiaTweaks updates it instead, keeping the patches), and it's re-signed with a certificate created on your Mac, in your login keychain, called "DiaPatches Local Signing". Nothing is signed with anyone else's certificate.
- The add-on only rearranges Dia's own interface. It doesn't read your pages, history or passwords, and its only network use is checking Dia's update feed.
- Nothing runs in the background. Updates are checked when Dia opens and installed when you quit it.
- Because the app is now signed by you, macOS asks once per Dia update before Dia can read "Dia Safe Storage". Dia's built-in passkey support is off, since only The Browser Company can sign for it; signing in with a password works.

### Uninstall

In Dia, open **Settings → Patches** and click **Restore…**. Dia reopens as the original app with its own updates.

Or from Terminal:

```sh
~/Library/Application\ Support/DiaPatches/bin/diapatch uninstall --relaunch --purge
```

## Windows

Dia for Windows is a WinUI 3 app (Swift, via the Windows App SDK) with Chromium in `ArcCore.dll`. Build 1.4.0.424 is the MSIX package `TheBrowserCompany.Dia`. Windows keeps that package read-only and signed, so DiaTweaks does not edit it, does not turn off Dia's updater, and does not add a service or a startup task.

What you get is a Start menu shortcut, **Dia (Tweaks)**. It starts the installed Dia and loads `diatweaks.dll` into that `Dia.exe` only. Quit Dia and the add-on is gone. The normal Dia shortcut is unchanged.

Chromium flags, enterprise policies, and extensions do not move these controls. Dia does accept Chromium flags (the macOS build does), and it runs Manifest V3 extensions, but the toolbar, sidebar button, and chat button are WinUI elements in `NavigationBar.xbf`, not Chromium's toolbar. Repacking the MSIX would break its signature, so that is not the approach.

### Requirements

- Windows 11 x64 (the package's minimum OS is build 22000)
- Dia installed for your user
- No administrator account

The checked layout is Dia 1.4.0.424 x64. ARM64 is not built.

### Install

From this repo:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\src\windows\install.ps1
```

That copies `diawrap.exe`, `diatweaks.dll`, and `diatweaks.ps1` to `%LOCALAPPDATA%\DiaPatches`, adds the shortcut, and starts Dia. Pass `-NoLaunch` to skip the start. Nothing is downloaded.

### Toggles

```powershell
& "$env:LOCALAPPDATA\DiaPatches\diatweaks.ps1" status
& "$env:LOCALAPPDATA\DiaPatches\diatweaks.ps1" on hideToolbar
& "$env:LOCALAPPDATA\DiaPatches\diatweaks.ps1" off hideToolbar
```

Keys: `edgeToEdge`, `squareSidebarCorners`, `hideToolbar`, `hideSidebarButton`, `hideChatButton`.

The file is `%LOCALAPPDATA%\DiaPatches\tweaks.json`. All five start off, so Dia looks unchanged until you opt in. Square corners stay off unless edge-to-edge is also on, same as the macOS pane.

`inset` in that file is the window padding to remove, in device-independent pixels. It starts at 6, which is the macOS padding. The Windows XAML does not publish that number. If `%LOCALAPPDATA%\DiaPatches\diatweaks.log` shows a different margin, change `inset`.

`dry` logs the decision and does not change the window:

```powershell
& "$env:LOCALAPPDATA\DiaPatches\diatweaks.ps1" dry on
```

While Dia was started from **Dia (Tweaks)**, a saved toggle is picked up within about a second. `on` and `off` also load the add-on into a Dia that is already open. Opening Dia from the normal icon does not load it.

### Uninstall

Quit Dia if a file is reported in use, then:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\src\windows\uninstall.ps1
```

That deletes `%LOCALAPPDATA%\DiaPatches` and the shortcut. The Dia package is left as Windows installed it.

### How it works

On Dia's UI thread the add-on calls `Window.Current` and `ContentIsland.FindAllForCurrentThread`, then reads each island's `AppData` and `CustomProperties` through WinRT. It does not call through window userdata. There is no public list of `Window` or `XamlIsland` objects when `Window.Current` is null, so those two properties are the only supported way from the island back to a XAML element. With `dry` on, a changed tree is dumped once. A failed lookup is logged once, then retried at 1s, 2s, 4s, up to 30s, and stops after about three minutes until a new top-level window appears. Each lookup logs `step …` before the call and `discovery finished` when it returns. If the log does not end on `discovery finished` or `shutdown`, the next launch skips discovery and logs `previous run crashed; disabled` until `tweaks.json` is saved again. A disabled run does not write `shutdown`, so quitting Dia does not clear that skip. The log is cut back to empty at 1 MB.

| Tweak | What it changes |
| --- | --- |
| Hide sidebar button | Collapses `SidebarToggleButton` (the label in the XAML is "Toggle sidebar") |
| Hide Chat button | Collapses `ChatButton` (the Windows label is "Chat") |
| Hide toolbar | Collapses `NavigationBarHost`. Experimental, as on macOS |
| Edge-to-edge web content | Clears a margin or padding of about `inset` DIP on the outer content host (`ContentGroup`, `ContentHost`, `VariantHost`, or a root with that padding). Skipped while chat is open (`AssistantPanelHost`) |
| Square corners by the sidebar | Clears the corner radius on the sidebar side of a large content border. Needs edge-to-edge on |

It does not read pages, history, or passwords. It does not use the network. After Dia quits, nothing of DiaTweaks is running.

### What was not run on Windows

`make -C src/windows test` checks which element gets which change. On Dia 1.4.0.424 the first build found `WinUIDesktopWin32WindowClass` and `Microsoft.UI.Content.DesktopChildSiteBridge` but UI Automation did not return a XAML root. This build asks `ContentIsland.FindAllForCurrentThread` on that UI thread. Edge-to-edge and square corners still need the new log from a live window. If a name is missing, that tweak does nothing and a line is written to the log.

### Risks

- **Vanguard.** The launcher calls `CreateRemoteThread` to load the DLL into `Dia.exe` only. There is no kernel driver, no service, no `AppInit_DLLs`, and no system-wide hook. Inside Dia, a hook is attached to Dia's own UI thread so WinUI calls run there. Vanguard can still treat a remote-thread load as suspicious while Valorant is running. Quit Valorant before starting **Dia (Tweaks)** if you want to avoid that.
- **Unsigned files.** `diawrap.exe` and `diatweaks.dll` are not signed. SmartScreen or Defender may warn. They are the files this repo builds.
- **Dia updates.** Dia keeps the MSIX updater. An update can rename the XAML elements; the add-on then leaves the window alone. The normal Dia shortcut never loads the DLL.
- **Package signature.** The installed files are not edited, so Windows does not re-sign the package.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). PRs need a screenshot or screen recording of the change.
