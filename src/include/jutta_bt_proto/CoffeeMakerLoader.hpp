#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_set>

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

    std::size_t operator()(const Machine& machine) const {
        return machine.articleNumber;
    }

    bool operator==(const Machine& other) const {
        return articleNumber == other.articleNumber;
    }

    // NOLINTNEXTLINE (altera-struct-pack-align)
    struct HashFunction {
        size_t operator()(const Machine& machine) const {
            return machine.articleNumber;
        }
    };
} __attribute__((aligned(128)));

std::unordered_set<Machine, Machine::HashFunction> load_machines(const std::filesystem::path& path);
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------