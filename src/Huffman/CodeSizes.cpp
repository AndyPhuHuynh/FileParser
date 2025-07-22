#pragma once

#include <stdexcept>

#include "FileParser/Huffman/CodeSizes.hpp"

auto FileParser::CodeSizes::getFrequencyOf(const uint8_t codeSize) const -> uint8_t {
    if (codeSize <= 0 || codeSize > 16) {
        throw std::invalid_argument("Code size must be in the range [1, 16]");
    }
    return m_frequency[codeSize - 1];
}

auto FileParser::CodeSizes::getFrequencies() -> std::array<uint8_t, 16>& {
    return m_frequency;
}

auto FileParser::CodeSizes::getFrequencies() const -> const std::array<uint8_t, 16>& {
    return m_frequency;
}
