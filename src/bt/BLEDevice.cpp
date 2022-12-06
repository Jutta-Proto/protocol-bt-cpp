#include <gattlib.h>  // Include first since we have some structs forward declared

#include "bt/BLEDevice.hpp"
#include "bt/ByteEncDecoder.hpp"
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iomanip>
#include <ios>
#include <iostream>
#include <logger/Logger.hpp>
#include <vector>
#include <bluetooth/sdp.h>
#include <spdlog/spdlog.h>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
BLEDevice::BLEDevice(std::string&& name, std::string&& addr, OnCharacteristicReadFunc onCharacteristicRead, OnConnectedFunc onConnected, OnDisconnectedFunc onDisconnected, OnCharacteristicNotificationFunc onCharacteristicNotification) : name(std::move(name)),
                                                                                                                                                                                                                                             addr(std::move(addr)),
                                                                                                                                                                                                                                             onCharacteristicRead(std::move(onCharacteristicRead)),
                                                                                                                                                                                                                                             onConnected(std::move(onConnected)),
                                                                                                                                                                                                                                             onDisconnected(std::move(onDisconnected)),
                                                                                                                                                                                                                                             onCharacteristicNotification(std::move(onCharacteristicNotification)) {}

const std::vector<uint8_t> BLEDevice::to_vec(const uint8_t* data, size_t len) {
    const uint8_t* dataBuf = static_cast<const uint8_t*>(data);
    std::vector<uint8_t> result;
    result.resize(len);
    for (size_t i = 0; i < len; i++) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        result[i] = static_cast<uint8_t>(dataBuf[i]);
    }
    return result;
}

const std::vector<uint8_t> BLEDevice::to_vec(const void* data, size_t len) {
    const uint8_t* dataBuf = static_cast<const uint8_t*>(data);
    return to_vec(dataBuf, len);
}

const std::vector<uint8_t> BLEDevice::get_mam_data() {
    gattlib_advertisement_data_t* adData = nullptr;
    size_t adDataCount = 0;
    uint16_t manId = 0;
    uint8_t* manData = nullptr;
    size_t manDataCount = 0;
    if (gattlib_get_advertisement_data(connection, &adData, &adDataCount, &manId, &manData, &manDataCount) != GATTLIB_SUCCESS) {
        SPDLOG_ERROR("BLE device advertisement data analysis failed.");
        return std::vector<uint8_t>();
    }
    return to_vec(manData, manDataCount);
}

bool BLEDevice::connect() {
    assert(!connection);
    assert(!connected);
    connection = gattlib_connect(nullptr, addr.c_str(), GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
    if (!connection) {
        return false;
    }

    int result = gattlib_discover_primary(connection, &services, &serviceCount);
    if (result != GATTLIB_SUCCESS || serviceCount <= 0) {
        SPDLOG_ERROR("BLE device GATT discovery failed with error code {}.", result);
        result = gattlib_disconnect(connection);
        if (result != GATTLIB_SUCCESS) {
            SPDLOG_ERROR("BLE device disconnect failed with error code {}.", result);
        }
        connection = nullptr;
        return false;
    }

    SPDLOG_DEBUG("Discovered {} services.", serviceCount);
    SPDLOG_DEBUG("BLEDevice connected.");
    gattlib_register_on_disconnect(connection, &BLEDevice::on_disconnected, this);
    gattlib_register_notification(connection, &BLEDevice::on_notification, this);
    connected = true;
    onConnected();
    return true;
}

void BLEDevice::disconnect() {
    if (connection) {
        gattlib_disconnect(connection);
    }
}

bool BLEDevice::is_connected() const {
    return connected;
}

void BLEDevice::read_characteristic(const uuid_t& characteristic) {
    if (!connected) {
        SPDLOG_WARN("Skipping read. Not connected.");
        return;
    }

    uuid_t uuid = characteristic;
    std::array<char, MAX_LEN_UUID_STR + 1> uuidStr{};
    gattlib_uuid_to_string(&uuid, uuidStr.data(), uuidStr.size());

    void* buffer = nullptr;
    size_t bufLen = 0;
    int result = gattlib_read_char_by_uuid(connection, &uuid, &buffer, &bufLen);
    if (result != GATTLIB_SUCCESS) {
        SPDLOG_WARN("Failed to read characteristic '{}' with error code {}.", uuidStr.data(), result);
        return;
    }
    // Convert to a vector:
    const std::vector<uint8_t> data = to_vec(buffer, bufLen);
    // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
    onCharacteristicRead(data, characteristic);
    SPDLOG_TRACE("Read {} bytes from '{}'.", bufLen, uuidStr.data());
}

void BLEDevice::read_characteristics() {
    std::array<char, MAX_LEN_UUID_STR + 1> uuidStr{};
    for (int i = 0; i < serviceCount; i++) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        gattlib_uuid_to_string(&services[i].uuid, uuidStr.data(), uuidStr.size());
        SPDLOG_DEBUG("Found service with UUID: {}", uuidStr.data());
    }

    int characteristics_count = 0;
    gattlib_characteristic_t* characteristics{nullptr};
    gattlib_discover_char(connection, &characteristics, &characteristics_count);
    for (int i = 0; i < characteristics_count; i++) {
        // NOLINTNEXTLINE (cppcoreguidelines-pro-bounds-pointer-arithmetic)
        gattlib_uuid_to_string(&characteristics[i].uuid, uuidStr.data(), uuidStr.size());
        SPDLOG_DEBUG("Found characteristic with UUID: {}", uuidStr.data());
    }
}

bool BLEDevice::write(const uuid_t& characteristic, const std::vector<uint8_t>& data) {
    if (!connected) {
        SPDLOG_WARN("Skipping write. Not connected.");
        return false;
    }
    uuid_t uuid = characteristic;
    std::array<char, MAX_LEN_UUID_STR + 1> uuidStr{};
    gattlib_uuid_to_string(&uuid, uuidStr.data(), uuidStr.size());
    int result = gattlib_write_char_by_uuid(connection, &uuid, data.data(), data.size());
    if (result == GATTLIB_SUCCESS) {
        SPDLOG_TRACE("Wrote {} byte to characteristic '{}'.", data.size(), uuidStr.data());
        return true;
    }
    SPDLOG_ERROR("Failed to write to characteristic '{}' with error code {}!", uuidStr.data(), result);
    return false;
}

bool BLEDevice::subscribe(const uuid_t& characteristic) {
    const uuid_t uuid = characteristic;
    std::array<char, MAX_LEN_UUID_STR + 1> uuidStr{};
    gattlib_uuid_to_string(&uuid, uuidStr.data(), uuidStr.size());
    int result = gattlib_notification_start(connection, &uuid);
    if (result == GATTLIB_SUCCESS) {
        SPDLOG_DEBUG("Subscribed to characteristic '{}'.", uuidStr.data());
        return true;
    }
    SPDLOG_ERROR("Failed to subscribe to characteristic '{}' with error code {}!", uuidStr.data(), result);
    return false;
}

void BLEDevice::on_disconnected(void* arg) {
    BLEDevice* device = static_cast<BLEDevice*>(arg);
    if (device->connected) {
        device->connected = false;
        device->connection = nullptr;
        device->onDisconnected();
        SPDLOG_DEBUG("BLEDevice disconnected.");
    }
}

void BLEDevice::on_notification(const uuid_t* uuid, const uint8_t* data, size_t len, void* arg) {
    BLEDevice* device = static_cast<BLEDevice*>(arg);
    const std::vector<uint8_t> dataVec = BLEDevice::to_vec(data, len);
    device->onCharacteristicNotification(dataVec, *uuid);
}
//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------