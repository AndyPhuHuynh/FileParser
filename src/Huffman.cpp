#include "Huffman.hpp"

#include <array>
#include <iostream>

FileParser::HuffmanTable::HuffmanTable(const std::vector<HuffmanEncoding>& encodings) {
    for (auto& encoding : encodings) {
        encodingLookup.insert(std::make_pair(encoding.value, encoding));
    }
    generateLookupTable(encodings);
    isInitialized = true;
}

auto FileParser::HuffmanTable::decode(const uint16_t word) const -> uint8_t {
    const auto& decoding = (*table)[static_cast<uint8_t>(word >> 8 & 0xFF)];
    if (decoding.nestedTable == nullptr) {
        if (decoding.bitLength == 0) {
            std::cerr << "Huffman encoding does not exist!\n";
        }
        return decoding.value;
    }
    const auto& decoding2 = (*decoding.nestedTable)[static_cast<uint8_t>(word & 0xFF)];
    if (decoding2.nestedTable == nullptr && decoding2.bitLength == 0) {
        std::cerr << "Huffman encoding does not exist!\n";
    }
    return decoding2.value;
}

// Used in the Encoder to find the encoding for a symbol
auto FileParser::HuffmanTable::encode(const uint8_t symbol) const -> HuffmanEncoding {
    return encodingLookup.at(symbol);
}

// Main function that generates label
void FileParser::HuffmanTable::generateLookupTable(const std::vector<HuffmanEncoding>& encodings) {
    table = std::make_unique<std::array<HuffmanTableEntry, 256>>();
    for (const auto& encoding : encodings) {
        if (encoding.bitLength <= 8) {
            const auto leftShifted = static_cast<uint8_t>(encoding.encoding << (8 - encoding.bitLength));
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (8 - encoding.bitLength)); i++) {
                const uint8_t index = i | leftShifted;
                (*table)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value, nullptr);
            }
        } else {
            const auto partOne = static_cast<uint8_t>(encoding.encoding >> (encoding.bitLength - 8) & 0xFF);
            const auto partTwo = static_cast<uint8_t>(encoding.encoding << (maxEncodingLength - encoding.bitLength) & 0xFF);
            if ((*table)[partOne].nestedTable == nullptr) {
                (*table)[partOne].nestedTable = std::make_unique<std::array<HuffmanTableEntry, 256>>();
            }
            const auto partTwoTable = (*table)[partOne].nestedTable.get();
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (16 - encoding.bitLength)); i++) {
                const uint8_t index = i | partTwo;
                (*partTwoTable)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value, nullptr);
            }
        }
    }
}
