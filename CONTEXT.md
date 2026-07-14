# Project Context — AstroRemote

## What it is

Firmware for the **M5StickC** (ESP32 with a small LCD and a few buttons) that
turns the device into a **Bluetooth remote control for Sony Alpha cameras**. It
is aimed primarily at **astrophotography and long-exposure work** (bulb-mode
exposure sequences), with general photo/video remote features as well.

Written in C++, built with **PlatformIO** on the **Arduino framework**, using
the **M5Unified** library for hardware access.

This file is the project **glossary**. For how the code is organized and how it
runs, see [docs/architecture.md](docs/architecture.md); for design decisions,
[docs/adr/](docs/adr/); for the camera protocol,
[docs/sony-ble-protocol.md](docs/sony-ble-protocol.md); for how to build, test,
flash, and extend, see [CLAUDE.md](CLAUDE.md).

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

**Phase**:
The current segment of a running **Sequence**'s timeline — one of *initial
delay*, *exposing* (shutter open for the current **Frame**), or *interval* (the
wait before the next Frame). These are the state machine's non-idle states. The
remote's *phase* progress tracks time within the current segment; the *sequence*
progress tracks the whole run.
_Avoid_: step, stage, subexposure (the exposing phase **is** the Frame's open
period, not a distinct thing — and "sub" is reserved for the Frame).

**Shutter status**:
The camera's own report of whether the shutter is open (active) or closed
(ready), delivered as a BLE status notification. The source of truth for
whether an exposure is actually happening — distinct from the sequence state,
which is the remote's timer-driven belief.
_Avoid_: exposure state, shooting flag.

**Camera link** vs **Remote link**:
Two separate BLE connections. The *camera link* is the M5 acting as BLE client
to the Sony camera. The *remote link* is the M5 acting as BLE server to an
external client (the web remote).
_Avoid_: "the connection" (ambiguous — always say which link).
