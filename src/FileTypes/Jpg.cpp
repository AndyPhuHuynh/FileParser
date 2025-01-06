#include "Jpg.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <numbers>
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
        std::cout << "Error - Invalid root for huffman tree" << "\n";
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
            std::cout << "Error - Code not contained in huffman tree:" << code << "\n";
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
}

int EntropyDecoder::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = 0;
    for (int i = 0; i < SSSS; i++) {
        coefficient <<= 1;
        coefficient |= bitReader.getBit();
    }
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

int EntropyDecoder::decodeDcCoefficient(BitReader& bitReader, HuffmanTable& huffmanTable) {
    int sCategory = huffmanTable.tree.decodeNextValue(bitReader);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

std::pair<int, int> EntropyDecoder::decodeAcCoefficient(BitReader& bitReader, HuffmanTable& huffmanTable) {
    uint8_t rs = huffmanTable.tree.decodeNextValue(bitReader);
    uint8_t r = GetNibble(rs, 0);
    uint8_t s = GetNibble(rs, 1);
    return {r, s};
}

DataUnit EntropyDecoder::decodeMcu(Jpg* jpg, BitReader& bitReader, int (&prevDc)[3]) {
    DataUnit mcu;
    std::vector<QuantizationTable> qTables;
    for (auto& component : jpg->scanHeader.componentSpecifications) {
        qTables.push_back(jpg->quantizationTables[jpg->frameHeader.componentSpecifications[component.componentId].quantizationTableSelector]);
        std::array<int, 64> *compTable = mcu.getComponent(component.componentId);
        
        // DC Coefficient
        HuffmanTable &dcTable = jpg->dcHuffmanTables[component.dcTableSelector];
        int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[component.componentId - 1];
        (*compTable)[0] = dcCoefficient;
        prevDc[component.componentId - 1] = dcCoefficient;

        // AC Coefficients
        int index = 1;
        while (index < DataUnit::dataUnitLength) {
            HuffmanTable &acTable = jpg->acHuffmanTables[component.acTableSelector];
            auto [r, s] = decodeAcCoefficient(bitReader, acTable);
            if (r == 0x00 && s == 0x00) {
                break;
            }
            if (r == 0xFF && s == 0x00) {
                index += 16;
                continue;
            }
            index += r;
            if (index >= DataUnit::dataUnitLength) {
                break;
            }
            int coefficient = decodeSSSS(bitReader, s);
            (*compTable)[zigZagMap[index]] = coefficient;
            index++;
        }
    }

    mcu.dequantize(qTables);
    mcu.performInverseDCT();
    mcu.convertToRGB();
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

void DataUnit::print() const {
    std::cout << std::dec;
    std::cout << "Luminance (Y)        | Blue Chrominance (Cb)        | Red Chrominance (Cr)" << std::endl;
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // Print Luminance
            int luminanceIndex = y * 8 + x;
            std::cout << std::setw(5) << Y[luminanceIndex] << " ";
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

std::array<int, DataUnit::dataUnitLength> *DataUnit::getComponent(const int id) {
    if (id < 1 || id > 3) {
        std::cout << "Error - Component id must be in the range 1-3 inclusive\n";
        return nullptr;
    }
    if (id == 1) {
        return &Y;
    } else if (id == 2) {
        return &Cb;
    } else {
        return &Cr;
    }
}


void DataUnit::convertToRGB() {
    if (rgbMode) {
        std::cout << "Warning - Data unit is already in rgb mode, cannot convert to rgb again\n";
        return;
    }
    for (int i = 0; i < dataUnitLength; i++) {
        double y = Y[i] + 128;
        double cb = Cb[i];
        double cr = Cr[i];
            
        int r = static_cast<int>(y +              1.402 * cr);
        int g = static_cast<int>(y - 0.344 * cb - 0.714 * cr);
        int b = static_cast<int>(y + 1.772 * cb             );
        
        Y[i] = std::clamp(r, 0, 255);  // Red
        Cb[i] = std::clamp(g, 0, 255); // Green
        Cr[i] = std::clamp(b, 0, 255); // Blue
    }
    rgbMode = true;
}

// Uses AAN DCT
void DataUnit::performInverseDCT() {
    if (postDctMode == false) {
        std::cout << "Warning - Has not been transformed via DCT, cannot perform inverse DCT";
        return;
    }
    for (int id = 1; id <= 3; id++) {
        std::array<int, 64>* table = getComponent(id);
        // Calculate the rows
        float results[64];
        for (int i = 0; i < 8; i++) {
            const float g0 = static_cast<float>((*table)[0 * 8 + i]) * s0;
            const float g1 = static_cast<float>((*table)[4 * 8 + i]) * s4;
            const float g2 = static_cast<float>((*table)[2 * 8 + i]) * s2;
            const float g3 = static_cast<float>((*table)[6 * 8 + i]) * s6;
            const float g4 = static_cast<float>((*table)[5 * 8 + i]) * s5;
            const float g5 = static_cast<float>((*table)[1 * 8 + i]) * s1;
            const float g6 = static_cast<float>((*table)[7 * 8 + i]) * s7;
            const float g7 = static_cast<float>((*table)[3 * 8 + i]) * s3;

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

            (*table)[i * 8 + 0] = static_cast<int>(b0 + b7);
            (*table)[i * 8 + 1] = static_cast<int>(b1 + b6);
            (*table)[i * 8 + 2] = static_cast<int>(b2 + b5);
            (*table)[i * 8 + 3] = static_cast<int>(b3 + b4);
            (*table)[i * 8 + 4] = static_cast<int>(b3 - b4);
            (*table)[i * 8 + 5] = static_cast<int>(b2 - b5);
            (*table)[i * 8 + 6] = static_cast<int>(b1 - b6);
            (*table)[i * 8 + 7] = static_cast<int>(b0 - b7);
        }
    }
    postDctMode = false;
}

void DataUnit::dequantize(const std::vector<QuantizationTable>& quantizationTables) {
    for (int i = 0; i < quantizationTables.size(); i++) {
        std::array<int, 64>* table = getComponent(i + 1);
        QuantizationTable qt = quantizationTables[i]; 
        for (int i = 0; i < 64; i++) {
            (*table)[i] *= qt.is8Bit ? qt.table8[i] : qt.table16[i];
        }
    }
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
        length -= (file.tellg() - dataStartIndex);
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
        
        length -= (file.tellg() - dataStartIndex);
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
    if (frameHeader.componentSpecifications[1].horizontalSamplingFactor == frameHeader.componentSpecifications[2].horizontalSamplingFactor &&
        frameHeader.componentSpecifications[2].horizontalSamplingFactor == frameHeader.componentSpecifications[3].horizontalSamplingFactor) {
        std::cout << "Same horizontal scale\n";
    } else {
        std::cout << "Not same horizontal scale, not supported yet!\n";
    }

    if (frameHeader.componentSpecifications[1].verticalSamplingFactor == frameHeader.componentSpecifications[2].verticalSamplingFactor &&
        frameHeader.componentSpecifications[2].verticalSamplingFactor == frameHeader.componentSpecifications[3].verticalSamplingFactor) {
        std::cout << "Same vertical scale\n";
    } else {
        std::cout << "Not same vertical scale, not supported yet!\n";
    }

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
                std::cout << "Error - Unknown marker encountered in SOS data: " << std::hex << static_cast<int>(byte) << std::dec << "\n";
            }
        } else {
            bytes.push_back(previousByte);
        }
    }
    BitReader bitReader(bytes);
    int prevDc[3] = {};
    
    int mcuHeight = (frameHeader.height + 7) / 8;
    int mcuWidth = (frameHeader.width + 7) / 8;
    for (int i = 0; i < mcuHeight * mcuWidth; i++) {
        if (restartInterval != 0 && i % restartInterval == 0) {
            bitReader.alignToByte();
            prevDc[0] = 0;
            prevDc[1] = 0;
            prevDc[2] = 0;
        }
        mcus.push_back(EntropyDecoder::decodeMcu(this, bitReader, prevDc));
    }
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
    
    for (int i = 0; i < quantizationTables.size(); i++) {
        std::cout << "Table #" << i << "\n";
        quantizationTables[i].printTable();
    }
    
    for (int i = 0; i < dcHuffmanTables.size(); i++) {
        std::cout << "DC Table #" << i << "\n";
        dcHuffmanTables[i].tree.printTree();
    }
    
    for (int i = 0; i < acHuffmanTables.size(); i++) {
        std::cout << "AC Table #" << i << "\n";
        acHuffmanTables[i].tree.printTree();
    }

    if (!comment.empty()) {
        std::cout << "Comment: " << comment << "\n";
    }
}

Jpg::Jpg(const std::string& path) : quantizationTables() {
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

void Jpg::writeBmp(FrameHeader* image, const std::string& filename, std::vector<DataUnit>& mcu) {
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

    int mcuWidth = (image->width + 7) / 8;
    for (uint y = image->height - 1; y < image->height; --y) {
        const uint blockRow = y / 8;
        const uint pixelRow = y % 8;
        for (uint x = 0; x < image->width; ++x) {
            const uint blockColumn = x / 8;
            const uint pixelColumn = x % 8;
            const uint blockIndex = blockRow * mcuWidth + blockColumn;
            const uint pixelIndex = pixelRow * 8 + pixelColumn;
            
            *bufferPos++ = mcu[blockIndex].Cr[pixelIndex];
            *bufferPos++ = mcu[blockIndex].Cb[pixelIndex];
            *bufferPos++ = mcu[blockIndex].Y[pixelIndex];
        }
        for (uint i = 0; i < paddingSize; ++i) {
            *bufferPos++ = 0;
        }
    }

    outFile.write((char*)buffer, size);
    outFile.close();
    delete[] buffer;
}

