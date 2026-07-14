# Guard the closing bulb toggle on reported shutter status

Because a bulb frame is a *toggle* (see ADR 0001), sending a toggle when the
shutter is already closed would **re-open** it — the opposite of the intent.
`AstroProcess::stopExposure()` therefore only sends the closing toggle when the
camera's reported shutter status (`CameraCommands::isShutterActive()`, from the
`0xFF02` notification stream) says the shutter is actually open. If it is
already closed (external stop, timeout, a missed toggle), we skip the toggle.

This is viable because the camera reliably reports shutter open/closed
(`02 A0 20` / `02 A0 00`), verified over serial. Without the guard, any desync
between the remote's sequence state and the real shutter state would invert
every subsequent toggle and silently ruin the sequence.

## Consequences

- The camera's shutter status is treated as the source of truth for closing;
  the sequence state is only the remote's timer-driven belief.
- The *opening* toggle in `startExposure()` is not yet guarded — it assumes the
  shutter is closed at frame start (true on clean INTERVAL/DELAY transitions).
