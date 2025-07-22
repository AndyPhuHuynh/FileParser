#pragma once

#include <array>
#include <cstdint>

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
        static auto adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> CodeSizes;

    public:
        [[nodiscard]] static auto getCodeSizes(const ByteFrequencies& frequencies) -> CodeSizes;
    };

    class HuffmanEncoder {
        ByteFrequencies m_coefficientFrequencies{};
        std::vector<uint8_t> m_symbolsByFrequency;
        CodeSizes m_codeSizes;
        HuffmanTable m_table;
    public:
        explicit HuffmanEncoder(const std::vector<Encoder::Coefficient>& coefficients);
        [[nodiscard]] auto getSymbolsByFrequencies() const -> const std::vector<uint8_t>&;
        [[nodiscard]] auto getCodeSizes() const -> const CodeSizes&;
        [[nodiscard]] auto getTable() const -> const HuffmanTable&;
    private:
        static auto countFrequencies(const std::vector<Encoder::Coefficient>& coefficients) -> ByteFrequencies;
        static auto getSymbolsOrderedByFrequency(const std::array<uint32_t, 256>& frequencies) -> std::vector<uint8_t>;
    };
}
