#pragma once
#include "JpegEncoder.h"

namespace FileParser::Jpeg {
    class HuffmanEncoder {

    public:
        static void countFrequencies(const std::vector<Encoder::Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies);
        static void generateCodeSizes(const std::array<uint32_t, 256>& frequencies, std::array<uint8_t, 257>& outCodeSizes);
        static void adjustCodeSizes(std::array<uint8_t, 33>& codeSizeFrequencies);
        static void countCodeSizes(const std::array<uint8_t, 257>& codeSizes, std::array<uint8_t, 33>& outCodeSizeFrequencies);
        static void countCodeSizes(const std::vector<HuffmanEncoding>& encodings, std::array<uint8_t, 33>& outCodeSizeFrequencies);
        static void sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols);
        static void sortEncodingsByLength(const std::vector<HuffmanEncoding>& encodings, std::vector<uint8_t>& outSortedSymbols);
    };
}
