#include "jutta_bt_proto/Utils.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
std::string to_hex_string(const std::vector<uint8_t>& data) {
    static const std::array<char, 16> HEX_CHARS{'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

    std::string result;
    result.resize(data.size() * 2);
    for (size_t i = 0; i < data.size(); i++) {
        result[i * 2] = HEX_CHARS[data[i] >> 4];
        result[(i * 2) + 1] = HEX_CHARS[data[i] & 0x0F];
    }

    return result;
}
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------