#include "jutta_bt_proto/Utils.hpp"
#include <array>
#include <cassert>
#include <cctype>
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

uint8_t get_hex_char_val(char c) {
    uint8_t i = static_cast<uint8_t>(std::toupper(c));
    if (i >= 0x30 && i <= 0x39) {
        return i -= 0x30;
    }
    if (i >= 0x41 && i <= 0x46) {
        return i = i - 0x41 + 10;
    }
    assert(false);
}

std::vector<uint8_t> from_hex_string(const std::string& hex) {
    std::vector<uint8_t> result;
    result.resize(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        result[i / 2] = (get_hex_char_val(hex[i]) << 4) | get_hex_char_val(hex[i + 1]);
    }
    return result;
}
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------