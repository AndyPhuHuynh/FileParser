#include "FileParser/Jpeg/HuffmanEncoder.hpp"

#include <algorithm>
#include <expected>
#include <iostream>
#include <ranges>

#include "FileParser/Jpeg/JpegEncoder.h"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
#include "FileParser/Jpeg/Markers.hpp"

auto FileParser::Jpeg::CodeSizeEncoder::getCodeSizesPerByte(const ByteFrequencies& frequencies) -> CodeSizePerByte {
    // Initialize frequencies with an extra code point at index 256. This is done so when creating the Huffman Table,
    // no symbol is given the encoding of all 1's
    std::array<uint32_t, 257> freq{};
    std::ranges::copy(frequencies, freq.begin());
    freq[256] = 1;
    // Initialize codeSize
    CodeSizePerByte codeSizes{};
    // Initialize others
    std::array<uint32_t, 257> others{};
    std::ranges::fill(others, std::numeric_limits<uint32_t>::max());

    while (true) {
        // v1 is the least frequent, v2 is the second least frequent
        uint32_t v1 = std::numeric_limits<uint32_t>::max();
        uint32_t v2 = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < freq.size(); i++) {
            if (freq.at(i) == 0) continue;
            if (v1 == std::numeric_limits<uint32_t>::max() ||
                freq.at(i) < freq.at(v1) || (freq.at(i) == freq.at(v1) && i > v1)) {
                v2 = v1;
                v1 = i;
                } else if (v2 == std::numeric_limits<uint32_t>::max() || freq.at(i) < freq.at(v2)) {
                    v2 = i;
                }
        }
        const bool oneFound = v1 == std::numeric_limits<uint32_t>::max() || v2 == std::numeric_limits<uint32_t>::max();
        if (oneFound) break; // If there is only one item with non-zero frequency, the algorithm is finished

        // Combine the nodes
        freq.at(v1) += freq.at(v2);
        freq.at(v2) = 0;

        // Increment the code size of every node under v1
        codeSizes.at(v1)++;
        while (others.at(v1) != std::numeric_limits<uint32_t>::max()) {
            v1 = others.at(v1);
            codeSizes.at(v1)++;
        }

        // Add v2 to the end of the chain
        others.at(v1) = v2;

        // Increment the code size of every node under v2
        codeSizes.at(v2)++;
        while (others.at(v2) != std::numeric_limits<uint32_t>::max()) {
            v2 = others.at(v2);
            codeSizes.at(v2)++;
        }
    }
    return codeSizes;
}

auto FileParser::Jpeg::CodeSizeEncoder::countCodeSizes(const CodeSizePerByte& codeSizes) -> UnadjustedCodeSizeFrequencies {
    UnadjustedCodeSizeFrequencies unadjusted{};
    for (auto i : codeSizes) {
        if (i > 32) {
            std::cerr << "Invalid code size: " << i << ", clamping to 32\n";
            i = 32;
        }
        unadjusted[i]++;
    }
    return unadjusted;
}

auto FileParser::Jpeg::CodeSizeEncoder::adjustCodeSizes(
    UnadjustedCodeSizeFrequencies& unadjusted
) -> std::expected<CodeSizes, std::string> {
    uint32_t i = 32;
    while (true) {
        if (unadjusted.at(i) > 0) {
            // Find the first shorter, non-zero code length
            uint32_t j = i - 1;
            do {
                j--;
            } while (j > 0 && unadjusted.at(j) == 0);

            if (j == 0) {
                return std::unexpected(
                    "Unable to properly adjust code sizes: no shorter non-zero code length found during adjustment at "
                    "i=" + std::to_string(i));
            }

            // Move the prefixes around to make the codes shorter
            unadjusted.at(i) -= 2;
            unadjusted.at(i - 1) += 1;
            unadjusted.at(j + 1) += 2;
            unadjusted.at(j) -= 1;
        } else {
            i--;
            if (i <= 16) {
                // Remove the reserved code point
                while (unadjusted.at(i) == 0) {
                    i--;
                }
                unadjusted.at(i)--;
                break;
            }
        }
    }
    // Verify that all codeSizes are <= 16
    for (size_t j = Encoder::MaxHuffmanBits + 1; j < unadjusted.size(); j++) {
        if (unadjusted[j] != 0) {
            return std::unexpected("Error adjusting code sizes, invalid Huffman code size: " + std::to_string(unadjusted[j]));
        }
    }
    auto codeSizes = std::expected<CodeSizes, std::string>(std::in_place);
    std::ranges::copy(unadjusted | std::views::drop(1) | std::views::take(16), codeSizes.value().getFrequencies().begin());
    return codeSizes;
}

auto FileParser::Jpeg::CodeSizeEncoder::getCodeSizes(
    const ByteFrequencies& frequencies
) -> std::expected<CodeSizes, std::string> {
    const auto codeSizesPerByte = getCodeSizesPerByte(frequencies);
    auto unadjustedSizes = countCodeSizes(codeSizesPerByte);
    return adjustCodeSizes(unadjustedSizes);
}

auto FileParser::Jpeg::HuffmanEncoder::create(
    const std::vector<Encoder::Coefficient>& coefficients
) -> std::expected<HuffmanEncoder, std::string> {
    const auto frequencies = countFrequencies(coefficients);
    auto sortedSymbols = getSymbolsOrderedByFrequency(frequencies);

    auto codeSizesExpected = CodeSizeEncoder::getCodeSizes(frequencies);
    if (!codeSizesExpected) {
        return std::unexpected("Failed to generate code sizes: " + codeSizesExpected.error());
    }
    auto& codeSizes = codeSizesExpected.value();

    auto table = HuffmanTable(HuffmanBuilder::generateEncodings(sortedSymbols, codeSizes.getFrequencies()));
    return HuffmanEncoder(frequencies, std::move(sortedSymbols), codeSizes, std::move(table));
}

auto FileParser::Jpeg::HuffmanEncoder::getSymbolsByFrequencies() const -> const std::vector<uint8_t>& {
    return m_symbolsByFrequency;
}

auto FileParser::Jpeg::HuffmanEncoder::getCodeSizes() const -> const CodeSizes& {
    return m_codeSizes;
}

auto FileParser::Jpeg::HuffmanEncoder::getTable() const -> const HuffmanTable& {
    return m_table;
}

auto FileParser::Jpeg::HuffmanEncoder::writeToFile(JpegBitWriter& bitWriter, const TableDescription description) const -> void {
    // Write marker
    bitWriter << MarkerHeader << DHT;

    // Write length
    constexpr uint16_t markerLength = 2, tableDescriptionLength = 1, codeSizesLength = 16;
    const uint16_t length =
        markerLength + tableDescriptionLength + codeSizesLength + static_cast<uint16_t>(m_symbolsByFrequency.size());
    bitWriter << length;

    // Write table info
    bitWriter << static_cast<uint8_t>(description);

    // Write code size frequencies
    for (uint8_t i = 1; i <= Encoder::MaxHuffmanBits; i++) {
        bitWriter << m_codeSizes.getFrequencyOf(i);
    }

    // Write symbols
    for (auto symbol : m_symbolsByFrequency) {
        bitWriter << symbol;
    }
}

auto FileParser::Jpeg::HuffmanEncoder::countFrequencies(
    const std::vector<Encoder::Coefficient>& coefficients
) -> ByteFrequencies {
    ByteFrequencies frequencies{};
    for (auto& coefficient : coefficients) {
        frequencies[coefficient.encoding]++;
    }
    return frequencies;
}

auto FileParser::Jpeg::HuffmanEncoder::getSymbolsOrderedByFrequency(
    const std::array<uint32_t, 256>& frequencies
) -> std::vector<uint8_t> {
    // Each pair stores a pair of symbol to frequency
    std::vector<std::pair<uint8_t, int>> freqPairs;
    for (size_t i = 0 ; i < 256; i++) {
        if (frequencies.at(i) == 0) {
            continue;
        }
        freqPairs.emplace_back(static_cast<uint8_t>(i), frequencies.at(i));
    }

    // Sort by descending frequency; for ties, sort by ascending symbol
    auto comparePair = [](const std::pair<uint8_t, int>& one, const std::pair<uint8_t, int>& two) -> bool {
        return (one.second > two.second) || (one.second == two.second && one.first < two.first);
    };
    std::ranges::sort(freqPairs, comparePair);

    // Store the sorted symbols into the output vector
    std::vector<uint8_t> symbolsByFrequency;
    symbolsByFrequency.reserve(freqPairs.size());
    for (auto key : freqPairs | std::views::keys) {
        symbolsByFrequency.emplace_back(key);
    }
    return symbolsByFrequency;
}
