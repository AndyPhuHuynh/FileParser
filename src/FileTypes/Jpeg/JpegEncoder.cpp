#include "Jpeg/JpegEncoder.h"

#include <iostream>
#include <ranges>

void ImageProcessing::Jpeg::Encoder::forwardDCT(std::array<float, Mcu::DataUnitLength>& component) {
    for (int i = 0; i < 8; ++i) {
        const float a0 = component.at(0 * 8 + i);
        const float a1 = component.at(1 * 8 + i);
        const float a2 = component.at(2 * 8 + i);
        const float a3 = component.at(3 * 8 + i);
        const float a4 = component.at(4 * 8 + i);
        const float a5 = component.at(5 * 8 + i);
        const float a6 = component.at(6 * 8 + i);
        const float a7 = component.at(7 * 8 + i);

        const float b0 = a0 + a7;
        const float b1 = a1 + a6;
        const float b2 = a2 + a5;
        const float b3 = a3 + a4;
        const float b4 = a3 - a4;
        const float b5 = a2 - a5;
        const float b6 = a1 - a6;
        const float b7 = a0 - a7;

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 = b1 - b2;
        const float c3 = b0 - b3;
        const float c4 = b4;
        const float c5 = b5 - b4;
        const float c6 = b6 - c5;
        const float c7 = b7 - b6;

        const float d0 = c0 + c1;
        const float d1 = c0 - c1;
        const float d2 = c2;
        const float d3 = c3 - c2;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c5 + c7;
        const float d8 = c4 - c6;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * m1;
        const float e3 = d3;
        const float e4 = d4 * m2;
        const float e5 = d5 * m3;
        const float e6 = d6 * m4;
        const float e7 = d7;
        const float e8 = d8 * m5;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4 + e8;
        const float f5 = e5 + e7;
        const float f6 = e6 + e8;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = f5 - f6;
        const float g7 = f7 - f4;

        component.at(0 * 8 + i) = g0 * s0;
        component.at(4 * 8 + i) = g1 * s4;
        component.at(2 * 8 + i) = g2 * s2;
        component.at(6 * 8 + i) = g3 * s6;
        component.at(5 * 8 + i) = g4 * s5;
        component.at(1 * 8 + i) = g5 * s1;
        component.at(7 * 8 + i) = g6 * s7;
        component.at(3 * 8 + i) = g7 * s3;
    }
    for (int i = 0; i < 8; ++i) {
        const float a0 = component.at(i * 8 + 0);
        const float a1 = component.at(i * 8 + 1);
        const float a2 = component.at(i * 8 + 2);
        const float a3 = component.at(i * 8 + 3);
        const float a4 = component.at(i * 8 + 4);
        const float a5 = component.at(i * 8 + 5);
        const float a6 = component.at(i * 8 + 6);
        const float a7 = component.at(i * 8 + 7);

        const float b0 = a0 + a7;
        const float b1 = a1 + a6;
        const float b2 = a2 + a5;
        const float b3 = a3 + a4;
        const float b4 = a3 - a4;
        const float b5 = a2 - a5;
        const float b6 = a1 - a6;
        const float b7 = a0 - a7;

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 = b1 - b2;
        const float c3 = b0 - b3;
        const float c4 = b4;
        const float c5 = b5 - b4;
        const float c6 = b6 - c5;
        const float c7 = b7 - b6;

        const float d0 = c0 + c1;
        const float d1 = c0 - c1;
        const float d2 = c2;
        const float d3 = c3 - c2;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c5 + c7;
        const float d8 = c4 - c6;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * m1;
        const float e3 = d3;
        const float e4 = d4 * m2;
        const float e5 = d5 * m3;
        const float e6 = d6 * m4;
        const float e7 = d7;
        const float e8 = d8 * m5;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4 + e8;
        const float f5 = e5 + e7;
        const float f6 = e6 + e8;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = f5 - f6;
        const float g7 = f7 - f4;

        component.at(i * 8 + 0) = g0 * s0;
        component.at(i * 8 + 4) = g1 * s4;
        component.at(i * 8 + 2) = g2 * s2;
        component.at(i * 8 + 6) = g3 * s6;
        component.at(i * 8 + 5) = g4 * s5;
        component.at(i * 8 + 1) = g5 * s1;
        component.at(i * 8 + 7) = g6 * s7;
        component.at(i * 8 + 3) = g7 * s3;
    }
}

void ImageProcessing::Jpeg::Encoder::quantize(const QuantizationTable& quantizationTable, std::array<float, Mcu::DataUnitLength>& component) {
    for (int i = 0; i < Mcu::DataUnitLength; i++) {
        component.at(i) = std::floor(component.at(i) / quantizationTable.table.at(i));
    }
}

void ImageProcessing::Jpeg::Encoder::encodeBlock(const std::array<float, Mcu::DataUnitLength>& component, std::ofstream& outFile) {
    // Encode DC coefficient
    int dcDiff = static_cast<int>(component.at(0));
    int SSSS = GetMinNumBits(dcDiff);
    int numToCode = dcDiff < 0 ? (dcDiff - 1) : dcDiff;
    // Encode ac coefficients
}

void ImageProcessing::Jpeg::Encoder::writeFrameHeaderComponentSpecification(
    const FrameHeaderComponentSpecification& component, std::ofstream& outFile) {
    
    uint8_t identifier = component.identifier;
    uint8_t samplingFactor = static_cast<uint8_t>((component.horizontalSamplingFactor << 4) | component.verticalSamplingFactor);
    uint8_t quantizationTableSelector = component.quantizationTableSelector;
    
    outFile.write(reinterpret_cast<const char*>(&identifier), 1);
    outFile.write(reinterpret_cast<const char*>(&samplingFactor), 1);
    outFile.write(reinterpret_cast<const char*>(&quantizationTableSelector), 1);
}

void ImageProcessing::Jpeg::Encoder::writeFrameHeader(const FrameHeader& frameHeader, std::ofstream& outFile) {
    uint8_t frameType = frameHeader.encodingProcess;
    
    uint16_t length = 8 + frameHeader.numOfChannels * 3;
    length = SwapBytes(length);

    outFile.write(reinterpret_cast<const char*>(&MarkerHeader), 1);
    outFile.write(reinterpret_cast<const char*>(&frameType), 1);
    
    outFile.write(reinterpret_cast<const char*>(&length), 2);
    outFile.write(reinterpret_cast<const char*>(&frameHeader.precision), 1);
    
    uint16_t height = SwapBytes(frameHeader.height);
    uint16_t width = SwapBytes(frameHeader.width);
    outFile.write(reinterpret_cast<const char*>(&height), 2);
    outFile.write(reinterpret_cast<const char*>(&width), 2);
    outFile.write(reinterpret_cast<const char*>(&frameHeader.numOfChannels), 1);
    
    for (auto& component : frameHeader.componentSpecifications | std::views::values) {
        writeFrameHeaderComponentSpecification(component, outFile);
    }
}

ImageProcessing::Jpeg::QuantizationTable ImageProcessing::Jpeg::Encoder::createQuantizationTable(
    const std::array<float, QuantizationTable::TableLength>& table, int quality, const bool is8Bit, const uint8_t tableDestination) {
    if (quality < 1 || quality > 100) {
        std::cout << "Quality must be between 1 and 100\n";
        quality = std::clamp(quality, 1, 100);
    }

    int scale = quality < 50 ? (5000 / quality) : (200 - 2 * quality);
    std::array<float, QuantizationTable::TableLength> scaledTable;
    for (int i = 0; i < QuantizationTable::TableLength; i++) {
        scaledTable[i] = std::round(std::clamp(table[i] * static_cast<float>(scale) / 100, 1.0f, 255.0f));
    }
    
    return QuantizationTable(scaledTable, is8Bit, tableDestination);
}

void ImageProcessing::Jpeg::Encoder::writeQuantizationTable(const QuantizationTable& quantizationTable, std::ofstream& outFile) {
    uint8_t bytesPerEntry = quantizationTable.is8Bit ? 1 : 2;
    uint16_t length = 2 + 1 + (64 * bytesPerEntry);
    length = SwapBytes(length);
    
    outFile.write(reinterpret_cast<const char*>(&MarkerHeader), 1);
    outFile.write(reinterpret_cast<const char*>(&DQT), 1);
    outFile.write(reinterpret_cast<const char*>(&length), 2);
    
    uint8_t precision = quantizationTable.is8Bit ? 0 : 1;
    uint8_t precisionAndTableId = static_cast<uint8_t>((precision << 4) | quantizationTable.tableDestination);
    outFile.write(reinterpret_cast<const char*>(&precisionAndTableId), 1);

    if (quantizationTable.is8Bit) {
        for (unsigned char i : zigZagMap) {
            uint8_t byte = static_cast<uint8_t>(quantizationTable.table.at(i));
            outFile.write(reinterpret_cast<const char*>(&byte), 1);
        }
    } else {
        for (unsigned char i : zigZagMap) {
            uint16_t word = static_cast<uint16_t>(quantizationTable.table.at(i));
            word = SwapBytes(word);
            outFile.write(reinterpret_cast<const char*>(&word), 2);
        }
    }
}
