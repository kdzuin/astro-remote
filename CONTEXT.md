# Project Context — AstroRemote

## What it is

Firmware for the **M5StickC** (ESP32 with a small LCD and a few buttons) that
turns the device into a **Bluetooth remote control for Sony Alpha cameras**. It
is aimed primarily at **astrophotography and long-exposure work** (bulb-mode
exposure sequences), with general photo/video remote features as well.

Written in C++, built with **PlatformIO** on the **Arduino framework**, using
the **M5Unified** library for hardware access.

## Language

Terms specific to this project. Use these; avoid the listed alternatives.

**Bulb toggle**:
One full-press + release of the camera shutter sent over the Sony BT remote
protocol. In Bulb exposure mode this *toggles* the shutter: the first toggle
opens the exposure, the next closes it. Contrast the physical shutter button,
where Bulb is a press-and-hold. The remote cannot "hold", so it opens with one
toggle and closes with another.
_Avoid_: take bulb, bulb photo, bulb shot.

**Frame** (a.k.a. subframe):
One captured exposure in an astro sequence. A frame spans two bulb toggles
(open, then close) with the exposure time between them.
_Avoid_: shot, picture, image, sub.

**Sequence**:
The full astro run: an initial delay, then N frames each separated by an
interval. Owned by the astro state machine.
_Avoid_: session, run, program.

**Interval**:
The wait between the end of one frame and the start of the next. Distinct from
the initial delay before the first frame.
_Avoid_: gap, pause, delay (reserve "delay" for the initial pre-sequence wait).

**Shutter status**:
The camera's own report of whether the shutter is open (active) or closed
(ready), delivered as a BLE status notification. The source of truth for
whether an exposure is actually happening — distinct from the sequence state,
which is the remote's timer-driven belief.
_Avoid_: exposure state, shooting flag.

**Camera link** vs **Remote link**:
Two separate BLE connections. The *camera link* is the M5 acting as BLE client
to the Sony camera. The *remote link* is the M5 acting as BLE server to an
external client (the web remote). See the two-BLE-roles section below.
_Avoid_: "the connection" (ambiguous — always say which link).

## The key architectural idea: two BLE roles

The device runs two Bluetooth stacks at once:

1. **BLE Client** (`BLEDeviceManager`) — connects *out* to a Sony camera and
   speaks Sony's remote protocol (service `8000FF00-…`, characteristic `0xFF01`
   for write commands, `0xFF02` for status notifications; pairing passkey
   `000000`).

2. **BLE Server** (`BLERemoteServer`, advertised as `"M5Remote"`) — makes the
   M5StickC itself a peripheral, exposing a custom `180F10xx` service so an
   external client can drive the stick.

This produces a relay chain:

```
Web browser ──180F10xx──► M5StickC ──Sony 8000FF00──► Sony camera
 (webclient)            (BLERemoteServer)          (BLEDeviceManager)
```

## The web client

`src/webclient/` is a static HTML + JavaScript page (Web Bluetooth, no build
step, GitHub-Pages hostable). It is **another interface on top of the M5**, not
a browser-direct camera controller — it only knows the `180F10xx` UUIDs and has
no knowledge of the Sony protocol. It duplicates the physical stick's buttons
and astro controls on a larger screen, while the M5 keeps the camera link and
runs the exposure state machine. Useful because astro sequences run for hours
and the stick's own UI is tiny.

## Feature areas

- **Astro** — bulb-mode exposure sequences: initial delay, exposure length,
  subframe count, interval between frames; auto-stop; live progress. Driven by
  the `AstroProcess` state machine.
- **Photo** — shutter remote plus a photo counter.
- **Video** — start/stop recording with a tally light and recording timer.
- **Focus** — autofocus / manual-focus control.
- **Manual** — low-level direct camera commands.
- **Settings** — screen brightness, battery level, auto-connect to last device.
- **Scan** — BLE device discovery / pairing.

## Code layout

```
src/
  main.cpp            Arduino entry (setup/loop) → Application
  app.h               Application: init sequence, main loop, emergency-stop hook
  debug.h/.cpp        LOG_APP / LOG_PERIPHERAL / LOG_DEBUG macros (level-gated)

  transport/          Everything Bluetooth + camera protocol
    ble_device.*        BLE client → Sony camera; scan, pair, connect
    ble_remote_server.* BLE server → external clients; command + astro-status
    camera_commands.*   Sony command codes + high-level takePhoto/takeBulb/…
    remote_control_manager.*  Unified button state (physical + remote)
    button_id.h         ButtonId enum (UP/DOWN/LEFT/RIGHT/CONFIRM/BACK + HW A/B/PWR/EMERGENCY)

  processes/          Feature logic behind the screens
    astro.*             AstroProcess: singleton exposure-sequence state machine
    photo.h video.h focus.h manual.h scan.h settings.h

  screens/            UI, one per feature
    base_screen.*       BaseScreen<MenuItemType> template: status bar, connection
    main_screen.*       Root menu (menu items depend on connection state)
    astro/video/photo/focus/manual/settings/scan screens

  components/
    menu_system.*       Screen stack via type-erased IScreen / ScreenWrapper<T>
    selectable_list.*   SelectableList<IdType>: scrollable menu with separators

  utils/
    preferences.*       PreferencesManager: NVS storage (brightness, auto-connect, saved device)
    colors.h            RGB palette → M5 color format

  webclient/          Static web remote (index.html + ble.js)
```

## Control flow

`main.cpp` → `Application::setup()` initializes preferences, display, both BLE
roles, and `MenuSystem`. `Application::loop()` checks emergency-stop, then
updates BLE state, remote-control state, and the active screen each tick.

Screens are pushed/popped through `MenuSystem` (a stack of `IScreen`). Each
concrete screen extends `BaseScreen<MenuItemType>` and owns a
`SelectableList<MenuItemType>` for navigation. All button input — whether from
the physical buttons or from the web client over BLE — flows through
`RemoteControlManager`, so screens read one unified source.

The `AstroProcess` state machine
(`IDLE → INITIAL_DELAY → EXPOSING → INTERVAL → … → STOPPED`) drives bulb
exposures via `CameraCommands::takeBulb()` and pushes an `AstroStatusPacket`
back to any connected web client over BLE.

## Buttons

- **A** (M5 button): confirm current action
- **B**: cycle between options
- **PWR**: back to main menu
- Remote clients send the same button events over BLE.

## Hardware abstraction (partial / aspirational)

The README describes an `IDisplay` / `IInput` / `IHardware` abstraction layer
for testability and multi-board support. This is **not yet implemented** — no
`src/hardware/` directory exists (though `platformio.ini` reserves the include
path). Today the code calls `M5.*` directly. Input is already funneled through
`RemoteControlManager`, and BLE is isolated to `transport/`, so those seams are
close; the display layer (many direct `M5.Display.*` calls across the screens)
is the main coupling.

## Build & flash

```sh
pio run                 # build for m5stick-c
pio run -t upload       # flash over USB-C (auto-detect port)
```

Board: `m5stick-c`, platform `espressif32`, framework `arduino`, dependency
`m5stack/M5Unified`. Partition scheme `huge_app.csv`. Current firmware uses
~48% of flash and ~14% of RAM.

There is also a `native` PlatformIO environment for host-side unit tests of the
device-independent logic (`AstroProcess`, `SelectableList`) — see `test/`.

## IDE support

- **VSCode** — `.vscode/c_cpp_properties.json` (include path + forced `debug.h`).
- **Zed** — clangd via `compile_commands.json` (`pio run -t compiledb`), config
  in `.zed/settings.json`.

Both are documented in `README.md`.

## Status & known gaps

The codebase is a work in progress. Notable items:

- **`AstroProcess::status_.isCameraConnected` is read but never written**
  anywhere in the firmware. Because `start()` refuses to run when it is false,
  astro sequences currently cannot start on a real device. Needs a setter wired
  from the BLE connection state.
- **Emergency stop is a stub** — `Application::emergencyStop()` only logs.
- The pairing-code display in `MySecurity::onPassKeyNotify` is commented out.
- `.backup/camera_control.*` is superseded legacy code (pre-refactor monolith).
