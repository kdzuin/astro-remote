# Sony Alpha BLE remote protocol (as used by AstroRemote)

Reference for the Sony camera side of the **camera link** (the M5 acting as BLE
client to a Sony Alpha). Reverse-engineered / observed, not from an official
spec. Each fact is tagged:

- **[verified]** — observed on the test camera via serial this session
- **[code]** — encoded in the firmware constants but not re-confirmed live
- **[unverified]** — assumption; confirm on a clean run

Test camera BT address seen this session: `d0:40:ef:39:fd:76`.

## Discovery

The camera is identified during a BLE scan by its **manufacturer advertisement
data**, not by service UUID:

- Sony company ID `0x012D` **[verified]** — matched against the first two
  advertisement manufacturer bytes (little-endian).
- Camera type byte `0x03` **[unverified]** — third manufacturer byte. Observed
  on the test camera; may be model-specific. If a different Alpha advertises a
  different type byte it will be silently skipped by discovery.

## Pairing & security

- Bonded, encrypted LE connection. Authentication mode `SC_MITM_BOND`;
  encryption required. **[code]**
- Passkey is fixed at `000000` (`onPassKeyRequest` returns it). No user PIN
  entry on the remote. **[code]**
- On first pair the **camera may show a confirmation screen** — **[unverified]**,
  to be checked on a clean run.
- Persistence: the peer address is stored in NVS (`preferences`, key
  `device_address`); the BLE bond keys are kept in the ESP32 bond store.
  Reconnect to a saved address re-authenticates without re-pairing
  (`Authentication Success` seen after a full reflash). **[verified]**

## GATT layout

Service `8000FF00-FF00-FFFF-FFFF-FFFFFFFFFFFF` **[verified]** with:

| Characteristic | UUID | Direction | Purpose |
|---|---|---|---|
| Remote control | `0xFF01` | write | command bytes to the camera |
| Status notify  | `0xFF02` | notify | status stream from the camera |
| Status read    | `0000cc05-0000-1000-8000-00805f9b34fb` | read | initial status, read once on connect |

## Commands (write to `0xFF01`)

Two byte formats:
- **16-bit**: `[MSB, LSB]` — e.g. shutter/record.
- **24-bit**: `[MSB, LSB, PARAM]` — e.g. zoom/focus, PARAM is step intensity.
  For manual focus, `0x00` releases; press steps verified on-device from `0x01`
  (smallest) up to `0x7F`.

Command codes (all **[code]** unless noted):

| Code | Meaning |
|---|---|
| `0x0106` / `0x0107` | shutter half-press release / press |
| `0x0108` / `0x0109` | shutter full-press release / press **[verified]** (used by bulb toggle) |
| `0x010E` / `0x010F` | record stop / start |
| `0x0114` / `0x0115` | autofocus release / start |
| `0x0120` / `0x0121` | C1 release / press |
| `0x0244`–`0x0247` | zoom tele/wide release/press (24-bit, +intensity) |
| `0x026a`–`0x026d` | manual focus in/out release/press (24-bit, +intensity) |

## Bulb exposure (the important one)

The protocol has **no "expose for N seconds" command**. Physical Bulb is
press-and-hold; the BT remote cannot hold. Instead, with the camera in **Bulb
exposure mode**, a full-press+release **toggles** the shutter:

```
0x0109 (full-press down) then 0x0108 (full-press up)  = one "bulb toggle"
  toggle 1 -> shutter opens   (status 02 A0 20)
  ...remote times the exposure...
  toggle 2 -> shutter closes  (status 02 A0 00)
```

**[verified]** end-to-end via serial: toggle→`02 A0 20`, later toggle→`02 A0 00`.

Consequences:
- A **frame** = two toggles; the exposure duration is owned by the remote's
  timer, not the camera.
- The camera **must be in Bulb mode**. In other exposure modes the same
  press+release takes a single normal photo — the remote cannot detect or
  enforce the dial position. **[unverified]** precondition.

See [ADR 0001](adr/0001-bulb-as-timed-shutter-toggles.md).

## Status notifications (from `0xFF02`)

3-byte frames: `[0x02, TYPE, VALUE]`.

- Prefix byte `0x02`.
- `VALUE` `0x20` = on/active, `0x00` = off/ready.

| TYPE | Meaning | on (`0x20`) | off (`0x00`) | Status |
|---|---|---|---|---|
| `0xA0` | shutter | active (open) | ready (closed) | **[verified]** |
| `0x3F` | focus | acquired | lost | **[unverified]** |
| `0xD5` | recording | started | stopped | **[code]** |

Shutter status is the source of truth for whether an exposure is really
happening; the astro sequence state is only the remote's timer-driven belief.
See [ADR 0003](adr/0003-guard-bulb-toggle-on-shutter-status.md).
