﻿#include "Jpeg/JpegEncoder.h"

#include <iostream>
#include <numeric>
#include <ranges>

#include "Bmp.h"

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

void ImageProcessing::Jpeg::Encoder::encodeCoefficients(const std::array<float, 64>& component, std::vector<EncodedBlock>& blocks,
    std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, int& prevDc) {

    blocks.emplace_back();
    EncodedBlock& currentBlock = blocks.back();
    
    // Encode the dc coefficient
    int dcCoefficient = static_cast<int>(component.at(0)) - prevDc;
    uint8_t dcCode = dcCoefficient == 0 ? 0 : static_cast<uint8_t>(GetMinNumBits(dcCoefficient));
    dcCoefficients.emplace_back(dcCode, dcCoefficient);
    currentBlock.coefficients.emplace_back(dcCode, dcCoefficient);
    prevDc = static_cast<int>(component.at(0));

    // Find the index of the EOB
    bool eob = true;
    int eobIndex = Mcu::DataUnitLength - 1;
    while (eobIndex >= 1) {
        int coefficient = static_cast<int>(std::round(component.at(zigZagMap[eobIndex])));
        if (coefficient != 0) {
            // If the last coefficient is non-zero, there is no eob
            if (eobIndex == Mcu::DataUnitLength - 1) {
                eob = false;
            }
            // eobIndex is currently index of last non-zero
            // Add one to make it index of last zero
            eobIndex++;
            break;
        }
        eobIndex--;
    }
    eobIndex = std::max(1, eobIndex);
    
    // Look through all the ac coefficients and find their encodings
    uint8_t r = 0;
    for (int i = 1; i < eobIndex; i++) {
        int ac = static_cast<int>(std::round(component.at(zigZagMap[i])));
        if (ac == 0) {
            if (r == 15) {
                r = 0;
                acCoefficients.emplace_back(static_cast<uint8_t>(0xF0), 0x00);
                currentBlock.coefficients.emplace_back(static_cast<uint8_t>(0xF0), 0x00);
            } else {
                r++;   
            }
        } else {
            uint8_t s = static_cast<uint8_t>(GetMinNumBits(ac));
            uint8_t code = static_cast<uint8_t>((r << 4) | s);
            acCoefficients.emplace_back(code, ac);
            currentBlock.coefficients.emplace_back(code, ac);
            r = 0;
        }
    }

    if (eob) {
        constexpr uint8_t EOBMarker = 0x00;
        acCoefficients.emplace_back(EOBMarker, 0x00);
        currentBlock.coefficients.emplace_back(EOBMarker, 0x00);
    }
}

void ImageProcessing::Jpeg::Encoder::encodeCoefficients(const Mcu& mcu, std::vector<EncodedBlock>& blocks,
    std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, std::array<int, 3>& prevDc) {
    for (auto& y : mcu.Y) {
        encodeCoefficients(*y, blocks, dcCoefficients, acCoefficients, prevDc[0]);
    }
    encodeCoefficients(*mcu.Cb, blocks, dcCoefficients, acCoefficients, prevDc[1]);
    encodeCoefficients(*mcu.Cr, blocks, dcCoefficients, acCoefficients, prevDc[2]);
}

void ImageProcessing::Jpeg::Encoder::encodeCoefficients(const std::vector<Mcu>& mcus, std::vector<EncodedBlock>& blocks,
    std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients) {
    std::array prevDc = {0, 0, 0};
    for (auto& mcu : mcus) {
        encodeCoefficients(mcu, blocks, dcCoefficients, acCoefficients, prevDc);
    }
}

void ImageProcessing::Jpeg::Encoder::countFrequencies(const std::vector<Coefficient>& Coefficients, std::array<uint32_t, 256>& outFrequencies) {
    for (auto& coeff : Coefficients) {
        outFrequencies[coeff.encoding]++;
    }
}

void ImageProcessing::Jpeg::Encoder::generateCodeSizes(const std::array<uint32_t, 256>& frequencies, std::array<uint8_t, 257>& outCodeSizes) {
    // Initialize frequencies
    std::array<uint32_t, 257> freq;
    std::ranges::copy(frequencies, freq.begin());
    freq[256] = 1;
    // Initialize codeSize
    std::ranges::fill(outCodeSizes, 0);
    // Initialize others
    std::array<uint32_t, 257> others;
    std::ranges::fill(others, std::numeric_limits<uint32_t>::max());
    
    while (true) {
        // v1 is the least frequent, v2 is the second least frequent
        uint32_t v1 = std::numeric_limits<uint32_t>::max();
        uint32_t v2 = std::numeric_limits<uint32_t>::max();
        for (uint32_t i = 0; i < freq.size(); i++) {
            if (freq.at(i) == 0) continue;
            if (v1 == std::numeric_limits<uint32_t>::max() ||
                freq.at(i) < freq.at(v1) || (freq.at(i) == freq.at(v1) && i > v1)) {
                v2 = v1;
                v1 = i;
            } else if (v2 == std::numeric_limits<uint32_t>::max() || freq.at(i) < freq.at(v2)) {
                v2 = i;
            }
        }
        bool oneFound = v1 == std::numeric_limits<uint32_t>::max() || v2 == std::numeric_limits<uint32_t>::max();
        if (oneFound) break; // If there is only one item with non-zero frequency, the algorithm is finished

        // Combine the nodes
        freq.at(v1) += freq.at(v2);
        freq.at(v2) = 0;

        // Increment the code size of every node under v1
        outCodeSizes.at(v1)++;
        while (others.at(v1) != std::numeric_limits<uint32_t>::max()) {
            v1 = others.at(v1);
            outCodeSizes.at(v1)++;
        }

        // Add v2 to the end of the chain
        others.at(v1) = v2;

        // Increment the code size of every node under v2
        outCodeSizes.at(v2)++;
        while (others.at(v2) != std::numeric_limits<uint32_t>::max()) {
            v2 = others.at(v2);
            outCodeSizes.at(v2)++;
        }
    }
}

void ImageProcessing::Jpeg::Encoder::adjustCodeSizes(std::array<uint8_t, 33>& codeSizeFrequencies) {
    uint32_t i = 32;
    while (true) {
        if (codeSizeFrequencies.at(i) > 0) {
            // Find the first shorter, non-zero code length
            uint32_t j = i - 1;
            do {
                j--;
            } while (j > 0 && codeSizeFrequencies.at(j) == 0);

            if (j == 0) {
                std::cerr << "Error, no shorter non-zero code found\n";
            }
            
            // Move the prefixes around to make the codes shorter
            codeSizeFrequencies.at(i) -= 2;
            codeSizeFrequencies.at(i - 1) += 1;
            codeSizeFrequencies.at(j + 1) += 2;
            codeSizeFrequencies.at(j) -= 1;
        } else {
            i--;
            if (i <= 16) {
                 // Remove the reserved code point
                while (codeSizeFrequencies.at(i) == 0) {
                    i--;
                }
                codeSizeFrequencies.at(i)--;
                break;
            }
        }
    }
}

void ImageProcessing::Jpeg::Encoder::countCodeSizes(const std::array<uint8_t, 257>& codeSizes,
                                                    std::array<uint8_t, 33>& outCodeSizeFrequencies) {
    for (auto i : codeSizes) {
        if (i > 32) {
            std::cerr << "Invalid code size: " << i << ", clamping to 32\n";
            i = 32;
        }
        outCodeSizeFrequencies[i]++;
    }
    adjustCodeSizes(outCodeSizeFrequencies);
}

void ImageProcessing::Jpeg::Encoder::sortSymbolsByFrequencies(const std::array<uint32_t, 256>& frequencies, std::vector<uint8_t>& outSortedSymbols) {
    // Each pair stores a pair of symbol to frequency
    std::vector<std::pair<uint32_t, int>> freqPairs;
    for (int i = 0 ; i < 256; i++) {
        if (frequencies.at(i) == 0) {
            continue;
        }
        freqPairs.emplace_back(i, frequencies.at(i));
    }

    // Sort by descending frequency; for ties, sort by ascending symbol
    auto comparePair = [](const std::pair<uint32_t, int>& one, const std::pair<uint32_t, int>& two) -> bool {
        return (one.second > two.second) || (one.second == two.second && one.first < two.first);
    };
    std::ranges::sort(freqPairs, comparePair);

    // Store the sorted symbols into the output vector
    outSortedSymbols.clear();
    outSortedSymbols.reserve(freqPairs.size());
    for (auto key : freqPairs | std::views::keys) {
        outSortedSymbols.emplace_back(key);
    }
}

void ImageProcessing::Jpeg::Encoder::writeMarker(const uint8_t marker, std::ofstream& outFile) {
    outFile.write(reinterpret_cast<const char*>(&MarkerHeader), sizeof(uint8_t));
    outFile.write(reinterpret_cast<const char*>(&marker), sizeof(uint8_t));
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

void ImageProcessing::Jpeg::Encoder::writeQuantizationTableNoMarker(const QuantizationTable& quantizationTable, std::ofstream& outFile) {
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

void ImageProcessing::Jpeg::Encoder::writeQuantizationTable(const QuantizationTable& quantizationTable, std::ofstream& outFile) {
    outFile.write(reinterpret_cast<const char*>(&MarkerHeader), 1);
    outFile.write(reinterpret_cast<const char*>(&DQT), 1);
    
    uint8_t bytesPerEntry = quantizationTable.is8Bit ? 1 : 2;
    uint16_t length = 2 + 1 + (64 * bytesPerEntry);
    length = SwapBytes(length);
    outFile.write(reinterpret_cast<const char*>(&length), 2);
    
    writeQuantizationTableNoMarker(quantizationTable, outFile);
}

// Todo: buffer writing until the end
// Todo: make a class for buffered file writing
void ImageProcessing::Jpeg::Encoder::writeHuffmanTableNoMarker(
    const uint8_t tableClass, const uint8_t tableDestination, const std::vector<uint8_t>& sortedSymbols,
    const std::array<uint8_t, 33>& codeSizesFrequencies, std::ofstream& outFile) {
    if (!outFile) {
        throw std::ios_base::failure("Failed to write to output file");
    }

    // Validate that symbols and code sizes match, ignoring the frequency of code size 0
    if (sortedSymbols.size() != std::accumulate(codeSizesFrequencies.begin() + 1, codeSizesFrequencies.end(), 0u)) {
        throw std::invalid_argument("Number of symbols and code sizes do not match");
    }

    // Validate code size frequencies
    constexpr int maxHuffmanBits = 16;
    for (size_t i = maxHuffmanBits + 1; i < codeSizesFrequencies.size(); i++) {
        if (codeSizesFrequencies[i] != 0) {
            throw std::invalid_argument("Invalid Huffman code size: " + std::to_string(codeSizesFrequencies[i]));
        }
    }

    // Write table info
    uint8_t tableInfo = static_cast<uint8_t>((tableClass << 4) | tableDestination);
    outFile.write(reinterpret_cast<const char*>(&tableInfo), 1);
    
    // Write code size frequencies
    for (size_t i = 1; i <= maxHuffmanBits; i++) {
        outFile.write(reinterpret_cast<const char*>(&codeSizesFrequencies[i]), 1);
    }

    // Write codes
    for (auto symbol : sortedSymbols) {
        outFile.write(reinterpret_cast<const char*>(&symbol), 1);
    }
}

void ImageProcessing::Jpeg::Encoder::writeHuffmanTable(const uint8_t tableClass, const uint8_t tableDestination,
    const std::vector<uint8_t>& sortedSymbols, const std::array<uint8_t, 33>& codeSizesFrequencies,
    std::ofstream& outFile) {
    outFile.write(reinterpret_cast<const char*>(&MarkerHeader), 1);
    outFile.write(reinterpret_cast<const char*>(&DHT), 1);
    
    uint16_t length = 2 + 1 + 16 + static_cast<uint16_t>(sortedSymbols.size());
    length = SwapBytes(length);
    outFile.write(reinterpret_cast<const char*>(&length), 2);
    
    writeHuffmanTableNoMarker(tableClass, tableDestination, sortedSymbols, codeSizesFrequencies, outFile);
}

void ImageProcessing::Jpeg::Encoder::writeHuffmanTable(const std::vector<Coefficient>& coefficients,
    const uint8_t tableClass, const uint8_t tableDestination, std::ofstream& outFile) {
    std::array<uint32_t, 256> frequencies{};
    countFrequencies(coefficients, frequencies);
    
    std::array<uint8_t, 257> codeSizes{};
    generateCodeSizes(frequencies, codeSizes);
    
    std::array<uint8_t, 33> codeFrequencies{};
    countCodeSizes(codeSizes, codeFrequencies);
    
    std::vector<uint8_t> sortedSymbols;
    sortSymbolsByFrequencies(frequencies, sortedSymbols);

    writeHuffmanTableNoMarker(tableClass, tableDestination, sortedSymbols, codeFrequencies, outFile);
}

void ImageProcessing::Jpeg::Encoder::writeScanHeaderComponentSpecification(
    const ScanHeaderComponentSpecification& component, std::ofstream& outFile) {
    uint8_t tableDestination =  static_cast<uint8_t>((component.dcTableSelector << 4) | component.acTableSelector);
    outFile.write(reinterpret_cast<const char*>(&component.componentId), 1);
    outFile.write(reinterpret_cast<const char*>(&tableDestination), 1);
}

void ImageProcessing::Jpeg::Encoder::writeScanHeader(const ScanHeader& scanHeader, std::ofstream& outFile) {
    writeMarker(SOS, outFile);

    uint8_t numComponents = static_cast<uint8_t>(scanHeader.componentSpecifications.size());
    uint16_t length = 6 + 2 * numComponents;

    outFile.write(reinterpret_cast<const char*>(&length), 2);
    outFile.write(reinterpret_cast<const char*>(&numComponents), 1);

    for (auto component : scanHeader.componentSpecifications) {
        writeScanHeaderComponentSpecification(component, outFile);
    }

    outFile.write(reinterpret_cast<const char*>(&scanHeader.spectralSelectionStart), 1);
    outFile.write(reinterpret_cast<const char*>(&scanHeader.spectralSelectionEnd), 1);

    uint8_t successiveApproximation = static_cast<uint8_t>(scanHeader.successiveApproximationHigh << 4 | scanHeader.successiveApproximationLow);
    outFile.write(reinterpret_cast<const char*>(&successiveApproximation), 1);
}

std::vector<ImageProcessing::Jpeg::Mcu> ImageProcessing::Jpeg::Encoder::getMcus(Bmp& bmp) {
    auto points = bmp.getPoints();
    
    int columnCount = static_cast<int>(bmp.info.width + 7) / 8;
    int rowCount = static_cast<int>(bmp.info.height + 7) / 8;
    int height = static_cast<int>(bmp.info.height);
    int width = static_cast<int>(bmp.info.width);
    
    std::vector rows(rowCount, std::vector<ColorBlock>(columnCount));
    
    for (auto& point: *points) {
        int blockRow = (static_cast<int>(point.y) / 8);
        int pixelRow = (static_cast<int>(point.y) % 8);
        int blockCol = (static_cast<int>(point.x) / 8);
        int pixelCol = (static_cast<int>(point.x) % 8);
        rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = point.color.r;
        rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = point.color.g;
        rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = point.color.b;
    }

    // Pad points outside the height of the image with same color as the last color within the column
    if (height % 8 != 0) {
        for (int y = height; y < rowCount * 8; y++) {
            int blockRow = height / 8;
            int pixelRow = y % 8;
            for (int x = 0; x < width; x++) {
                int blockCol = x / 8;
                int pixelCol = x % 8;
                float prevRed = rows[blockRow][blockCol].R[(pixelRow - 1) * 8 + pixelCol];
                float prevGreen = rows[blockRow][blockCol].G[(pixelRow - 1) * 8 + pixelCol];
                float prevBlue = rows[blockRow][blockCol].B[(pixelRow - 1) * 8 + pixelCol];
                rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = prevRed;
                rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = prevGreen;
                rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = prevBlue;
            }
        }
    }

    // Pad points outside the width of the image with same color as the last color within the row
    if (width % 8 != 0) {
        for (int y = 0; y < rowCount * 8; y++) {
            int blockRow = height / 8;
            int pixelRow = y % 8;
            for (int x = width; x < columnCount * 8; x++) {
                int blockCol = x / 8;
                int pixelCol = x % 8;
                float prevRed = rows[blockRow][blockCol].R[pixelRow * 8 + (pixelCol - 1)];
                float prevGreen = rows[blockRow][blockCol].G[pixelRow * 8 + (pixelCol - 1)];
                float prevBlue = rows[blockRow][blockCol].B[pixelRow * 8 + (pixelCol - 1)];
                rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = prevRed;
                rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = prevGreen;
                rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = prevBlue;
            }
        }
    }

    std::vector<Mcu> mcus;
    for (auto& row : rows) {
        for (auto& block : row) {
            mcus.emplace_back(block);
        }
    }
    return mcus;
}

void ImageProcessing::Jpeg::Encoder::writeJpeg(Bmp& bmp) {
    std::ofstream outFile("writeJpeg.jpeg");
    // SOI
    writeMarker(SOI, outFile);
    // Tables/Misc
        // Qtable
        QuantizationTable qTableLuminance = createQuantizationTable(LuminanceTable, 100, true, 0);
        QuantizationTable qTableChrominance = createQuantizationTable(ChrominanceTable, 100, true, 1);
        writeQuantizationTable(qTableLuminance, outFile);
        writeQuantizationTable(qTableChrominance, outFile);
        // Htable
        std::vector<Mcu> mcus = getMcus(bmp);
        std::vector<EncodedBlock> encodedBlocks;
        std::vector<Coefficient> dcCoefficients;
        std::vector<Coefficient> acCoefficients;
        encodeCoefficients(mcus, encodedBlocks, dcCoefficients, acCoefficients);
        writeHuffmanTable(dcCoefficients, 0, 0, outFile);
        writeHuffmanTable(acCoefficients, 1, 0, outFile);
    
        // Restart Interval
        // Comment
        // App data
        // Number of lines (height of image)
    // Frame header
    FrameHeaderComponentSpecification frameCompY(1, 1, 1, 0);
    FrameHeaderComponentSpecification frameCompCb(2, 1, 1, 1);
    FrameHeaderComponentSpecification frameCompCr(3, 1, 1, 1);
    std::vector frameComponents{frameCompY, frameCompCb, frameCompCr};
    FrameHeader frameHeader(SOF0, 8, static_cast<uint16_t>(bmp.info.height), static_cast<uint16_t>(bmp.info.width), frameComponents);
    writeFrameHeader(frameHeader, outFile);
    // Scan
    ScanHeaderComponentSpecification component1(1, 0, 0, 0, 0, 0);
    ScanHeaderComponentSpecification component2(2, 0, 0, 0, 0, 0);
    ScanHeaderComponentSpecification component3(3, 0, 0, 0, 0, 0);
    std::vector scanComponents{component1, component2, component3};
    ScanHeader scanHeader(scanComponents, 0, 63, 0, 0);
    writeScanHeader(scanHeader, outFile);
    // Entropy Data
    
    //EOI
    writeMarker(EOI, outFile);
}
