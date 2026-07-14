# M5StickC Sony Alpha Camera Remote

A Bluetooth remote control for Sony Alpha cameras using M5StickC, specifically designed for astrophotography and long exposure shots, and some nice benefits of remote controlling your camera.

## Features

- Bluetooth connection to Sony Alpha cameras
- Astro Remote
  - Bulb mode control for exposures longer than 30 seconds
  - Adjustable exposure time
  - Auto-stop functionality
  - Connection status display
  - Current exposure time display
- Photo Remote
  - Ability to use as a remote control
  - Photo Counter
- Video Remote
  - Ability to use as a remote control to start/stop video recording
  - Tally Light with recording time
- Manual Control Section
  - Explicit Camera actions, that are a low-level interface for interaction with the camera
- Settings
  - Control the brightness of remote control device screen
  - Remote control battery level (in case is not connected to a power source)
  - Auto-Connect functionality to a last saved device

## Optional Web-based remote control for your remote control

The project also includes a web-based remote control, that can we hosted anywhere since it's just a static files not requiring any build process, or can be accessed from the GitHub pages for this repository.

## Hardware Requirements

- M5StickC, or any other M5 device supported by M5Unified
- Abstract Hardware layer supports further integration with different boards/devices
- Sony Alpha camera with Bluetooth remote capability

## Project Architecture

See [docs/architecture.md](docs/architecture.md) for how the code is organized
and how it runs, [CONTEXT.md](CONTEXT.md) for the glossary, and
[docs/adr/](docs/adr/) for the design decisions.

A hardware-abstraction layer (`IDisplay` / `IInput` / `IHardware`) for
testability and multi-board support is a **goal, not yet implemented** — the
code currently calls `M5.*` directly. See the hardware-abstraction notes in
docs/architecture.md for the current state and the seams already in place.

## Setup Instructions

### PlatformIO Setup

Prerequisites: **Python 3**, **git**, and a **USB data cable** for the M5StickC
(the M5StickC enumerates over an FTDI USB-serial chip; macOS/Linux need no extra
driver, Windows may need the FTDI VCP driver).

PlatformIO can be used either as a standalone CLI or via the VSCode extension.

**CLI (recommended for build/test/flash):**

```sh
git clone <this repo> && cd AstroRemote
python3 -m venv ~/.pio-venv           # isolated env for the pio CLI
~/.pio-venv/bin/pip install platformio
git config core.hooksPath .githooks   # enable the native-test pre-commit hook

~/.pio-venv/bin/pio run -e m5stick-c  # first build downloads the ESP32
                                      # platform + toolchain — slow, be patient
```

Flashing needs the device's serial port — see
[CLAUDE.md](CLAUDE.md#development-environment) for finding it, the flash command,
and reading serial output.

**VSCode:** install the PlatformIO IDE extension, open the repo folder, and use
its Build / Upload buttons. The extension bundles its own `pio`.

### Tests

Device-independent logic (`AstroProcess`, `SelectableList`) has native unit
tests that run on the host:

```sh
pio test -e native
```

A pre-commit hook runs these before every commit. Enable it once per clone:

```sh
git config core.hooksPath .githooks
```

Bypass in an emergency with `git commit --no-verify`.

### VSCode Configuration

To ensure proper code navigation and eliminate false errors:

1. Create/update `.vscode/c_cpp_properties.json`:

   ```json
   {
     "configurations": [
       {
         "name": "PlatformIO",
         "includePath": ["${workspaceFolder}/**", "${workspaceFolder}/src"],
         "forcedInclude": ["${workspaceFolder}/src/debug.h"]
       }
     ],
     "version": 4
   }
   ```

2. Key settings:
   - `includePath`: Ensures proper header resolution
   - `forcedInclude`: Handles global debug.h inclusion
   - PlatformIO will handle all ESP32/Arduino includes automatically

### Zed Configuration

Zed uses `clangd` for C++, which reads a `compile_commands.json` instead of a
hand-written include path. This captures the exact build flags (including the
force-included `debug.h`), so navigation and diagnostics match the real build.

1. Generate the compilation database (re-run after changing `platformio.ini`
   or adding files):

   ```sh
   pio run -e m5stick-c -t compiledb
   ```

   This writes `compile_commands.json` to the project root. It contains
   machine-specific absolute paths and is git-ignored.

2. The repo ships `.zed/settings.json` pointing `clangd` at that database:

   ```json
   {
     "lsp": {
       "clangd": {
         "arguments": [
           "--background-index",
           "--compile-commands-dir=.",
           "--query-driver=**/xtensa-esp32*",
           "--header-insertion=never"
         ]
       }
     }
   }
   ```

   - `--compile-commands-dir=.`: locate `compile_commands.json` at the root
   - `--query-driver`: let clangd query the Xtensa cross-compiler so ESP32
     system headers resolve
   - `--header-insertion=never`: stop clangd auto-adding includes

## Usage

- Button A (M5 button): Confirm current action
- Button B: Cycle between options
- Button PWR: Go back to main menu

## Development Notes

### Hardware Layer Usage

```cpp
// Example of hardware abstraction usage
auto& display = hardware->getDisplay();
display.setTextColor(display.getColor(255, 255, 255));  // White text
display.fillScreen(display.getColor(0, 0, 0));         // Black background
```

### Debug Support

The project includes a global debug header that provides logging macros. These are automatically included in all source files via the PlatformIO configuration.
