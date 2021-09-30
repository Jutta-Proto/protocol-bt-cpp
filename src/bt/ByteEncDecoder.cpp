#include "bt/ByteEncDecoder.hpp"
#include <array>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <vector>

//---------------------------------------------------------------------------
namespace bt {
//---------------------------------------------------------------------------
const std::array<uint8_t, 16> numbers1 = {14, 4, 3, 2, 1, 13, 8, 11, 6, 15, 12, 7, 10, 5, 0, 9};
const std::array<uint8_t, 16> numbers2 = {10, 6, 13, 12, 14, 11, 1, 9, 15, 7, 0, 5, 3, 2, 4, 8};

uint8_t mod256(int i) {
    while (i > 255) {
        i -= 256;
    }
    while (i < 0) {
        i += 256;
    }
    return static_cast<uint8_t>(i);
}

uint8_t shuffle(int dataNibble, int nibbleCount, int keyLeftNibbel, int keyRightNibbel) {
    uint8_t i5 = mod256(nibbleCount >> 4);
    uint8_t tmp1 = numbers1[mod256(dataNibble + nibbleCount + keyLeftNibbel) % 16];
    uint8_t tmp2 = numbers2[mod256(tmp1 + keyRightNibbel + i5 - nibbleCount - keyLeftNibbel) % 16];
    uint8_t tmp3 = numbers1[mod256(tmp2 + keyLeftNibbel + nibbleCount - keyRightNibbel - i5) % 16];
    return mod256(tmp3 - nibbleCount - keyLeftNibbel) % 16;
}

std::vector<uint8_t> encDecBytes(const std::vector<uint8_t>& data, uint8_t key) {
    std::vector<uint8_t> result;
    result.resize(data.size());
    uint8_t keyLeftNibbel = key >> 4;
    uint8_t keyRightNibbel = key & 15;
    int nibbelCount = 0;
    for (size_t offset = 0; offset < data.size(); offset++) {
        uint8_t d = data[offset];
        uint8_t dataLeftNibbel = d >> 4;
        uint8_t dataRightNibbel = d & 15;
        uint8_t resultLeftNibbel = shuffle(dataLeftNibbel, nibbelCount++, keyLeftNibbel, keyRightNibbel);
        uint8_t resultRightNibbel = shuffle(dataRightNibbel, nibbelCount++, keyLeftNibbel, keyRightNibbel);
        result[offset] = (resultLeftNibbel << 4) | resultRightNibbel;
    }
    return result;
}

//---------------------------------------------------------------------------
}  // namespace bt
//---------------------------------------------------------------------------