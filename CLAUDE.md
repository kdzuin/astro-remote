# Working conventions

Project overview and architecture live in [CONTEXT.md](CONTEXT.md). This file
captures the working conventions for anyone (human or agent) contributing.

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

Building and testing are fine to run unprompted:

```sh
pio run -e m5stick-c        # build
pio test -e native          # host tests
```

**Never flash the device without explicit confirmation.** The M5StickC may be
mid-test or connected to a camera. Ask first before:

```sh
pio run -e m5stick-c -t upload   # flashing — confirm first
```

## Hardware abstraction

Keep application/UI logic free of direct `M5.*` calls where practical; the goal
is to route hardware access through interfaces (see the HAL notes in
CONTEXT.md) so logic stays testable and portable. Prefer adding to the status
line / existing UI primitives over new direct display manipulation.
