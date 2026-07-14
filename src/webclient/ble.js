import {
  ASTRO_STATE,
  decodeAstroStatus,
  decodeAstroParams,
  interpolate,
  formatMMSS,
  formatDuration,
  barFraction,
  stateLabel,
  isFinished,
  sequenceTotalSec,
  smoothProgress,
} from "./astro-status.js";

class M5RemoteClient {
  constructor() {
    // UUIDs matching the server (BLERemoteServer, lower-cased for Web BLE).
    this.SERVICE_UUID = "180f1000-1234-5678-90ab-cdef12345678";
    this.CONTROL_CHAR_UUID = "180f1001-1234-5678-90ab-cdef12345678";
    this.FEEDBACK_CHAR_UUID = "180f1002-1234-5678-90ab-cdef12345678";
    this.ASTRO_STATUS_CHAR_UUID = "180f1003-1234-5678-90ab-cdef12345678";
    this.ASTRO_PARAMS_CHAR_UUID = "180f1005-1234-5678-90ab-cdef12345678";

    // Button command words (0x01XX) + release, matching RemoteCmd.
    this.BUTTON_DOWN = 0x0100;
    this.BUTTON_UP = 0x0101;

    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    this.astroStatusChar = null;
    this.astroParamsChar = null;

    this.currentButtonId = null; // Which button is held (null = none).

    // Latest decoded status + when it arrived (performance.now ms), for
    // local interpolation between the device's ~1 Hz notifications.
    this.lastStatus = null;
    this.lastStatusAt = 0;
    this.lastParams = null; // configured sequence plan (delay/exposure/…)
    this.tickTimer = null;
    this.pollTimer = null; // polls status while idle (no notifications then)

    this.el = {
      deviceStatus: document.getElementById("status"),
      unsupported: document.getElementById("unsupported"),
      scanButton: document.getElementById("scanButton"),
      disconnectButton: document.getElementById("disconnectButton"),
      controls: document.getElementById("controlKeysContainer"),
      // status panel
      stateDot: document.getElementById("stateDot"),
      stateLabel: document.getElementById("stateLabel"),
      cameraDot: document.getElementById("cameraDot"),
      cameraLabel: document.getElementById("cameraLabel"),
      errorBanner: document.getElementById("errorBanner"),
      planExposure: document.getElementById("planExposure"),
      planInterval: document.getElementById("planInterval"),
      planDelay: document.getElementById("planDelay"),
      planFrames: document.getElementById("planFrames"),
      planTotal: document.getElementById("planTotal"),
      frameCount: document.getElementById("frameCount"),
      seqTimes: document.getElementById("seqTimes"),
      seqBar: document.getElementById("seqBar"),
      phaseLabel: document.getElementById("phaseLabel"),
      phaseTime: document.getElementById("phaseTime"),
      phaseBar: document.getElementById("phaseBar"),
    };

    this.keys = Array.from(document.querySelectorAll(".key"));

    this.el.scanButton.addEventListener("click", () => this.startScan());
    this.el.disconnectButton.addEventListener("click", () => this.disconnect());

    this.bindPointerControls();
    this.bindKeyboardControls();
    this.bindSafetyReleases();

    this.setConnectedUI(false);
    this.renderStatus(null); // idle placeholder
  }

  // Show exactly one of Connect / Disconnect; toggle the d-pad with it.
  setConnectedUI(connected) {
    this.el.scanButton.classList.toggle("hidden", connected);
    this.el.disconnectButton.classList.toggle("hidden", !connected);
    this.el.controls.classList.toggle("hidden", !connected);
    this.el.scanButton.disabled = false;
  }

  // --- Input: pointer (mouse + touch + pen, one path) --------------------

  bindPointerControls() {
    for (const key of this.keys) {
      const id = Number(key.dataset.btn);
      key.addEventListener("pointerdown", (e) => {
        e.preventDefault();
        // Capture so we still get pointerup even if the finger slides off.
        try {
          key.setPointerCapture(e.pointerId);
        } catch {}
        this.pressButton(id, key);
      });
      const release = (e) => {
        e.preventDefault();
        this.releaseButton(key);
      };
      key.addEventListener("pointerup", release);
      key.addEventListener("pointercancel", release);
    }
  }

  bindKeyboardControls() {
    const keyMap = {
      ArrowUp: 1,
      ArrowDown: 2,
      ArrowLeft: 3,
      ArrowRight: 4,
      Enter: 5,
      Escape: 6,
    };
    document.addEventListener("keydown", (e) => {
      const id = keyMap[e.key];
      if (id === undefined) return;
      e.preventDefault();
      if (e.repeat) return; // Hold = single press, not autorepeat spam.
      this.pressButton(id, this.keyEl(id));
    });
    document.addEventListener("keyup", (e) => {
      const id = keyMap[e.key];
      if (id === undefined) return;
      e.preventDefault();
      this.releaseButton(this.keyEl(id));
    });
  }

  // On any interruption, force a button-up so the M5 is never left holding a
  // key (which would keep navigating the device UI).
  bindSafetyReleases() {
    const forceRelease = () => this.releaseButton(null, true);
    document.addEventListener("visibilitychange", () => {
      if (document.hidden) forceRelease();
    });
    window.addEventListener("blur", forceRelease);
    window.addEventListener("pagehide", forceRelease);
  }

  keyEl(id) {
    return this.keys.find((k) => Number(k.dataset.btn) === id) || null;
  }

  async pressButton(id, el) {
    if (this.currentButtonId !== null) return; // one at a time
    if (!this.controlChar) return;
    const ok = await this.sendButton(this.BUTTON_DOWN, id);
    if (ok) {
      this.currentButtonId = id;
      if (el) el.classList.add("pressed");
    }
  }

  // el: element to un-highlight (or null). force: send release even if we
  // think nothing is held (used by safety handlers after a lost event).
  async releaseButton(el, force = false) {
    const held = this.currentButtonId;
    if (held === null && !force) return;
    if (el) el.classList.remove("pressed");
    else this.keys.forEach((k) => k.classList.remove("pressed"));
    this.currentButtonId = null;
    if (held !== null && this.controlChar) {
      await this.sendButton(this.BUTTON_UP, held);
    }
  }

  async sendButton(cmd, buttonId) {
    if (!this.controlChar) return false;
    const data = new Uint8Array([(cmd >> 8) & 0xff, cmd & 0xff, buttonId & 0xff]);
    try {
      await this.controlChar.writeValue(data);
      return true;
    } catch (err) {
      console.error("[BLE] button write failed:", err);
      return false;
    }
  }

  // --- Connection --------------------------------------------------------

  setDeviceStatus(message, type = "info") {
    const tones = {
      info: "text-muted",
      success: "text-ok",
      warning: "text-warn",
      error: "text-danger",
    };
    this.el.deviceStatus.textContent = message;
    this.el.deviceStatus.className =
      "mb-4 rounded-lg border border-border bg-surface px-3 py-2 text-sm " +
      (tones[type] || tones.info);
  }

  async startScan() {
    try {
      this.setDeviceStatus("Scanning for M5Remote…", "info");
      this.el.scanButton.disabled = true;
      const device = await navigator.bluetooth.requestDevice({
        filters: [{ services: [this.SERVICE_UUID] }],
      });
      this.setDeviceStatus("Connecting…", "info");
      await this.connect(device);
    } catch (err) {
      console.error("[BLE] scan error:", err);
      this.setDeviceStatus("Error: " + err.message, "error");
      this.el.scanButton.disabled = false;
    }
  }

  async connect(device) {
    try {
      this.device = device;
      device.addEventListener("gattserverdisconnected", () =>
        this.onDisconnected(),
      );

      this.server = await device.gatt.connect();
      this.service = await this.server.getPrimaryService(this.SERVICE_UUID);
      this.controlChar = await this.service.getCharacteristic(
        this.CONTROL_CHAR_UUID,
      );

      // Feedback (1-byte command ack) — optional, best-effort.
      try {
        this.feedbackChar = await this.service.getCharacteristic(
          this.FEEDBACK_CHAR_UUID,
        );
        this.feedbackChar.addEventListener("characteristicvaluechanged", (e) =>
          this.handleFeedback(e.target.value),
        );
        await this.feedbackChar.startNotifications();
      } catch (err) {
        console.warn("[BLE] feedback characteristic unavailable:", err);
      }

      // Astro status broadcast — the live sequence progress.
      try {
        this.astroStatusChar = await this.service.getCharacteristic(
          this.ASTRO_STATUS_CHAR_UUID,
        );
        this.astroStatusChar.addEventListener(
          "characteristicvaluechanged",
          (e) => this.handleAstroStatus(e.target.value),
        );
        await this.astroStatusChar.startNotifications();
        // Prime with the current value so the panel isn't blank until the
        // next change.
        try {
          const v = await this.astroStatusChar.readValue();
          this.handleAstroStatus(v);
        } catch {}
      } catch (err) {
        console.warn("[BLE] astro-status characteristic unavailable:", err);
      }

      // Sequence plan (delay/exposure/frames/interval) — for the pre-start
      // display. READ to prime, NOTIFY on change.
      try {
        this.astroParamsChar = await this.service.getCharacteristic(
          this.ASTRO_PARAMS_CHAR_UUID,
        );
        this.astroParamsChar.addEventListener(
          "characteristicvaluechanged",
          (e) => this.handleAstroParams(e.target.value),
        );
        await this.astroParamsChar.startNotifications();
        try {
          const v = await this.astroParamsChar.readValue();
          this.handleAstroParams(v);
        } catch {}
      } catch (err) {
        console.warn("[BLE] astro-params characteristic unavailable:", err);
      }

      this.setDeviceStatus("Connected to " + (device.name || "M5Remote"), "success");
      this.setConnectedUI(true);
      this.startTicker();
      this.startPolling();
    } catch (err) {
      console.error("[BLE] connection error:", err);
      this.setDeviceStatus("Connection error: " + err.message, "error");
      this.disconnect();
    }
  }

  disconnect() {
    if (this.device && this.device.gatt.connected) {
      this.device.gatt.disconnect();
    } else {
      this.onDisconnected();
    }
  }

  onDisconnected() {
    this.setDeviceStatus("Disconnected", "info");
    this.stopTicker();
    this.stopPolling();
    this.releaseButton(null, true);
    this.setConnectedUI(false);
    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    this.astroStatusChar = null;
    this.astroParamsChar = null;
    this.lastStatus = null;
    this.lastParams = null;
    this.renderPlan(null);
    this.renderStatus(null);
  }

  // --- Astro status ------------------------------------------------------

  handleAstroStatus(value) {
    try {
      this.lastStatus = decodeAstroStatus(value);
      this.lastStatusAt = performance.now();
      this.renderStatus(this.lastStatus, 0);
    } catch (err) {
      console.error("[BLE] bad astro status packet:", err);
    }
  }

  handleAstroParams(value) {
    try {
      this.lastParams = decodeAstroParams(value);
      this.renderPlan(this.lastParams);
    } catch (err) {
      console.error("[BLE] bad astro params packet:", err);
    }
  }

  // Local 1 Hz interpolation between packets; each real packet re-snaps.
  startTicker() {
    this.stopTicker();
    this.tickTimer = setInterval(() => {
      if (!this.lastStatus) return;
      const dt = performance.now() - this.lastStatusAt;
      this.renderStatus(interpolate(this.lastStatus, dt), dt);
    }, 200); // matches the bars' 200ms transition for continuous motion
  }

  stopTicker() {
    if (this.tickTimer) {
      clearInterval(this.tickTimer);
      this.tickTimer = null;
    }
  }

  // Poll status while idle: the device only notifies on change or ~1 Hz while
  // running, so before a sequence starts (and for live camera state) we read
  // the current value periodically. Reads re-enter handleAstroStatus.
  startPolling() {
    this.stopPolling();
    this.pollTimer = setInterval(async () => {
      if (!this.astroStatusChar) return;
      // While running, notifications already drive updates — skip the poll.
      if (this.lastStatus && this.isRunningState(this.lastStatus.state)) return;
      try {
        this.handleAstroStatus(await this.astroStatusChar.readValue());
      } catch {}
    }, 1500);
  }

  stopPolling() {
    if (this.pollTimer) {
      clearInterval(this.pollTimer);
      this.pollTimer = null;
    }
  }

  isRunningState(state) {
    return (
      state === ASTRO_STATE.INITIAL_DELAY ||
      state === ASTRO_STATE.EXPOSING ||
      state === ASTRO_STATE.INTERVAL
    );
  }

  renderPlan(p) {
    const e = this.el;
    if (!p) {
      for (const el of [
        e.planExposure,
        e.planInterval,
        e.planDelay,
        e.planFrames,
        e.planTotal,
      ]) {
        el.textContent = "—";
      }
      return;
    }
    // Exposure/interval/delay in raw seconds (how astro exposures are set);
    // Total stays compact since a sequence can run for hours.
    e.planExposure.textContent = `${p.exposureSec}s`;
    e.planInterval.textContent = `${p.intervalSec}s`;
    e.planDelay.textContent = `${p.initialDelaySec}s`;
    e.planFrames.textContent = String(p.subframeCount);
    e.planTotal.textContent = formatDuration(sequenceTotalSec(p));
  }

  renderStatus(s, msSincePacket = 0) {
    const e = this.el;
    if (!s) {
      e.stateLabel.textContent = "Idle";
      e.stateDot.className = "inline-block h-2.5 w-2.5 rounded-full bg-muted";
      e.frameCount.textContent = "0 / 0";
      e.seqTimes.textContent = "00:00 / 00:00";
      e.seqBar.style.width = "0%";
      e.phaseLabel.textContent = "Phase";
      e.phaseTime.textContent = "00:00";
      e.phaseBar.style.width = "0%";
      e.phaseBar.classList.remove("bar-active");
      e.seqBar.classList.remove("bar-active");
      e.errorBanner.classList.add("hidden");
      e.cameraDot.className = "inline-block h-2 w-2 rounded-full bg-muted";
      e.cameraLabel.textContent = "Camera unknown";
      return;
    }

    const running = this.isRunningState(s.state);
    const finished = isFinished(s);

    // State label + dot colour. "Finished" (all frames done) is distinguished
    // from a user "Stopped" — the firmware reports STOPPED for both.
    e.stateLabel.textContent = finished ? "Finished" : stateLabel(s.state);
    let dotTone =
      {
        [ASTRO_STATE.EXPOSING]: "bg-ok",
        [ASTRO_STATE.INTERVAL]: "bg-accent",
        [ASTRO_STATE.INITIAL_DELAY]: "bg-accent",
        [ASTRO_STATE.PAUSED]: "bg-warn",
        [ASTRO_STATE.ERROR]: "bg-danger",
      }[s.state] || "bg-muted";
    if (finished) dotTone = "bg-ok";
    e.stateDot.className =
      "inline-block h-2.5 w-2.5 rounded-full " +
      dotTone +
      (running ? " bar-active" : "");

    // Frames.
    e.frameCount.textContent = `${s.completedFrames} / ${s.totalFrames}`;

    // A terminal, non-finished state (user Stopped, Error, Idle) has stale
    // leftover timings — don't draw them as live progress.
    const aborted =
      !running && !finished && s.state !== ASTRO_STATE.PAUSED;

    // Fractional fills so the bars glide instead of stepping once per second.
    const prog = smoothProgress(s, msSincePacket);
    const seqTotal = s.elapsedSec + s.remainingSec;
    const seqFrac = finished ? 1 : aborted ? 0 : prog.seq;
    e.seqBar.style.width = (seqFrac * 100).toFixed(2) + "%";
    e.seqTimes.textContent = aborted
      ? "00:00 / 00:00"
      : `${formatMMSS(s.elapsedSec)} / ${formatMMSS(seqTotal)}`;

    // Phase bar.
    e.phaseLabel.textContent = finished
      ? "Complete"
      : s.state === ASTRO_STATE.PAUSED
        ? "Phase (paused)"
        : aborted
          ? "Phase"
          : stateLabel(s.state);
    e.phaseTime.textContent = aborted ? "00:00" : formatMMSS(s.phaseRemainingSec);
    const phaseFrac = finished ? 1 : aborted ? 0 : prog.phase;
    e.phaseBar.style.width = (phaseFrac * 100).toFixed(2) + "%";
    e.phaseBar.classList.toggle("bar-active", running);

    // Camera link indicator.
    e.cameraDot.className =
      "inline-block h-2 w-2 rounded-full " +
      (s.isCameraConnected ? "bg-ok" : "bg-danger");
    e.cameraLabel.textContent = s.isCameraConnected
      ? "Camera linked"
      : "Camera off";

    // Error banner.
    if (s.errorCode) {
      const msg = {
        1: "Invalid parameters — sequence cannot start.",
        2: "Camera not connected.",
      }[s.errorCode] || `Error code ${s.errorCode}.`;
      e.errorBanner.textContent = msg;
      e.errorBanner.classList.remove("hidden");
    } else {
      e.errorBanner.classList.add("hidden");
    }
  }

  handleFeedback(value) {
    if (value.byteLength < 1) return;
    const status = value.getUint8(0);
    const map = {
      0: ["Command OK", "success"],
      1: ["Command failed", "error"],
      2: ["Device busy", "warning"],
      3: ["Invalid command", "error"],
      4: ["Invalid button state", "error"],
      5: ["Astro error", "error"],
    };
    const [text, type] = map[status] || [`Status ${status}`, "error"];
    // Only surface non-success feedback; success is noisy on every keypress.
    if (status !== 0) this.setDeviceStatus(text, type);
  }
}

window.addEventListener("load", () => {
  // Register the service worker for offline use (ADR 0006). Best-effort:
  // needs a secure context, absent in some Web-BLE browsers — never blocks.
  if ("serviceWorker" in navigator) {
    navigator.serviceWorker
      .register("sw.js")
      .catch((err) => console.warn("[PWA] service worker registration failed:", err));
  }

  if (!navigator.bluetooth) {
    document.getElementById("unsupported").classList.remove("hidden");
    document.getElementById("scanButton").disabled = true;
    document.getElementById("status").textContent =
      "Web Bluetooth unavailable in this browser.";
    return;
  }
  // Exposed for debugging in the console (e.g. window.m5.service).
  window.m5 = new M5RemoteClient();
});
