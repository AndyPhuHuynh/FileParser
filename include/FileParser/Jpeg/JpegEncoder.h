﻿#pragma once

#include <filesystem>

#include "FileParser/Jpeg/HuffmanEncoder.hpp"
#include "FileParser/Jpeg/JpegBitWriter.h"
#include "FileParser/Jpeg/JpegImage.h"

namespace FileParser::Jpeg::Encoder {
    const std::array<float, QuantizationTable::length> LuminanceTable = {
        16, 11, 10, 16, 24, 40, 51, 61,
        12, 12, 14, 19, 26, 58, 60, 55,
        14, 13, 16, 24, 40, 57, 69, 56,
        14, 17, 22, 29, 51, 87, 80, 62,
        18, 22, 37, 56, 68, 109, 103, 77,
        24, 35, 55, 64, 81, 104, 113, 92,
        49, 64, 78, 87, 103, 121, 120, 101,
        72, 92, 95, 98, 112, 100, 103, 99
    };

    const std::array<float, QuantizationTable::length> ChrominanceTable = {
        17, 18, 24, 47, 99, 99, 99, 99,
        18, 21, 26, 66, 99, 99, 99, 99,
        24, 26, 56, 99, 99, 99, 99, 99,
        47, 66, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99,
        99, 99, 99, 99, 99, 99, 99, 99
    };

    struct EncodingSettings {
        int luminanceQuality;
        int chrominanceQuality;
        bool optimizeHuffmanTables;
    };
    
    constexpr int MaxHuffmanBits = 16;
    const HuffmanTable& getDefaultLuminanceDcTable();
    const HuffmanTable& getDefaultLuminanceAcTable();
    const HuffmanTable& getDefaultChrominanceDcTable();
    const HuffmanTable& getDefaultChrominanceAcTable();
    
    struct Coefficient {
        uint8_t encoding; // Run-length encoded symbol
        int value; // Actual value of the coefficient
        Coefficient() = default;
        Coefficient(const uint8_t encoding, const int value) : encoding(encoding), value(value) {}
    };
    
    struct EncodedMcu {
        std::vector<std::vector<Coefficient>> Y;
        std::vector<Coefficient> Cb;
        std::vector<Coefficient> Cr;
    };
    
    void encodeCoefficients(const Component& component, std::vector<Coefficient>& outCoefficients,
                            std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, int& prevDc);
    void encodeCoefficients(const Mcu& mcu, std::vector<EncodedMcu>& outEncodedMcus, std::array<int, 3>& prevDc,
        std::vector<Coefficient>& outLuminanceDcCoefficients, std::vector<Coefficient>& outLuminanceAcCoefficients,
        std::vector<Coefficient>& outChromaDcCoefficients, std::vector<Coefficient>& outChromaAcCoefficients);
    void encodeCoefficients(const std::vector<Mcu>& mcus, std::vector<EncodedMcu>& outEncodedMcus,
        std::vector<Coefficient>& outLuminanceDcCoefficients, std::vector<Coefficient>& outLuminanceAcCoefficients,
        std::vector<Coefficient>& outChromaDcCoefficients, std::vector<Coefficient>& outChromaAcCoefficients);

    // Huffman encoding
    
    // Writing to file
    
    void writeMarker(uint8_t marker, JpegBitWriter& bitWriter);
    void writeFrameHeaderComponentSpecification(const FrameHeaderComponentSpecification& component, JpegBitWriter& bitWriter);
    void writeFrameHeader(const FrameHeader& frameHeader, JpegBitWriter& bitWriter);

    QuantizationTable createQuantizationTable(const std::array<float, 64>& table, int quality, bool is8Bit, uint8_t tableDestination);
    void writeQuantizationTableNoMarker(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter);
    void writeQuantizationTable(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter);

    void writeScanHeaderComponentSpecification(const ScanHeaderComponentSpecification& component, JpegBitWriter& bitWriter);
    void writeScanHeader(const ScanHeader& scanHeader, JpegBitWriter& bitWriter);

    int encodeSSSS(uint8_t SSSS, int value);
    void writeCoefficients(const std::vector<Coefficient>& coefficients,
        const HuffmanTable& dcTable, const HuffmanTable& acTable, JpegBitWriter& bitWriter);
    void writeEncodedMcu(const EncodedMcu& mcu, const HuffmanTable& luminanceDcTable, const HuffmanTable& luminanceAcTable,
        const HuffmanTable& chrominanceDcTable, const HuffmanTable& chrominanceAcTable, JpegBitWriter& bitWriter);
    void writeEncodedMcu(const std::vector<EncodedMcu>& mcus, const HuffmanTable& luminanceDcTable, const HuffmanTable& luminanceAcTable,
        const HuffmanTable& chrominanceDcTable, const HuffmanTable& chrominanceAcTable, JpegBitWriter& bitWriter);

    std::expected<void, std::string> writeJpeg(const std::string& filepath, std::vector<Mcu>& mcus,
                                               const EncodingSettings& settings, uint16_t pixelHeight,
                                               uint16_t pixelWidth);
}
