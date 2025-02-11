class M5RemoteClient {
  constructor() {
    // UUIDs matching the server
    this.SERVICE_UUID = "180f1000-1234-5678-90ab-cdef12345678";
    this.CONTROL_CHAR_UUID = "180f1001-1234-5678-90ab-cdef12345678";
    this.FEEDBACK_CHAR_UUID = "180f1002-1234-5678-90ab-cdef12345678";

    // Command constants
    this.BUTTON_DOWN = 0x01;
    this.BUTTON_UP = 0x02;

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

  async sendButtonCommand(buttonId) {
    if (!this.controlChar) {
      console.warn(
        "[BLE] Attempted to send button command without control characteristic"
      );
      return;
    }

    try {
      console.log(`[BLE] Sending button command: 0x${buttonId.toString(16)}`);
      const data = new Uint8Array([this.BUTTON_DOWN, 1, buttonId]);
      console.log(
        "[BLE] Command data:",
        Array.from(data).map((b) => "0x" + b.toString(16))
      );
      await this.controlChar.writeValue(data);
      this.isButtonPressed = true;
      this.currentButtonId = buttonId; // Store the current button ID
      console.log("[BLE] Button command sent successfully");
    } catch (error) {
      console.error("[BLE] Error sending button command:", error);
      this.updateStatus(
        "Error sending button command: " + error.message,
        "error"
      );
    }
  }

  async sendButtonRelease() {
    if (
      !this.controlChar ||
      !this.isButtonPressed ||
      this.currentButtonId === null
    ) {
      return;
    }

    try {
      console.log(
        `[BLE] Sending button release command for button: 0x${this.currentButtonId.toString(
          16
        )}`
      );
      const data = new Uint8Array([this.BUTTON_UP, 1, this.currentButtonId]); // Include the current button ID in release
      console.log(
        "[BLE] Release data:",
        Array.from(data).map((b) => "0x" + b.toString(16))
      );
      await this.controlChar.writeValue(data);
      this.isButtonPressed = false;
      this.currentButtonId = null; // Clear the current button ID
      console.log("[BLE] Button release command sent successfully");
    } catch (error) {
      console.error("[BLE] Error sending button release:", error);
      this.updateStatus(
        "Error sending button release: " + error.message,
        "error"
      );
    }
  }

  handleFeedback(value) {
    const status = value.getUint8(0);
    console.log("[BLE] Received feedback status:", status);

    let message;
    let type;

    switch (status) {
      case 0: // SUCCESS
        message = "Command successful";
        type = "success";
        break;
      case 1: // FAILURE
        message = "Command failed";
        type = "error";
        break;
      case 2: // BUSY
        message = "Device is busy";
        type = "info";
        break;
      case 3: // INVALID
        message = "Invalid command";
        type = "error";
        break;
      case 4: // BUTTON_STATE_ERROR
        message = "Invalid button state";
        type = "error";
        break;
      default:
        message = "Unknown status: " + status;
        type = "error";
    }

    console.log(`[BLE] Processing feedback: ${message} (${type})`);
    this.updateStatus(message, type);
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
