// Pure astro-status logic for the AstroRemote web client: decode the BLE
// status packet, format timers, and interpolate between the device's ~1 Hz
// notifications. No DOM access — unit-tested with `node --test`
// (see astro-status.test.js), imported by ble.js for rendering.

// Mirrors AstroProcess::State (astro.h). Broadcast as a single byte.
export const ASTRO_STATE = Object.freeze({
  IDLE: 0,
  INITIAL_DELAY: 1,
  EXPOSING: 2,
  INTERVAL: 3,
  PAUSED: 4,
  STOPPED: 5,
  ERROR: 6,
});

// Packed AstroStatusPacket size (ble_remote_server.h): 1 + 2 + 2 + 4*6 + 1 + 1.
export const ASTRO_STATUS_PACKET_BYTES = 31;

const STATE_LABELS = Object.freeze({
  [ASTRO_STATE.IDLE]: "Idle",
  [ASTRO_STATE.INITIAL_DELAY]: "Initial delay",
  [ASTRO_STATE.EXPOSING]: "Exposing",
  [ASTRO_STATE.INTERVAL]: "Interval",
  [ASTRO_STATE.PAUSED]: "Paused",
  [ASTRO_STATE.STOPPED]: "Stopped",
  [ASTRO_STATE.ERROR]: "Error",
});

// A running sequence's timers tick; PAUSED and terminal states are frozen.
const RUNNING_STATES = new Set([
  ASTRO_STATE.INITIAL_DELAY,
  ASTRO_STATE.EXPOSING,
  ASTRO_STATE.INTERVAL,
]);

// Decode a little-endian AstroStatusPacket from a DataView (the value handed
// to us by the Web Bluetooth `characteristicvaluechanged` event).
export function decodeAstroStatus(view) {
  if (!view || view.byteLength < ASTRO_STATUS_PACKET_BYTES) {
    throw new RangeError(
      `astro status packet too short: ${view ? view.byteLength : 0} < ${ASTRO_STATUS_PACKET_BYTES}`,
    );
  }
  return {
    state: view.getUint8(0),
    completedFrames: view.getUint16(1, true),
    totalFrames: view.getUint16(3, true),
    sequenceStartTime: view.getUint32(5, true),
    currentFrameStartTime: view.getUint32(9, true),
    elapsedSec: view.getUint32(13, true),
    remainingSec: view.getUint32(17, true),
    phaseRemainingSec: view.getUint32(21, true),
    phaseTotalSec: view.getUint32(25, true),
    isCameraConnected: view.getUint8(29) !== 0,
    errorCode: view.getUint8(30),
  };
}

export function stateLabel(state) {
  return STATE_LABELS[state] ?? "Unknown";
}

// Whole seconds → "mm:ss". mm is not capped at 59 (a >1h sequence reads
// "61:01"); negatives clamp to zero.
export function formatMMSS(totalSec) {
  const s = Math.max(0, Math.floor(totalSec));
  const mm = Math.floor(s / 60);
  const ss = s % 60;
  return `${String(mm).padStart(2, "0")}:${String(ss).padStart(2, "0")}`;
}

// Progress-bar fill in 0..1. total===0 (idle phase) → 0, never NaN.
export function barFraction(elapsed, total) {
  if (!total || total <= 0) return 0;
  const f = elapsed / total;
  if (f < 0) return 0;
  if (f > 1) return 1;
  return f;
}

// Advance a decoded status by the wall-clock ms elapsed since it arrived, so
// the UI ticks smoothly between the device's ~1 Hz packets. The next real
// packet re-snaps everything (it is authoritative). Frozen unless running.
export function interpolate(status, msSincePacket) {
  if (!RUNNING_STATES.has(status.state)) return status;
  const delta = Math.floor(Math.max(0, msSincePacket) / 1000);
  if (delta === 0) return status;
  return {
    ...status,
    elapsedSec: status.elapsedSec + delta,
    remainingSec: Math.max(0, status.remainingSec - delta),
    phaseRemainingSec: Math.max(0, status.phaseRemainingSec - delta),
  };
}
