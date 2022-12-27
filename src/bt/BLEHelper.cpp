#include "bt/BLEHelper.hpp"
#include <chrono>
#include <logger/Logger.hpp>
#include <memory>
#include <optional>
#include <regex>
#include <thread>
#include <gattlib.h>
#include <spdlog/spdlog.h>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
void on_device_discovered(void* adapter, const char* addr, const char* name, void* userData) {
    ScanArgs* args = static_cast<ScanArgs*>(userData);
    args->m.lock();
    if (name && std::regex_match(name, args->nameRegex)) {
        args->success = true;
        args->name = name;
        args->addr = addr;
        gattlib_adapter_scan_disable(adapter);
        SPDLOG_INFO("Coffee maker found!");
        args->doneMutex.unlock();
    }
    args->m.unlock();
    if (name) {
        SPDLOG_DEBUG("FOUND: {}", name);
    }
}

std::shared_ptr<ScanArgs> scan_for_device(const std::string& regexStr, const bool* canceled) {
    SPDLOG_DEBUG("Scanning for devices...");
    void* adapter = nullptr;
    int result = gattlib_adapter_open(nullptr, &adapter);
    if (result != GATTLIB_SUCCESS) {
        SPDLOG_ERROR("Failed to open Bluetooth adapter with error code {}.", result);
        return nullptr;
    }

    std::shared_ptr<ScanArgs> args = std::make_shared<ScanArgs>();
    args->nameRegex = std::regex(regexStr);
    args->doneMutex.lock();

    size_t timeoutSeconds = 0;
    if (gattlib_adapter_scan_enable(adapter, &on_device_discovered, timeoutSeconds, args.get())) {
        SPDLOG_ERROR("Bluetooth scan failed.");
        gattlib_adapter_close(adapter);
        return nullptr;
    }
    while (true) {
        if (args->doneMutex.try_lock()) {
            args->doneMutex.unlock();
            break;
        }
        if (*canceled) {
            SPDLOG_DEBUG("Stopping scann...");
            gattlib_adapter_scan_disable(adapter);
            // Wait for the scann to finish:
            args->doneMutex.lock();
            args->doneMutex.unlock();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    gattlib_adapter_close(adapter);
    SPDLOG_INFO("Scan stoped");
    if (args->success) {
        return args;
    }
    return nullptr;
}
//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------