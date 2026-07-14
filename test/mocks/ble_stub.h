// Native-build fakes for the ESP32 BLE stack.
// These exist only so the project's real transport headers PARSE and their
// structs (AstroParamPacket, etc.) stay the single source of truth. No radio
// behaviour is emulated; the command layer is mocked separately.
#pragma once

#include <cstdint>
#include <cstring>
#include <string>

// ---- UUID -------------------------------------------------------------------
class BLEUUID {
public:
    BLEUUID() = default;
    BLEUUID(const char*) {}
    BLEUUID(uint16_t) {}
};

// ---- Addresses / advertised devices ----------------------------------------
class BLEAddress {
public:
    std::string toString() const { return address_; }
    std::string address_;
};

class BLEAdvertisedDevice {
public:
    BLEAddress getAddress() { return {}; }
    std::string getName() { return {}; }
    int getRSSI() { return 0; }
};

// ---- Client-side characteristics / services --------------------------------
class BLERemoteCharacteristic {
public:
    void writeValue(uint8_t*, size_t, bool) {}
};
class BLERemoteService {};
class BLEClient {};
class BLEScan {};

// ---- Server-side ------------------------------------------------------------
class BLECharacteristic {
public:
    void setValue(uint8_t*, size_t) {}
    void notify() {}
    std::string getValue() { return {}; }
};
class BLEService {};
class BLEAdvertising {
public:
    void stop() {}
};
class BLEServer {
public:
    BLEAdvertising* getAdvertising() { return nullptr; }
};

// ---- Callback base classes --------------------------------------------------
struct esp_ble_auth_cmpl_t {
    bool success;
};
class BLESecurityCallbacks {
public:
    virtual ~BLESecurityCallbacks() = default;
    virtual uint32_t onPassKeyRequest() { return 0; }
    virtual void onPassKeyNotify(uint32_t) {}
    virtual bool onConfirmPIN(uint32_t) { return true; }
    virtual bool onSecurityRequest() { return true; }
    virtual void onAuthenticationComplete(esp_ble_auth_cmpl_t) {}
};
class BLEServerCallbacks {
public:
    virtual ~BLEServerCallbacks() = default;
    virtual void onConnect(BLEServer*) {}
    virtual void onDisconnect(BLEServer*) {}
};
class BLECharacteristicCallbacks {
public:
    virtual ~BLECharacteristicCallbacks() = default;
    virtual void onWrite(BLECharacteristic*) {}
};

// ---- Device entry point -----------------------------------------------------
class BLEDevice {
public:
    static void deinit(bool) {}
};
