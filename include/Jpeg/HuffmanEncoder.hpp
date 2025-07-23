#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <string>

#include "Jpeg/JpegBitWriter.h"
#include "FileParser/Huffman/CodeSizes.hpp"
#include "FileParser/Huffman/Table.hpp"

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

    class CodeSizeEncoder {
        using CodeSizesPerByte = std::array<uint8_t, 257>;
        using UnadjustedCodeSizeFrequencies = std::array<uint8_t, 33>;

        static auto getCodeSizesPerByte(const ByteFrequencies& frequencies) -> CodeSizesPerByte;
        static auto countCodeSizes(const CodeSizesPerByte& codeSizes) -> UnadjustedCodeSizeFrequencies;
        static auto adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> std::expected<CodeSizes, std::string>;

    public:
        [[nodiscard]] static auto getCodeSizes(const ByteFrequencies& frequencies) -> std::expected<CodeSizes, std::
            string>;
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
