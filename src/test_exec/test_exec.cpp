#include "bt/BLEHelper.hpp"
#include "jutta_bt_proto/CoffeeMaker.hpp"
#include "logger/Logger.hpp"
#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <spdlog/spdlog.h>

int main(int /*argc*/, char** /*argv*/) {
    logger::setup_logger(spdlog::level::debug);
    SPDLOG_INFO("Starting test exec...");
    while (true) {
        SPDLOG_INFO("Scanning...");
        std::shared_ptr<bt::ScanArgs> result = bt::scan_for_device("TT214H BlueFrog");
        if (!result) {
            SPDLOG_INFO("No coffee maker found. Sleeping...");
            std::this_thread::sleep_for(std::chrono::seconds{2});
            continue;
        }
        SPDLOG_INFO("Coffee maker found.");
        jutta_bt_proto::CoffeeMaker coffeeMaker(std::string{result->name}, std::string{result->addr});
        coffeeMaker.connect();
        for (size_t i = 0; i < 40; i++) {
            std::this_thread::sleep_for(std::chrono::seconds{1});
        }
        coffeeMaker.disconnect();
        std::string s;
        std::cin >> s;
    }
    return 0;
}
