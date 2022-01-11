#include "bt/BLEHelper.hpp"
#include "jutta_bt_proto/CoffeeMaker.hpp"
#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
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
        bool canceled = false;
        std::shared_ptr<bt::ScanArgs> result = bt::scan_for_device("TT214H BlueFrog", &canceled);
        if (!result) {
            SPDLOG_INFO("No coffee maker found. Sleeping...");
            std::this_thread::sleep_for(std::chrono::seconds{2});
            continue;
        }
        SPDLOG_INFO("Coffee maker found.");
        jutta_bt_proto::CoffeeMaker coffeeMaker(std::string{result->name}, std::string{result->addr});
        coffeeMaker.set_alerts_changed_event_handler([](const std::vector<const jutta_bt_proto::Alert*>& alerts) {
            for (const jutta_bt_proto::Alert* alert : alerts) {
                // NOLINTNEXTLINE (bugprone-lambda-function-name)
                SPDLOG_INFO("New alert '{}' with type '{}'.", alert->name, alert->type);
            }
        });
        if (coffeeMaker.connect()) {
            size_t i = 0;
            while (coffeeMaker.get_state() == jutta_bt_proto::CONNECTED) {
                std::this_thread::sleep_for(std::chrono::seconds{1});
                if (++i == 10) {
                    coffeeMaker.request_coffee(coffeeMaker.get_joe()->products[1]);
                }
            }
        }
        coffeeMaker.disconnect();
        SPDLOG_INFO("Disconnected. Waiting 5 seconds before reconnecting.");
        std::this_thread::sleep_for(std::chrono::seconds{5});
    }
    return 0;
}
