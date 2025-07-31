#pragma once

#include <complex.h>
#include <expected>

#include "FileParser/Huffman/Table.hpp"

namespace FileParser::Jpeg {
    class HuffmanBuilder {
        public:
            /**
             * @brief Reads a single Huffman table from a JPEG file stream.
             *
             * Parses Huffman table data starting at the specified file offset. This reads exactly one Huffman table
             * from the DHT marker segment.
             *
             * @param file The input file stream pointing to the JPEG file.
             * @return A HuffmanTable constructed from the parsed file data
             *
             * @note If the DHT segment contains multiple Huffman tables, call this function repeatedly, updating the
             * file position each time, to read each table in sequence
             */
            static auto readFromFile(std::ifstream& file) -> std::expected<
                HuffmanTable, std::string>;

            /**
             * @param symbols The list of symbols to be encoded.
             * @param codeSizeFrequencies An array that represents the amount of symbols with each code size.
             * Array[i] is the amount of symbols with code size i + 1.
             * @return A vector of the HuffmanEncodings for each given symbol.
             */
            static auto generateEncodings(
                const std::vector<uint8_t>& symbols, const std::array<uint8_t,
                HuffmanTable::maxEncodingLength>& codeSizeFrequencies) -> std::vector<HuffmanEncoding>;
    };
}