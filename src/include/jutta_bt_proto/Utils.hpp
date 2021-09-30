#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

//---------------------------------------------------------------------------
namespace jutta_bt_proto {
//---------------------------------------------------------------------------
std::string to_hex_string(const std::vector<uint8_t>& data);
//---------------------------------------------------------------------------
}  // namespace jutta_bt_proto
//---------------------------------------------------------------------------