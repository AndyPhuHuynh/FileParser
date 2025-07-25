#pragma once

#include <array>
#include <fstream>

namespace FileParser::Jpeg {

    const unsigned char zigZagMap[] = {
        0,   1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };

    struct QuantizationTable {
        static constexpr size_t length = 64;

        uint8_t precision = 0; // 0 => 8-bit, 1 => 16-bit
        uint8_t destination = 0;
        std::array<float, length> table{}; // De-zigzagged values

        auto operator[](const size_t index) -> float& { return table[index]; }
        auto operator[](const size_t index) const -> const float& { return table[index]; }
    };
}
