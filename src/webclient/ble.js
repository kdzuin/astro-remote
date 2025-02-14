class M5RemoteClient {
  constructor() {
    // UUIDs matching the server
    this.SERVICE_UUID = "180f1000-1234-5678-90ab-cdef12345678";
    this.CONTROL_CHAR_UUID = "180f1001-1234-5678-90ab-cdef12345678";
    this.FEEDBACK_CHAR_UUID = "180f1002-1234-5678-90ab-cdef12345678";

    // Command types
    this.CMD_TYPE_BUTTON = 0x01;
    this.CMD_TYPE_ASTRO = 0x02;

    // Button commands (0x01XX)
    this.BUTTON_DOWN = 0x0100;
    this.BUTTON_UP = 0x0101;

    // Astro commands (0x02XX)
    this.ASTRO_START = 0x0200;
    this.ASTRO_PAUSE = 0x0201;
    this.ASTRO_STOP = 0x0202;
    this.ASTRO_RESET = 0x0203;
    this.ASTRO_SET_PARAMS = 0x0204;

    // Button IDs matching the M5 device's ButtonId enum
    this.BUTTON_UP_ARROW = 0x01; // ButtonId::UP
    this.BUTTON_DOWN_ARROW = 0x02; // ButtonId::DOWN
    this.BUTTON_LEFT_ARROW = 0x03; // ButtonId::LEFT
    this.BUTTON_RIGHT_ARROW = 0x04; // ButtonId::RIGHT
    this.BUTTON_CONFIRM = 0x05; // ButtonId::CONFIRM
    this.BUTTON_BACK = 0x06; // ButtonId::BACK

    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    this.isButtonPressed = false; // Track button state
    this.currentButtonId = null; // Track which button is currently pressed

    // UI elements
    this.deviceList = document.getElementById("deviceList");
    this.status = document.getElementById("status");
    this.scanButton = document.getElementById("scanButton");
    this.disconnectButton = document.getElementById("disconnectButton");
    this.controlKeysContainer = document.getElementById("controlKeysContainer");

    this.confirmButton = document.getElementById("confirmButton");
    this.upButton = document.getElementById("upButton");
    this.downButton = document.getElementById("downButton");
    this.leftButton = document.getElementById("leftButton");
    this.rightButton = document.getElementById("rightButton");
    this.backButton = document.getElementById("backButton");

    console.log(
      "[BLE] Client initialized with service UUID:",
      this.SERVICE_UUID
    );

    // Bind event listeners
    this.scanButton.addEventListener("click", () => this.startScan());
    this.disconnectButton.addEventListener("click", () => this.disconnect());

    // Bind control button events
    this.upButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_UP_ARROW)
    );
    this.upButton.addEventListener("mouseup", () => this.sendButtonRelease());
    this.upButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    this.downButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_DOWN_ARROW)
    );
    this.downButton.addEventListener("mouseup", () => this.sendButtonRelease());
    this.downButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    this.leftButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_LEFT_ARROW)
    );
    this.leftButton.addEventListener("mouseup", () => this.sendButtonRelease());
    this.leftButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    this.rightButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_RIGHT_ARROW)
    );
    this.rightButton.addEventListener("mouseup", () =>
      this.sendButtonRelease()
    );
    this.rightButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    this.confirmButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_CONFIRM)
    );
    this.confirmButton.addEventListener("mouseup", () =>
      this.sendButtonRelease()
    );
    this.confirmButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    this.backButton.addEventListener("mousedown", () =>
      this.sendButtonCommand(this.BUTTON_BACK)
    );
    this.backButton.addEventListener("mouseup", () => this.sendButtonRelease());
    this.backButton.addEventListener("mouseleave", () =>
      this.sendButtonRelease()
    );

    // Add touch events for mobile support
    [
      this.upButton,
      this.downButton,
      this.leftButton,
      this.rightButton,
      this.confirmButton,
      this.backButton,
    ].forEach((button) => {
      button.addEventListener("touchstart", (e) => {
        e.preventDefault();
        const buttonId = {
          [this.upButton.id]: this.BUTTON_UP_ARROW,
          [this.downButton.id]: this.BUTTON_DOWN_ARROW,
          [this.leftButton.id]: this.BUTTON_LEFT_ARROW,
          [this.rightButton.id]: this.BUTTON_RIGHT_ARROW,
          [this.confirmButton.id]: this.BUTTON_CONFIRM,
          [this.backButton.id]: this.BUTTON_BACK,
        }[button.id];
        this.sendButtonCommand(buttonId);
      });

      button.addEventListener("touchend", (e) => {
        e.preventDefault();
        this.sendButtonRelease();
      });

      button.addEventListener("touchcancel", (e) => {
        e.preventDefault();
        this.sendButtonRelease();
      });
    });

    // Add keyboard controls
    document.addEventListener("keydown", (e) => {
      // Prevent default behavior for arrow keys to avoid page scrolling
      if (["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight"].includes(e.key)) {
        e.preventDefault();
      }

      // Only process if we're not already pressing a key and we're connected
      if (!this.isButtonPressed && this.controlChar) {
        const buttonId = {
          ArrowUp: this.BUTTON_UP_ARROW,
          ArrowDown: this.BUTTON_DOWN_ARROW,
          ArrowLeft: this.BUTTON_LEFT_ARROW,
          ArrowRight: this.BUTTON_RIGHT_ARROW,
          Enter: this.BUTTON_CONFIRM,
          Escape: this.BUTTON_BACK
        }[e.key];

        if (buttonId) {
          this.sendButtonCommand(buttonId);
          // Add visual feedback by adding a pressed class
          const buttonElement = {
            [this.BUTTON_UP_ARROW]: this.upButton,
            [this.BUTTON_DOWN_ARROW]: this.downButton,
            [this.BUTTON_LEFT_ARROW]: this.leftButton,
            [this.BUTTON_RIGHT_ARROW]: this.rightButton,
            [this.BUTTON_CONFIRM]: this.confirmButton,
            [this.BUTTON_BACK]: this.backButton
          }[buttonId];
          
          if (buttonElement) {
            buttonElement.classList.add("pressed");
          }
        }
      }
    });

    document.addEventListener("keyup", (e) => {
      const buttonId = {
        ArrowUp: this.BUTTON_UP_ARROW,
        ArrowDown: this.BUTTON_DOWN_ARROW,
        ArrowLeft: this.BUTTON_LEFT_ARROW,
        ArrowRight: this.BUTTON_RIGHT_ARROW,
        Enter: this.BUTTON_CONFIRM,
        Escape: this.BUTTON_BACK
      }[e.key];

      if (buttonId) {
        this.sendButtonRelease();
        // Remove visual feedback
        const buttonElement = {
          [this.BUTTON_UP_ARROW]: this.upButton,
          [this.BUTTON_DOWN_ARROW]: this.downButton,
          [this.BUTTON_LEFT_ARROW]: this.leftButton,
          [this.BUTTON_RIGHT_ARROW]: this.rightButton,
          [this.BUTTON_CONFIRM]: this.confirmButton,
          [this.BUTTON_BACK]: this.backButton
        }[buttonId];
        
        if (buttonElement) {
          buttonElement.classList.remove("pressed");
        }
      }
    });

    // Initialize UI state
    this.disconnectButton.disabled = true;
    this.confirmButton.disabled = true;
  }

  updateStatus(message, type = "info") {
    console.log(`[BLE] Status update (${type}):`, message);
    this.status.textContent = message;
    this.status.className = type;
  }

  async startScan() {
    try {
      console.log("[BLE] Starting device scan...");
      this.updateStatus("Scanning for M5Remote devices...", "info");
      this.scanButton.disabled = true;
      this.deviceList.innerHTML = "";

      console.log(
        "[BLE] Requesting Bluetooth device with service:",
        this.SERVICE_UUID
      );
      const device = await navigator.bluetooth.requestDevice({
        filters: [
          {
            services: [this.SERVICE_UUID],
          },
        ],
        optionalServices: [],
      });

      console.log("[BLE] Device selected:", device.name, "ID:", device.id);
      this.updateStatus("Device selected, connecting...", "info");
      await this.connect(device);
    } catch (error) {
      console.error("[BLE] Scan error:", error);
      this.updateStatus("Error: " + error.message, "error");
      this.scanButton.disabled = false;
    }
  }

  async connect(device) {
    try {
      console.log("[BLE] Starting connection process to device:", device.name);
      this.device = device;

      // Setup disconnect listener
      this.device.addEventListener("gattserverdisconnected", () => {
        console.log("[BLE] GATT server disconnected event received");
        this.onDisconnected();
      });

      console.log("[BLE] Connecting to GATT server...");
      this.server = await device.gatt.connect();
      console.log("[BLE] GATT server connected");

      console.log("[BLE] Getting primary service...");
      this.service = await this.server.getPrimaryService(this.SERVICE_UUID);
      console.log("[BLE] Service obtained:", this.SERVICE_UUID);

      console.log("[BLE] Getting control characteristic...");
      this.controlChar = await this.service.getCharacteristic(
        this.CONTROL_CHAR_UUID
      );
      console.log("[BLE] Control characteristic obtained");

      console.log("[BLE] Getting feedback characteristic...");
      this.feedbackChar = await this.service.getCharacteristic(
        this.FEEDBACK_CHAR_UUID
      );
      console.log("[BLE] Feedback characteristic obtained");

      // Try notifications first, fall back to indications
      console.log("[BLE] Starting notifications/indications for feedback...");
      try {
        // Add event listener before starting notifications
        this.feedbackChar.addEventListener(
          "characteristicvaluechanged",
          (event) => {
            console.log("[BLE] Received feedback notification/indication");
            this.handleFeedback(event.target.value);
          }
        );

        // Start notifications
        await this.feedbackChar.startNotifications();
        console.log("[BLE] Notifications started successfully");

        this.updateStatus("Connected to " + device.name, "success");
        console.log("[BLE] Connection process completed successfully");
        this.scanButton.disabled = true;
        this.disconnectButton.disabled = false;
        this.confirmButton.disabled = false;
        
        // Show control keys container when connected
        this.controlKeysContainer.classList.remove("hidden");
      } catch (error) {
        console.error("[BLE] Connection error:", error);
        this.updateStatus("Connection error: " + error.message, "error");
        this.disconnect();
      }
    } catch (error) {
      console.error("[BLE] Connection error:", error);
      this.updateStatus("Connection error: " + error.message, "error");
      this.disconnect();
    }
  }

  disconnect() {
    console.log("[BLE] Initiating disconnect...");
    if (this.device && this.device.gatt.connected) {
      console.log("[BLE] Disconnecting GATT server");
      this.device.gatt.disconnect();
    } else {
      console.log("[BLE] No active connection, cleaning up");
      this.onDisconnected();
    }
  }

  onDisconnected() {
    console.log("[BLE] Processing disconnect");
    this.updateStatus("Device disconnected", "info");

    // Log connection state
    if (this.device) {
      console.log("[BLE] Device state:", {
        name: this.device.name,
        id: this.device.id,
        gattConnected: this.device.gatt.connected,
      });
    }

    this.scanButton.disabled = false;
    this.disconnectButton.disabled = true;
    this.confirmButton.disabled = true;
    
    // Hide control keys container when disconnected
    this.controlKeysContainer.classList.add("hidden");

    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    console.log("[BLE] Cleanup completed");
  }

  // Send a command with no parameters
  async sendCommand16(cmd) {
    if (!this.controlChar) {
      console.error("[BLE] Not connected");
      return false;
    }

    const data = new Uint8Array([
      (cmd >> 8) & 0xFF,  // High byte
      cmd & 0xFF          // Low byte
    ]);

    try {
      await this.controlChar.writeValue(data);
      return true;
    } catch (error) {
      console.error("[BLE] Error sending command:", error);
      return false;
    }
  }

  // Send a command with one byte parameter
  async sendCommand24(cmd, param) {
    if (!this.controlChar) {
      console.error("[BLE] Not connected");
      return false;
    }

    const data = new Uint8Array([
      (cmd >> 8) & 0xFF,  // High byte
      cmd & 0xFF,         // Low byte
      param & 0xFF        // Parameter
    ]);

    try {
      await this.controlChar.writeValue(data);
      return true;
    } catch (error) {
      console.error("[BLE] Error sending command:", error);
      return false;
    }
  }

  // Send a command with payload
  async sendCommandWithPayload(cmd, payload) {
    if (!this.controlChar) {
      console.error("[BLE] Not connected");
      return false;
    }

    const data = new Uint8Array(2 + payload.length);
    data[0] = (cmd >> 8) & 0xFF;  // High byte
    data[1] = cmd & 0xFF;         // Low byte
    data.set(payload, 2);         // Payload

    try {
      await this.controlChar.writeValue(data);
      return true;
    } catch (error) {
      console.error("[BLE] Error sending command:", error);
      return false;
    }
  }

  async sendButtonCommand(buttonId) {
    if (this.isButtonPressed) {
      console.warn("[BLE] Button already pressed");
      return;
    }

    console.log("[BLE] Sending button press:", buttonId);
    const success = await this.sendCommand24(this.BUTTON_DOWN, buttonId);
    
    if (success) {
      this.isButtonPressed = true;
      this.currentButtonId = buttonId;
    }
  }

  async sendButtonRelease() {
    if (!this.isButtonPressed || this.currentButtonId === null) {
      return;
    }

    console.log("[BLE] Sending button release:", this.currentButtonId);
    const success = await this.sendCommand24(this.BUTTON_UP, this.currentButtonId);
    
    if (success) {
      this.isButtonPressed = false;
      this.currentButtonId = null;
    }
  }

  // Astro control methods
  async sendAstroStart() {
    console.log("[BLE] Sending astro start command");
    return await this.sendCommand16(this.ASTRO_START);
  }

  async sendAstroPause() {
    console.log("[BLE] Sending astro pause command");
    return await this.sendCommand16(this.ASTRO_PAUSE);
  }

  async sendAstroStop() {
    console.log("[BLE] Sending astro stop command");
    return await this.sendCommand16(this.ASTRO_STOP);
  }

  async sendAstroReset() {
    console.log("[BLE] Sending astro reset command");
    return await this.sendCommand16(this.ASTRO_RESET);
  }

  async sendAstroParams(params) {
    if (!params || typeof params !== 'object') {
      console.error("[BLE] Invalid astro params");
      return false;
    }

    // Convert params to binary format matching AstroParamPacket
    const data = new Uint8Array(8); // 4 x uint16_t
    const view = new DataView(data.buffer);

    // Pack parameters in little-endian format
    view.setUint16(0, params.initialDelaySec || 0, true);
    view.setUint16(2, params.exposureSec || 0, true);
    view.setUint16(4, params.subframeCount || 0, true);
    view.setUint16(6, params.intervalSec || 0, true);

    console.log("[BLE] Sending astro params:", params);
    return await this.sendCommandWithPayload(this.ASTRO_SET_PARAMS, data);
  }

  handleFeedback(value) {
    if (value.byteLength < 1) {
      console.error("[BLE] Invalid feedback length");
      return;
    }

    const status = value.getUint8(0);
    let statusText;
    let statusType;

    switch (status) {
      case 0: // SUCCESS
        statusText = "Command executed successfully";
        statusType = "success";
        break;
      case 1: // FAILURE
        statusText = "Command failed";
        statusType = "error";
        break;
      case 2: // BUSY
        statusText = "Device is busy";
        statusType = "warning";
        break;
      case 3: // INVALID
        statusText = "Invalid command";
        statusType = "error";
        break;
      case 4: // BUTTON_STATE_ERROR
        statusText = "Invalid button state transition";
        statusType = "error";
        break;
      case 5: // ASTRO_ERROR
        statusText = "Astro operation error";
        statusType = "error";
        break;
      default:
        statusText = "Unknown status code: " + status;
        statusType = "error";
    }

    console.log(`[BLE] Feedback received: ${statusText} (${status})`);
    this.updateStatus(statusText, statusType);
  }
}

// Initialize the client when the page loads
window.addEventListener("load", () => {
  console.log("[BLE] Checking Web Bluetooth support...");
  if (!navigator.bluetooth) {
    console.warn("[BLE] Web Bluetooth not supported");
    document.getElementById("status").textContent =
      "Web Bluetooth is not supported in this browser. Please use Chrome or Edge.";
    document.getElementById("status").className = "error";
    document.getElementById("scanButton").disabled = true;
    return;
  }

  console.log("[BLE] Web Bluetooth supported, initializing client");
  new M5RemoteClient();
});
