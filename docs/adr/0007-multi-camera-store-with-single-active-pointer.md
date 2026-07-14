# Remembering multiple cameras via a CameraStore with a single "active" pointer

AstroRemote now remembers more than one Sony camera and lets the user pick which
one to use (one at a time, between sessions — never simultaneously). The list of
remembered cameras and which one is active lives in a pure `CameraStore` class
(`address` + `name` per camera, plus an `activeAddr`), persisted to NVS under
indexed keys (`cam_count`, `cam_addr_N`, `cam_name_N`, `active`; capped at 8).
The store's decision logic — insert-if-absent, never clobber a saved name with
an empty one, and clearing `active` when the active camera is forgotten — is
unit-tested on the host with no mocks, because it holds no hardware or NVS
dependency itself.

We kept `BLEDeviceManager::cachedAddress` as the single "active camera" pointer:
connecting any camera (via scan or via the camera list) sets it, and all the
existing reconnect paths (`connectToSavedDevice`, `checkConnection`, `isPaired`,
boot auto-connect) keep working **unchanged** because they already read
`cachedAddress`. Only pairing, the saved-list UI, and active-selection are new.

## Considered options

- **Beef up the Preferences mock to a real in-memory map and test the static
  `BLEDeviceManager` directly.** Rejected: static state leaks across tests and
  couples the store logic to the hardware singleton.
- **Store the list as one delimited blob string.** Rejected: BLE advertised
  names can contain any byte (delimiter collision), it needs a hand-rolled
  parser, and a single corrupt write loses every camera instead of one row.

## Consequences

- No migration of the old single `device_address` key — upgrading users re-pair
  once. The orphaned key is harmless.
- Because reconnect-by-address carries no advertised name, a camera's `name` is
  captured once at scan-time and never refreshed; connecting from the saved list
  must not overwrite it with an empty string.
- Forgetting the active camera clears `active` with no auto-pick, so the next
  boot does not silently auto-connect a camera the user did not choose.
