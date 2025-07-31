#include "FileParser/Jpeg/HuffmanBuilder.hpp"

#include <array>
#include <fstream>
#include <numeric>

#include "FileParser/BitManipulationUtil.h"

auto FileParser::Jpeg::HuffmanBuilder::readFromFile(
    std::ifstream& file
) -> std::expected<HuffmanTable, std::string> {
    // Read num of each encoding
    const auto codeSizesExpected = read_uint8(file, HuffmanTable::maxEncodingLength);
    if (!codeSizesExpected) {
        return getUnexpected(codeSizesExpected, "Unable to parse code sizes");
    }
    if (codeSizesExpected->size() != HuffmanTable::maxEncodingLength) {
        return std::unexpected("Incorrect number of code sizes");
    }
    std::array<uint8_t, HuffmanTable::maxEncodingLength> codeSizes{};
    std::ranges::copy(codeSizesExpected->begin(), codeSizesExpected->end(), codeSizes.begin());

    // Read symbols
    const int symbolCount = std::accumulate(codeSizes.begin(), codeSizes.end(), 0);
    const auto symbols = read_uint8(file, symbolCount);
    if (!symbols) {
        return getUnexpected(symbols, "Unable to parse symbols");
    }

    const std::vector<HuffmanEncoding> encodings = generateEncodings(*symbols, codeSizes);
    return HuffmanTable(encodings);
}

auto FileParser::Jpeg::HuffmanBuilder::generateEncodings(
    const std::vector<uint8_t>& symbols, const std::array<uint8_t,
    HuffmanTable::maxEncodingLength>& codeSizeFrequencies
) -> std::vector<HuffmanEncoding> {
    if (symbols.size() != std::accumulate(codeSizeFrequencies.begin(), codeSizeFrequencies.end(), 0u)) {
        throw std::invalid_argument("Number of symbols and code sizes do not match");
    }
    std::vector<HuffmanEncoding> encodings;
    int symbolIndex = 0;
    uint16_t code = 0;
    for (int i = 0; i < HuffmanTable::maxEncodingLength; i++) {
        for (int j = 0; j < codeSizeFrequencies[i]; j++) {
            encodings.emplace_back(code, static_cast<uint8_t>(i + 1), symbols[symbolIndex++]);
            code++;
        }
        code = static_cast<uint16_t>(code << 1);
    }

    return encodings;
}
