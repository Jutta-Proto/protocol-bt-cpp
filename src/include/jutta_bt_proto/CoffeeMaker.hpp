#pragma once

#include "bt/BLEDevice.hpp"
#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include <chrono>
#include <cstddef>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <bluetooth/sdp.h>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
struct RelevantUUIDs {
    uuid_t DEFAULT_SERVICE_UUID{};
    uuid_t ABOUT_MACHINE_CHARACTERISTIC_UUID{};
    uuid_t MACHINE_STATUS_CHARACTERISTIC_UUID{};
    uuid_t BARISTA_MODE_CHARACTERISTIC_UUID{};
    uuid_t PRODUCT_PROGRESS_CHARACTERISTIC_UUID{};
    uuid_t P_MODE_CHARACTERISTIC_UUID{};
    uuid_t P_MODE_READ_CHARACTERISTIC_UUID{};
    uuid_t START_PRODUCT_CHARACTERISTIC_UUID{};
    uuid_t STATISTICS_COMMAND_CHARACTERISTIC_UUID{};
    uuid_t STATISTICS_DATA_CHARACTERISTIC_UUID{};
    uuid_t UPDATE_PRODUCT_CHARACTERISTIC_UUID{};

    uuid_t UART_SERVICE_UUID{};
    uuid_t UART_TX_CHARACTERISTIC_UUID{};
    uuid_t UART_RX_CHARACTERISTIC_UUID{};

    RelevantUUIDs() noexcept;

 private:
    static void to_uuid(const std::string& s, uuid_t* uuid);
} __attribute__((aligned(128)));

enum CoffeeMakerState {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    DISCONNECTING
};

class CoffeeMaker {
 public:
    static const RelevantUUIDs RELEVANT_UUIDS;

 private:
    bt::BLEDevice bleDevice;
    CoffeeMakerState state{CoffeeMakerState::DISCONNECTED};
    std::optional<std::thread> heartbeatThread{std::nullopt};

    const std::unordered_map<size_t, const Machine> machines;
    const Machine* machine{nullptr};

    // Manufacturer advertisment data:
    uint8_t key{0};
    uint8_t bfMajVer{0};
    uint8_t bfMinVer{0};
    uint16_t articleNumber{0};
    uint16_t machineNumber{0};
    uint16_t serialNumber{0};
    std::chrono::year_month_day machineProdDate{};
    std::chrono::year_month_day machineProdDateUCHI{};
    uint8_t unusedSecond{0};
    uint8_t statusBits{0};

    // About data:
    std::string blueFrogVersion{};
    std::string coffeeMachineVersion{};

 public:
    explicit CoffeeMaker(std::string&& name, std::string&& addr);
    CoffeeMaker(CoffeeMaker&&) = default;
    CoffeeMaker(const CoffeeMaker&) = delete;
    CoffeeMaker& operator=(CoffeeMaker&&) = delete;
    CoffeeMaker& operator=(const CoffeeMaker&) = delete;
    ~CoffeeMaker() = default;

    /**
     * Connects to the bluetooth device and returns true on success.
     **/
    bool connect();
    /**
     * Gracefully disconnects from the coffee maker.
     **/
    void disconnect();
    /**
     * Returns the CoffeeMakerState indicating the current connection state.
     **/
    CoffeeMakerState get_state();
    /**
     * Performs a gracefull shutdown with rinsing.
     **/
    void shutdown();
    /**
     * Requests the current status of the coffee maker.
     **/
    void request_status();
    /**
     * Requests the current product progress.
     **/
    void request_progress();
    /**
     * Requests the about coffee maker info.
     **/
    void request_about_info();
    /**
     * Heartbeat that should be send at least once every ten seconds, so the coffee maker stays connected.
     **/
    void stay_in_ble();
    /**
     * Reads from the RX characteristic.
     **/
    void read_rx();
    /**
     * Sends the given data to the TX characteristic.
     * Before sending, the data will be encoded.
     **/
    void write_tx(const std::vector<uint8_t>& data);
    /**
     * Sends the given string to the TX characteristic.
     * Before sending, the data will be encoded.
     **/
    void write_tx(const std::string& s);
    void request_coffee();

 private:
    /**
     * Analyzes the manufacturer specific data from the advertisement send by the coffee maker.
     * Extracts the encryption key, machine number, serial number, ...
     **/
    void analyze_man_data();
    void parse_man_data(const std::vector<uint8_t>& data);
    void parse_about_data(const std::vector<uint8_t>& data);
    static void parse_product_progress(const std::vector<uint8_t>& data, uint8_t key);
    static void parse_machine_status(const std::vector<uint8_t>& data, uint8_t key);
    static void parse_rx(const std::vector<uint8_t>& data, uint8_t key);
    static std::string parse_version(const std::vector<uint8_t>& data, size_t from, size_t to);
    /**
     * Converts the given data to an uint16_t from little endian.
     **/
    static uint16_t to_uint16_t_little_endian(const std::vector<uint8_t>& data, size_t offset);
    /**
     * Parses the given data as a std::chrono::year_month_day object.
     **/
    static std::chrono::year_month_day to_ymd(const std::vector<uint8_t>& data, size_t offset);
    /**
     * Writes the given data to the given characteristic.
     * Allows you to specify wether the data should be encoded and the key inside the data should be overriden.
     * Usually you only want to set encode to true.
     **/
    bool write(const uuid_t& characteristic, const std::vector<uint8_t>& data, bool encode, bool overrideKey);
    /**
     * Event handler that gets triggered when a characteristic got read.
     * data: The data read which might be encoded and has to be decoded.
     **/
    void on_characteristic_read(const std::vector<uint8_t>& data, const uuid_t& uuid);
    /**
     * Event handler that gets triggered when the coffee maker is connetced.
     **/
    void on_connected();
    /**
     * Event handler that gets triggered when the coffee maker is disconnected.
     **/
    void on_disconnected();
    /**
     * Handels the periodic sending of the heartbeat to the coffee maker.
     * Should be the entry point of a new thread.
     **/
    void heartbeat_run();
};
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------