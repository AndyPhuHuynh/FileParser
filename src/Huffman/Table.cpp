#include "FileParser/Huffman/Table.hpp"

#include <array>
#include <iostream>

FileParser::HuffmanSubtable::HuffmanSubtable(const HuffmanSubtable& other) {
    if (other.table != nullptr) {
        table = std::make_unique<std::array<HuffmanTableEntry, 256>>(*other.table);
    }
}

FileParser::HuffmanSubtable& FileParser::HuffmanSubtable::operator=(const HuffmanSubtable& other) {
    if (this != &other) {
        if (other.table != nullptr) {
            table = std::make_unique<std::array<HuffmanTableEntry, 256>>(*other.table);
        }
    }
    return *this;
}

FileParser::HuffmanTable::HuffmanTable(const std::vector<HuffmanEncoding>& encodings) {
    this->encodings = encodings;
    for (auto& encoding : encodings) {
        m_encodingLookup.insert(std::make_pair(encoding.value, encoding));
    }
    generateLookupTable(encodings);
    m_isInitialized = true;
}

auto FileParser::HuffmanTable::decode(const uint16_t word) const -> std::pair<uint8_t, uint8_t> {
    const auto& decoding = (*m_table.table)[static_cast<uint8_t>(word >> 8 & 0xFF)];
    if (decoding.nestedTable.table == nullptr) {
        if (decoding.bitLength == 0) {
            std::cerr << "Huffman encoding does not exist!\n";
        }
        return {decoding.bitLength, decoding.value};
    }
    const auto& decoding2 = (*decoding.nestedTable.table)[static_cast<uint8_t>(word & 0xFF)];
    if (decoding2.nestedTable.table == nullptr && decoding2.bitLength == 0) {
        std::cerr << "Huffman encoding does not exist!\n";
    }
    return {decoding2.bitLength, decoding2.value};
}

// Used in the Encoder to find the encoding for a symbol
auto FileParser::HuffmanTable::encode(const uint8_t symbol) const -> HuffmanEncoding {
    return m_encodingLookup.at(symbol);
}

auto FileParser::HuffmanTable::isInitialized() const -> bool {
    return m_isInitialized;
}

// Main function that generates label
void FileParser::HuffmanTable::generateLookupTable(const std::vector<HuffmanEncoding>& encodings) {
    m_table.table = std::make_unique<std::array<HuffmanTableEntry, 256>>();
    for (const auto& encoding : encodings) {
        if (encoding.bitLength <= 8) {
            const auto leftShifted = static_cast<uint8_t>(encoding.encoding << (8 - encoding.bitLength));
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (8 - encoding.bitLength)); i++) {
                const uint8_t index = i | leftShifted;
                (*m_table.table)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value);
            }
        } else {
            const auto partOne = static_cast<uint8_t>(encoding.encoding >> (encoding.bitLength - 8) & 0xFF);
            const auto partTwo = static_cast<uint8_t>(encoding.encoding << (maxEncodingLength - encoding.bitLength) & 0xFF);
            if ((*m_table.table)[partOne].nestedTable.table == nullptr) {
                (*m_table.table)[partOne].nestedTable.table = std::make_unique<std::array<HuffmanTableEntry, 256>>();
            }
            const auto partTwoTable = (*m_table.table)[partOne].nestedTable.table.get();
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (16 - encoding.bitLength)); i++) {
                const uint8_t index = i | partTwo;
                (*partTwoTable)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value);
            }
        }
    }
}
