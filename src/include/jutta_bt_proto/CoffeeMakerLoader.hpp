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
} __attribute__((aligned(64)));

struct ItemsOption {
    std::string argument;
    std::string defaultValue;
    std::vector<Item> items;
} __attribute__((aligned(128)));

struct MinMaxOption {
    std::string argument;
    uint8_t value;
    uint8_t min;
    uint8_t max;
    uint8_t step;
} __attribute__((aligned(64)));

struct Product {
    std::string name;
    std::string code;

    std::optional<ItemsOption> strength;
    std::optional<ItemsOption> temperature;
    std::optional<MinMaxOption> waterAmount;
    std::optional<MinMaxOption> milkFoamAmount;
} __attribute__((aligned(128)));

struct Alert {
    size_t bit;
    std::string name;
    std::string type;
} __attribute__((aligned(64)));

struct Joe {
    std::string dated;
    const Machine* machine;
    std::vector<Product> products;
    std::vector<Alert> alerts;
} __attribute__((aligned(128)));

std::unordered_map<size_t, const Machine> load_machines(const std::filesystem::path& path);
std::shared_ptr<Joe> load_joe(const Machine* machine);
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------