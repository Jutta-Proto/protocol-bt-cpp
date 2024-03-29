#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include "io/csv.hpp"
#include "jutta_bt_proto/Utils.hpp"
#include "logger/Logger.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>
#include <tinyxml2.h>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
void ItemsOption::to_bt_command(std::vector<std::string>& command) const {
    if (argument[0] == 'F') {
        std::string offsetStr = argument.substr(1);
        std::istringstream iss(offsetStr);
        size_t offset = 0;
        iss >> offset;
        command[offset - 1] = defaultValue;
    } else {
        SPDLOG_ERROR("Invalid argument when converting to a BT command '{}'", argument);
    }
}

void MinMaxOption::to_bt_command(std::vector<std::string>& command) const {
    if (argument[0] == 'F') {
        std::string offsetStr = argument.substr(1);
        std::istringstream iss(offsetStr);
        size_t offset = 0;
        iss >> offset;
        uint8_t v = value / step;
        command[offset - 1] = to_hex_string(std::vector<uint8_t>{v});
    } else {
        SPDLOG_ERROR("Invalid argument when converting to a BT command '{}'", argument);
    }
}

std::string Product::to_bt_command() const {
    std::vector<std::string> command;
    command.resize(17);
    for (std::string& s : command) {
        s = "00";
    }

    if (strength) {
        strength->to_bt_command(command);
    }

    if (temperature) {
        temperature->to_bt_command(command);
    }

    if (waterAmount) {
        waterAmount->to_bt_command(command);
    }

    if (milkFoamAmount) {
        milkFoamAmount->to_bt_command(command);
    }

    // TODO: Add GRINDER_FREENESS

    command[0] = code;

    std::string result = "00";
    for (const std::string& s : command) {
        result += s;
    }
    SPDLOG_DEBUG("Product command: {}", result);
    return result;
}

size_t Product::code_to_size_t() const {
    std::vector<uint8_t> codeVec = from_hex_string(code);
    size_t codeVal = 0;
    for (const uint8_t c : codeVec) {
        codeVal <<= 8;
        codeVal |= c;
    }
    return codeVal;
}

std::unordered_map<size_t, const Machine> load_machines(const std::filesystem::path& path) {
    SPDLOG_INFO("Loading machines...");
    std::unordered_map<size_t, const Machine> result;
    io::CSVReader<4, io::trim_chars<' ', '\t'>, io::no_quote_escape<';'>> in(path);
    // Skip the first line:
    in.next_line();
    size_t articleNumber = 0;
    std::string name;
    std::string fileName;
    uint8_t version = 0;
    while (in.read_row(articleNumber, name, fileName, version)) {
        result.emplace(articleNumber, Machine(articleNumber, std::string{name}, std::string{fileName}, version));
    }
    SPDLOG_INFO("Loaded {} machines.", result.size());
    return result;
}

std::optional<ItemsOption> load_items_option(const tinyxml2::XMLElement* option) {
    std::string argument = option->Attribute("Argument");
    std::string defaultValue = option->Attribute("Default");
    std::vector<Item> items;
    for (const tinyxml2::XMLElement* e = option->FirstChildElement("ITEM"); e != nullptr; e = e->NextSiblingElement("ITEM")) {
        std::string name = e->Attribute("Name");
        std::string value = e->Attribute("Value");
        items.emplace_back(std::move(name), std::move(value));
    }
    return std::make_optional<ItemsOption>(std::move(argument), std::move(defaultValue), std::move(items));
}

std::optional<MinMaxOption> load_min_max_option(const tinyxml2::XMLElement* option) {
    std::string argument = option->Attribute("Argument");
    uint8_t value = option->IntAttribute("Value");
    uint8_t min = option->IntAttribute("Min");
    uint8_t max = option->IntAttribute("Max");
    uint8_t step = option->IntAttribute("Step");
    return std::make_optional<MinMaxOption>(std::move(argument), value, min, max, step);
}

void load_products(std::vector<Product>* products, tinyxml2::XMLElement* joe) {
    tinyxml2::XMLElement* productsXml = joe->FirstChildElement("PRODUCTS");
    for (const tinyxml2::XMLElement* e = productsXml->FirstChildElement("PRODUCT"); e != nullptr; e = e->NextSiblingElement("PRODUCT")) {
        std::string name = e->Attribute("Name");
        std::string code = e->Attribute("Code");

        // Strength:
        std::optional<ItemsOption> strength;
        const tinyxml2::XMLElement* strengthXml = e->FirstChildElement("COFFEE_STRENGTH");
        if (strengthXml) {
            strength = load_items_option(strengthXml);
        }

        // Temperature:
        std::optional<ItemsOption> temperature;
        const tinyxml2::XMLElement* temperatureXml = e->FirstChildElement("TEMPERATURE");
        if (temperatureXml) {
            temperature = load_items_option(temperatureXml);
        }

        // Water amount:
        std::optional<MinMaxOption> waterAmount;
        const tinyxml2::XMLElement* waterAmountXml = e->FirstChildElement("WATER_AMOUNT");
        if (waterAmountXml) {
            waterAmount = load_min_max_option(waterAmountXml);
        }

        // Milk Foam Amount:
        std::optional<MinMaxOption> milkFoamAmount;
        const tinyxml2::XMLElement* milkFoamAmountXml = e->FirstChildElement("MILK_FOAM_AMOUNT");
        if (milkFoamAmountXml) {
            milkFoamAmount = load_min_max_option(milkFoamAmountXml);
        }

        products->emplace_back(std::move(name), std::move(code), std::move(strength), std::move(temperature), std::move(waterAmount), std::move(milkFoamAmount));
    }
}

void load_alerts(std::vector<Alert>* alerts, tinyxml2::XMLElement* joe) {
    tinyxml2::XMLElement* alertsXml = joe->FirstChildElement("ALERTS");
    for (const tinyxml2::XMLElement* e = alertsXml->FirstChildElement("ALERT"); e != nullptr; e = e->NextSiblingElement("ALERT")) {
        size_t bit = e->IntAttribute("Bit");
        std::string name = e->Attribute("Name");
        const char* typeCStr = e->Attribute("Type");
        std::string type;
        if (typeCStr) {
            type = typeCStr;
        }
        alerts->emplace_back(bit, std::move(name), std::move(type));
    }
}

void load_maintenance_counters(std::vector<MaintenanceCounter>& maintenanceCounters, tinyxml2::XMLElement* joe) {
    tinyxml2::XMLElement* statisticXml = joe->FirstChildElement("STATISTIC");
    assert(statisticXml);
    tinyxml2::XMLElement* maintenanceXml = statisticXml->FirstChildElement("MAINTENANCEPAGE");
    assert(maintenanceXml);

    for (const tinyxml2::XMLElement* bankXml = maintenanceXml->FirstChildElement("BANK"); bankXml != nullptr; bankXml = bankXml->NextSiblingElement("BANK")) {
        std::string name = bankXml->Attribute("Name");
        if (name == "Maintenance Counter") {
            for (const tinyxml2::XMLElement* e = bankXml->FirstChildElement("TEXTITEM"); e != nullptr; e = e->NextSiblingElement("TEXTITEM")) {
                std::string name = e->Attribute("Type");
                maintenanceCounters.emplace_back(std::move(name), 0);
            }
            break;
        }
    }
}

void load_maintenance_percentages(std::vector<MaintenancePercentage>& maintenancePercentages, tinyxml2::XMLElement* joe) {
    tinyxml2::XMLElement* statisticXml = joe->FirstChildElement("STATISTIC");
    assert(statisticXml);
    tinyxml2::XMLElement* maintenanceXml = statisticXml->FirstChildElement("MAINTENANCEPAGE");
    assert(maintenanceXml);

    for (const tinyxml2::XMLElement* bankXml = maintenanceXml->FirstChildElement("BANK"); bankXml != nullptr; bankXml = bankXml->NextSiblingElement("BANK")) {
        std::string name = bankXml->Attribute("Name");
        if (name == "Maintenance Percent") {
            for (const tinyxml2::XMLElement* e = bankXml->FirstChildElement("TEXTITEM"); e != nullptr; e = e->NextSiblingElement("TEXTITEM")) {
                std::string name = e->Attribute("Type");
                maintenancePercentages.emplace_back(std::move(name), 0);
            }
            break;
        }
    }
}

std::shared_ptr<Joe> load_joe(const Machine* machine) {
    tinyxml2::XMLDocument doc;
    std::string path = "machinefiles/" + machine->fileName + ".xml";
    SPDLOG_INFO("Loading JOE from '{}'...", path);
    tinyxml2::XMLError result = doc.LoadFile(path.c_str());
    assert(result == tinyxml2::XML_SUCCESS);
    tinyxml2::XMLElement* joe = doc.FirstChildElement("JOE");

    std::string dated = joe->Attribute("dated");
    std::vector<Product> products;
    load_products(&products, joe);
    std::vector<Alert> alerts;
    load_alerts(&alerts, joe);

    std::vector<MaintenanceCounter> maintenanceCounters;
    load_maintenance_counters(maintenanceCounters, joe);

    std::vector<MaintenancePercentage> maintenancePercentages;
    load_maintenance_percentages(maintenancePercentages, joe);

    SPDLOG_INFO("JOE loaded.");
    return std::make_shared<Joe>(std::move(dated), machine, std::move(products), std::move(alerts), std::move(maintenanceCounters), std::move(maintenancePercentages));
}
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------