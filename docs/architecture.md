# Architecture

How AstroRemote is organized and how it runs. Terms in **bold** are defined in
[../CONTEXT.md](../CONTEXT.md); design rationale lives in [adr/](adr/); the Sony
camera protocol in [sony-ble-protocol.md](sony-ble-protocol.md).

## Two BLE roles

The device runs two Bluetooth stacks at once (see
[ADR 0002](adr/0002-dual-ble-roles-camera-client-and-remote-server.md)):

1. **Camera link** — `BLEDeviceManager` is a BLE *client* connecting out to a
   Sony camera and speaking its remote protocol.
2. **Remote link** — `BLERemoteServer` (advertised as `M5Remote`) is a BLE
   *server* exposing a custom `180F10xx` service so an external client can drive
   the stick.

Relay chain:

```
Web browser ──180F10xx──► M5StickC ──Sony 8000FF00──► Sony camera
 (webclient)            (BLERemoteServer)          (BLEDeviceManager)
```

## The web client

`src/webclient/` is a static HTML + JS page (Web Bluetooth, no build step,
GitHub-Pages hostable). It is **another interface on top of the M5**, not a
browser-direct camera controller — it only knows the `180F10xx` remote-link
UUIDs, never the Sony protocol. It duplicates the stick's buttons and astro
controls on a larger screen while the M5 keeps the camera link and runs the
sequence. (It does not yet consume astro status — see Gaps.)

## Feature areas

- **Astro** — bulb-mode exposure **sequences**; driven by `AstroProcess`.
- **Photo** — shutter remote plus a photo counter.
- **Video** — start/stop recording with a tally light and timer.
- **Focus** — autofocus / manual-focus control.
- **Manual** — low-level direct camera commands.
- **Settings** — brightness, battery level, auto-connect to last device.
- **Scan** — BLE device discovery / pairing.

## Code layout

```
src/
  main.cpp            Arduino entry (setup/loop) → Application
  app_astro.h         Application: init sequence + main loop (boots into Astro)
  debug.h/.cpp        LOG_APP / LOG_PERIPHERAL / LOG_DEBUG macros (level-gated)

  transport/          Everything Bluetooth + camera protocol
    ble_device.*        BLE client → Sony camera; scan, pair, connect
    ble_remote_server.* BLE server → external clients; command + astro-status
    ble_astro_observer.h  Pushes AstroStatusPacket over the remote link
    camera_commands.*   Sony command codes + takePhoto/triggerBulb/record/…
    remote_control_manager.*  Unified button state (physical + remote)
    button_id.h         ButtonId enum (UP/DOWN/LEFT/RIGHT/CONFIRM/BACK + A/B/PWR)

  processes/          Feature logic behind the screens
    astro.*             AstroProcess: singleton exposure-sequence state machine
    photo.h video.h focus.h manual.h scan.h settings.h

  screens/            UI, one per feature
    base_screen.*       BaseScreen<MenuItemType> template: status bar, connection
    main_screen.*       Root menu (items depend on connection state)
    astro_screen.*      Astro config menu (Connect/Start/Focus/params)
    astro_run_screen.*  Astro in-progress display (canvas-backed, flash-free)
    video/photo/focus/manual/settings/scan screens

  components/
    menu_system.*       Screen stack via type-erased IScreen / ScreenWrapper<T>
    selectable_list.*   SelectableList<IdType>: scrollable menu; draw() targets
                        the display or an off-screen canvas (ADR 0004)

  utils/
    preferences.*       PreferencesManager: NVS (brightness, auto-connect, device)
    colors.h            RGB palette → M5 color format

  webclient/          Static web remote (index.html + ble.js)
```

Note: `app.h` (the pre-astro-flow entry) was removed; `main.cpp` includes
`app_astro.h`, which boots directly into the Astro screen.

## Control flow

`main.cpp` → `Application::setup()` (app_astro.h) initializes preferences,
display, both BLE roles, `AstroProcess`, and `MenuSystem`, then shows the Astro
screen. `Application::loop()` each tick: updates BLE state, remote-control
state, feeds the camera-connection flag into `AstroProcess` and ticks it, then
updates the active screen.

Screens are pushed through `MenuSystem` (a stack of `IScreen`). Each concrete
screen extends `BaseScreen<MenuItemType>` and owns a `SelectableList<MenuItemType>`.
All button input — physical or from the web client over BLE — flows through
`RemoteControlManager`, so screens read one unified source. See
[CLAUDE.md](../CLAUDE.md) for the pattern to follow when adding a screen.

The `AstroProcess` state machine
(`IDLE → INITIAL_DELAY → EXPOSING → INTERVAL → … → STOPPED`, plus `PAUSED`)
drives bulb exposures via `CameraCommands::triggerBulb()` (two toggles per
**frame** — see [ADR 0001](adr/0001-bulb-as-timed-shutter-toggles.md)) and
notifies observers on change: `BLEAstroObserver` pushes an `AstroStatusPacket`
over the remote link, and the Astro run screen refreshes its display.

## Buttons

- **A**: confirm current action
- **B**: cycle between options
- **PWR**: back to main menu (swallowed on the Astro run screen so a stray press
  cannot interrupt a running sequence)
- Remote clients send the same button events over BLE.

## Hardware abstraction (partial / aspirational)

The README describes an `IDisplay` / `IInput` / `IHardware` layer for
testability and multi-board support. Not yet implemented — no `src/hardware/`
exists (though `platformio.ini` reserves the include path). Today the code calls
`M5.*` directly. Input is funneled through `RemoteControlManager` and BLE is
isolated to `transport/`, so those seams are close; the display layer (many
`M5.Display.*` calls across screens) is the main coupling. `SelectableList::draw`
already renders to any target (ADR 0004), a step toward that seam.

## Status & gaps

The Astro flow works end-to-end on real hardware: connect, configure, run a
bulb sequence (verified against a Sony camera over serial), pause/resume, stop.

- **Web client does not consume astro status.** It only sends commands and reads
  a 1-byte feedback; it never subscribes to the astro-status characteristic, so
  the browser shows no live progress. Firmware side (packet + notify) is done.
- **Unverified protocol details** (see sony-ble-protocol.md): camera-type
  advertisement byte `0x03`, focus/record status frames, first-pair UX.
- Only the Astro run screen uses off-screen canvases
  ([ADR 0005](adr/0005-flicker-free-rendering-via-offscreen-canvases.md)); other
  screens draw directly and can flicker.
- `.backup/camera_control.*` is superseded legacy code.
