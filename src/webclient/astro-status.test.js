import { test } from "node:test";
import assert from "node:assert/strict";
import {
  ASTRO_STATE,
  ASTRO_STATUS_PACKET_BYTES,
  decodeAstroStatus,
  formatMMSS,
  barFraction,
  stateLabel,
  interpolate,
} from "./astro-status.js";

// Build a packed little-endian AstroStatusPacket buffer for tests. Mirrors the
// firmware struct in ble_remote_server.h (packed, no padding).
function packStatus(f = {}) {
  const buf = new ArrayBuffer(ASTRO_STATUS_PACKET_BYTES);
  const v = new DataView(buf);
  v.setUint8(0, f.state ?? 0);
  v.setUint16(1, f.completedFrames ?? 0, true);
  v.setUint16(3, f.totalFrames ?? 0, true);
  v.setUint32(5, f.sequenceStartTime ?? 0, true);
  v.setUint32(9, f.currentFrameStartTime ?? 0, true);
  v.setUint32(13, f.elapsedSec ?? 0, true);
  v.setUint32(17, f.remainingSec ?? 0, true);
  v.setUint32(21, f.phaseRemainingSec ?? 0, true);
  v.setUint32(25, f.phaseTotalSec ?? 0, true);
  v.setUint8(29, f.isCameraConnected ?? 0);
  v.setUint8(30, f.errorCode ?? 0);
  return v;
}

test("packet size matches the 31-byte firmware struct", () => {
  assert.equal(ASTRO_STATUS_PACKET_BYTES, 31);
});

test("decodeAstroStatus reads every field little-endian", () => {
  const v = packStatus({
    state: ASTRO_STATE.EXPOSING,
    completedFrames: 3,
    totalFrames: 20,
    sequenceStartTime: 1000,
    currentFrameStartTime: 1100,
    elapsedSec: 130,
    remainingSec: 470,
    phaseRemainingSec: 18,
    phaseTotalSec: 30,
    isCameraConnected: 1,
    errorCode: 0,
  });
  const s = decodeAstroStatus(v);
  assert.equal(s.state, ASTRO_STATE.EXPOSING);
  assert.equal(s.completedFrames, 3);
  assert.equal(s.totalFrames, 20);
  assert.equal(s.sequenceStartTime, 1000);
  assert.equal(s.currentFrameStartTime, 1100);
  assert.equal(s.elapsedSec, 130);
  assert.equal(s.remainingSec, 470);
  assert.equal(s.phaseRemainingSec, 18);
  assert.equal(s.phaseTotalSec, 30);
  assert.equal(s.isCameraConnected, true);
  assert.equal(s.errorCode, 0);
});

test("decodeAstroStatus rejects a short buffer", () => {
  const short = new DataView(new ArrayBuffer(10));
  assert.throws(() => decodeAstroStatus(short));
});

test("formatMMSS zero-pads minutes and seconds", () => {
  assert.equal(formatMMSS(0), "00:00");
  assert.equal(formatMMSS(9), "00:09");
  assert.equal(formatMMSS(65), "01:05");
  assert.equal(formatMMSS(600), "10:00");
});

test("formatMMSS rolls hours into minutes (mm can exceed 59)", () => {
  assert.equal(formatMMSS(3661), "61:01");
});

test("formatMMSS clamps negatives to zero", () => {
  assert.equal(formatMMSS(-5), "00:00");
});

test("barFraction returns elapsed/total clamped to 0..1", () => {
  assert.equal(barFraction(0, 30), 0);
  assert.equal(barFraction(15, 30), 0.5);
  assert.equal(barFraction(30, 30), 1);
  assert.equal(barFraction(40, 30), 1); // over-run clamps
});

test("barFraction is 0 when total is 0 (idle phase), never NaN", () => {
  assert.equal(barFraction(0, 0), 0);
  assert.equal(barFraction(5, 0), 0);
});

test("stateLabel maps each state to a human label", () => {
  assert.equal(stateLabel(ASTRO_STATE.IDLE), "Idle");
  assert.equal(stateLabel(ASTRO_STATE.INITIAL_DELAY), "Initial delay");
  assert.equal(stateLabel(ASTRO_STATE.EXPOSING), "Exposing");
  assert.equal(stateLabel(ASTRO_STATE.INTERVAL), "Interval");
  assert.equal(stateLabel(ASTRO_STATE.PAUSED), "Paused");
  assert.equal(stateLabel(ASTRO_STATE.STOPPED), "Stopped");
  assert.equal(stateLabel(ASTRO_STATE.ERROR), "Error");
  assert.equal(stateLabel(255), "Unknown");
});

test("interpolate advances timers while running, snapping down remaining", () => {
  const base = {
    state: ASTRO_STATE.EXPOSING,
    elapsedSec: 100,
    remainingSec: 200,
    phaseRemainingSec: 20,
    phaseTotalSec: 30,
  };
  const out = interpolate(base, 2500); // 2.5s since packet -> +2s
  assert.equal(out.elapsedSec, 102);
  assert.equal(out.remainingSec, 198);
  assert.equal(out.phaseRemainingSec, 18);
});

test("interpolate never drives remaining/phase below zero", () => {
  const base = {
    state: ASTRO_STATE.INTERVAL,
    elapsedSec: 100,
    remainingSec: 1,
    phaseRemainingSec: 1,
    phaseTotalSec: 3,
  };
  const out = interpolate(base, 5000); // +5s, would underflow
  assert.equal(out.remainingSec, 0);
  assert.equal(out.phaseRemainingSec, 0);
  assert.equal(out.elapsedSec, 105); // elapsed keeps climbing
});

test("interpolate is frozen when paused or not running", () => {
  for (const state of [
    ASTRO_STATE.PAUSED,
    ASTRO_STATE.IDLE,
    ASTRO_STATE.STOPPED,
    ASTRO_STATE.ERROR,
  ]) {
    const base = {
      state,
      elapsedSec: 50,
      remainingSec: 100,
      phaseRemainingSec: 10,
      phaseTotalSec: 30,
    };
    const out = interpolate(base, 4000);
    assert.deepEqual(out, base, `state ${state} should freeze`);
  }
});
