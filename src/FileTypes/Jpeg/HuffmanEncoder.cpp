#include "Jpeg/HuffmanEncoder.hpp"

#include <iostream>
#include <ranges>

void FileParser::Jpeg::HuffmanEncoder::countFrequencies(const std::vector<Encoder::Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies) {
    for (auto& coefficient : coefficients) {
        outFrequencies[coefficient.encoding]++;
    }
}

void FileParser::Jpeg::HuffmanEncoder::generateCodeSizes(const std::array<uint32_t, 256>& frequencies, std::array<uint8_t, 257>& outCodeSizes) {
    // Initialize frequencies
    std::array<uint32_t, 257> freq{};
    std::ranges::copy(frequencies, freq.begin());
    freq[256] = 1;
    // Initialize codeSize
    std::ranges::fill(outCodeSizes, static_cast<uint8_t>(0));
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
        outCodeSizes.at(v1)++;
        while (others.at(v1) != std::numeric_limits<uint32_t>::max()) {
            v1 = others.at(v1);
            outCodeSizes.at(v1)++;
        }

        // Add v2 to the end of the chain
        others.at(v1) = v2;

        // Increment the code size of every node under v2
        outCodeSizes.at(v2)++;
        while (others.at(v2) != std::numeric_limits<uint32_t>::max()) {
            v2 = others.at(v2);
            outCodeSizes.at(v2)++;
        }
    }
}

void FileParser::Jpeg::HuffmanEncoder::adjustCodeSizes(std::array<uint8_t, 33>& codeSizeFrequencies) {
    uint32_t i = 32;
    while (true) {
        if (codeSizeFrequencies.at(i) > 0) {
            // Find the first shorter, non-zero code length
            uint32_t j = i - 1;
            do {
                j--;
            } while (j > 0 && codeSizeFrequencies.at(j) == 0);

            if (j == 0) {
                std::cerr << "Error, no shorter non-zero code found\n";
            }

            // Move the prefixes around to make the codes shorter
            codeSizeFrequencies.at(i) -= 2;
            codeSizeFrequencies.at(i - 1) += 1;
            codeSizeFrequencies.at(j + 1) += 2;
            codeSizeFrequencies.at(j) -= 1;
        } else {
            i--;
            if (i <= 16) {
                 // Remove the reserved code point
                while (codeSizeFrequencies.at(i) == 0) {
                    i--;
                }
                codeSizeFrequencies.at(i)--;
                break;
            }
        }
    }
}

void FileParser::Jpeg::HuffmanEncoder::countCodeSizes(const std::array<uint8_t, 257>& codeSizes,
    std::array<uint8_t, 33>& outCodeSizeFrequencies) {
    outCodeSizeFrequencies.fill(0);
    for (auto i : codeSizes) {
        if (i > 32) {
            std::cerr << "Invalid code size: " << i << ", clamping to 32\n";
            i = 32;
        }
        outCodeSizeFrequencies[i]++;
    }
    adjustCodeSizes(outCodeSizeFrequencies);
}

void FileParser::Jpeg::HuffmanEncoder::countCodeSizes(const std::vector<HuffmanEncoding>& encodings,
    std::array<uint8_t, 33>& outCodeSizeFrequencies) {
    outCodeSizeFrequencies.fill(0);
    for (const auto encoding : encodings) {
        outCodeSizeFrequencies[encoding.bitLength]++;
    }
}

void FileParser::Jpeg::HuffmanEncoder::sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols) {
    // Each pair stores a pair of symbol to frequency
    std::vector<std::pair<uint8_t, int>> freqPairs;
    for (int i = 0 ; i < 256; i++) {
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
    outSortedSymbols.clear();
    outSortedSymbols.reserve(freqPairs.size());
    for (auto key : freqPairs | std::views::keys) {
        outSortedSymbols.emplace_back(key);
    }
}

void FileParser::Jpeg::HuffmanEncoder::sortEncodingsByLength(const std::vector<HuffmanEncoding>& encodings,
    std::vector<uint8_t>& outSortedSymbols) {
    std::vector<HuffmanEncoding> sortedEncodings = encodings;
    // Sort by ascending the bit length; for ties, sort by ascending symbol
    auto compareEncoding = [](const HuffmanEncoding& encoding1, const HuffmanEncoding& encoding2) -> bool {
        return (encoding1.bitLength < encoding2.bitLength) ||
            (encoding1.bitLength == encoding2.bitLength && encoding1.value < encoding2.value);
    };

    std::ranges::sort(sortedEncodings, compareEncoding);
    outSortedSymbols.clear();
    outSortedSymbols.reserve(sortedEncodings.size());
    for (const auto& encoding : sortedEncodings) {
        outSortedSymbols.emplace_back(encoding.value);
    }
}