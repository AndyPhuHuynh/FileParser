#include "Jpg.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numbers>
#include <ctime>
#include <ranges>
#include <vector>
#include <Gl/glew.h>
#include <GLFW/glfw3.h>

#include "BitManipulationUtil.h"
#include "Bmp.h"
#include "ShaderUtil.h"

void FrameHeaderComponentSpecification::print() const {
    std::cout << std::setw(25) << "Identifier: " << static_cast<int>(identifier) << "\n";
    std::cout << std::setw(25) << "HorizontalSampleFactor: " << static_cast<int>(horizontalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "VerticalSampleFactor: " << static_cast<int>(verticalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "QuantizationTable: " << static_cast<int>(quantizationTableSelector) << "\n";
}

void FrameHeader::print() const {
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

FrameHeader::FrameHeader(const uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex) {
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
                subsampling = None;
            } else if (component.horizontalSamplingFactor == 1 && component.verticalSamplingFactor == 2) {
                subsampling = VerticalHalved;
            } else if (component.horizontalSamplingFactor == 2 && component.verticalSamplingFactor == 1) {
                subsampling = HorizontalHalved;
            } else if (component.horizontalSamplingFactor == 2 && component.verticalSamplingFactor == 2) {
                subsampling = Quartered;
            } else {
                std::cout << "Error: Subsampling not supported\n";
                std::terminate();
            }
        }
    }
    
    if (subsampling == HorizontalHalved || subsampling == VerticalHalved) {
        luminanceComponentsPerMcu = 2;
    } else if (subsampling == Quartered) {
        luminanceComponentsPerMcu = 4;
    } else {
        luminanceComponentsPerMcu = 1;
    }

    mcuPixelWidth = maxHorizontalSample * 8;
    mcuPixelHeight = maxVerticalSample * 8;
    mcuImageWidth = (width + mcuPixelWidth - 1) / mcuPixelWidth;
    mcuImageHeight = (height + mcuPixelHeight - 1) / mcuPixelHeight;
}

QuantizationTable::QuantizationTable(std::ifstream& file, const std::streampos& dataStartIndex, const bool is8Bit)
    : is8Bit(is8Bit) {
    file.seekg(dataStartIndex, std::ios::beg);
    for (unsigned char i : zigZagMap) {
        if (is8Bit) {
            file.read(reinterpret_cast<char*>(&table8[i]), 1);
        } else {
            file.read(reinterpret_cast<char*>(&table16[i]), 2);
        }
    }
}

void QuantizationTable::printTable() const {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (is8Bit) {
                std::cout << std::setw(3) << static_cast<int>(table8[i * 8 + j]) << " ";
            } else {
                std::cout << std::setw(6) << static_cast<int>(table16[i * 8 + j]) << " ";
            }
        }
        std::cout << "\n";
    }
}

bool HuffmanNode::isLeaf() const {
    return left == nullptr && right == nullptr;
}

uint8_t HuffmanNode::getValue() const {
    return value;
}

void HuffmanNode::setValue(const uint8_t newValue) {
    this->value = newValue;
    this->isNodeSet = true;
}

bool HuffmanNode::isSet() const {
    return isNodeSet;
}

void HuffmanTree::addEncoding(const HuffmanEncoding& encoding) {
    HuffmanNode *currentNode = root.get();
    for (int i = encoding.bitLength - 1; i >= 0; i--) {
        if (currentNode->isSet()) {
            std::cout << "Error: Attempting to add a code which contains another code as a prefix\n";
        }
        uint8_t bit = GetBitFromRight(encoding.encoding, i);
        if (bit == 0) {
            if (!currentNode->left) {
                currentNode->left = std::make_unique<HuffmanNode>();
            }
            currentNode = currentNode->left.get();
        } else if (bit == 1) {
            if (!currentNode->right) {
                currentNode->right = std::make_unique<HuffmanNode>();
            }
            currentNode = currentNode->right.get();
        }
    }

    if (currentNode->isSet()) {
        std::cout << "Error: Attempting to add encoding that already exists\n";
    }

    if (!currentNode->isLeaf()) {
        std::cout << "Error: Attempting to add encoding to internal node\n";
    }
    currentNode->setValue(encoding.value);
}

void HuffmanTree::printTree() const {
    printTree(*root, "");
}

void HuffmanTree::printTree(const HuffmanNode& node, const std::string& currentCode) {
    if (node.left != nullptr) {
        printTree(*node.left, currentCode + "0");   
    }
    if (node.right != nullptr) {
        printTree(*node.right, currentCode + "1");
    }

    if (node.isSet()) {
        std::cout << std::left << std::setw(10) << "Encoding: "
              << std::setw(17) << currentCode
              << std::setw(7) << "Value: "
              << std::hex << std::setw(10) << static_cast<int>(node.getValue()) << std::dec << "\n";
    }
}

uint8_t HuffmanTree::decodeNextValue(BitReader& bitReader) {
    HuffmanNode *current = root.get();
    if (root == nullptr) {
        std::cout << "Error: Invalid root for huffman tree" << "\n";
        return 0;
    }
    std::string code;
    while (!current->isLeaf()) {
        uint8_t bit = bitReader.getBit();
        if (bit == 0) {
            current = current->left.get();
            code += "0";
        } else {
            current = current->right.get();
            code += "1";
        }
        if (current == nullptr) {
            std::cout << "Error: Code not contained in huffman tree:" << code << "\n";
            return 0;
        }
    }
    return current->getValue();
}

HuffmanTree::HuffmanTree() {
    root = std::make_unique<HuffmanNode>();
}

HuffmanTree::HuffmanTree(const std::vector<HuffmanEncoding>& encodings) {
    root = std::make_unique<HuffmanNode>();
    for (auto& encoding : encodings) {
        addEncoding(encoding);
    }
}

HuffmanTable::HuffmanTable(std::ifstream& file, const std::streampos& dataStartIndex) {
    file.seekg(dataStartIndex, std::ios::beg);
    std::array<uint8_t, maxEncodingLength> numOfEachEncoding;
    numOfEachEncoding.fill(0);
    for (int i = 0; i < maxEncodingLength; i++) {
        file.read(reinterpret_cast<char*>(&numOfEachEncoding[i]), 1);
    }
    // Generate encodings
    int code = 0;
    for (int i = 0; i < maxEncodingLength; i++) {
        for (int j = 0; j < numOfEachEncoding[i]; j++) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            encodings.emplace_back(code, i + 1, byte);
            code++;
        }
        code = code << 1;
    }
    tree = HuffmanTree(encodings);
    generateLookupTable();
}

void HuffmanTable::generateLookupTable() {
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

uint8_t HuffmanTable::decodeNextValue(BitReader& bitReader) const {
    uint16_t word = bitReader.getWordConstant();
    auto& decoding = (*table)[static_cast<uint8_t>(word >> 8 & 0xFF)];
    if (decoding.table == nullptr) {
        bitReader.skipBits(decoding.bitLength);
        return decoding.value;
    }
    auto& decoding2 = (*decoding.table)[static_cast<uint8_t>(word & 0xFF)];
    bitReader.skipBits(decoding2.bitLength);
    return decoding2.value;
}

int EntropyDecoder::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = static_cast<int>(bitReader.getNBits(SSSS));
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

int EntropyDecoder::decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    int sCategory = huffmanTable.decodeNextValue(bitReader);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

std::pair<int, int> EntropyDecoder::decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    uint8_t rs = huffmanTable.decodeNextValue(bitReader);
    uint8_t r = GetNibble(rs, 0);
    uint8_t s = GetNibble(rs, 1);
    return {r, s};
}

std::array<int, 64> EntropyDecoder::decodeComponent(Jpg* jpg, BitReader& bitReader, const ScanHeaderComponentSpecification component, int (&prevDc)[3]) {
    std::array<int, 64> result{0};
    // DC Coefficient
    HuffmanTable &dcTable = jpg->dcHuffmanTables[component.dcTableSelector];
    int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[component.componentId - 1];
    result[0] = dcCoefficient;
    prevDc[component.componentId - 1] = dcCoefficient;

    // AC Coefficients
    int index = 1;
    while (index < Mcu::dataUnitLength) {
        HuffmanTable &acTable = jpg->acHuffmanTables[component.acTableSelector];
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
        result[zigZagMap[index]] = coefficient;
        index++;
    }
    return result;
}

Mcu EntropyDecoder::decodeMcu(Jpg* jpg, BitReader& bitReader, int (&prevDc)[3]) {
    Mcu mcu(jpg->frameHeader.luminanceComponentsPerMcu, jpg->frameHeader.maxHorizontalSample, jpg->frameHeader.maxVerticalSample);
    std::vector<QuantizationTable> qTables;
    for (auto& component : jpg->scanHeader.componentSpecifications) {
        qTables.push_back(jpg->quantizationTables[jpg->frameHeader.componentSpecifications[component.componentId].quantizationTableSelector]);
        if (component.componentId == 1) {
            for (int i = 0; i < jpg->frameHeader.luminanceComponentsPerMcu; i++) {
                mcu.Y[i] = decodeComponent(jpg, bitReader, component, prevDc);
            }
        } else if (component.componentId == 2) {
            mcu.Cb = decodeComponent(jpg, bitReader, component, prevDc);
        } else if (component.componentId == 3) {
            mcu.Cr = decodeComponent(jpg, bitReader, component, prevDc);
        }
    }

    mcu.dequantize(qTables);
    mcu.performInverseDCT();
    mcu.generateColorBlocks();
    return mcu;
}

void ScanHeaderComponentSpecification::print() const {
    std::cout << std::setw(25) << "Component Id: " << static_cast<int>(componentId) << "\n";
    std::cout << std::setw(25) << "DC Table Id: " << static_cast<int>(dcTableSelector) << "\n";
    std::cout << std::setw(25) << "AC Table Id: " << static_cast<int>(acTableSelector) << "\n";
}

void ScanHeader::print() const {
    std::cout << std::setw(25) << "Number of Components: " << componentSpecifications.size() << "\n";
    for (auto& component : componentSpecifications) {
        component.print();
    }
    std::cout << std::setw(26) << "Spectral Selection Start: " << static_cast<int>(spectralSelectionStart) << "\n";
    std::cout << std::setw(26) << "Spectral Selection End: " << static_cast<int>(spectralSelectionEnd) << "\n";
    std::cout << std::setw(26) << "Approximation High: " << static_cast<int>(successiveApproximationHigh) << "\n";
    std::cout << std::setw(26) << "Approximation Low: " << static_cast<int>(successiveApproximationLow) << "\n";
}

ScanHeader::ScanHeader(std::ifstream& file, const std::streampos& dataStartIndex) {
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
        componentSpecifications.emplace_back(componentId, GetNibble(tables, 0), GetNibble(tables, 1));
    }
    file.read(reinterpret_cast<char*>(&spectralSelectionStart), 1);
    file.read(reinterpret_cast<char*>(&spectralSelectionEnd), 1);
    file.read(reinterpret_cast<char*>(&successiveApproximationHigh), 1);
    successiveApproximationLow = GetNibble(successiveApproximationHigh, 1);
    successiveApproximationHigh = GetNibble(successiveApproximationLow, 1);
}

void ColorBlock::print() const {
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


void Mcu::print() const {
    std::cout << std::dec;
    std::cout << "Luminance (Y)        | Blue Chrominance (Cb)        | Red Chrominance (Cr)" << '\n';
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (const auto& i : Y) {
            for (int x = 0; x < 8; x++) {
                // Print Luminance
                int luminanceIndex = y * 8 + x;
                std::cout << std::setw(5) << i[luminanceIndex] << " ";
            }
            std::cout << "| ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Blue Chrominance
            int blueIndex = y * 8 + x;
            std::cout << std::setw(5) << Cb[blueIndex] << " ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Red Chrominance
            int redIndex = y * 8 + x;
            std::cout << std::setw(5) << Cr[redIndex] << " ";
        }
        std::cout << '\n';
    }
}

int Mcu::getColorIndex(const int blockIndex, const int pixelIndex, const int horizontalFactor, const int verticalFactor) {
    int blockRow = blockIndex / horizontalFactor;
    int blockCol = blockIndex % horizontalFactor;
    
    int pixelRow = pixelIndex / 8;
    int pixelCol = pixelIndex % 8;
    
    int chromaRow = (blockRow * 8 + pixelRow) / verticalFactor;
    int chromaCol = (blockCol * 8 + pixelCol) / horizontalFactor;
    
    return chromaRow * 8 + chromaCol;
}

void Mcu::generateColorBlocks() {
    for (int i = 0; i < static_cast<int>(Y.size()); i++) {
        auto& [R, G, B] = colorBlocks[i];
        for (int j = 0; j < dataUnitLength; j++) {
            int colorIndex = getColorIndex(i, j, horizontalSampleSize, verticalSampleSize);
            
            double y = Y[i][j] + 128;
            double cb = Cb[colorIndex];
            double cr = Cr[colorIndex];
            
            int r = static_cast<int>(y +              1.402 * cr);
            int g = static_cast<int>(y - 0.344 * cb - 0.714 * cr);
            int b = static_cast<int>(y + 1.772 * cb             );
        
            R[j] = static_cast<uint8_t>(std::clamp(r, 0, 255)); 
            G[j] = static_cast<uint8_t>(std::clamp(g, 0, 255)); 
            B[j] = static_cast<uint8_t>(std::clamp(b, 0, 255)); 
        }
    }
}

std::tuple<uint8_t, uint8_t, uint8_t> Mcu::getColor(int index) {
    int blockRow = index / (horizontalSampleSize * 8) / 8;
    int blockCol = index % (horizontalSampleSize * 8) / 8;

    int pixelRow = index / (horizontalSampleSize * 8) % 8;
    int pixelColumn = index % 8;
    
    int blockIndex = blockRow * horizontalSampleSize + blockCol;
    int pixelIndex = pixelRow * 8 + pixelColumn;
    
    return {colorBlocks[blockIndex].R[pixelIndex],
        colorBlocks[blockIndex].G[pixelIndex],
        colorBlocks[blockIndex].B[pixelIndex]};
}

// Uses AAN DCT
void Mcu::performInverseDCT(std::array<int, dataUnitLength>& array) {
    float results[64];
    // Calculates the rows
    for (int i = 0; i < 8; i++) {
        const float g0 = static_cast<float>(array[0 * 8 + i]) * s0;
         const float g1 = static_cast<float>(array[4 * 8 + i]) * s4;
         const float g2 = static_cast<float>(array[2 * 8 + i]) * s2;
         const float g3 = static_cast<float>(array[6 * 8 + i]) * s6;
         const float g4 = static_cast<float>(array[5 * 8 + i]) * s5;
         const float g5 = static_cast<float>(array[1 * 8 + i]) * s1;
         const float g6 = static_cast<float>(array[7 * 8 + i]) * s7;
         const float g7 = static_cast<float>(array[3 * 8 + i]) * s3;

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

        array[i * 8 + 0] = static_cast<int>(b0 + b7);
        array[i * 8 + 1] = static_cast<int>(b1 + b6);
        array[i * 8 + 2] = static_cast<int>(b2 + b5);
        array[i * 8 + 3] = static_cast<int>(b3 + b4);
        array[i * 8 + 4] = static_cast<int>(b3 - b4);
        array[i * 8 + 5] = static_cast<int>(b2 - b5);
        array[i * 8 + 6] = static_cast<int>(b1 - b6);
        array[i * 8 + 7] = static_cast<int>(b0 - b7);
    }
}

void Mcu::performInverseDCT() {
    if (postDctMode == false) {
        std::cout << "Warning: Has not been transformed via DCT, cannot perform inverse DCT";
        return;
    }
    for (auto& y : Y) {
        performInverseDCT(y);
    }
    performInverseDCT(Cb);
    performInverseDCT(Cr);
    postDctMode = false;
}

void Mcu::dequantize(std::array<int, dataUnitLength>& array, const QuantizationTable& quantizationTable) {
    for (int i = 0; i < dataUnitLength; i++) {
        array[i] *= quantizationTable.is8Bit ? quantizationTable.table8[i] : quantizationTable.table16[i];
    }
}

void Mcu::dequantize(const std::vector<QuantizationTable>& quantizationTables) {
    if (!isQuantized) {
        std::cout << "Warning: Data unit has already been dequantized, cannot dequantize again";
        return;
    }
    for (auto& y : Y) {
        dequantize(y, quantizationTables[0]);
    }
    if (quantizationTables.size() > 1) {
        dequantize(Cb, quantizationTables[1]);
        dequantize(Cr, quantizationTables[2]);
    }
    isQuantized = false;
}

Mcu::Mcu() {
    Y = std::vector(1, std::array<int, 64>({0}));
    colorBlocks = std::vector(1, ColorBlock());
}

Mcu::Mcu(const int luminanceComponents, const int horizontalSampleSize, const int verticalSampleSize) {
    Y = std::vector(luminanceComponents, std::array<int, 64>({0}));
    colorBlocks = std::vector(luminanceComponents, ColorBlock());
    this->horizontalSampleSize = horizontalSampleSize;
    this->verticalSampleSize = verticalSampleSize;
}

uint16_t Jpg::readLengthBytes() {
    uint16_t length;
    file.read(reinterpret_cast<char*>(&length), 2);
    length = SwapBytes(length);
    length -= 2; // Account for the bytes that store the length
    return length;
}

void Jpg::readFrameHeader(const uint8_t frameMarker) {
    file.seekg(2, std::ios::cur); // Skip past length bytes
    frameHeader = FrameHeader(frameMarker, file, file.tellg());
}

void Jpg::readQuantizationTables() {
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
        quantizationTables[tableId] = quantizationTable;
        length -= (static_cast<uint16_t>(file.tellg()) - static_cast<uint16_t>(dataStartIndex));
    }
}

void Jpg::readHuffmanTables() {
    uint16_t length = readLengthBytes();

    while (length > 0) {
        uint8_t tableId;
        file.read(reinterpret_cast<char*>(&tableId), 1);
        length--;
        uint8_t tableClass = GetNibble(tableId, 0);
        tableId = GetNibble(tableId, 1);
        std::streampos dataStartIndex = file.tellg();
        
        if (tableClass == 0) {
            dcHuffmanTables[tableId] = HuffmanTable(file, dataStartIndex);
        } else {
            acHuffmanTables[tableId] = HuffmanTable(file, dataStartIndex);
        }
        
        length -= (static_cast<uint16_t>(file.tellg()) - static_cast<uint16_t>(dataStartIndex));
    }
}

void Jpg::readDefineNumberOfLines() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&frameHeader.height), 2);
    frameHeader.height = SwapBytes(frameHeader.height);
}

void Jpg::readDefineRestartIntervals() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&restartInterval), 2);
    restartInterval = SwapBytes(restartInterval);
}

void Jpg::readComments() {
    uint16_t length = readLengthBytes();
    comment = std::string(length + 1, '\0');
    file.read(comment.data(), length);
}

void Jpg::readScanHeader() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    scanHeader = ScanHeader(file, file.tellg());
}

void Jpg::readStartOfScan() {
    readScanHeader();
    // Read entropy compressed bit stream
    clock_t begin = clock();
    std::vector<uint8_t> bytes;
    uint8_t byte;
    while (file.read(reinterpret_cast<char*>(&byte), 1)) {
        uint8_t previousByte = byte;
        if (previousByte == 0xFF) {
            if (!file.read(reinterpret_cast<char*>(&byte), 1)) {
                break;
            }
            if (byte >= RST0 && byte <= RST7) {
                continue;
            } else if (byte == 0x00) {
                bytes.push_back(previousByte);
            } else if (byte == EOI) {
                file.seekg(-2, std::ios::cur);
                break; 
            } else {
                std::cout << "Error: Unknown marker encountered in SOS data: " << std::hex << static_cast<int>(byte) << std::dec << "\n";
            }
        } else {
            bytes.push_back(previousByte);
        }
    }
    clock_t afterRead = clock();
    BitReader bitReader(bytes);
    int prevDc[3] = {};
    
    int mcuHeight = (frameHeader.height + (frameHeader.maxVerticalSample * 8 - 1)) / (frameHeader.maxVerticalSample * 8);
    int mcuWidth = (frameHeader.width + (frameHeader.maxHorizontalSample * 8 - 1)) / (frameHeader.maxHorizontalSample * 8);
    for (int i = 0; i < mcuHeight * mcuWidth; i++) {
        if (restartInterval != 0 && i % restartInterval == 0) {
            bitReader.alignToByte();
            prevDc[0] = 0;
            prevDc[1] = 0;
            prevDc[2] = 0;
        }
        mcus.push_back(EntropyDecoder::decodeMcu(this, bitReader, prevDc));
    }
    clock_t afterDecode = clock();
    std::cout << "Time to read bytes: " << static_cast<double>(afterRead - begin) / CLOCKS_PER_SEC << " seconds\n";
    std::cout << "Time to decode: " << static_cast<double>(afterDecode - afterRead) / CLOCKS_PER_SEC << " seconds\n";
}

void Jpg::readFile() {
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
                break;
            }
        }
    }
}

void Jpg::printInfo() const {
    std::cout << "Restart Interval: " << restartInterval << '\n';
    frameHeader.print();
    scanHeader.print();
    
    for (size_t i = 0; i < quantizationTables.size(); i++) {
        std::cout << "Table #" << i << "\n";
        quantizationTables[i].printTable();
    }
    
    for (size_t i = 0; i < dcHuffmanTables.size(); i++) {
        std::cout << "DC Table #" << i << "\n";
        dcHuffmanTables[i].tree.printTree();
    }
    
    for (size_t i = 0; i < acHuffmanTables.size(); i++) {
        std::cout << "AC Table #" << i << "\n";
        acHuffmanTables[i].tree.printTree();
    }

    if (!comment.empty()) {
        std::cout << "Comment: " << comment << "\n";
    }
}

Jpg::Jpg(const std::string& path) {
    file = std::ifstream(path, std::ifstream::binary);
    readFile();
}

typedef unsigned char byte;
typedef unsigned int uint;

void putInt(byte*& bufferPos, const uint v) {
    *bufferPos++ = v >>  0;
    *bufferPos++ = v >>  8;
    *bufferPos++ = v >> 16;
    *bufferPos++ = v >> 24;
}

// helper function to write a 2-byte short integer in little-endian
void putShort(byte*& bufferPos, const uint v) {
    *bufferPos++ = v >> 0;
    *bufferPos++ = v >> 8;
}

void Jpg::writeBmp(FrameHeader* image, const std::string& filename, std::vector<Mcu>& mcus) {
    clock_t begin = clock();
    // open file
    std::cout << "Writing " << filename << "...\n";
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "Error - Error opening output file\n";
        return;
    }
    
    const uint paddingSize = image->width % 4;
    const uint size = 14 + 12 + image->height * image->width * 3 + paddingSize * image->height;

    byte* buffer = new (std::nothrow) byte[size];
    if (buffer == nullptr) {
        std::cout << "Error - Memory error\n";
        outFile.close();
        return;
    }
    byte* bufferPos = buffer;

    *bufferPos++ = 'B';
    *bufferPos++ = 'M';
    putInt(bufferPos, size);
    putInt(bufferPos, 0);
    putInt(bufferPos, 0x1A);
    putInt(bufferPos, 12);
    putShort(bufferPos, image->width);
    putShort(bufferPos, image->height);
    putShort(bufferPos, 1);
    putShort(bufferPos, 24);
    
    for (uint y = image->height - 1; y < image->height; --y) {
        const uint blockRow = y / image->mcuPixelHeight;
        const uint pixelRow = y % image->mcuPixelHeight;
        for (uint x = 0; x < image->width; ++x) {
            const uint blockColumn = x / image->mcuPixelWidth;
            const uint pixelColumn = x % image->mcuPixelWidth;
            const uint blockIndex = blockRow * image->mcuImageWidth + blockColumn;
            const uint pixelIndex = pixelRow * image->mcuPixelWidth + pixelColumn;
            auto [R, G, B] = mcus[blockIndex].getColor(pixelIndex);

            *bufferPos++ = B;
            *bufferPos++ = G;
            *bufferPos++ = R;
        }
        for (uint i = 0; i < paddingSize; ++i) {
            *bufferPos++ = 0;
        }
    }

    outFile.write((char*)buffer, size);
    outFile.close();
    delete[] buffer;
    clock_t end = clock();
    std::cout << "Time to write: " << static_cast<double>(end - begin) / CLOCKS_PER_SEC << " seconds\n";
}
