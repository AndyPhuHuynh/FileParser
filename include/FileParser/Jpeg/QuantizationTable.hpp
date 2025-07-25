#pragma once

#include <array>
#include <fstream>

namespace FileParser::Jpeg {
    struct QuantizationTable {
        static constexpr size_t length = 64;
        std::array<float, length> table{};
        bool is8Bit = true;
        uint8_t tableDestination = 0;

        QuantizationTable() = default;
        QuantizationTable(std::ifstream& file, const std::streampos& dataStartIndex, bool is8Bit, uint8_t tableDestination);
        QuantizationTable(const std::array<float, length>& table, const bool is8Bit, const uint8_t tableDestination)
        : table(table), is8Bit(is8Bit), tableDestination(tableDestination) {}
        void print() const;
    };
}
