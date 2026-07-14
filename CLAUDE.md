# Working conventions

Glossary is [CONTEXT.md](CONTEXT.md); architecture is
[docs/architecture.md](docs/architecture.md); decisions are [docs/adr/](docs/adr/);
the camera protocol is [docs/sony-ble-protocol.md](docs/sony-ble-protocol.md).
This file captures the working conventions for anyone (human or agent)
contributing.

## Development environment

The PlatformIO CLI (`pio`) is **not on PATH** — it lives in a venv. Use the full
path (adjust if yours differs):

```sh
~/.pio-venv/bin/pio run -e m5stick-c        # build
~/.pio-venv/bin/pio test -e native          # host unit tests
~/.pio-venv/bin/pio device list             # find the M5's serial port
```

Flashing needs the device's serial port (per-machine; discover with
`device list`, looks like `/dev/cu.usbserial-XXXXXXXX`):

```sh
~/.pio-venv/bin/pio run -e m5stick-c -t upload --upload-port <port>
```

**Serial monitor:** `pio device monitor` needs an interactive TTY and fails when
run non-interactively (e.g. from an agent — `termios ... not supported`). Read
the port directly with the venv's pyserial instead:

```sh
~/.pio-venv/bin/python - <<'PY'
import serial, time
s = serial.Serial('<port>', 115200, timeout=1)
end = time.time() + 20
while time.time() < end:
    line = s.readline().decode('utf-8','replace').rstrip()
    if line: print(line)
s.close()
PY
```

## Tests first

Write or update tests **before** the implementation. For a bug fix or behavior
change, adjust the native tests to express the expected behavior, confirm they
fail (red), then implement until they pass (green).

- Native tests run on the host: `pio test -e native`
- Test sources: `test/test_astro/`, `test/test_selectable_list/`
- The harness unity-builds the code under test with mocked collaborators
  (`test/mocks/`). Pure logic — the astro state machine, timings, parameter
  validation, list navigation — is unit-testable this way.
- UI drawing (`M5.Display.*`) is **not** unit-testable; verify it on-device.
- A pre-commit hook runs the native tests (enable once per clone:
  `git config core.hooksPath .githooks`).

## Do not flash without confirmation

Building and host tests are fine to run unprompted. **Never flash the device
without explicit confirmation** — the M5StickC may be mid-test or connected to a
camera. Ask first before `... -t upload`.

## Adding a screen

Screens follow one pattern (see `src/screens/`):

1. Subclass `BaseScreen<YourMenuItem>` with a `YourMenuItem` enum; own a
   `SelectableList<YourMenuItem>` for navigation.
2. Implement `updateMenuItems()`, `drawContent()`, `update()`, and the
   select/next/prev hooks. Read input via `RemoteControlManager` (never M5
   buttons directly), so web-remote buttons work too.
3. Navigate with `MenuSystem::setScreen(new OtherScreen())`. **Caution:** that
   deletes the current screen; do not touch any member after a call that may
   navigate (see `AstroScreen::handleSelect` returning a "navigated" bool and
   `update()` bailing on it — a fix for a use-after-free).
4. For frequently-repainting screens, draw into an off-screen `M5Canvas` and
   `pushSprite` to avoid flicker (ADR 0005). `SelectableList::draw(canvas)` can
   render the menu into a canvas region (ADR 0004).

## Hardware abstraction

Keep application/UI logic free of direct `M5.*` calls where practical; route
hardware access through interfaces (see the HAL notes in
[docs/architecture.md](docs/architecture.md)) so logic stays testable and
portable. Prefer existing UI primitives over new direct display manipulation.
