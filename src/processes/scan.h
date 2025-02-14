#pragma once

#include "components/menu_system.h"
#include "transport/ble_device.h"
#include "utils/display_constants.h"

class ScanProcess {
public:
    enum class Status { Scanning, NoDevices, DevicesFound, Connecting, Connected, Failed };

    struct ScanState {
        Status status;
        bool isScanning;
        bool isConnecting;
        const std::vector<DeviceInfo>& discoveredDevices;
    };

    static bool startScan(int duration = 5) { return BLEDeviceManager::startScan(duration); }

    static ScanState getState() {
        Status status;
        bool isScanning = BLEDeviceManager::isScanning();
        const auto& devices = BLEDeviceManager::getDiscoveredDevices();

        if (isScanning) {
            status = Status::Scanning;
        } else if (devices.empty()) {
            status = Status::NoDevices;
        } else {
            status = Status::DevicesFound;
        }

        return ScanState{status, isScanning, false, devices};
    }

    static bool connectToDevice(const BLEAdvertisedDevice* device) {
        if (!device) {
            return false;
        }
        return BLEDeviceManager::connectToCamera(device);
    }

    static void clearDevices() { BLEDeviceManager::clearDiscoveredDevices(); }

    static uint16_t getStatusColor(Status status) {
        auto& display = MenuSystem::getHardware()->getDisplay();
        switch (status) {
            case Status::Scanning:
            case Status::Connecting:
                return display.getColor(display::colors::YELLOW);
            case Status::Connected:
                return display.getColor(display::colors::GREEN);
            case Status::Failed:
            case Status::NoDevices:
                return display.getColor(display::colors::RED);
            case Status::DevicesFound:
                return display.getColor(display::colors::GRAY_800);
        }
        return display.getColor(display::colors::GRAY_800);
    }

    static const char* getStatusText(Status status) {
        switch (status) {
            case Status::Scanning:
                return "Scanning...";
            case Status::Connecting:
                return "Connecting...";
            case Status::Connected:
                return "Connected!";
            case Status::Failed:
                return "Connection failed!";
            case Status::NoDevices:
                return "No devices";
            case Status::DevicesFound:
                return "Select camera";
        }
        return "";
    }
};
