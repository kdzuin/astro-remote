# The M5 runs two BLE roles at once: camera client and remote server

AstroRemote acts simultaneously as a BLE **client** to the Sony camera (the
camera link, `BLEDeviceManager`) and a BLE **server** advertising as `M5Remote`
(the remote link, `BLERemoteServer`) so an external web client can drive it.
Both call `BLEDevice::init()` on the single ESP32 radio during setup. The result
is a relay: web client → M5 → camera.

We chose this over making the browser talk to the camera directly because the
browser cannot speak Sony's protocol or hold the bond, and astro sequences run
for hours — the M5 must own the camera link and the sequence state machine
regardless of whether a phone is attached.

## Consequences

- One radio serves both links; advertising for the remote link coexists with an
  active camera-client connection. This is fragile and has shown up as
  "web scan finds nothing" until a clean boot.
- The web client only knows the `180F10xx` remote-link service; it has no
  knowledge of the Sony protocol.
