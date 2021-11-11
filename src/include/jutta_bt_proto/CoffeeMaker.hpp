#pragma once

#include "bt/BLEDevice.hpp"
#include "date/date.hpp"
#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

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

struct ManufacturerData {
    uint8_t key{0};
    uint8_t bfMajVer{0};
    uint8_t bfMinVer{0};
    uint16_t articleNumber{0};
    uint16_t machineNumber{0};
    uint16_t serialNumber{0};
    date::year_month_day machineProdDate{};
    date::year_month_day machineProdDateUCHI{};
    uint8_t unusedSecond{0};
    uint8_t statusBits{0};
} __attribute__((aligned(32)));

struct AboutData {
    std::string blueFrogVersion{};
    std::string coffeeMachineVersion{};
} __attribute__((aligned(64)));

class CoffeeMaker {
 public:
    static const RelevantUUIDs RELEVANT_UUIDS;

    using StateChangedEventHandler = std::function<void(const CoffeeMakerState&)>;
    using ManDataChangedEventHandler = std::function<void(const ManufacturerData&)>;
    using AboutDataChangedEventHandler = std::function<void(const AboutData&)>;
    using JoeChangedEventHandler = std::function<void(const std::shared_ptr<Joe>&)>;
    using AlertsChangedEventHandler = std::function<void(const std::vector<const Alert*>&)>;

 private:
    bt::BLEDevice bleDevice;
    CoffeeMakerState state{CoffeeMakerState::DISCONNECTED};
    std::optional<std::thread> heartbeatThread{std::nullopt};

    const std::unordered_map<size_t, const Machine> machines;

    std::shared_ptr<Joe> joe{nullptr};
    ManufacturerData manData{};
    AboutData aboutData{};
    std::vector<const Alert*> alerts{};

    // Event handler:
    std::unique_ptr<StateChangedEventHandler> stateChangedEventHandler{nullptr};
    std::unique_ptr<ManDataChangedEventHandler> manDataChangedEventHandler{nullptr};
    std::unique_ptr<AboutDataChangedEventHandler> aboutDataChangedEventHandler{nullptr};
    std::unique_ptr<JoeChangedEventHandler> joeChangedEventHandler{nullptr};
    std::unique_ptr<AlertsChangedEventHandler> alertsChangedEventHandler{nullptr};

 public:
    explicit CoffeeMaker(std::string&& name, std::string&& addr);
    CoffeeMaker(CoffeeMaker&&) = default;
    CoffeeMaker(const CoffeeMaker&) = delete;
    CoffeeMaker& operator=(CoffeeMaker&&) = delete;
    CoffeeMaker& operator=(const CoffeeMaker&) = delete;
    ~CoffeeMaker() = default;

    void set_state_changed_event_handler(StateChangedEventHandler handler);
    void set_man_data_changed_event_handler(ManDataChangedEventHandler handler);
    void set_about_data_changed_event_handler(AboutDataChangedEventHandler handler);
    void set_joe_changed_event_handler(JoeChangedEventHandler handler);
    void set_alerts_changed_event_handler(AlertsChangedEventHandler handler);

    void clear_state_changed_event_handler();
    void clear_man_data_changed_event_handler();
    void clear_about_data_changed_event_handler();
    void clear_joe_changed_event_handler();
    void clear_alerts_changed_event_handler();

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
    [[nodiscard]] CoffeeMakerState get_state() const;
    [[nodiscard]] const std::shared_ptr<Joe>& get_joe() const;
    [[nodiscard]] const ManufacturerData& get_man_data() const;
    [[nodiscard]] const AboutData& get_about_data() const;
    [[nodiscard]] const std::vector<const Alert*>& get_alerts() const;
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
    void request_coffee(const Product& product);

 private:
    void set_state(CoffeeMakerState state);
    /**
     * Analyzes the manufacturer specific data from the advertisement send by the coffee maker.
     * Extracts the encryption key, machine number, serial number, ...
     **/
    void analyze_man_data();
    void parse_man_data(const std::vector<uint8_t>& data);
    void parse_about_data(const std::vector<uint8_t>& data);
    static void parse_product_progress(const std::vector<uint8_t>& data, uint8_t key);
    void parse_machine_status(const std::vector<uint8_t>& data, uint8_t key);
    static void parse_rx(const std::vector<uint8_t>& data, uint8_t key);
    static std::string parse_version(const std::vector<uint8_t>& data, size_t from, size_t to);
    /**
     * Converts the given data to an uint16_t from little endian.
     **/
    static uint16_t to_uint16_t_little_endian(const std::vector<uint8_t>& data, size_t offset);
    /**
     * Parses the given data as a date::year_month_day object.
     **/
    static date::year_month_day to_ymd(const std::vector<uint8_t>& data, size_t offset);
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