class M5RemoteClient {
  constructor() {
    // UUIDs matching the server
    this.SERVICE_UUID = "180f1000-1234-5678-90ab-cdef12345678";
    this.CONTROL_CHAR_UUID = "180f1001-1234-5678-90ab-cdef12345678";
    this.FEEDBACK_CHAR_UUID = "180f1002-1234-5678-90ab-cdef12345678";

    // Command constants
    this.BUTTON_DOWN = 0x01;
    this.BUTTON_UP = 0x02;
    this.BUTTON_CONFIRM = 0x05; // ButtonId::CONFIRM

    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    this.isButtonPressed = false;  // Track button state

    // UI elements
    this.scanButton = document.getElementById("scanButton");
    this.disconnectButton = document.getElementById("disconnectButton");
    this.confirmButton = document.getElementById("confirmButton");
    this.deviceList = document.getElementById("deviceList");
    this.status = document.getElementById("status");

    console.log("[BLE] Client initialized with service UUID:", this.SERVICE_UUID);

    // Bind event listeners
    this.scanButton.addEventListener("click", () => this.startScan());
    this.disconnectButton.addEventListener("click", () => this.disconnect());
    this.confirmButton.addEventListener("mousedown", () => this.sendButtonDown());
    this.confirmButton.addEventListener("mouseup", () => this.sendButtonUp());
    this.confirmButton.addEventListener("mouseleave", () => {
      if (this.isButtonPressed) {
        this.sendButtonUp();
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

      console.log("[BLE] Requesting Bluetooth device with service:", this.SERVICE_UUID);
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
        gattConnected: this.device.gatt.connected
      });
    }

    this.scanButton.disabled = false;
    this.disconnectButton.disabled = true;
    this.confirmButton.disabled = true;
    this.device = null;
    this.server = null;
    this.service = null;
    this.controlChar = null;
    this.feedbackChar = null;
    console.log("[BLE] Cleanup completed");
  }

  async sendButtonDown() {
    if (!this.controlChar) {
      console.warn("[BLE] Attempted to send button down without control characteristic");
      return;
    }

    try {
      console.log("[BLE] Sending button down command");
      const data = new Uint8Array([this.BUTTON_DOWN, 1, this.BUTTON_CONFIRM]);
      console.log("[BLE] Command data:", Array.from(data).map(b => "0x" + b.toString(16)));
      await this.controlChar.writeValue(data);
      this.isButtonPressed = true;
      console.log("[BLE] Button down command sent successfully");
    } catch (error) {
      console.error("[BLE] Error sending button down:", error);
      this.updateStatus("Error sending button down: " + error.message, "error");
    }
  }

  async sendButtonUp() {
    if (!this.controlChar) {
      console.warn("[BLE] Attempted to send button up without control characteristic");
      return;
    }

    if (!this.isButtonPressed) {
      console.log("[BLE] Ignoring button up - button was not pressed");
      return;
    }

    try {
      console.log("[BLE] Sending button up command");
      const data = new Uint8Array([this.BUTTON_UP, 1, this.BUTTON_CONFIRM]);
      console.log("[BLE] Command data:", Array.from(data).map(b => "0x" + b.toString(16)));
      await this.controlChar.writeValue(data);
      this.isButtonPressed = false;
      console.log("[BLE] Button up command sent successfully");
    } catch (error) {
      console.error("[BLE] Error sending button up:", error);
      this.updateStatus("Error sending button up: " + error.message, "error");
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
