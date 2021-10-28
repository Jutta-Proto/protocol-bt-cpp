#include "bt/BLEHelper.hpp"
#include "logger/Logger.hpp"
#include <chrono>
#include <string>
#include <thread>
#include <unordered_set>
#include <spdlog/common.h>
#include <spdlog/spdlog.h>

int main(int /*argc*/, char** /*argv*/) {
    logger::setup_logger(spdlog::level::info);
    SPDLOG_INFO("Starting scanner...");
    std::unordered_set<std::string> devices;
    while (true) {
        SPDLOG_DEBUG("Scanning...");
        bool canceled = false;
        std::shared_ptr<bt::ScanArgs> result = bt::scan_for_device("TT214H BlueFrog", &canceled);
        if (result && !devices.contains(result->addr)) {
            devices.insert(result->addr);
            SPDLOG_INFO("New device found: {} ({})", result->name, result->addr);
            SPDLOG_INFO("Total: {}", devices.size());
            continue;
        }
        SPDLOG_INFO("No new deviced found.");
    }
    return 0;
}
