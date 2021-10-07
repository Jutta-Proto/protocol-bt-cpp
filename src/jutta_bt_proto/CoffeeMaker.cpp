#include "jutta_bt_proto/CoffeeMaker.hpp"
#include "bt/ByteEncDecoder.hpp"
#include "gattlib.h"
#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include "jutta_bt_proto/Utils.hpp"
#include "logger/Logger.hpp"
#include <cassert>
#include <chrono>
#include <cstddef>
#include <cstdint>
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
    blueFrogVersion = parse_version(data, 27, 34);
    coffeeMachineVersion = parse_version(data, 35, 50);
    SPDLOG_DEBUG("Found about data. BlueFrog Version: {} Coffee Machine Version: {}", blueFrogVersion, coffeeMachineVersion);
}

void CoffeeMaker::parse_machine_status(const std::vector<uint8_t>& data, uint8_t key) {
    if (!joe) {
        return;
    }

    std::vector<std::uint8_t> alertVec = bt::encDecBytes(data, key);
    for (size_t i = 0; i < (alertVec.size() - 1) << 3; i++) {
        size_t offsetAbs = (i >> 3) + 1;
        size_t offsetByte = 7 - (i & 0b111);
        if ((alertVec[offsetAbs] >> offsetByte) & 0b1) {
            for (const Alert& alert : joe->alerts) {
                if (alert.bit == i) {
                    SPDLOG_INFO("ALERT '{}' set.", alert.name);
                }
            }
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
    key = data[0];
    bfMajVer = data[1];
    bfMinVer = data[2];
    articleNumber = to_uint16_t_little_endian(data, 4);
    machineNumber = to_uint16_t_little_endian(data, 6);
    serialNumber = to_uint16_t_little_endian(data, 8);
    machineProdDate = to_ymd(data, 10);
    machineProdDateUCHI = to_ymd(data, 12);
    unusedSecond = data[14];
    statusBits = data[15];

    // Load machine:
    if (!machines.contains(articleNumber)) {
        SPDLOG_ERROR("Coffee maker with article number '{}' not supported with the given machine files.", articleNumber);
        // NOLINTNEXTLINE(concurrency-mt-unsafe)
        exit(-1);
    }
    const Machine* machine = &(machines.at(articleNumber));
    joe = load_joe(machine);
    SPDLOG_INFO("Found machine '{}' Version: {} with {} products.", machine->name, machine->version, joe->products.size());
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
    SPDLOG_INFO("Received {} bytes of data from characteristic '{}'.", data.size(), uuidStr.data());

    // About UUID:
    if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.ABOUT_MACHINE_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_about_data(data);
    }
    // Machine status:
    else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.MACHINE_STATUS_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_machine_status(data, key);
    }  // Product progress:
    else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.PRODUCT_PROGRESS_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_product_progress(data, key);
    } else if (gattlib_uuid_cmp(&uuid, &RELEVANT_UUIDS.UART_RX_CHARACTERISTIC_UUID) == GATTLIB_SUCCESS) {
        parse_rx(data, key);
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
        encodedData[0] = key;
        if (overrideKey) {
            encodedData[encodedData.size() - 1] = key;
        }
        encodedData = bt::encDecBytes(encodedData, key);
    }
    SPDLOG_TRACE("Wrote: {}", to_hex_string(encodedData));
    return bleDevice.write(characteristic, encodedData);
}

void CoffeeMaker::shutdown() {
    SPDLOG_INFO("Shutting down the coffee maker...");
    static const std::vector<uint8_t> command{0x00, 0x46, 0x02};
    write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::request_coffee() {
    SPDLOG_INFO("Requesting coffee...");
    static const std::string commandHexStr = "0003000428000002000100000000002A";
    // static const std::string commandHexStr = "77e93dd55381d3dba32bfa98a4a3faf9";  // Decoded: 2A03000414000001000100000000002A
    static const std::vector<uint8_t> command = from_hex_string(commandHexStr);
    write(RELEVANT_UUIDS.START_PRODUCT_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::stay_in_ble() {
    SPDLOG_INFO("Sending stay in BLE mode...");
    static const std::vector<uint8_t> command{0x00, 0x7F, 0x80};
    write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);
}

void CoffeeMaker::on_connected() {
    // Ensure we have the key for deobfuscation ready:
    analyze_man_data();
    // Send the initial hearbeat:
    stay_in_ble();
    // Request basic information:
    request_about_info();
}

void CoffeeMaker::on_disconnected() {}

bool CoffeeMaker::connect() {
    state = CoffeeMakerState::CONNECTING;
    if (bleDevice.connect()) {
        state = CoffeeMakerState::CONNECTED;

        // Start the heartbeat thread:
        assert(!heartbeatThread);
        heartbeatThread = std::make_optional<std::thread>(&CoffeeMaker::heartbeat_run, this);
        SPDLOG_INFO("Connected.");
        return true;
    }
    state = CoffeeMakerState::DISCONNECTED;
    SPDLOG_WARN("Failed to connect.");
    return false;
}

void CoffeeMaker::disconnect() {
    if (state == CoffeeMakerState::CONNECTING || state == CoffeeMakerState::CONNECTED) {
        state = CoffeeMakerState::DISCONNECTING;

        // Send the disconnect command:
        static const std::vector<uint8_t> command{0x00, 0x7F, 0x81};
        write(RELEVANT_UUIDS.P_MODE_CHARACTERISTIC_UUID, command, true, false);

        // Join the heartbeat thread:
        assert(heartbeatThread);
        heartbeatThread->join();
        heartbeatThread = std::nullopt;
        state = CoffeeMakerState::DISCONNECTED;
        SPDLOG_INFO("Disconnected.");
    }
}

CoffeeMakerState CoffeeMaker::get_state() {
    return state;
}

void CoffeeMaker::heartbeat_run() {
    SPDLOG_INFO("Heartbeat thread started.");
    while (state == CoffeeMakerState::CONNECTED || state == CoffeeMakerState::CONNECTING) {
        stay_in_ble();
        std::this_thread::sleep_for(std::chrono::seconds{1});
    }
    SPDLOG_INFO("Heartbeat thread ready to be joined.");
}
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------