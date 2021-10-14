#include <gattlib.h>  // Include first since we have some structs forward declared

#include "bt/ByteEncDecoder.hpp"
#include "jutta_bt_proto/CoffeeMaker.hpp"
#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include "jutta_bt_proto/Utils.hpp"
#include "logger/Logger.hpp"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <bluetooth/sdp.h>
#include <spdlog/spdlog.h>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
RelevantUUIDs::RelevantUUIDs() noexcept {
    try {
        to_uuid("5a401523-ab2e-2548-c435-08c300000710", &DEFAULT_SERVICE_UUID);
        to_uuid("5A401531-AB2E-2548-C435-08C300000710", &ABOUT_MACHINE_CHARACTERISTIC_UUID);
        to_uuid("5a401524-ab2e-2548-c435-08c300000710", &MACHINE_STATUS_CHARACTERISTIC_UUID);
        to_uuid("5a401530-ab2e-2548-c435-08c300000710", &BARISTA_MODE_CHARACTERISTIC_UUID);
        to_uuid("5a401527-ab2e-2548-c435-08c300000710", &PRODUCT_PROGRESS_CHARACTERISTIC_UUID);
        to_uuid("5a401529-ab2e-2548-c435-08c300000710", &P_MODE_CHARACTERISTIC_UUID);
        to_uuid("5a401538-ab2e-2548-c435-08c300000710", &P_MODE_READ_CHARACTERISTIC_UUID);
        to_uuid("5a401525-ab2e-2548-c435-08c300000710", &START_PRODUCT_CHARACTERISTIC_UUID);
        to_uuid("5A401533-ab2e-2548-c435-08c300000710", &STATISTICS_COMMAND_CHARACTERISTIC_UUID);
        to_uuid("5A401534-ab2e-2548-c435-08c300000710", &STATISTICS_DATA_CHARACTERISTIC_UUID);
        to_uuid("5a401528-ab2e-2548-c435-08c300000710", &UPDATE_PRODUCT_CHARACTERISTIC_UUID);

        to_uuid("5a401623-ab2e-2548-c435-08c300000710", &UART_SERVICE_UUID);
        to_uuid("5a401624-ab2e-2548-c435-08c300000710", &UART_RX_CHARACTERISTIC_UUID);
        to_uuid("5a401625-ab2e-2548-c435-08c300000710", &UART_TX_CHARACTERISTIC_UUID);
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Loading UUIDS failed with: {}", e.what());
        std::terminate();
    }
}

void RelevantUUIDs::to_uuid(const std::string& s, uuid_t* uuid) {
    gattlib_string_to_uuid(s.data(), s.size(), uuid);
}

const RelevantUUIDs CoffeeMaker::RELEVANT_UUIDS{};

CoffeeMaker::CoffeeMaker(std::string&& name, std::string&& addr) : bleDevice(
                                                                       std::move(name),
                                                                       std::move(addr),
                                                                       [this](const std::vector<uint8_t>& data, const uuid_t& uuid) { this->on_characteristic_read(data, uuid); },
                                                                       [this]() { this->on_connected(); },
                                                                       [this]() { this->on_disconnected(); },
                                                                       [this](const std::vector<uint8_t>& data, const uuid_t& uuid) { this->on_characteristic_read(data, uuid); }),
                                                                   machines(load_machines("../resources/machinefiles/JOE_MACHINES.TXT")) {}

std::string CoffeeMaker::parse_version(const std::vector<uint8_t>& data, size_t from, size_t to) {
    std::string result;
    for (size_t i = from; i <= to; i++) {
        if (data[i]) {
            result += static_cast<char>(data[i]);
        }
    }
    return result;
}

void CoffeeMaker::parse_about_data(const std::vector<uint8_t>& data) {
    std::string blueFrogVersion = parse_version(data, 27, 34);
    std::string coffeeMachineVersion = parse_version(data, 35, 50);
    if (blueFrogVersion != aboutData.blueFrogVersion || coffeeMachineVersion != aboutData.coffeeMachineVersion) {
        aboutData.blueFrogVersion = std::move(blueFrogVersion);
        aboutData.coffeeMachineVersion = std::move(coffeeMachineVersion);
        SPDLOG_DEBUG("Found new about data. BlueFrog Version: {} Coffee Machine Version: {}", aboutData.blueFrogVersion, aboutData.coffeeMachineVersion);

        // Invoke the about data event handler:
        if (aboutDataChangedEventHandler) {
            (*aboutDataChangedEventHandler)(aboutData);
        }
    }
}

void CoffeeMaker::parse_machine_status(const std::vector<uint8_t>& data, uint8_t key) {
    if (!joe) {
        return;
    }

    std::vector<const Alert*> newAlerts;
    std::vector<std::uint8_t> alertVec = bt::encDecBytes(data, key);
    for (size_t i = 0; i < (alertVec.size() - 1) << 3; i++) {
        size_t offsetAbs = (i >> 3) + 1;
        size_t offsetByte = 7 - (i & 0b111);
        if ((alertVec[offsetAbs] >> offsetByte) & 0b1) {
            for (const Alert& alert : joe->alerts) {
                if (alert.bit == i) {
                    newAlerts.push_back(&alert);
                }
            }
        }
    }

    if (alerts != newAlerts) {
        alerts.clear();
        alerts.insert(alerts.end(), newAlerts.begin(), newAlerts.end());

        // Invoke the alerts event handler:
        if (alertsChangedEventHandler) {
            (*alertsChangedEventHandler)(alerts);
        }
    }
}

void CoffeeMaker::parse_product_progress(const std::vector<uint8_t>& data, uint8_t key) {
    std::vector<std::uint8_t> actData = bt::encDecBytes(data, key);
}

void CoffeeMaker::analyze_man_data() {
    parse_man_data(bleDevice.get_mam_data());
}

void CoffeeMaker::parse_man_data(const std::vector<uint8_t>& data) {
    manData.key = data[0];
    manData.bfMajVer = data[1];
    manData.bfMinVer = data[2];
    manData.articleNumber = to_uint16_t_little_endian(data, 4);
    manData.machineNumber = to_uint16_t_little_endian(data, 6);
    manData.serialNumber = to_uint16_t_little_endian(data, 8);
    manData.machineProdDate = to_ymd(data, 10);
    manData.machineProdDateUCHI = to_ymd(data, 12);
    manData.unusedSecond = data[14];
    manData.statusBits = data[15];

    // Invoke the manufacturer data event handler:
    if (manDataChangedEventHandler) {
        (*manDataChangedEventHandler)(manData);
    }

    // Load machine:
    if (!machines.contains(manData.articleNumber)) {
        SPDLOG_ERROR("Coffee maker with article number '{}' not supported with the given machine files.", manData.articleNumber);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(-1);
    }
    const Machine* machine = &(machines.at(manData.articleNumber));
    joe = load_joe(machine);
    alerts.clear();
    SPDLOG_INFO("Found machine '{}' Version: {} with {} products.", machine->name, machine->version, joe->products.size());

    // Invoke the JOE event handler:
    if (joeChangedEventHandler) {
        (*joeChangedEventHandler)(joe);
    }
}

void CoffeeMaker::parse_rx(const std::vector<uint8_t>& data, uint8_t key) {
    std::vector<std::uint8_t> actData = bt::encDecBytes(data, key);
    SPDLOG_INFO("Read from RX: {}", to_hex_string(actData));
}

uint16_t CoffeeMaker::to_uint16_t_little_endian(const std::vector<uint8_t>& data, size_t offset) {
    return (static_cast<uint16_t>(data[offset + 1]) << 8) | static_cast<uint16_t>(data[offset]);
}

std::chrono::year_month_day CoffeeMaker::to_ymd(const std::vector<uint8_t>& data, size_t offset) {
    uint16_t date = to_uint16_t_little_endian(data, offset);
    return std::chrono::year(((date & 65024) >> 9) + 1990) / ((date & 480) >> 5) / (date & 31);
}

void CoffeeMaker::on_characteristic_read(const std::vector<uint8_t>& data, const uuid_t& uuid) {
    std::array<char, MAX_LEN_UUID_STR + 1> uuidStr{};
    gattlib_uuid_to_string(&uuid, uuidStr.data(), uuidStr.size());
    SPDLOG_TRACE("Received {} bytes of data from characteristic '{}'.", data.size(), uuidStr.data());

    // About UUID:
    if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.ABOUT_MACHINE_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_about_data(data);
    }
    // Machine status:
    else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.MACHINE_STATUS_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_machine_status(data, manData.key);
    }
    // Product progress:
    else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.PRODUCT_PROGRESS_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_product_progress(data, manData.key);
    }
    // RX:
    else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.UART_RX_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_rx(data, manData.key);
    } else {
        // TODO print
    }
}
void CoffeeMaker::request_status() {
    bleDevice.read_characteristic(RELEVANT_UUIDS.MACHINE_STATUS_CHARACTERISTIC_UUID);
}

void CoffeeMaker::request_progress() {
    bleDevice.read_characteristic(RELEVANT_UUIDS.PRODUCT_PROGRESS_CHARACTERISTIC_UUID);
}

void CoffeeMaker::request_about_info() {
    bleDevice.read_characteristic(RELEVANT_UUIDS.ABOUT_MACHINE_CHARACTERISTIC_UUID);
}

void CoffeeMaker::read_rx() {
    bleDevice.read_characteristic(RELEVANT_UUIDS.UART_RX_CHARACTERISTIC_UUID);
}

void CoffeeMaker::write_tx(const std::string& s) {
    const std::vector<uint8_t> data(s.begin(), s.end());
    write_tx(data);
}

void CoffeeMaker::write_tx(const std::vector<uint8_t>& data) {
    write(RELEVANT_UUIDS.UART_TX_CHARACTERISTIC_UUID, data, true, false);
}

bool CoffeeMaker::write(const uuid_t& characteristic, const std::vector<uint8_t>& data, bool encode, bool overrideKey) {
    std::vector<uint8_t> encodedData = data;
    if (encode) {
        encodedData[0] = manData.key;
        if (overrideKey) {
            encodedData[encodedData.size() - 1] = manData.key;
        }
        encodedData = bt::encDecBytes(encodedData, manData.key);
    }
    SPDLOG_TRACE("Wrote: {}", to_hex_string(encodedData));
    return bleDevice.write(characteristic, encodedData);
}

void CoffeeMaker::shutdown() {
    SPDLOG_DEBUG("Shutting down the coffee maker...");
    static const std::vector<uint8_t> command{0x00, 0x46, 0x02};
    write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::request_coffee() {
    SPDLOG_DEBUG("Requesting coffee...");
    static const std::string commandHexStr = "00030004280000020001000000000000";
    // static const std::string commandHexStr = "77e93dd55381d3dba32bfa98a4a3faf9";  // Decoded: 2A03000414000001000100000000002A
    static const std::vector<uint8_t> command = from_hex_string(commandHexStr);
    write(RELEVANT_UUIDS.START_PRODUCT_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::request_coffee(const Product& product) {
    const std::string commandHexStr = product.to_bt_command();
    static const std::vector<uint8_t> command = from_hex_string(commandHexStr);
    write(RELEVANT_UUIDS.START_PRODUCT_CHARACTERISTIC_UUID, command, true, true);
}

void CoffeeMaker::stay_in_ble() {
    SPDLOG_DEBUG("Sending stay in BLE mode...");
    static const std::vector<uint8_t> command{0x00, 0x7F, 0x80};
    write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::on_connected() {
    // Ensure we have the key for deobfuscation ready:
    const std::vector<uint8_t>& manData = bleDevice.get_mam_data();
    if (manData.empty()) {
        SPDLOG_WARN("Failed to connect. Invalid manufacturer data.");
        disconnect();
        return;
    }
    parse_man_data(manData);

    // Send the initial hearbeat:
    stay_in_ble();

    // Request basic information:
    request_about_info();

    // Start the heartbeat thread:
    assert(!heartbeatThread);
    heartbeatThread = std::make_optional<std::thread>(&CoffeeMaker::heartbeat_run, this);
    SPDLOG_INFO("Connected.");
}

void CoffeeMaker::on_disconnected() {}

bool CoffeeMaker::connect() {
    set_state(CoffeeMakerState::CONNECTING);
    if (bleDevice.connect()) {
        set_state(CoffeeMakerState::CONNECTED);
        return true;
    }
    set_state(CoffeeMakerState::DISCONNECTED);
    SPDLOG_WARN("Failed to connect.");
    return false;
}

void CoffeeMaker::disconnect() {
    if (state == CoffeeMakerState::CONNECTING || state == CoffeeMakerState::CONNECTED) {
        set_state(CoffeeMakerState::CONNECTING);

        // Send the disconnect command:
        static const std::vector<uint8_t> command{0x00, 0x7F, 0x81};
        write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);

        // Join the heartbeat thread:
        assert(heartbeatThread);
        heartbeatThread->join();
        heartbeatThread = std::nullopt;
        set_state(CoffeeMakerState::DISCONNECTED);
        SPDLOG_INFO("Disconnected.");
    }
}

CoffeeMakerState CoffeeMaker::get_state() const { return state; }

const std::shared_ptr<Joe>& CoffeeMaker::get_joe() const { return joe; }

const ManufacturerData& CoffeeMaker::get_man_data() const { return manData; }

const AboutData& CoffeeMaker::get_about_data() const { return aboutData; }

const std::vector<const Alert*>& CoffeeMaker::get_alerts() const { return alerts; }

void CoffeeMaker::set_state(CoffeeMakerState state) {
    if (state != this->state) {
        this->state = state;
        // Invoke the state event handler:
        if (stateChangedEventHandler) {
            (*stateChangedEventHandler)(this->state);
        }
    }
}

void CoffeeMaker::heartbeat_run() {
    SPDLOG_INFO("Heartbeat thread started.");
    while (state == CoffeeMakerState::CONNECTED || state == CoffeeMakerState::CONNECTING) {
        stay_in_ble();
        request_status();
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
    SPDLOG_INFO("Heartbeat thread ready to be joined.");
}

void CoffeeMaker::set_state_changed_event_handler(StateChangedEventHandler handler) { stateChangedEventHandler = std::make_unique<StateChangedEventHandler>(std::move(handler)); }
void CoffeeMaker::set_man_data_changed_event_handler(ManDataChangedEventHandler handler) { manDataChangedEventHandler = std::make_unique<ManDataChangedEventHandler>(std::move(handler)); }
void CoffeeMaker::set_about_data_changed_event_handler(AboutDataChangedEventHandler handler) { aboutDataChangedEventHandler = std::make_unique<AboutDataChangedEventHandler>(std::move(handler)); }
void CoffeeMaker::set_joe_changed_event_handler(JoeChangedEventHandler handler) { joeChangedEventHandler = std::make_unique<JoeChangedEventHandler>(std::move(handler)); }
void CoffeeMaker::set_alerts_changed_event_handler(AlertsChangedEventHandler handler) { alertsChangedEventHandler = std::make_unique<AlertsChangedEventHandler>(std::move(handler)); }

void CoffeeMaker::clear_state_changed_event_handler() { stateChangedEventHandler = nullptr; }
void CoffeeMaker::clear_man_data_changed_event_handler() { manDataChangedEventHandler = nullptr; }
void CoffeeMaker::clear_about_data_changed_event_handler() { aboutDataChangedEventHandler = nullptr; }
void CoffeeMaker::clear_joe_changed_event_handler() { joeChangedEventHandler = nullptr; }
void CoffeeMaker::clear_alerts_changed_event_handler() { alertsChangedEventHandler = nullptr; }
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------