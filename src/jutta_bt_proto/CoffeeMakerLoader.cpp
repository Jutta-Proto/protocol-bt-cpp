#include "jutta_bt_proto/CoffeeMakerLoader.hpp"
#include "io/csv.hpp"
#include "logger/Logger.hpp"
#include <cstddef>
#include <cstdint>
#include <spdlog/spdlog.h>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
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
        result.emplace(std::make_pair(articleNumber, Machine(articleNumber, std::string{name}, std::string{fileName}, version)));
    }
    SPDLOG_INFO("Loaded {} machines.", result.size());
    return result;
}
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------