#pragma once

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

    void forwardDCT(std::array<float, Mcu::DataUnitLength>& component);
    void quantize(const QuantizationTable& quantizationTable, std::array<float, Mcu::DataUnitLength>& component);
    void encodeBlock(const std::array<float, Mcu::DataUnitLength>& component, std::ofstream& outFile);
    
    void writeFrameHeaderComponentSpecification(const FrameHeaderComponentSpecification& component, std::ofstream& outFile);
    void writeFrameHeader(const FrameHeader& frameHeader, std::ofstream& outFile);

    QuantizationTable createQuantizationTable(const std::array<float, 64>& table, int quality, bool is8Bit, uint8_t tableDestination);
    void writeQuantizationTable(const QuantizationTable& quantizationTable, std::ofstream& outFile);
}
