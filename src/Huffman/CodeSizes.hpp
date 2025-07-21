#pragma once

#include <stdexcept>

#include "FileParser/Huffman/CodeSizes.hpp"

inline auto FileParser::CodeSizes::getFrequencyOf(const uint8_t codeSize) const -> uint8_t {
    if (codeSize <= 0 || codeSize > 16) {
        throw std::invalid_argument("Code size must be in the range [1, 16]");
    }
    return frequency[codeSize - 1];
}
