#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <vector>

namespace FileParser {
    struct HuffmanEncoding {
        uint16_t encoding;
        uint8_t bitLength;
        uint8_t value;
        HuffmanEncoding() = default;
        HuffmanEncoding(const uint16_t encoding, const uint8_t bitLength, const uint8_t value)
            : encoding(encoding), bitLength(bitLength), value(value) {}
    };

    struct HuffmanTableEntry;
    using HuffmanSubtable = std::unique_ptr<std::array<HuffmanTableEntry, 256>>;

    struct HuffmanTableEntry {
        uint8_t bitLength;
        uint8_t value;
        HuffmanSubtable nestedTable;

        HuffmanTableEntry() : bitLength(0), value(0), nestedTable(nullptr) {}
        HuffmanTableEntry(const uint8_t bitLength, const uint8_t value) : bitLength(bitLength), value(value) {}
        HuffmanTableEntry(const uint8_t bitLength, const uint8_t value, HuffmanSubtable table)
        : bitLength(bitLength), value(value), nestedTable(std::move(table)) {}
    };

    class HuffmanTable {
    public:
        static constexpr int maxEncodingLength = 16;
    private:
        HuffmanSubtable table;
        std::map<uint8_t, HuffmanEncoding> encodingLookup;
        bool isInitialized = false;
    public:
        HuffmanTable() = default;
        explicit HuffmanTable(const std::vector<HuffmanEncoding>& encodings);

        auto decode(uint16_t word) const -> uint8_t;
        [[nodiscard]] auto encode(uint8_t symbol) const -> HuffmanEncoding;
    private:
        void generateLookupTable(const std::vector<HuffmanEncoding>& encodings);
    };
}