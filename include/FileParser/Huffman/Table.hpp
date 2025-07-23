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
        HuffmanEncoding(const uint16_t encoding, const uint8_t bitLength, const uint8_t value)
            : encoding(encoding), bitLength(bitLength), value(value) {}
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
        HuffmanTableEntry(const uint8_t bitLength, const uint8_t value) : bitLength(bitLength), value(value) {}
        HuffmanTableEntry(const uint8_t bitLength, const uint8_t value, HuffmanSubtable table)
        : bitLength(bitLength), value(value), nestedTable(std::move(table)) {}
    };

    class HuffmanTable {
    public:
        static constexpr int maxEncodingLength = 16;

        std::vector<HuffmanEncoding> encodings;
    private:
        HuffmanSubtable m_table;
        std::map<uint8_t, HuffmanEncoding> m_encodingLookup;
        bool m_isInitialized = false;
    public:
        HuffmanTable() = default;
        HuffmanTable(const HuffmanTable&) = default;
        HuffmanTable& operator=(const HuffmanTable&) = default;
        HuffmanTable(HuffmanTable&&) noexcept = default;
        HuffmanTable& operator=(HuffmanTable&&) = default;

        explicit HuffmanTable(const std::vector<HuffmanEncoding>& encodings);

        [[nodiscard]] auto decode(uint16_t word) const -> std::pair<uint8_t, uint8_t>;
        [[nodiscard]] auto encode(uint8_t symbol) const -> HuffmanEncoding;

        [[nodiscard]] auto isInitialized() const -> bool;
    private:
        void generateLookupTable(const std::vector<HuffmanEncoding>& encodings);
    };
}