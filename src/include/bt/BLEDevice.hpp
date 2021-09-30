#pragma once

#include "gattlib.h"
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <bluetooth/sdp.h>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
class BLEDevice {
 public:
    using OnCharacteristicReadFunc = std::function<void(const std::vector<uint8_t>&, const uuid_t&)>;
    using OnCharacteristicNotificationFunc = std::function<void(const std::vector<uint8_t>&, const uuid_t&)>;
    using OnConnectedFunc = std::function<void()>;
    using OnDisconnectedFunc = std::function<void()>;

 private:
    const std::string name;
    const std::string addr;

    OnCharacteristicReadFunc onCharacteristicRead;
    OnConnectedFunc onConnected;
    OnDisconnectedFunc onDisconnected;
    OnCharacteristicNotificationFunc onCharacteristicNotification;

    gatt_connection_t* connection{nullptr};
    int serviceCount{0};
    gattlib_primary_service_t* services{nullptr};

    bool connected{false};

 public:
    BLEDevice(std::string&& name, std::string&& addr, OnCharacteristicReadFunc onCharacteristicRead, OnConnectedFunc onConnected, OnDisconnectedFunc onDisconnected, OnCharacteristicNotificationFunc onCharacteristicNotification);
    BLEDevice(BLEDevice&&) = default;
    BLEDevice(const BLEDevice&) = default;
    BLEDevice& operator=(BLEDevice&&) = delete;
    BLEDevice& operator=(const BLEDevice&) = delete;
    ~BLEDevice() = default;

    bool connect();
    [[nodiscard]] bool is_connected() const;
    const std::vector<uint8_t> get_mam_data();
    void read_characteristics();
    void read_characteristic(const uuid_t& characteristic);
    bool write(const uuid_t& characteristic, const std::vector<uint8_t>& data);
    bool subscribe(const uuid_t& characteristic);

 private:
    static const std::vector<uint8_t> to_vec(const void* data, size_t len);
    static const std::vector<uint8_t> to_vec(const uint8_t* data, size_t len);

    static void on_disconnected(void* arg);
    static void on_notification(const uuid_t* uuid, const uint8_t* data, size_t len, void* arg);
};
//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------