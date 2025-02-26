#include "Jpeg/JpegImage.h"

#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <ctime>
#include <ranges>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <simde/x86/avx512.h>

#include "BitManipulationUtil.h"

void ImageProcessing::Jpeg::FrameHeaderComponentSpecification::print() const {
    std::cout << std::setw(25) << "Identifier: " << static_cast<int>(identifier) << "\n";
    std::cout << std::setw(25) << "HorizontalSampleFactor: " << static_cast<int>(horizontalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "VerticalSampleFactor: " << static_cast<int>(verticalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "QuantizationTable: " << static_cast<int>(quantizationTableSelector) << "\n";
}

void ImageProcessing::Jpeg::FrameHeader::print() const {
    std::cout << std::setw(15) << "Encoding: " << std::hex << static_cast<int>(encodingProcess) << std::dec << "\n";
    std::cout << std::setw(15) << "Precision: " << static_cast<int>(precision) << "\n";
    std::cout << std::setw(15) << "Height: " << static_cast<int>(height) << "\n";
    std::cout << std::setw(15) << "Width: " << static_cast<int>(width) << "\n";
    std::cout << std::setw(15) << "NumOfChannels: " << static_cast<int>(numOfChannels) << '\n';
    for (const auto& component : componentSpecifications | std::views::values) {
        component.print();
    }
    std::cout << std::dec << "\n";
}

ImageProcessing::Jpeg::FrameHeader::FrameHeader(const uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex) {
    this->encodingProcess = encodingProcess;
    file.seekg(dataStartIndex, std::ios::beg);
    file.read(reinterpret_cast<char*>(&precision), 1);
    file.read(reinterpret_cast<char*>(&height), 2);
    height = SwapBytes(height);
    file.read(reinterpret_cast<char*>(&width), 2);
    width = SwapBytes(width);
    file.read(reinterpret_cast<char*>(&numOfChannels), 1);

    if (numOfChannels > 4) {
        std::cout << "Error: number of channels should be less than 4\n";
        std::cout << "Num of channels: " << numOfChannels << '\n';
    }

    bool zeroBased = false;
    for (uint32_t i = 0; i < numOfChannels; i++) {
        uint8_t identifier;
        uint8_t sampleFactor;
        uint8_t quantizationTable;
        file.read(reinterpret_cast<char*>(&identifier), 1);
        file.read(reinterpret_cast<char*>(&sampleFactor), 1);
        file.read(reinterpret_cast<char*>(&quantizationTable), 1);
        if (identifier == 0) zeroBased = true;
        if (zeroBased) identifier++;
        componentSpecifications[identifier] =  FrameHeaderComponentSpecification(identifier, GetNibble(sampleFactor, 0), GetNibble(sampleFactor, 1), quantizationTable);
    }

    if (zeroBased) {
        std::cout << "Error: Component has an identifier of zero - adjusting all identifiers to be interpreted as one higher\n";
    }

    int sampleSum = 0;
    for (auto& component : componentSpecifications | std::views::values) {
        sampleSum += component.horizontalSamplingFactor * component.verticalSamplingFactor;
    }
    if (sampleSum > 10) {
        std::cout << "Error: Sample sum is greater than 10\n";
    }

    for (auto& component : componentSpecifications | std::views::values) {
        if (component.identifier != 1) {
            if (component.horizontalSamplingFactor != 1) {
                std::cout << "Error: Horizontal sampling factor of " << static_cast<int>(component.identifier) <<
                    " is not supported for chroma component" << "\n";
                std::terminate();
            }
            if (component.verticalSamplingFactor != 1) {
                std::cout << "Error: Vertical sampling factor of " << static_cast<int>(component.identifier) <<
                    " is not supported for chroma component" << "\n";
                std::terminate();
            }
        }
        if (component.identifier == 1) {
            maxHorizontalSample = component.horizontalSamplingFactor;
            maxVerticalSample = component.verticalSamplingFactor;
            if (component.horizontalSamplingFactor == 1 && component.verticalSamplingFactor == 1) {
                luminanceComponentsPerMcu = 1;
            } else if (component.horizontalSamplingFactor == 1 && component.verticalSamplingFactor == 2
                    || component.horizontalSamplingFactor == 2 && component.verticalSamplingFactor == 1) {
                luminanceComponentsPerMcu = 2;
            } else if (component.horizontalSamplingFactor == 2 && component.verticalSamplingFactor == 2) {
                luminanceComponentsPerMcu = 4;
            } else {
                std::cout << "Error: Subsampling not supported\n";
                std::terminate();
            }
        }
    }

    mcuPixelWidth = maxHorizontalSample * 8;
    mcuPixelHeight = maxVerticalSample * 8;
    mcuImageWidth = (width + mcuPixelWidth - 1) / mcuPixelWidth;
    mcuImageHeight = (height + mcuPixelHeight - 1) / mcuPixelHeight;
}

ImageProcessing::Jpeg::QuantizationTable::QuantizationTable(std::ifstream& file, const std::streampos& dataStartIndex, const bool is8Bit) {
    file.seekg(dataStartIndex, std::ios::beg);
    for (unsigned char i : zigZagMap) {
        if (is8Bit) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            table[i] = static_cast<float>(byte);
        } else {
            uint16_t word;
            file.read(reinterpret_cast<char*>(&word), 2);
            word = static_cast<uint16_t>((word >> 8) | (word << 8));
            table[i] = static_cast<float>(word);
        }
    }
    isSet = true;
}

void ImageProcessing::Jpeg::QuantizationTable::print() const {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << std::setw(6) << static_cast<int>(table[i * 8 + j]) << " ";
        }
        std::cout << "\n";
    }
}

ImageProcessing::Jpeg::HuffmanTable::HuffmanTable(std::ifstream& file, const std::streampos& dataStartIndex) {
    file.seekg(dataStartIndex, std::ios::beg);
    std::array<uint8_t, maxEncodingLength> numOfEachEncoding;
    numOfEachEncoding.fill(0);
    for (int i = 0; i < maxEncodingLength; i++) {
        file.read(reinterpret_cast<char*>(&numOfEachEncoding[i]), 1);
    }
    // Generate encodings
    uint16_t code = 0;
    for (int i = 0; i < maxEncodingLength; i++) {
        for (int j = 0; j < numOfEachEncoding[i]; j++) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            encodings.emplace_back(code, static_cast<uint8_t>(i + 1), byte);
            code++;
        }
        code = static_cast<uint16_t>(code << 1);
    }
    generateLookupTable();
    isInitialized = true;
}

void ImageProcessing::Jpeg::HuffmanTable::generateLookupTable() {
    table = std::make_unique<std::array<HuffmanTableEntry, 256>>();
    for (auto& encoding : encodings) {
        if (encoding.bitLength <= 8) {
            uint8_t leftShifted = static_cast<uint8_t>(encoding.encoding << (8 - encoding.bitLength));
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (8 - encoding.bitLength)); i++) {
                uint8_t index = i | leftShifted;
                (*table)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value, nullptr);
            }
        } else {
            uint8_t partOne = static_cast<uint8_t>(encoding.encoding >> (encoding.bitLength - 8) & 0xFF);
            uint8_t partTwo = static_cast<uint8_t>(encoding.encoding << (maxEncodingLength - encoding.bitLength) & 0xFF);
            if ((*table)[partOne].table == nullptr) {
                (*table)[partOne].table = std::make_unique<std::array<HuffmanTableEntry, 256>>();
            }
            auto partTwoTable = (*table)[partOne].table.get();
            for (uint8_t i = 0; i < static_cast<uint8_t>(1 << (16 - encoding.bitLength)); i++) {
                uint8_t index = i | partTwo;
                (*partTwoTable)[index] = HuffmanTableEntry(encoding.bitLength, encoding.value, nullptr);
            }
        }
    }
}

uint8_t ImageProcessing::Jpeg::HuffmanTable::decodeNextValue(BitReader& bitReader) const {
    uint16_t word = bitReader.getWordConstant();
    auto& decoding = (*table)[static_cast<uint8_t>(word >> 8 & 0xFF)];
    if (decoding.table == nullptr) {
        if (decoding.bitLength == 0) {
            std::cerr << "Huffman encoding does not exist!\n";
        }
        bitReader.skipBits(decoding.bitLength);
        return decoding.value;
    }
    auto& decoding2 = (*decoding.table)[static_cast<uint8_t>(word & 0xFF)];
    if (decoding2.table == nullptr && decoding2.bitLength == 0) {
        std::cerr << "Huffman encoding does not exist!\n";
    }
    bitReader.skipBits(decoding2.bitLength);
    return decoding2.value;
}

void ImageProcessing::Jpeg::HuffmanTable::print() const {
    std::cout << std::setw(17) << "Encoding" << std::setw(11) << "Value\n";
    for (auto& encoding : encodings) {
        std::string code;
        for (int i = encoding.bitLength - 1; i >= 0; i--) {
            int bit = encoding.encoding >> i & 1;
            code += bit == 0 ? "0" : "1";
        }
        std::cout << std::setw(17) << code << std::setw(10) << std::hex << static_cast<int>(encoding.value) << std::dec <<  "\n";
    }
}


int ImageProcessing::Jpeg::JpegImage::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = static_cast<int>(bitReader.getNBits(SSSS));
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

int ImageProcessing::Jpeg::JpegImage::decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    int sCategory = huffmanTable.decodeNextValue(bitReader);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

std::pair<int, int> ImageProcessing::Jpeg::JpegImage::decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    uint8_t rs = huffmanTable.decodeNextValue(bitReader);
    uint8_t r = GetNibble(rs, 0);
    uint8_t s = GetNibble(rs, 1);
    return {r, s};
}

std::array<float, 64>* ImageProcessing::Jpeg::JpegImage::decodeComponent(BitReader& bitReader, const ScanHeaderComponentSpecification& scanComp, int (&prevDc)[3]) {
    std::array<float, 64>* result = new std::array<float, 64>();
    result->fill(0);
    // DC Coefficient
    HuffmanTable &dcTable = dcHuffmanTables[scanComp.dcTableIteration][scanComp.dcTableSelector];
    int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[scanComp.componentId - 1];
    (*result)[0] = static_cast<float>(dcCoefficient);
    prevDc[scanComp.componentId - 1] = dcCoefficient;

    // AC Coefficients
    int index = 1;
    while (index < Mcu::dataUnitLength) {
        HuffmanTable &acTable = acHuffmanTables[scanComp.acTableIteration][scanComp.acTableSelector];
        auto [r, s] = decodeAcCoefficient(bitReader, acTable);
        if (r == 0x0 && s == 0x0) {
            break;
        }
        if (r == 0xF && s == 0x0) {
            index += 16;
            continue;
        }
        index += r;
        if (index >= Mcu::dataUnitLength) {
            break;
        }
        int coefficient = decodeSSSS(bitReader, s);
        (*result)[zigZagMap[index]] = static_cast<float>(coefficient);
        index++;
    }
    return result;
}

void ImageProcessing::Jpeg::JpegImage::decodeMcu(Mcu* mcu, ScanHeader& scanHeader, int (&prevDc)[3]) {
    for (auto& component : scanHeader.componentSpecifications) {
        if (component.componentId == 1) {
            for (int i = 0; i < info.luminanceComponentsPerMcu; i++) {
                mcu->Y[i] = (std::unique_ptr<std::array<float, Mcu::dataUnitLength>>(decodeComponent(scanHeader.bitReader, component, prevDc)));
            }
        } else if (component.componentId == 2) {
            mcu->Cb = std::unique_ptr<std::array<float, Mcu::dataUnitLength>>(decodeComponent(scanHeader.bitReader, component, prevDc));
        } else if (component.componentId == 3) {
            mcu->Cr = std::unique_ptr<std::array<float, Mcu::dataUnitLength>>(decodeComponent(scanHeader.bitReader, component, prevDc));
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::skipZeros(BitReader& bitReader, std::array<float, 64>*& component, const int numToSkip, int& index, const int approximationLow) {
    if (numToSkip + index >= Mcu::dataUnitLength) {
        std::cerr << "Invalid number of zeros to skip: " << numToSkip << "\n";
        std::cerr << "Index: " << index << "\n";
    }
    int positive = 1 << approximationLow;
    int negative = -(1 << approximationLow);
    int zerosRead = 0;
    
    if (numToSkip == 0) {
        while (!AreFloatsEqual((*component)[zigZagMap[index]], 0.0f)) {
            int bit = bitReader.getBit();
            if (bit == 1) {
                if ((*component)[zigZagMap[index]] > 0) {
                    (*component)[zigZagMap[index]] += static_cast<float>(positive);
                } else if ((*component)[zigZagMap[index]] < 0) {
                    (*component)[zigZagMap[index]] += static_cast<float>(negative);
                }
            }
            index++;
        }
    }

    while (zerosRead < numToSkip) {
        if (!AreFloatsEqual((*component)[zigZagMap[index]], 0.0f)) {
            int bit = bitReader.getBit();
            if (bit == 1) {
                if ((*component)[zigZagMap[index]] > 0) {
                    (*component)[zigZagMap[index]] += static_cast<float>(positive);
                } else if ((*component)[zigZagMap[index]] < 0) {
                    (*component)[zigZagMap[index]] += static_cast<float>(negative);
                }
            }
            index++;
        } else {
            zerosRead++;
            index++;
        }
    }

    if (numToSkip == 16) {
        return;
    }
    
    while (!AreFloatsEqual((*component)[zigZagMap[index]], 0.0f)) {
        int bit = bitReader.getBit();
        if (bit == 1) {
            if ((*component)[zigZagMap[index]] > 0) {
                (*component)[zigZagMap[index]] += static_cast<float>(positive);
            } else if ((*component)[zigZagMap[index]] < 0) {
                (*component)[zigZagMap[index]] += static_cast<float>(negative);
            }
        }
        index++;
    }
}

void ImageProcessing::Jpeg::JpegImage::decodeProgressiveComponent(std::array<float, 64>* component, ScanHeader& scanHeader,
                                                                  const ScanHeaderComponentSpecification& componentInfo, int (&prevDc)[3], int& numBlocksToSkip) {
    const int approximationHigh = scanHeader.successiveApproximationHigh;
    const int approximationLow = scanHeader.successiveApproximationLow;
    const int spectralStart = scanHeader.spectralSelectionStart;
    const int spectralEnd = scanHeader.spectralSelectionEnd;
    
    if (numBlocksToSkip > 0) {
        if (approximationHigh == 0) {
            // Do nothing
        } else {
            int positive = 1 << approximationLow;
            int negative = -(1 << approximationLow);
            for (int j = spectralStart; j <= spectralEnd; j++) {
                if (!AreFloatsEqual((*component)[zigZagMap[j]], 0.0f)) {
                    int bit = scanHeader.bitReader.getBit();
                    if (bit == 1) {
                        if ((*component)[zigZagMap[j]] > 0) {
                            (*component)[zigZagMap[j]] += static_cast<float>(positive);
                        } else if ((*component)[zigZagMap[j]] < 0) {
                            (*component)[zigZagMap[j]] += static_cast<float>(negative);
                        }
                    }
                }
            }
        }
        numBlocksToSkip--;
        return;
    }

    // DC Coefficient
    if (spectralStart == 0 && spectralEnd == 0) {
        // First scan
        if (approximationHigh == 0) {
            HuffmanTable& dcTable = dcHuffmanTables[componentInfo.dcTableIteration][componentInfo.dcTableSelector];
            int dcCoefficient = (decodeDcCoefficient(scanHeader.bitReader, dcTable) << approximationLow) + prevDc[componentInfo.componentId - 1];
            (*component)[0] = static_cast<float>(dcCoefficient);
            prevDc[componentInfo.componentId - 1] = dcCoefficient;
        }
        // Refinement scan
        else {
            (*component)[0] += static_cast<float>(scanHeader.bitReader.getBit() << approximationLow);
        }
        return;
    }

    // AC Coefficients first scan
    if (approximationHigh == 0) {
        int index = spectralStart;
        while (index <= spectralEnd) {
            HuffmanTable& acTable = acHuffmanTables[componentInfo.acTableIteration][componentInfo.acTableSelector];
            auto [r, s] = decodeAcCoefficient(scanHeader.bitReader, acTable);
            if (r == 0xF && s == 0x0) {
                index += 16;
                continue;
            }
            // End of Band 
            if (s == 0x0) {
                numBlocksToSkip = (1 << r) - 1 + static_cast<int>(scanHeader.bitReader.getNBits(r));
                return;
            }
            index += r;
            if (index > spectralEnd) {
                break;
            }
            int coefficient = decodeSSSS(scanHeader.bitReader, s) << approximationLow;
            (*component)[zigZagMap[index]] = static_cast<float>(coefficient);
            index++;
        }
        return;
    }

    // Ac Coefficients refinement scan
    int index = spectralStart;
    int numToAdd = 1 << approximationLow;
    while (index <= spectralEnd) {
        HuffmanTable& acTable = acHuffmanTables[componentInfo.acTableIteration][componentInfo.acTableSelector];
        auto [r, s] = decodeAcCoefficient(scanHeader.bitReader, acTable);
        if (r == 0xF && s == 0x0) {
            skipZeros(scanHeader.bitReader, component, 16, index, approximationLow);
            continue;
        }
        // End of Band 
        if (s == 0x0) {
            numBlocksToSkip = (1 << r) - 1 + static_cast<int>(scanHeader.bitReader.getNBits(r));
            int positive = 1 << approximationLow;
            int negative = -(1 << approximationLow);
            while (index <= spectralEnd) {
                if (!AreFloatsEqual((*component)[zigZagMap[index]], 0.0f)) {
                    int bit = scanHeader.bitReader.getBit();
                    if (bit == 1) {
                        if ((*component)[zigZagMap[index]] > 0) {
                            (*component)[zigZagMap[index]] += static_cast<float>(positive);
                        } else if ((*component)[zigZagMap[index]] < 0) {
                            (*component)[zigZagMap[index]] += static_cast<float>(negative);
                        }
                    }
                }
                index++;
            }
            break;
        }
        int coeff = scanHeader.bitReader.getBit() == 1 ? numToAdd : numToAdd * -1;
        skipZeros(scanHeader.bitReader, component, r, index, approximationLow);
        (*component)[zigZagMap[index]] += static_cast<float>(coeff);
        index++;
    }
}

void ImageProcessing::Jpeg::ScanHeaderComponentSpecification::print() const {
    std::cout << std::setw(30) << "Component Id: " << static_cast<int>(componentId) << "\n";
    std::cout << std::setw(30) << "DC Table Id: " << static_cast<int>(dcTableSelector) << "\n";
    std::cout << std::setw(30) << "AC Table Id: " << static_cast<int>(acTableSelector) << "\n";
    std::cout << std::setw(30) << "Quantization Table Iteration: " << static_cast<int>(quantizationTableIteration) << "\n";
    std::cout << std::setw(30) << "DC Table Iteration: " << static_cast<int>(dcTableIteration) << "\n";
    std::cout << std::setw(30) << "AC Table Iteration: " << static_cast<int>(acTableIteration) << "\n";
}

void ImageProcessing::Jpeg::ScanHeader::print() const {
    std::cout << std::setw(30) << "Number of Components: " << componentSpecifications.size() << "\n";
    for (auto& component : componentSpecifications) {
        component.print();
    }
    std::cout << std::setw(30) << "Spectral Selection Start: " << static_cast<int>(spectralSelectionStart) << "\n";
    std::cout << std::setw(30) << "Spectral Selection End: " << static_cast<int>(spectralSelectionEnd) << "\n";
    std::cout << std::setw(30) << "Approximation High: " << static_cast<int>(successiveApproximationHigh) << "\n";
    std::cout << std::setw(30) << "Approximation Low: " << static_cast<int>(successiveApproximationLow) << "\n";
    std::cout << std::setw(30) << "Restart Interval: " << static_cast<int>(restartInterval) << "\n";
}

ImageProcessing::Jpeg::ScanHeader::ScanHeader(JpegImage* jpeg, const std::streampos& dataStartIndex) {
    std::ifstream& file = jpeg->file;
    file.seekg(dataStartIndex, std::ios::beg);
    uint8_t numComponents;
    file.read(reinterpret_cast<char*>(&numComponents), 1);
    componentSpecifications.reserve(numComponents);
    bool zeroBased = false;
    for (int i = 0; i < numComponents; i++) {
        uint8_t componentId;
        uint8_t tables;
        file.read(reinterpret_cast<char*>(&componentId), 1);
        file.read(reinterpret_cast<char*>(&tables), 1);
        if (componentId == 0) zeroBased = true;
        if (zeroBased) componentId++;

        // Get the iterations for quantization and huffman tables
        int qTableSelector = jpeg->info.componentSpecifications[componentId].quantizationTableSelector;
        int quantizationTableIteration = static_cast<uint8_t>(jpeg->quantizationTables.size()) - 1;
        while (quantizationTableIteration > 0 && !jpeg->quantizationTables[quantizationTableIteration][qTableSelector].isSet) {
            quantizationTableIteration--;
        }
        
        int dcTableSelector = GetNibble(tables, 0);
        int dcTableIteration = static_cast<int>(jpeg->dcHuffmanTables.size()) - 1;
        while (dcTableIteration > 0 && !jpeg->dcHuffmanTables[dcTableIteration][dcTableSelector].isInitialized) {
            dcTableIteration--;
        }
        int acTableSelector = GetNibble(tables, 1);
        int acIteration = static_cast<int>(jpeg->acHuffmanTables.size()) - 1;
        while (acIteration > 0 && !jpeg->acHuffmanTables[acIteration][acTableSelector].isInitialized) {
            acIteration--;
        }
        
        componentSpecifications.emplace_back(componentId,
            static_cast<uint8_t>(dcTableSelector), static_cast<uint8_t>(acTableSelector),
            static_cast<uint8_t>(quantizationTableIteration), static_cast<uint8_t>(dcTableIteration),
            static_cast<uint8_t>(acIteration));
    }
    file.read(reinterpret_cast<char*>(&spectralSelectionStart), 1);
    file.read(reinterpret_cast<char*>(&spectralSelectionEnd), 1);
    file.read(reinterpret_cast<char*>(&successiveApproximationHigh), 1);
    successiveApproximationLow = GetNibble(successiveApproximationHigh, 1);
    successiveApproximationHigh = GetNibble(successiveApproximationHigh, 0);
    if (spectralSelectionStart > spectralSelectionEnd) {
        std::cerr << "Error: Spectral selection start must be smaller than spectral selection end\n";
    }
    if (spectralSelectionStart > 63) {
        std::cerr << "Error: Spectral selection start must be in the range 0-63\n";
    }
    if (spectralSelectionEnd > 63) {
        std::cerr << "Error: Spectral selection end must be in the range 0-63\n";
    }

    restartInterval = jpeg->currentRestartInterval;
}

bool ImageProcessing::Jpeg::ScanHeader::containsComponentId(const int id) {
    return std::ranges::any_of(componentSpecifications, [id](auto& comp) {
        return comp.componentId == id;
    });
}

ImageProcessing::Jpeg::ScanHeaderComponentSpecification& ImageProcessing::Jpeg::ScanHeader::getComponent(const int id) {
    for (auto& comp : componentSpecifications) {
        if (comp.componentId == id) return comp;
    }
    std::cerr << "Error: No component with id " << id << " found\n";
    return componentSpecifications[0];
}

void ImageProcessing::Jpeg::ColorBlock::print() const {
    std::cout << std::dec;
    std::cout << "Red (R)        | Green (G)        | Blue (B)" << '\n';
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // Print Red
            int luminanceIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(R[luminanceIndex]) << " ";
        }
        std::cout << "| ";


        for (int x = 0; x < 8; x++) {
            // Print Green
            int blueIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(G[blueIndex]) << " ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Blue
            int redIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(B[redIndex]) << " ";
        }
        std::cout << '\n';
    }
}

void ImageProcessing::Jpeg::Mcu::print() const {
    std::cout << std::dec;
    std::cout << "Luminance (Y)(x" << Y.size() << ")        | Blue Chrominance (Cb)        | Red Chrominance (Cr)" << '\n';
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (const auto& i : Y) {
            for (int x = 0; x < 8; x++) {
                // Print Luminance
                int luminanceIndex = y * 8 + x;
                std::cout << std::setw(5) << static_cast<int>((*i)[luminanceIndex]) << " ";
            }
            std::cout << "| ";
        }

        for (int x = 0; x < 8; x++) {
            // Print Blue Chrominance
            int blueIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>((*Cb)[blueIndex]) << " ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Red Chrominance
            int redIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>((*Cr)[redIndex]) << " ";
        }
        std::cout << '\n';
    }
}

int ImageProcessing::Jpeg::Mcu::getColorIndex(const int blockIndex, const int pixelIndex, const int horizontalFactor, const int verticalFactor) {
    int blockRow = blockIndex / horizontalFactor;
    int blockCol = blockIndex % horizontalFactor;
    
    int pixelRow = pixelIndex / 8;
    int pixelCol = pixelIndex % 8;
    
    int chromaRow = (blockRow * 8 + pixelRow) / verticalFactor;
    int chromaCol = (blockCol * 8 + pixelCol) / horizontalFactor;
    
    return chromaRow * 8 + chromaCol;
}

void ImageProcessing::Jpeg::Mcu::generateColorBlocks() {
    simde__m256 one_two_eight = simde_mm256_set1_ps(128);
    simde__m256 red_cr_scaler = simde_mm256_set1_ps(1.402f);
    simde__m256 green_cb_scaler = simde_mm256_set1_ps(-0.344f);
    simde__m256 green_cr_scaler = simde_mm256_set1_ps(-0.714f);
    simde__m256 blue_cb_scaler = simde_mm256_set1_ps(1.772f);
    
    for (int i = 0; i < static_cast<int>(Y.size()); i++) {
        auto& [R, G, B] = colorBlocks[i];
        constexpr int simdIncrement = 8;
        for (int j = 0; j < dataUnitLength; j += simdIncrement) {
            int colorIndices[simdIncrement];
            for (int k = 0; k < simdIncrement; k++) {
                colorIndices[k] = getColorIndex(i, j + k, horizontalSampleSize, verticalSampleSize);
            }

            // Load Y values (assumes contiguous memory layout)
            simde__m256 y = simde_mm256_loadu_ps(&(*Y[i])[j]);

            // Load Cb values using gathered indices
            simde__m256 cb = simde_mm256_set_ps(
                (*Cb)[colorIndices[7]], (*Cb)[colorIndices[6]], (*Cb)[colorIndices[5]], (*Cb)[colorIndices[4]],
                (*Cb)[colorIndices[3]], (*Cb)[colorIndices[2]], (*Cb)[colorIndices[1]], (*Cb)[colorIndices[0]]
            );

            // Load Cr values using gathered indices
            simde__m256 cr = simde_mm256_set_ps(
                (*Cr)[colorIndices[7]], (*Cr)[colorIndices[6]], (*Cr)[colorIndices[5]], (*Cr)[colorIndices[4]],
                (*Cr)[colorIndices[3]], (*Cr)[colorIndices[2]], (*Cr)[colorIndices[1]], (*Cr)[colorIndices[0]]
            );

            
            y = simde_mm256_add_ps(y, one_two_eight);
            
            simde__m256 r = simde_mm256_add_ps(y, simde_mm256_mul_ps(red_cr_scaler, cr));
            simde__m256 g = simde_mm256_add_ps(y, simde_mm256_add_ps(
                                simde_mm256_mul_ps(green_cb_scaler, cb),
                                simde_mm256_mul_ps(green_cr_scaler, cr)));
            simde__m256 b = simde_mm256_add_ps(y, simde_mm256_mul_ps(blue_cb_scaler, cb));
            
            simde__m256 r_clamped = simde_mm256_max_ps(simde_mm256_min_ps(r, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));
            simde__m256 g_clamped = simde_mm256_max_ps(simde_mm256_min_ps(g, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));
            simde__m256 b_clamped = simde_mm256_max_ps(simde_mm256_min_ps(b, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));
            
            simde_mm256_storeu_ps(&R[j], r_clamped);
            simde_mm256_storeu_ps(&G[j], g_clamped);
            simde_mm256_storeu_ps(&B[j], b_clamped);
        }
    }
}

std::tuple<uint8_t, uint8_t, uint8_t> ImageProcessing::Jpeg::Mcu::getColor(int index) {
    int blockRow = index / (horizontalSampleSize * 8) / 8;
    int blockCol = index % (horizontalSampleSize * 8) / 8;

    int pixelRow = index / (horizontalSampleSize * 8) % 8;
    int pixelColumn = index % 8;
    
    int blockIndex = blockRow * horizontalSampleSize + blockCol;
    int pixelIndex = pixelRow * 8 + pixelColumn;
    
    return {static_cast<uint8_t>(colorBlocks[blockIndex].R[pixelIndex]),
        static_cast<uint8_t>(colorBlocks[blockIndex].G[pixelIndex]) ,
        static_cast<uint8_t>(colorBlocks[blockIndex].B[pixelIndex])};
}

// Uses AAN DCT
void ImageProcessing::Jpeg::Mcu::performInverseDCT(std::array<float, dataUnitLength>& array) {
    float results[64];
    // Calculates the rows
    for (int i = 0; i < 8; i++) {
         const float g0 = array[0 * 8 + i] * s0;
         const float g1 = array[4 * 8 + i] * s4;
         const float g2 = array[2 * 8 + i] * s2;
         const float g3 = array[6 * 8 + i] * s6;
         const float g4 = array[5 * 8 + i] * s5;
         const float g5 = array[1 * 8 + i] * s1;
         const float g6 = array[7 * 8 + i] * s7;
         const float g7 = array[3 * 8 + i] * s3;

         const float f0 = g0;
         const float f1 = g1;
         const float f2 = g2;
         const float f3 = g3;
         const float f4 = g4 - g7;
         const float f5 = g5 + g6;
         const float f6 = g5 - g6;
         const float f7 = g4 + g7;

         const float e0 = f0;
         const float e1 = f1;
         const float e2 = f2 - f3;
         const float e3 = f2 + f3;
         const float e4 = f4;
         const float e5 = f5 - f7;
         const float e6 = f6;
         const float e7 = f5 + f7;
         const float e8 = f4 + f6;

         const float d0 = e0;
         const float d1 = e1;
         const float d2 = e2 * m1;
         const float d3 = e3;
         const float d4 = e4 * m2;
         const float d5 = e5 * m3;
         const float d6 = e6 * m4;
         const float d7 = e7;
         const float d8 = e8 * m5;

         const float c0 = d0 + d1;
         const float c1 = d0 - d1;
         const float c2 = d2 - d3;
         const float c3 = d3;
         const float c4 = d4 + d8;
         const float c5 = d5 + d7;
         const float c6 = d6 - d8;
         const float c7 = d7;
         const float c8 = c5 - c6;

         const float b0 = c0 + c3;
         const float b1 = c1 + c2;
         const float b2 = c1 - c2;
         const float b3 = c0 - c3;
         const float b4 = c4 - c8;
         const float b5 = c8;
         const float b6 = c6 - c7;
         const float b7 = c7;

         results[0 * 8 + i] = b0 + b7;
         results[1 * 8 + i] = b1 + b6;
         results[2 * 8 + i] = b2 + b5;
         results[3 * 8 + i] = b3 + b4;
         results[4 * 8 + i] = b3 - b4;
         results[5 * 8 + i] = b2 - b5;
         results[6 * 8 + i] = b1 - b6;
         results[7 * 8 + i] = b0 - b7;
     }
    // Calculates the columns
    for (int i = 0; i < 8; i++) {
        const float g0 = results[i * 8 + 0] * s0;
        const float g1 = results[i * 8 + 4] * s4;
        const float g2 = results[i * 8 + 2] * s2;
        const float g3 = results[i * 8 + 6] * s6;
        const float g4 = results[i * 8 + 5] * s5;
        const float g5 = results[i * 8 + 1] * s1;
        const float g6 = results[i * 8 + 7] * s7;
        const float g7 = results[i * 8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        array[i * 8 + 0] = b0 + b7;
        array[i * 8 + 1] = b1 + b6;
        array[i * 8 + 2] = b2 + b5;
        array[i * 8 + 3] = b3 + b4;
        array[i * 8 + 4] = b3 - b4;
        array[i * 8 + 5] = b2 - b5;
        array[i * 8 + 6] = b1 - b6;
        array[i * 8 + 7] = b0 - b7;
    }
}

void ImageProcessing::Jpeg::Mcu::performInverseDCT() {
    if (postDctMode == false) {
        std::cout << "Warning: Has not been transformed via DCT, cannot perform inverse DCT";
        return;
    }
    for (auto& y : Y) {
        performInverseDCT(*y);
    }
    performInverseDCT(*Cb);
    performInverseDCT(*Cr);
    postDctMode = false;
}

void ImageProcessing::Jpeg::Mcu::dequantize(std::array<float, dataUnitLength>& array, const QuantizationTable& quantizationTable) {
    for (size_t i = 0; i < dataUnitLength; i += 16) {
        simde__m512 arrayVec = simde_mm512_loadu_ps(&array[i]);
        simde__m512 quantTableVec = simde_mm512_loadu_ps(&quantizationTable.table[i]);
        simde__m512 resultVec = simde_mm512_mul_ps(arrayVec, quantTableVec);
        simde_mm512_storeu_ps(&array[i], resultVec);
    }
}

void ImageProcessing::Jpeg::Mcu::dequantize(JpegImage* jpeg, const ScanHeaderComponentSpecification& scanComp) {
    if (scanComp.componentId == 1) {
        for (auto& y : Y) {
            dequantize(*y, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[1].quantizationTableSelector]);
        }
    } else if (scanComp.componentId == 2){
        dequantize(*Cb, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[2].quantizationTableSelector]);
    } else if (scanComp.componentId == 3){
        dequantize(*Cr, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[3].quantizationTableSelector]);
    }
}

ImageProcessing::Jpeg::Mcu::Mcu() {
    Y.push_back(std::make_shared<std::array<float, dataUnitLength>>());
    Y[0]->fill(0);
    Cb = std::make_shared<std::array<float, dataUnitLength>>();
    Cb->fill(0);
    Cr = std::make_shared<std::array<float, dataUnitLength>>();
    Cr->fill(0);
    colorBlocks = std::vector(1, ColorBlock());
}

ImageProcessing::Jpeg::Mcu::Mcu(const int luminanceComponents, const int horizontalSampleSize, const int verticalSampleSize) {
    for (int i : std::ranges::views::iota(0, luminanceComponents)) {
        Y.push_back(std::make_shared<std::array<float, dataUnitLength>>());
        Y[i]->fill(0);
    }
    colorBlocks = std::vector(luminanceComponents, ColorBlock());
    this->horizontalSampleSize = horizontalSampleSize;
    this->verticalSampleSize = verticalSampleSize;
    Cb = std::make_shared<std::array<float, dataUnitLength>>();
    Cb->fill(0);
    Cr = std::make_shared<std::array<float, dataUnitLength>>();
    Cr->fill(0);
}

uint16_t ImageProcessing::Jpeg::JpegImage::readLengthBytes() {
    uint16_t length;
    file.read(reinterpret_cast<char*>(&length), 2);
    length = SwapBytes(length);
    length -= 2; // Account for the bytes that store the length
    return length;
}

void ImageProcessing::Jpeg::JpegImage::readFrameHeader(const uint8_t frameMarker) {
    file.seekg(2, std::ios::cur); // Skip past length bytes
    info = FrameHeader(frameMarker, file, file.tellg());
    mcus.reserve(static_cast<unsigned int>(info.mcuImageHeight * info.mcuImageWidth));
    for (int i = 0; i < info.mcuImageHeight * info.mcuImageWidth; i++) {
        mcus.emplace_back(std::make_shared<Mcu>(info.luminanceComponentsPerMcu, info.maxHorizontalSample, info.maxVerticalSample));
    }
}

void ImageProcessing::Jpeg::JpegImage::readQuantizationTables() {
    uint16_t length = readLengthBytes();
    
    while (length > 0) {
        uint8_t tableId;
        file.read(reinterpret_cast<char*>(&tableId), 1);
        length--;
        tableId = GetNibble(tableId, 1);
        
        uint8_t precision = GetNibble(tableId, 0);
        bool is8Bit = precision == 0;
    
        std::streampos dataStartIndex = file.tellg();

        QuantizationTable quantizationTable(file, dataStartIndex, is8Bit);
        int iteration = 0;
        while (quantizationTables[iteration][tableId].isSet) {
            iteration++;
            if (static_cast<int>(quantizationTables.size()) <= iteration) {
                quantizationTables.emplace_back();
            }
        }
        quantizationTables[iteration][tableId] = quantizationTable;
        length -= (static_cast<uint16_t>(file.tellg()) - static_cast<uint16_t>(dataStartIndex));
    }
}

void ImageProcessing::Jpeg::JpegImage::readHuffmanTables() {
    uint16_t length = readLengthBytes();

    while (length > 0) {
        uint8_t tableId;
        file.read(reinterpret_cast<char*>(&tableId), 1);
        length--;
        uint8_t tableClass = GetNibble(tableId, 0);
        tableId = GetNibble(tableId, 1);
        std::streampos dataStartIndex = file.tellg();

        auto& tables = tableClass == 0 ? dcHuffmanTables : acHuffmanTables;
        int iteration = 0;
        while (tables[iteration][tableId].isInitialized) {
            iteration++;
            if (static_cast<int>(tables.size()) <= iteration) {
                tables.emplace_back();
            }
        }
        tables[iteration][tableId] = HuffmanTable(file, dataStartIndex);
        length -= (static_cast<uint16_t>(file.tellg()) - static_cast<uint16_t>(dataStartIndex));
    }
}

void ImageProcessing::Jpeg::JpegImage::readDefineNumberOfLines() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&info.height), 2);
    info.height = SwapBytes(info.height);
}

void ImageProcessing::Jpeg::JpegImage::readDefineRestartIntervals() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&currentRestartInterval), 2);
    currentRestartInterval = SwapBytes(currentRestartInterval);
}

void ImageProcessing::Jpeg::JpegImage::readComments() {
    uint16_t length = readLengthBytes();
    comment = std::string(length + 1, '\0');
    file.read(comment.data(), length);
}

void ImageProcessing::Jpeg::JpegImage::processQuantizationQueue(const std::vector<ScanHeaderComponentSpecification>& scanComps) {
    static double totalTime = 0.0f;
    while (true) {
        std::unique_lock quantizationLock(quantizationQueue.mutex);
        quantizationQueue.condition.wait(quantizationLock, [&] {
            return !quantizationQueue.queue.empty() || quantizationQueue.allProductsAdded;
        });
        if (!quantizationQueue.queue.empty()) {
            auto mcu = quantizationQueue.queue.front();
            quantizationQueue.queue.pop();
            quantizationLock.unlock();
            clock_t begin = clock();
            for (const auto& scanComp : scanComps) {
                mcu->dequantize(this, scanComp);
            }
            clock_t end = clock();
            totalTime += (end - begin);
            {
                std::unique_lock lock(idctQueue.mutex);
                idctQueue.queue.push(mcu);
                idctQueue.condition.notify_one();
            }
        }
        if (quantizationQueue.allProductsAdded && quantizationQueue.queue.empty()) {
            std::cout << "Total time on quantization: " << (totalTime) / CLOCKS_PER_SEC << " seconds\n";
            std::unique_lock lock(idctQueue.mutex);
            idctQueue.allProductsAdded = true;
            idctQueue.condition.notify_all();
            break;
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::processIdctQueue() {
    static double totalTime = 0.0f;
    while (true) {
        std::unique_lock idctLock(idctQueue.mutex);
        idctQueue.condition.wait(idctLock, [&] {
            return !idctQueue.queue.empty() || idctQueue.allProductsAdded;
        });
        if (!idctQueue.queue.empty()) {
            auto mcu = idctQueue.queue.front();
            idctQueue.queue.pop();
            idctLock.unlock();
            clock_t begin = clock();
            mcu->performInverseDCT();
            clock_t end = clock();
            totalTime += (end - begin);
            {
                std::unique_lock lock(colorConversionQueue.mutex);
                colorConversionQueue.queue.push(mcu);
                colorConversionQueue.condition.notify_one();
            }
        }
        if (idctQueue.allProductsAdded && idctQueue.queue.empty()) {
            std::cout << "Total time on idct: " << (totalTime) / CLOCKS_PER_SEC << " seconds\n";
            std::unique_lock lock(colorConversionQueue.mutex);
            colorConversionQueue.allProductsAdded = true;
            colorConversionQueue.condition.notify_all();
            break;
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::processColorConversionQueue() {
    static double totalTime = 0.0f;
    while (true) {
        std::unique_lock colorLock(colorConversionQueue.mutex);
        colorConversionQueue.condition.wait(colorLock, [&] {
            return !colorConversionQueue.queue.empty() || colorConversionQueue.allProductsAdded;
        });
        if (!colorConversionQueue.queue.empty()) {
            auto mcu = colorConversionQueue.queue.front();
            colorConversionQueue.queue.pop();
            colorLock.unlock();
            clock_t begin = clock();
            mcu->generateColorBlocks();
            clock_t end = clock();
            totalTime += (end - begin);
        }
        if (colorConversionQueue.allProductsAdded && colorConversionQueue.queue.empty()) {
            break;
        }
    }
    std::cout << "Total time on color: " << (totalTime) / CLOCKS_PER_SEC << " seconds\n";
}

std::shared_ptr<ImageProcessing::Jpeg::ScanHeader> ImageProcessing::Jpeg::JpegImage::readScanHeader() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    std::shared_ptr<ScanHeader> scanHeader = std::make_shared<ScanHeader>(this, file.tellg());
    scanHeaders.emplace_back(scanHeader);
    return scanHeader;
}

void ImageProcessing::Jpeg::JpegImage::readBaselineStartOfScan() {
    std::shared_ptr<ScanHeader> scanHeader = readScanHeader();
    // Read entropy compressed bit stream
    uint8_t byte;
    while (file.read(reinterpret_cast<char*>(&byte), 1)) {
        uint8_t previousByte = byte;
        if (previousByte == 0xFF) {
            if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                break;
            }
            while (byte == 0xFF) {
                file.get();
                if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                    break;
                }
            }
            if (byte >= RST0 && byte <= RST7) {
                continue;
            } else if (byte == 0x00) {
                scanHeader->bitReader.addByte(previousByte);
            } else if (byte == EOI) {
                file.seekg(-2, std::ios::cur);
                break; 
            } else {
                std::cerr << "Error: Unknown marker encountered in SOS data: " << std::hex << static_cast<int>(byte) << std::dec << "\n";
            }
        } else {
            scanHeader->bitReader.addByte(previousByte);
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::pollCurrentScan(const int mcuIndex, const int scanNumber) {
    // Step 1: Lock to safely access scanIndices
    std::shared_ptr<AtomicCondition> scanData;
    {
        std::unique_lock scanLock(scanIndicesMutex);
        scanData = scanIndices[scanNumber];
    }

    // Step 2: Lock the specific scanData mutex and wait
    std::unique_lock lock(scanData->mutex);
    scanData->condition.wait(lock, [&] {
        return scanData->value >= mcuIndex;
    });
};

void ImageProcessing::Jpeg::JpegImage::pushToNextScan(const int mcuIndex, const int scanNumber) {
    std::shared_ptr<AtomicCondition> nextScanData;
    {
        std::unique_lock scanLock(scanIndicesMutex);
        nextScanData = scanIndices[scanNumber + 1];
    }
    std::unique_lock lock(nextScanData->mutex);
    nextScanData->value = mcuIndex;
    nextScanData->condition.notify_one();
}

void ImageProcessing::Jpeg::JpegImage::processProgressiveStartOfScan(const std::shared_ptr<ScanHeader>& scan, const int scanNumber) {
    const int finalScanNumber = static_cast<int>(scanHeaders.size() - 1);
    int prevDc[3] = {};
    int componentsToSkip = 0;
    
    // One component per scan
    if (scan->componentSpecifications.size() == 1) {
        auto& scanSpec = scan->componentSpecifications[0];
        auto& frameSpec = info.componentSpecifications[scan->componentSpecifications[0].componentId];

        // Only luminance component
        if (scanSpec.componentId == 1) {
            int blocksProcessed = 0;
            const int maxBlockY = (info.height + 7) / 8;
            const int maxBlockX = (info.width + 7) / 8;
            for (int blockY = 0; blockY < maxBlockY; blockY++) {
                for (int blockX = 0; blockX < maxBlockX; blockX++) {
                    // Get mcuIndex based on the current index
                    const int luminanceRow = blockY % frameSpec.verticalSamplingFactor;
                    const int luminanceColumn = blockX % frameSpec.horizontalSamplingFactor;
                    const int mcuRow = blockY / frameSpec.verticalSamplingFactor;
                    const int mcuColumn = blockX / frameSpec.horizontalSamplingFactor;
                    const int mcuIndex = mcuRow * info.mcuImageWidth + mcuColumn;
                    
                    if (scan->restartInterval != 0 && blocksProcessed % scan->restartInterval == 0) {
                        scan->bitReader.alignToByte();
                        prevDc[0] = 0;
                        prevDc[1] = 0;
                        prevDc[2] = 0;
                        componentsToSkip = 0;
                    }

                    pollCurrentScan(mcuIndex, scanNumber);
                    
                    const int luminanceIndex = luminanceColumn + luminanceRow * info.maxHorizontalSample;
                    decodeProgressiveComponent(mcus[mcuIndex]->Y[luminanceIndex].get(), *scan, scanSpec, prevDc, componentsToSkip);
                    
                    // Push the mcu to the next queue
                    bool pushToQueue = (luminanceIndex == info.luminanceComponentsPerMcu - 1) ||
                        (blockX == maxBlockX - 1 && luminanceRow == info.maxVerticalSample - 1) ||
                        (blockY == maxBlockY - 1 && luminanceColumn == info.maxHorizontalSample - 1) ||
                        (blockX == maxBlockX - 1 && blockY == maxBlockY - 1);

                    if (pushToQueue) {
                        if (scanNumber >= finalScanNumber) {
                            std::unique_lock queueLock(quantizationQueue.mutex);
                            quantizationQueue.queue.push(mcus[mcuIndex]);
                            quantizationQueue.condition.notify_one();
                        } else {
                            pushToNextScan(mcuIndex, scanNumber);
                        }
                    }
                    blocksProcessed++;
                }
            }
            if (scanNumber == finalScanNumber) {
                std::unique_lock queueLock(quantizationQueue.mutex);
                quantizationQueue.allProductsAdded = true;
            }
        }
        // Only chroma component
        else {
            for (int mcuIndex = 0; mcuIndex < info.mcuImageWidth * info.mcuImageHeight; mcuIndex++) {
                if (scan->restartInterval != 0 && mcuIndex % scan->restartInterval == 0) {
                    scan->bitReader.alignToByte();
                    prevDc[0] = 0;
                    prevDc[1] = 0;
                    prevDc[2] = 0;
                    componentsToSkip = 0;
                }

                pollCurrentScan(mcuIndex, scanNumber);
                
                if (scanSpec.componentId == 2) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cb.get(), *scan, scanSpec, prevDc, componentsToSkip);
                } else if (scanSpec.componentId == 3) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cr.get(), *scan, scanSpec, prevDc, componentsToSkip);
                }

                // Push mcu to next queue
                if (scanNumber >= finalScanNumber) {
                    std::unique_lock queueLock(quantizationQueue.mutex);
                    quantizationQueue.queue.push(mcus[mcuIndex]);
                    quantizationQueue.condition.notify_one();
                } else {
                    pushToNextScan(mcuIndex, scanNumber);
                }
            }
            if (scanNumber == finalScanNumber) {
                std::unique_lock queueLock(quantizationQueue.mutex);
                quantizationQueue.allProductsAdded = true; 
            }
        }
    }
    // Multiple components per scan (DC Coefficients only)
    else {
        for (int mcuIndex = 0; mcuIndex < info.mcuImageHeight * info.mcuImageWidth; mcuIndex++) {
            if (scan->restartInterval != 0 && mcuIndex % scan->restartInterval == 0) {
                scan->bitReader.alignToByte();
                prevDc[0] = 0;
                prevDc[1] = 0;
                prevDc[2] = 0;
                componentsToSkip = 0;
            }

            pollCurrentScan(mcuIndex, scanNumber);
            
            for (auto& componentInfo : scan->componentSpecifications) {
                std::shared_ptr<std::array<float, 64>> component;
                if (componentInfo.componentId == 1) {
                    for (int i = 0; i < info.luminanceComponentsPerMcu; i++) {
                        decodeProgressiveComponent(mcus[mcuIndex]->Y[i].get(), *scan, componentInfo, prevDc, componentsToSkip);
                    }
                } else if (componentInfo.componentId == 2) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cb.get(), *scan, componentInfo, prevDc, componentsToSkip);
                } else if (componentInfo.componentId == 3) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cr.get(), *scan, componentInfo, prevDc, componentsToSkip);
                }
            }
            // Push mcu to next queue
            if (scanNumber >= finalScanNumber) {
                std::unique_lock queueLock(quantizationQueue.mutex);
                quantizationQueue.queue.push(mcus[mcuIndex]);
                quantizationQueue.condition.notify_one();
            } else {
                pushToNextScan(mcuIndex, scanNumber);
            }
        }
        if (scanNumber == finalScanNumber) {
            std::unique_lock queueLock(quantizationQueue.mutex);
            quantizationQueue.allProductsAdded = true;
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::readProgressiveStartOfScan() {
    if (scanIndices.size() == 1) {
        std::unique_lock scanLock(scanIndicesMutex);
        scanIndices[0]->value = info.mcuImageHeight * info.mcuImageWidth - 1;
    }
    
    std::shared_ptr<ScanHeader> scanHeader = readScanHeader();
    if (scanHeader->spectralSelectionStart == 0 && scanHeader->spectralSelectionEnd != 0) {
        std::cerr << "Error: Spectral selection cannot encode DC and AC coefficients in the same scan\n";
    }
    // Read entropy compressed bit stream

    uint8_t byte;
    while (file.read(reinterpret_cast<char*>(&byte), 1)) {
        if (uint8_t previousByte = byte; previousByte == 0xFF) {
            if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                break;
            }
            while (byte == 0xFF) {
                file.get();
                if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                    break;
                }
            }
            if (byte >= RST0 && byte <= RST7) {
                continue;
            } else if (byte == 0x00) {
                scanHeader->bitReader.addByte(previousByte);
            } else if (byte == EOI || byte == DHT || byte == DQT || byte == SOS || byte == DRI) {
                file.seekg(-2, std::ios::cur);
                break;
            } else {
                std::cout << "Error: Unknown marker encountered in SOS data: " << std::hex << static_cast<int>(byte) << std::dec << "\n";
            }
        } else {
            scanHeader->bitReader.addByte(previousByte);
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::readStartOfScan() {
    scanIndices.emplace_back(std::make_shared<AtomicCondition>());
    if (info.encodingProcess == SOF0) {
        readBaselineStartOfScan();
    } else if (info.encodingProcess == SOF2) {
        readProgressiveStartOfScan();
    }
}

void ImageProcessing::Jpeg::JpegImage::decodeBaseLine() {
    auto& scanHeader = scanHeaders[0];
    int prevDc[3] = {};
    
    std::thread quantizationThread = std::thread([&] {processQuantizationQueue(scanHeader->componentSpecifications);});
    std::thread idctThread = std::thread([&] {processIdctQueue();});
    std::thread colorConversionThread = std::thread([&] {processColorConversionQueue();});
    
    for (int i = 0; i < info.mcuImageHeight * info.mcuImageWidth; i++) {
        if (scanHeader->restartInterval != 0 && i % scanHeader->restartInterval == 0) {
            scanHeader->bitReader.alignToByte();
            prevDc[0] = 0;
            prevDc[1] = 0;
            prevDc[2] = 0;
        }
        auto& mcu = mcus[i];
        decodeMcu(mcu.get(), *scanHeader, prevDc);
        std::unique_lock lock(quantizationQueue.mutex);
        quantizationQueue.queue.push(mcu);
        quantizationQueue.condition.notify_one();
    }
    std::unique_lock lock(quantizationQueue.mutex);
    quantizationQueue.allProductsAdded = true;
    lock.unlock();
    quantizationThread.join();
    idctThread.join();
    colorConversionThread.join();
}

void ImageProcessing::Jpeg::JpegImage::decodeProgressive() {
    std::vector<std::shared_ptr<std::thread>> threads;
    for (int i = 0; i < static_cast<int>(scanHeaders.size()); i++) {
        int scanNumber = i;
        threads.emplace_back(std::make_shared<std::thread>([this, scanNumber] {
            processProgressiveStartOfScan(scanHeaders[scanNumber], scanNumber);
        }));
    }

    std::vector<ScanHeaderComponentSpecification> scanHeaderComponents;
    for (int i = 1; i <= static_cast<int>(info.componentSpecifications.size()); i++) {
        for (int j = static_cast<int>(scanHeaders.size()) - 1; j >= 0; j--) {
            if (scanHeaders[j]->containsComponentId(i)) {
                scanHeaderComponents.push_back(scanHeaders[j]->getComponent(i));
                break;
            }
        }
    }
                    
    std::thread quantizationThread = std::thread([&] {processQuantizationQueue(scanHeaderComponents);});
    std::thread idctThread = std::thread([&] {processIdctQueue();});
    std::thread colorConversionThread = std::thread([&] {processColorConversionQueue();});
                    
    for (auto &thread : threads) {
        thread->join();
    }
    
    quantizationThread.join();
    idctThread.join();
    colorConversionThread.join();
}

void ImageProcessing::Jpeg::JpegImage::decode() {
    if (info.encodingProcess == SOF0) {
        decodeBaseLine();
    } else if (info.encodingProcess == SOF2) {
        decodeProgressive();
    }
}

void ImageProcessing::Jpeg::JpegImage::readFile() {
    uint8_t byte;
    while (!file.eof()) {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (byte == 0xFF) {
            uint8_t marker;
            file.read(reinterpret_cast<char*>(&marker), 1);
            if (marker == 0x00) continue;
            if (marker == DHT ) {
                readHuffmanTables();
            } else if (marker == DQT) {
                readQuantizationTables();
            } else if (marker >= SOF0 && marker <= SOF15 
                && !(marker == DHT || marker == JPG || marker == DAC)) {
                readFrameHeader(marker);
            } else if (marker == DNL) {
                readDefineNumberOfLines();
            } else if (marker == DRI) {
                readDefineRestartIntervals();
            } else if (marker == COM) {
                readComments();
            } else if (marker == SOS) {
                readStartOfScan();
            } else if (marker == EOI) {
                decode();
                break;
            }
        }
    }
}

void ImageProcessing::Jpeg::JpegImage::printInfo() const {
    info.print();
    
    std::cout << "\nScan Information:\n";
    for (int i = 0; i < static_cast<int>(scanHeaders.size()); i++) {
        std::cout << "Scan #" << i << ":\n";
        scanHeaders[i]->print();
    }

    std::cout << "\nQuantization Tables:\n";
    for (int iteration = 0; iteration < static_cast<int>(quantizationTables.size()); iteration++) {
        for (size_t i = 0; i < quantizationTables.size(); i++) {
            if (quantizationTables[iteration][i].isSet) {
                std::cout << "Quantization Iteration " << iteration << ", Table #" << i << "\n";
                quantizationTables[iteration][i].print();
            }
        }
    }

    std::cout << "\nDC Huffman Tables:\n";
    for (int iteration = 0; iteration < static_cast<int>(dcHuffmanTables.size()); iteration++) {
        for (size_t i = 0; i < dcHuffmanTables.size(); i++) {
            if (dcHuffmanTables[iteration][i].isInitialized) {
                std::cout << "DC Huffman Iteration " << iteration << ", Table #" << i << "\n";
                dcHuffmanTables[iteration][i].print();
            }
        }
    }

    std::cout << "\nAC Huffman Tables:\n";
    for (int iteration = 0; iteration < static_cast<int>(acHuffmanTables.size()); iteration++) {
        for (size_t i = 0; i < acHuffmanTables.size(); i++) {
            if (acHuffmanTables[iteration][i].isInitialized) {
                std::cout << "AC Huffman Iteration " << iteration << ", Table #" << i << "\n";
                acHuffmanTables[iteration][i].print();
            }
        }
    }

    if (!comment.empty()) {
        std::cout << "Comment: " << comment << "\n";
    }
}

// TODO: Make progressive jpegs progressively display
ImageProcessing::Jpeg::JpegImage::JpegImage(const std::string& path) {
    file = std::ifstream(path, std::ifstream::binary);
    readFile();
}