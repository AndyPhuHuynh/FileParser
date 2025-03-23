#pragma once

#include <filesystem>

#include "Bmp.h"
#include "JpegBitWriter.h"
#include "JpegImage.h"

namespace ImageProcessing::Jpeg::Encoder {
    const std::array<float, QuantizationTable::TableLength> LuminanceTable = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99
    };

    const std::array<float, QuantizationTable::TableLength> ChrominanceTable = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
    };

    constexpr int MaxHuffmanBits = 16;
    
    void forwardDCT(std::array<float, Mcu::DataUnitLength>& component);
    void forwardDCT(const Mcu& mcu);
    void forwardDCT(const std::vector<Mcu>& mcus);
    void quantize(std::array<float, Mcu::DataUnitLength>& component, const QuantizationTable& quantizationTable);
    void quantize(const Mcu& mcu, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);
    void quantize(const std::vector<Mcu>& mcus, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);
    
    struct Coefficient {
        uint8_t encoding; // Run-length encoded symbol
        int value; // Actual value of the coefficient
        Coefficient() = default;
        Coefficient(const uint8_t encoding, const int value) : encoding(encoding), value(value) {}
    };

    struct EncodedBlock {
        std::vector<Coefficient> coefficients;
    };
    
    void encodeCoefficients(const std::array<float, Mcu::DataUnitLength>& component, std::vector<EncodedBlock>& blocks,
        std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, int& prevDc);
    void encodeCoefficients(const Mcu& mcu, std::vector<EncodedBlock>& blocks,
        std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, std::array<int, 3>& prevDc);
    void encodeCoefficients(const std::vector<Mcu>& mcus, std::vector<EncodedBlock>& blocks,
        std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients);

    // Huffman encoding
    
    void countFrequencies(const std::vector<Coefficient>& coefficients, std::array<uint32_t, 256>& outFrequencies);
    void generateCodeSizes(const std::array<uint32_t, 256>& frequencies, std::array<uint8_t, 257>& outCodeSizes);
    void adjustCodeSizes(std::array<uint8_t, 33>& codeSizeFrequencies);
    void countCodeSizes(const std::array<uint8_t, 257>& codeSizes, std::array<uint8_t, 33>& outCodeSizeFrequencies);
    void sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols);

    // Writing to file
    
    void writeMarker(uint8_t marker, JpegBitWriter& bitWriter);
    void writeFrameHeaderComponentSpecification(const FrameHeaderComponentSpecification& component, JpegBitWriter& bitWriter);
    void writeFrameHeader(const FrameHeader& frameHeader, JpegBitWriter& bitWriter);

    QuantizationTable createQuantizationTable(const std::array<float, 64>& table, int quality, bool is8Bit, uint8_t tableDestination);
    void writeQuantizationTableNoMarker(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter);
    void writeQuantizationTable(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter);

    HuffmanTable createHuffmanTable(const std::vector<uint8_t>& sortedSymbols, const std::array<uint8_t, 33>& codeSizeFrequencies);
    HuffmanTable writeHuffmanTableNoMarker(uint8_t tableClass, uint8_t tableDestination,
        const std::vector<uint8_t>& sortedSymbols, const std::array<uint8_t, 33>& codeSizesFrequencies, JpegBitWriter& bitWriter);
    HuffmanTable writeHuffmanTable(uint8_t tableClass, uint8_t tableDestination,
        const std::vector<uint8_t>& sortedSymbols, const std::array<uint8_t, 33>& codeSizesFrequencies, JpegBitWriter& bitWriter);
    HuffmanTable writeHuffmanTable(const std::vector<Coefficient>& coefficients, uint8_t tableClass, uint8_t tableDestination, JpegBitWriter& bitWriter);

    void writeScanHeaderComponentSpecification(const ScanHeaderComponentSpecification& component, JpegBitWriter& bitWriter);
    void writeScanHeader(const ScanHeader& scanHeader, JpegBitWriter& bitWriter);

    int encodeSSSS(uint8_t SSSS, int value);
    void writeBlock(const EncodedBlock& block, const HuffmanTable& dcTable, const HuffmanTable& acTable, JpegBitWriter& bitWriter);
    void writeBlock(const std::vector<EncodedBlock>& blocks, const HuffmanTable& dcTable, const HuffmanTable& acTable, JpegBitWriter& bitWriter);
    
    std::vector<Mcu> getMcus(Bmp& bmp);
    void writeJpeg(Bmp& bmp);
}
