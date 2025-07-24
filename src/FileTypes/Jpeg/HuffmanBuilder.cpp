#include "FileParser/Jpeg/HuffmanBuilder.hpp"

#include <array>
#include <fstream>
#include <numeric>

auto FileParser::Jpeg::HuffmanBuilder::readFromFile(
    std::ifstream& file,
    const std::streampos& dataStartIndex
) -> HuffmanTable {
    file.seekg(dataStartIndex, std::ios::beg);
    // Read num of each encoding
    std::array<uint8_t, HuffmanTable::maxEncodingLength> codeSizes{};
    for (int i = 0; i < HuffmanTable::maxEncodingLength; i++) {
        file.read(reinterpret_cast<char*>(&codeSizes[i]), 1);
    }

    // Read symbols
    const int symbolCount = std::accumulate(codeSizes.begin(), codeSizes.end(), 0);
    std::vector<uint8_t> symbols;
    for (int i = 0; i < symbolCount; i++) {
        uint8_t byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        symbols.push_back(byte);
    }

    const std::vector<HuffmanEncoding> encodings = generateEncodings(symbols, codeSizes);
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
