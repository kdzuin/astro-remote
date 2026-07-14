# Bulb exposures are timed shutter toggles, not a held button

The Sony BT remote protocol has no "expose for N seconds" command, and unlike
the physical shutter button it cannot press-and-hold. With the camera in Bulb
mode, a full-press+release *toggles* the shutter. So AstroRemote implements a
bulb frame as **two toggles** — open, wait the exposure time on the M5, close —
with the exposure duration owned by the remote's timer rather than the camera.

This is why `AstroProcess` drives `CameraCommands::triggerBulb()` twice per
frame and why `triggerBulb` is a bare press+release (`0x0109` then `0x0108`)
rather than a "take a photo" workflow.

## Consequences

- The camera **must be set to Bulb exposure mode**. In other modes the same
  command takes a single normal photo; the remote cannot detect or enforce the
  dial position, so this is an unenforceable user precondition.
- Exposure accuracy is bounded by the remote's 1-second tick, not the camera.
