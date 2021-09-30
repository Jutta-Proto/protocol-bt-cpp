#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
/**
 * Encodes or decodes the given data with the given key.
 * When you are decoding data, the first byte of the result should be key if everything went well.
 * This function is reversible.
 * encDecBytes(encDecBytes(data)) == data
 **/
std::vector<uint8_t> encDecBytes(const std::vector<uint8_t>& data, uint8_t key);
//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------