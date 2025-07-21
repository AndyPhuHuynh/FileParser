#include "Jpeg/HuffmanEncoder.hpp"

#include <iostream>
#include <ranges>

#include "Jpeg/HuffmanBuilder.hpp"

auto FileParser::Jpeg::CodeSizeEncoder::getCodeSizesPerByte(const ByteFrequencies& frequencies) -> CodeSizesPerByte {
    // Initialize frequencies with an extra code point at index 256. This is done so when creating the Huffman Table,
    // no symbol is given the encoding of all 1's
    std::array<uint32_t, 257> freq{};
    std::ranges::copy(frequencies, freq.begin());
    freq[256] = 1;
    // Initialize codeSize
    CodeSizesPerByte codeSizes{};
    std::ranges::fill(codeSizes, static_cast<uint8_t>(0));
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

auto FileParser::Jpeg::CodeSizeEncoder::countCodeSizes(const CodeSizesPerByte& codeSizes) -> UnadjustedCodeSizeFrequencies {
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

auto FileParser::Jpeg::CodeSizeEncoder::adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> CodeSizes {
    uint32_t i = 32;
    while (true) {
        if (unadjusted.at(i) > 0) {
            // Find the first shorter, non-zero code length
            uint32_t j = i - 1;
            do {
                j--;
            } while (j > 0 && unadjusted.at(j) == 0);

            if (j == 0) {
                std::cerr << "Error, no shorter non-zero code found\n";
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
    CodeSizes codeSizes;
    std::ranges::copy(unadjusted | std::views::drop(1) | std::views::take(16), codeSizes.getFrequencies().begin());
    return codeSizes;
}

auto FileParser::Jpeg::CodeSizeEncoder::getCodeSizeFrequencies(const ByteFrequencies& frequencies) -> CodeSizes {
    const auto codeSizesPerByte = getCodeSizesPerByte(frequencies);
    auto unadjustedSizes = countCodeSizes(codeSizesPerByte);
    return adjustCodeSizes(unadjustedSizes);
}

void FileParser::Jpeg::HuffmanEncoder::countFrequencies(const std::vector<Encoder::Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies) {
    for (auto& coefficient : coefficients) {
        outFrequencies[coefficient.encoding]++;
    }
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

FileParser::Jpeg::HuffmanEncoder::HuffmanEncoder(const std::vector<Encoder::Coefficient>& coefficients)
    : m_coefficientFrequencies(countFrequencies(coefficients)),
      m_symbolsByFrequency(getSymbolsOrderedByFrequency(m_coefficientFrequencies)),
      m_codeSizes(CodeSizeEncoder::getCodeSizeFrequencies(m_coefficientFrequencies)),
      m_table(HuffmanBuilder::generateEncodings(m_symbolsByFrequency, m_codeSizes.getFrequencies()))
{

    // Return huffman
    // Validate that symbols and code sizes match, ignoring the frequency of code size 0
    // TODO: Validation below
    // if (sortedSymbols.size() != std::accumulate(codeSizeFrequencies.begin() + 1, codeSizeFrequencies.end(), 0u)) {
    //     throw std::invalid_argument("Number of symbols and code sizes do not match");
    // }
    //
    // // Validate code size frequencies
    // for (size_t i = MaxHuffmanBits + 1; i < codeSizeFrequencies.size(); i++) {
    //     if (codeSizeFrequencies[i] != 0) {
    //         throw std::invalid_argument("Invalid Huffman code size: " + std::to_string(codeSizeFrequencies[i]));
    //     }
    // }
}

auto FileParser::Jpeg::HuffmanEncoder::countFrequencies(
    const std::vector<Encoder::Coefficient>& coefficients
) -> ByteFrequencies {
    ByteFrequencies frequencies;
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
    std::vector<uint8_t> symbolsByFrequency;
    symbolsByFrequency.reserve(freqPairs.size());
    for (auto key : freqPairs | std::views::keys) {
        symbolsByFrequency.emplace_back(key);
    }
    return symbolsByFrequency;
}
