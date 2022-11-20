#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
struct Machine {
    size_t articleNumber;
    std::string name;
    std::string fileName;
    uint8_t version;

    Machine(size_t articleNumber, std::string&& name, std::string&& fileName, uint8_t version) : articleNumber(articleNumber),
                                                                                                 name(std::move(name)),
                                                                                                 fileName(std::move(fileName)),
                                                                                                 version(version){};
} __attribute__((aligned(128)));

struct Item {
    std::string name;
    std::string value;

    Item(std::string&& name, std::string&& value) : name(std::move(name)),
                                                    value(std::move(value)) {}
} __attribute__((aligned(64)));

struct ItemsOption {
    std::string argument;
    std::string defaultValue;
    std::vector<Item> items;

    ItemsOption(std::string&& argument, std::string&& defaultValue, std::vector<Item>&& items) : argument(std::move(argument)),
                                                                                                 defaultValue(std::move(defaultValue)),
                                                                                                 items(std::move(items)) {}

    void to_bt_command(std::vector<std::string>& command) const;
} __attribute__((aligned(128)));

struct MinMaxOption {
    std::string argument;
    uint8_t value;
    uint8_t min;
    uint8_t max;
    uint8_t step;

    MinMaxOption(std::string&& argument, uint8_t value, uint8_t min, uint8_t max, uint8_t step) : argument(std::move(argument)),
                                                                                                  value(value),
                                                                                                  min(min),
                                                                                                  max(max),
                                                                                                  step(step) {}

    void to_bt_command(std::vector<std::string>& command) const;
} __attribute__((aligned(64)));

struct Product {
    std::string name;
    std::string code;

    std::optional<ItemsOption> strength;
    std::optional<ItemsOption> temperature;
    std::optional<MinMaxOption> waterAmount;
    std::optional<MinMaxOption> milkFoamAmount;

    size_t statCounter{0};

    Product(std::string&& name, std::string&& code, std::optional<ItemsOption>&& strength, std::optional<ItemsOption>&& temperature, std::optional<MinMaxOption>&& waterAmount, std::optional<MinMaxOption> milkFoamAmount) : name(std::move(name)),
                                                                                                                                                                                                                              code(std::move(code)),
                                                                                                                                                                                                                              strength(std::move(strength)),
                                                                                                                                                                                                                              temperature(std::move(temperature)),
                                                                                                                                                                                                                              waterAmount(std::move(waterAmount)),
                                                                                                                                                                                                                              milkFoamAmount(std::move(milkFoamAmount)) {}

    [[nodiscard]] std::string to_bt_command() const;
    [[nodiscard]] size_t code_to_size_t() const;
} __attribute__((aligned(128)));

struct Alert {
    size_t bit;
    std::string name;
    std::string type;

    Alert(size_t bit, std::string&& name, std::string&& type) : bit(bit),
                                                                name(std::move(name)),
                                                                type(std::move(type)) {}
} __attribute__((aligned(128)));

struct Joe {
    std::string dated;
    const Machine* machine;
    std::vector<Product> products;
    std::vector<Alert> alerts;

    size_t statTotalCount{0};

    Joe(std::string&& dated, const Machine* machine, std::vector<Product>&& products, std::vector<Alert>&& alerts) : dated(std::move(dated)),
                                                                                                                     machine(machine),
                                                                                                                     products(std::move(products)),
                                                                                                                     alerts(std::move(alerts)) {}
} __attribute__((aligned(128)));

std::unordered_map<size_t, const Machine> load_machines(const std::filesystem::path& path);
std::shared_ptr<Joe> load_joe(const Machine* machine);
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------