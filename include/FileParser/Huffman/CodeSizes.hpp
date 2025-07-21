#pragma once

#include <array>
#include <cstdint>

namespace FileParser {
    /**
     * @brief Represents a histogram of Huffman code lengths.
     *
     * This class stores the number of symbols assigned to each possible code size (1-16), typically used in canonical
     * Huffman coding for formates such as JPEG or Deflate.
     */
    class CodeSizes {
        /**
         * m_frequency[i] represents the number of symbols with code size i + 1.
         */
        std::array<uint8_t, 16> m_frequency{};

    public:
        /**
         * @param codeSize The code size to query (must be in the range [1, 16]).
         * @return The frequency of the given code size.
         */
        [[nodiscard]] auto getFrequencyOf(uint8_t codeSize) const -> uint8_t;

        [[nodiscard]] auto getFrequencies() -> std::array<uint8_t, 16>&;
        [[nodiscard]] auto getFrequencies() const -> const std::array<uint8_t, 16>&;
    };
}