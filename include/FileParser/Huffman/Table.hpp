#pragma once

#include <array>
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
        HuffmanEncoding(const uint16_t encoding_, const uint8_t bitLength_, const uint8_t value_)
            : encoding(encoding_), bitLength(bitLength_), value(value_) {}
    };

    struct HuffmanTableEntry;

    struct HuffmanSubtable {
        std::unique_ptr<std::array<HuffmanTableEntry, 256>> table = nullptr;

        HuffmanSubtable() = default;

        HuffmanSubtable(const HuffmanSubtable& other);
        HuffmanSubtable& operator=(const HuffmanSubtable& other);

        HuffmanSubtable(HuffmanSubtable&& other) = default;
        HuffmanSubtable& operator=(HuffmanSubtable&& other) = default;
    };

    struct HuffmanTableEntry {
        uint8_t bitLength;
        uint8_t value;
        HuffmanSubtable nestedTable;

        HuffmanTableEntry() : bitLength(0), value(0) {}
        HuffmanTableEntry(const uint8_t bitLength_, const uint8_t value_) : bitLength(bitLength_), value(value_) {}
        HuffmanTableEntry(const uint8_t bitLength_, const uint8_t value_, HuffmanSubtable table)
        : bitLength(bitLength_), value(value_), nestedTable(std::move(table)) {}
    };

    class HuffmanTable {
    public:
        static constexpr size_t maxEncodingLength = 16;

        std::vector<HuffmanEncoding> encodings;
    private:
        HuffmanSubtable m_table;
        std::map<uint8_t, HuffmanEncoding> m_encodingLookup;
    public:
        HuffmanTable() = default;
        HuffmanTable(const HuffmanTable&) = default;
        HuffmanTable& operator=(const HuffmanTable&) = default;
        HuffmanTable(HuffmanTable&&) noexcept = default;
        HuffmanTable& operator=(HuffmanTable&&) = default;

        explicit HuffmanTable(const std::vector<HuffmanEncoding>& encodings_);

        [[nodiscard]] auto decode(uint16_t word) const -> std::pair<uint8_t, uint8_t>;
        [[nodiscard]] auto encode(uint8_t symbol) const -> HuffmanEncoding;
    private:
        void generateLookupTable(const std::vector<HuffmanEncoding>& encodingsVec);
    };
}