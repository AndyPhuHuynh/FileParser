#pragma once
#include "JpegEncoder.h"
#include "FileParser/Huffman/CodeSizes.hpp"

namespace FileParser::Jpeg {
    using ByteFrequencies = std::array<uint32_t, 256>;
    using CodeSizesPerByte = std::array<uint8_t, 257>;
    using UnadjustedCodeSizeFrequencies = std::array<uint8_t, 33>;

    class CodeSizeEncoder {
        static auto generateCodeSizes(const ByteFrequencies& frequencies) -> CodeSizesPerByte;
        static auto countCodeSizes(const CodeSizesPerByte& codeSizes) -> UnadjustedCodeSizeFrequencies;
        static auto adjustCodeSizes(UnadjustedCodeSizeFrequencies& unadjusted) -> CodeSizes;
    };

    class HuffmanEncoder {
        ByteFrequencies m_coefficientFrequencies{};
        std::array<uint8_t, 257> outCodeSizes{};
    public:
        static void countFrequencies(const std::vector<Encoder::Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies);
        static void generateCodeSizes(const std::array<uint32_t, 256>& frequencies, std::array<uint8_t, 257>& outCodeSizes);
        static void adjustCodeSizes(std::array<uint8_t, 33>& codeSizeFrequencies);
        static void countCodeSizes(const std::array<uint8_t, 257>& codeSizes, std::array<uint8_t, 33>& outCodeSizeFrequencies);

        // Used to extract info out of a huffman table encodings, might not need after refactor
        static void countCodeSizes(const std::vector<HuffmanEncoding>& encodings, std::array<uint8_t, 33>& outCodeSizeFrequencies);
        static void sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols);

        // Used to extract info out of a huffman table encodings, might not need after refactor
        static void sortEncodingsByLength(const std::vector<HuffmanEncoding>& encodings, std::vector<uint8_t>& outSortedSymbols);

        explicit HuffmanEncoder(const std::vector<Encoder::Coefficient>& coefficients);

        auto countFrequencies(const std::vector<Encoder::Coefficient>& coefficients) -> void;
    };
}
