#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>

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

std::unordered_map<size_t, const Machine> load_machines(const std::filesystem::path& path);
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------