#pragma once

#include <array>
#include <cstdint>
#include <expected>
#include <xstring>

#include "FileParser/Huffman/CodeSizes.hpp"
#include "FileParser/Huffman/Table.hpp"

namespace FileParser::Jpeg {
    namespace Encoder {
        struct Coefficient;
    }

    using ByteFrequencies = std::array<uint32_t, 256>;

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
    private:
        explicit HuffmanEncoder(
            ByteFrequencies frequencies, std::vector<uint8_t> symbolsByFrequency,
            CodeSizes codeSizes, HuffmanTable table)
            : m_coefficientFrequencies(std::move(frequencies)), m_symbolsByFrequency(std::move(symbolsByFrequency)),
              m_codeSizes(std::move(codeSizes)), m_table(std::move(table)) {}

        static auto countFrequencies(const std::vector<Encoder::Coefficient>& coefficients) -> ByteFrequencies;
        static auto getSymbolsOrderedByFrequency(const std::array<uint32_t, 256>& frequencies) -> std::vector<uint8_t>;
    };
}
