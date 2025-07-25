#include "FileParser/Jpeg/QuantizationTableBuilder.hpp"

auto FileParser::Jpeg::QuantizationTableBuilder::readFromBitReader(BitReader& bytes)
    -> std::expected<QuantizationTable, std::string> {
    if (bytes.reachedEnd()) {
        return std::unexpected("Unable to read precision and table id");
    }
    const uint8_t precisionAndId = static_cast<uint8_t>(bytes.getNBits(8));

    QuantizationTable table {
        .precision        = GetNibble(precisionAndId, 0),
        .destination = GetNibble(precisionAndId, 1),
    };

    if (table.precision == 0) {
        for (const unsigned char i : zigZagMap) {
            if (bytes.reachedEnd()) {
                return std::unexpected("Length was too short, unable to fully read table");
            }
            table[i] = static_cast<float>(bytes.getNBits(8));
        }
        return table;
    } else {
        for (const unsigned char i : zigZagMap) {
            if (bytes.reachedEnd()) return std::unexpected("Length was too short, unable to fully read table");
            const uint8_t high = static_cast<uint8_t>(bytes.getNBits(8));
            if (bytes.reachedEnd()) return std::unexpected("Length was too short, unable to fully read table");
            const uint8_t low = static_cast<uint8_t>(bytes.getNBits(8));
            table[i] = static_cast<float>((high << 8) | low);
        }
        return table;
    }
}
