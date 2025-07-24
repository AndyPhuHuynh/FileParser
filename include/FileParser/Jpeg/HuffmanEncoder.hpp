#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>

#include "FileParser/Huffman/CodeSizes.hpp"
#include "FileParser/Huffman/Table.hpp"
#include "FileParser/Jpeg/JpegBitWriter.h"

namespace FileParser::Jpeg {
    namespace Encoder {
        struct Coefficient;
    }

    using ByteFrequencies = std::array<uint32_t, 256>;

    enum class TableDescription : uint8_t {
        LuminanceDC   = 0x00,
        LuminanceAC   = 0x10,
        ChrominanceDC = 0x01,
        ChrominanceAC = 0x11,
    };

    /**
    * @class CodeSizeEncoder
    * @brief Provides utilities to compute and adjust Huffman code lengths for JPEG encoding.
    *
    * This class is responsible for generating the Huffman code lengths for each symbol
    * based on their frequency in the input data. It follows the JPEG standard requirements
    * that Huffman code lengths must be between 1 and 16 bits.
    */
    class CodeSizeEncoder {
        using CodeSizePerByte = std::array<uint8_t, 257>;
        using UnadjustedCodeSizeFrequencies = std::array<uint8_t, 33>;

        /**
         * @brief Calculates the Huffman code sizes of each symbol given a list of symbols and their frequencies.
         *
         * @param frequencies An array of length 256 where frequencies[symbol] represents the occurrence count of each
         *                    symbol (0–255) in the data.
         * @return An array of length 257 where codeSizes[symbol] is the Huffman code length (in bits) assigned to that
         *         symbol. The extra element at index 256 is set to 1 to guarantee that no symbol is assigned a code
         *         consisting entirely of 1s bits, which would conflict with JPEG markers.
         */
        static auto getCodeSizesPerByte(const ByteFrequencies& frequencies) -> CodeSizePerByte;

        /**
         * @brief Calculates the frequency of Huffman code sizes.
         *
         * @param codeSizes An array of length 257 where codeSizes[symbol] represents the Huffman code size of that symbol.
         * @return An array of length 33 where result[length] represents the number of symbols that have the given
         *         Huffman code length (0–32).
         */
        static auto countCodeSizes(const CodeSizePerByte& codeSizes) -> UnadjustedCodeSizeFrequencies;

        /**
         * @brief Adjusts code sizes so that code sizes are between 1 and 16.
         *
         * @param unadjusted An array of length 33 where unadjusted[length] indicates how many symbols have that code
         *                   length before adjustment.
         * @return A @code std::expected<CodeSizes, std::string>@endcode containing the adjusted code lengths if
         *         successful, or an error string if adjustment fails.
         */
        static auto adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> std::expected<CodeSizes, std::string>;

    public:
        /**
        * @brief Computes the final Huffman code lengths (1–16 bits) for each symbol.
        *
        * This method combines frequency analysis, counting, and adjustment to produce valid JPEG Huffman code lengths.
        *
        * @param frequencies An array of length 256 where frequencies[symbol] represents the occurrence count of each
        *                    symbol in the input data.
        * @return A @code std::expected<CodeSizes, std::string>@endcode containing the final Huffman code lengths if
        *         successful, or an error string if code generation fails.
        */
        [[nodiscard]] static auto getCodeSizes(const ByteFrequencies& frequencies)
            -> std::expected<CodeSizes, std::string>;
    };

    class HuffmanEncoder {
        ByteFrequencies m_coefficientFrequencies{};
        std::vector<uint8_t> m_symbolsByFrequency;
        CodeSizes m_codeSizes;
        HuffmanTable m_table;
    public:
        static auto create(const std::vector<Encoder::Coefficient>& coefficients)
            -> std::expected<HuffmanEncoder, std::string>;

        [[nodiscard]] auto getSymbolsByFrequencies() const -> const std::vector<uint8_t>&;
        [[nodiscard]] auto getCodeSizes() const -> const CodeSizes&;
        [[nodiscard]] auto getTable() const -> const HuffmanTable&;

        auto writeToFile(JpegBitWriter& bitWriter, TableDescription description) const -> void;
    private:
        explicit HuffmanEncoder(
            const ByteFrequencies& frequencies, std::vector<uint8_t> symbolsByFrequency,
            const CodeSizes codeSizes, HuffmanTable table)
            : m_coefficientFrequencies(frequencies), m_symbolsByFrequency(std::move(symbolsByFrequency)),
              m_codeSizes(codeSizes), m_table(std::move(table)) {}

        static auto countFrequencies(const std::vector<Encoder::Coefficient>& coefficients) -> ByteFrequencies;
        static auto getSymbolsOrderedByFrequency(const std::array<uint32_t, 256>& frequencies) -> std::vector<uint8_t>;
    };
}
