#pragma once
#include "JpegEncoder.h"
#include "FileParser/Huffman/CodeSizes.hpp"

namespace FileParser::Jpeg {
    using ByteFrequencies = std::array<uint32_t, 256>;

    class CodeSizeEncoder {
        using CodeSizesPerByte = std::array<uint8_t, 257>;
        using UnadjustedCodeSizeFrequencies = std::array<uint8_t, 33>;

        static auto getCodeSizesPerByte(const ByteFrequencies& frequencies) -> CodeSizesPerByte;
        static auto countCodeSizes(const CodeSizesPerByte& codeSizes) -> UnadjustedCodeSizeFrequencies;
        static auto adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> CodeSizes;

    public:
        [[nodiscard]] static auto getCodeSizeFrequencies(const ByteFrequencies& frequencies) -> CodeSizes;
    };

    class HuffmanEncoder {
        ByteFrequencies m_coefficientFrequencies{};
        std::vector<uint8_t> m_symbolsByFrequency;
        CodeSizes m_codeSizes;
        HuffmanTable m_table;
    public:
        static void countFrequencies(const std::vector<Encoder::Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies);
        static void sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols);



        // Used to extract info out of a huffman table encodings, might not need after refactor
        static void countCodeSizes(const std::vector<HuffmanEncoding>& encodings, std::array<uint8_t, 33>& outCodeSizeFrequencies);
        // Used to extract info out of a huffman table encodings, might not need after refactor
        static void sortEncodingsByLength(const std::vector<HuffmanEncoding>& encodings, std::vector<uint8_t>& outSortedSymbols);



        explicit HuffmanEncoder(const std::vector<Encoder::Coefficient>& coefficients);

        static auto countFrequencies(const std::vector<Encoder::Coefficient>& coefficients) -> ByteFrequencies;
        static auto getSymbolsOrderedByFrequency(const std::array<uint32_t, 256>& frequencies) -> std::vector<uint8_t>;
    };
}
