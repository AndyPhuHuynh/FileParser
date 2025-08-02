#include "FileParser/Jpeg/JpegImage.h"

#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>
#include <ctime>
#include <filesystem>
#include <ranges>
#include <vector>

#include <simde/x86/avx512.h>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
#include "FileParser/Jpeg/HuffmanDecoder.hpp"
#include "FileParser/Jpeg/Markers.hpp"
#include "FileParser/Jpeg/QuantizationTableBuilder.hpp"
#include "FileParser/Jpeg/Transform.hpp"

void FileParser::Jpeg::FrameHeaderComponentSpecification::print() const {
    std::cout << std::setw(25) << "Identifier: " << static_cast<int>(identifier) << "\n";
    std::cout << std::setw(25) << "HorizontalSampleFactor: " << static_cast<int>(horizontalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "VerticalSampleFactor: " << static_cast<int>(verticalSamplingFactor) << "\n";
    std::cout << std::setw(25) << "QuantizationTable: " << static_cast<int>(quantizationTableSelector) << "\n";
}

void FileParser::Jpeg::FrameHeader::print() const {
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

FileParser::Jpeg::FrameHeader::FrameHeader(const uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex) {
    this->encodingProcess = encodingProcess;
    file.seekg(dataStartIndex, std::ios::beg);
    file.read(reinterpret_cast<char*>(&precision), 1);
    file.read(reinterpret_cast<char*>(&height), 2);
    height = SwapBytes(height);
    file.read(reinterpret_cast<char*>(&width), 2);
    width = SwapBytes(width);
    file.read(reinterpret_cast<char*>(&numOfChannels), 1);

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
        std::cerr << "Error: Component has an identifier of zero - adjusting all identifiers to be interpreted as one higher\n";
    }

    initializeValuesFromComponents();
}

FileParser::Jpeg::FrameHeader::FrameHeader(const uint8_t SOF, const uint8_t precision, const uint16_t height, const uint16_t width,
    const std::vector<FrameHeaderComponentSpecification>& components) {
    encodingProcess = SOF;
    this->precision = precision;
    this->height = height;
    this->width = width;
    numOfChannels = static_cast<uint8_t>(components.size());
    for (auto& component : components) {
        componentSpecifications[component.identifier] = component;
    }
    initializeValuesFromComponents();
}

void FileParser::Jpeg::FrameHeader::initializeValuesFromComponents() {
    if (numOfChannels > 4) {
        std::cerr << "Error: number of channels should be less than 4\n";
        std::cerr << "Num of channels: " << numOfChannels << '\n';
    }

    int sampleSum = 0;
    for (auto& component : componentSpecifications | std::views::values) {
        sampleSum += component.horizontalSamplingFactor * component.verticalSamplingFactor;
    }
    if (sampleSum > 10) {
        std::cerr << "Error: Sample sum is greater than 10\n";
    }

    for (auto& component : componentSpecifications | std::views::values) {
        if (component.identifier != 1) {
            if (component.horizontalSamplingFactor != 1) {
                std::cerr << "Error: Horizontal sampling factor of " << static_cast<int>(component.identifier) <<
                    " is not supported for chroma component" << "\n";
                std::terminate();
            }
            if (component.verticalSamplingFactor != 1) {
                std::cerr << "Error: Vertical sampling factor of " << static_cast<int>(component.identifier) <<
                    " is not supported for chroma component" << "\n";
                std::terminate();
            }
        }
        if (component.identifier == 1) {
            maxHorizontalSample = component.horizontalSamplingFactor;
            maxVerticalSample = component.verticalSamplingFactor;
            if (component.horizontalSamplingFactor == 1 && component.verticalSamplingFactor == 1) {
                luminanceComponentsPerMcu = 1;
            } else if ((component.horizontalSamplingFactor == 1 && component.verticalSamplingFactor == 2)
                    || (component.horizontalSamplingFactor == 2 && component.verticalSamplingFactor == 1)) {
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

int FileParser::Jpeg::JpegImage::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = static_cast<int>(bitReader.getNBits(SSSS));
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

int FileParser::Jpeg::JpegImage::decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    int sCategory = decodeNextValue(bitReader, huffmanTable);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

std::pair<int, int> FileParser::Jpeg::JpegImage::decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    uint8_t rs = decodeNextValue(bitReader, huffmanTable);
    uint8_t r = GetNibble(rs, 0);
    uint8_t s = GetNibble(rs, 1);
    return {r, s};
}

std::ofstream oldLog = std::ofstream("./oldlog.txt");

FileParser::Jpeg::Component FileParser::Jpeg::JpegImage::decodeComponent(BitReader& bitReader,
                                                                         const ScanHeaderComponentSpecification&
                                                                         scanComp,
                                                                         int (&prevDc)[3]) {
    // TODO: Add error messages for getting values from huffman table optionals
    Component result;
    // DC Coefficient
    HuffmanTable &dcTable = *dcHuffmanTables[scanComp.dcTableIteration][scanComp.dcTableSelector];
    int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[scanComp.componentId - 1];
    result[0] = static_cast<float>(dcCoefficient);
    prevDc[scanComp.componentId - 1] = dcCoefficient;
    oldLog  << std::format("DC: {}\n", dcCoefficient);

    // AC Coefficients
    int index = 1;
    while (index < Mcu::DataUnitLength) {
        HuffmanTable &acTable = *acHuffmanTables[scanComp.acTableIteration][scanComp.acTableSelector];
        auto [r, s] = decodeAcCoefficient(bitReader, acTable);
        oldLog  << std::format("AC: {}\n", r << 4 | s);
        if (r == 0x0 && s == 0x0) {
            break;
        }
        if (r == 0xF && s == 0x0) {
            index += 16;
            continue;
        }
        index += r;
        if (index >= Mcu::DataUnitLength) {
            break;
        }
        int coefficient = decodeSSSS(bitReader, s);
        result[zigZagMap[index]] = static_cast<float>(coefficient);
        index++;
    }
    oldLog << "\n\n";
    return result;
}

void FileParser::Jpeg::JpegImage::decodeMcu(Mcu* mcu, ScanHeader& scanHeader, int (&prevDc)[3]) {
    static int i = 0;
    oldLog << std::format("MCU: {}\n", i++);
    for (auto& component : scanHeader.componentSpecifications) {
        if (component.componentId == 1) {
            for (int i = 0; i < info.luminanceComponentsPerMcu; i++) {
                mcu->Y[i] = decodeComponent(scanHeader.bitReader, component, prevDc);
            }
        } else if (component.componentId == 2) {
            mcu->Cb = decodeComponent(scanHeader.bitReader, component, prevDc);
        } else if (component.componentId == 3) {
            mcu->Cr = decodeComponent(scanHeader.bitReader, component, prevDc);
        }
    }
}

void FileParser::Jpeg::JpegImage::skipZeros(BitReader& bitReader, Component& component, const int numToSkip, int& index, const int approximationLow) {
    if (numToSkip + index >= Mcu::DataUnitLength) {
        std::cerr << "Invalid number of zeros to skip: " << numToSkip << "\n";
        std::cerr << "Index: " << index << "\n";
    }
    int positive = 1 << approximationLow;
    int negative = -(1 << approximationLow);
    int zerosRead = 0;

    if (numToSkip == 0) {
        while (!AreFloatsEqual(component[zigZagMap[index]], 0.0f)) {
            int bit = bitReader.getBit();
            if (bit == 1) {
                if (component[zigZagMap[index]] > 0) {
                    component[zigZagMap[index]] += static_cast<float>(positive);
                } else if (component[zigZagMap[index]] < 0) {
                    component[zigZagMap[index]] += static_cast<float>(negative);
                }
            }
            index++;
        }
    }

    while (zerosRead < numToSkip) {
        if (!AreFloatsEqual(component[zigZagMap[index]], 0.0f)) {
            int bit = bitReader.getBit();
            if (bit == 1) {
                if (component[zigZagMap[index]] > 0) {
                    component[zigZagMap[index]] += static_cast<float>(positive);
                } else if (component[zigZagMap[index]] < 0) {
                    component[zigZagMap[index]] += static_cast<float>(negative);
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

    while (!AreFloatsEqual(component[zigZagMap[index]], 0.0f)) {
        int bit = bitReader.getBit();
        if (bit == 1) {
            if (component[zigZagMap[index]] > 0) {
                component[zigZagMap[index]] += static_cast<float>(positive);
            } else if (component[zigZagMap[index]] < 0) {
                component[zigZagMap[index]] += static_cast<float>(negative);
            }
        }
        index++;
    }
}

void FileParser::Jpeg::JpegImage::decodeProgressiveComponent(Component& component, ScanHeader& scanHeader,
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
                if (!AreFloatsEqual(component[zigZagMap[j]], 0.0f)) {
                    int bit = scanHeader.bitReader.getBit();
                    if (bit == 1) {
                        if (component[zigZagMap[j]] > 0) {
                            component[zigZagMap[j]] += static_cast<float>(positive);
                        } else if (component[zigZagMap[j]] < 0) {
                            component[zigZagMap[j]] += static_cast<float>(negative);
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
            HuffmanTable& dcTable = *dcHuffmanTables[componentInfo.dcTableIteration][componentInfo.dcTableSelector];
            int dcCoefficient = (decodeDcCoefficient(scanHeader.bitReader, dcTable) << approximationLow) + prevDc[componentInfo.componentId - 1];
            component[0] = static_cast<float>(dcCoefficient);
            prevDc[componentInfo.componentId - 1] = dcCoefficient;
        }
        // Refinement scan
        else {
            component[0] += static_cast<float>(scanHeader.bitReader.getBit() << approximationLow);
        }
        return;
    }

    // AC Coefficients first scan
    if (approximationHigh == 0) {
        int index = spectralStart;
        while (index <= spectralEnd) {
            HuffmanTable& acTable = *acHuffmanTables[componentInfo.acTableIteration][componentInfo.acTableSelector];
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
            component[zigZagMap[index]] = static_cast<float>(coefficient);
            index++;
        }
        return;
    }

    // Ac Coefficients refinement scan
    int index = spectralStart;
    int numToAdd = 1 << approximationLow;
    while (index <= spectralEnd) {
        HuffmanTable& acTable = *acHuffmanTables[componentInfo.acTableIteration][componentInfo.acTableSelector];
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
                if (!AreFloatsEqual(component[zigZagMap[index]], 0.0f)) {
                    int bit = scanHeader.bitReader.getBit();
                    if (bit == 1) {
                        if (component[zigZagMap[index]] > 0) {
                            component[zigZagMap[index]] += static_cast<float>(positive);
                        } else if (component[zigZagMap[index]] < 0) {
                            component[zigZagMap[index]] += static_cast<float>(negative);
                        }
                    }
                }
                index++;
            }
            break;
        }
        const int coefficient = scanHeader.bitReader.getBit() == 1 ? numToAdd : numToAdd * -1;
        skipZeros(scanHeader.bitReader, component, r, index, approximationLow);
        component[zigZagMap[index]] += static_cast<float>(coefficient);
        index++;
    }
}

void FileParser::Jpeg::ScanHeaderComponentSpecification::print() const {
    std::cout << std::setw(30) << "Component Id: " << static_cast<int>(componentId) << "\n";
    std::cout << std::setw(30) << "DC Table Id: " << static_cast<int>(dcTableSelector) << "\n";
    std::cout << std::setw(30) << "AC Table Id: " << static_cast<int>(acTableSelector) << "\n";
    std::cout << std::setw(30) << "Quantization Table Iteration: " << static_cast<int>(quantizationTableIteration) << "\n";
    std::cout << std::setw(30) << "DC Table Iteration: " << static_cast<int>(dcTableIteration) << "\n";
    std::cout << std::setw(30) << "AC Table Iteration: " << static_cast<int>(acTableIteration) << "\n";
}

void FileParser::Jpeg::ScanHeader::print() const {
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

FileParser::Jpeg::ScanHeader::ScanHeader(JpegImage* jpeg, const std::streampos& dataStartIndex) {
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
        while (quantizationTableIteration > 0 && !jpeg->quantizationTables[quantizationTableIteration][qTableSelector].has_value()) {
            quantizationTableIteration--;
        }

        int dcTableSelector = GetNibble(tables, 0);
        int dcTableIteration = static_cast<int>(jpeg->dcHuffmanTables.size()) - 1;
        while (dcTableIteration > 0 && !jpeg->dcHuffmanTables[dcTableIteration][dcTableSelector].has_value()) {
            dcTableIteration--;
        }
        int acTableSelector = GetNibble(tables, 1);
        int acIteration = static_cast<int>(jpeg->acHuffmanTables.size()) - 1;
        while (acIteration > 0 && !jpeg->acHuffmanTables[acIteration][acTableSelector].has_value()) {
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

bool FileParser::Jpeg::ScanHeader::containsComponentId(const int id) {
    return std::ranges::any_of(componentSpecifications, [id](auto& comp) {
        return comp.componentId == id;
    });
}

FileParser::Jpeg::ScanHeaderComponentSpecification& FileParser::Jpeg::ScanHeader::getComponent(const int id) {
    for (auto& comp : componentSpecifications) {
        if (comp.componentId == id) return comp;
    }
    std::cerr << "Error: No component with id " << id << " found\n";
    return componentSpecifications[0];
}

void FileParser::Jpeg::ColorBlock::print() const {
    std::cout << std::dec;
    std::cout << "Red (R)        | Green (G)        | Blue (B)" << '\n';
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            // Print Red
            int luminanceIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(std::round(R[luminanceIndex])) << " ";
        }
        std::cout << "| ";


        for (int x = 0; x < 8; x++) {
            // Print Green
            int blueIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(std::round(G[blueIndex])) << " ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Blue
            int redIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(std::round(B[redIndex])) << " ";
        }
        std::cout << '\n';
    }
}

void FileParser::Jpeg::ColorBlock::rgbToYCbCr() {
    for (int i = 0; i < colorBlockLength; i++) {
        float Y =      0.299f * R[i] +    0.587f * G[i] +    0.114f *  B[i];
        float Cb = -0.168736f * R[i] - 0.331264f * G[i] +      0.5f *  B[i];
        float Cr =       0.5f * R[i] - 0.418688f * G[i] - 0.081312f *  B[i];
        R[i] = Y - 128;
        G[i] = Cb;
        B[i] = Cr;
    }
}

uint16_t FileParser::Jpeg::JpegImage::readLengthBytes() {
    uint16_t length;
    file.read(reinterpret_cast<char*>(&length), 2);
    length = SwapBytes(length);
    length -= 2; // Account for the bytes that store the length
    return length;
}

void FileParser::Jpeg::JpegImage::readFrameHeader(const uint8_t frameMarker) {
    file.seekg(2, std::ios::cur); // Skip past length bytes
    info = FrameHeader(frameMarker, file, file.tellg());
    mcus.reserve(static_cast<unsigned int>(info.mcuImageHeight * info.mcuImageWidth));
    for (int i = 0; i < info.mcuImageHeight * info.mcuImageWidth; i++) {
        mcus.emplace_back(std::make_shared<Mcu>(info.maxHorizontalSample, info.maxVerticalSample));
    }
}

// Called AFTER encountering the DQT marker
void FileParser::Jpeg::JpegImage::readQuantizationTables() {
    uint16_t length = readLengthBytes();

    BitReader bytes;
    for (uint16_t i = 0; i < length; i++) {
        uint8_t byte;
        file.read(reinterpret_cast<char *>(&byte), 1);
        bytes.addByte(byte);
    }

    while (!bytes.reachedEnd()) {
        // TODO: Handle std::expected
        auto table = *QuantizationTableBuilder::readFromBitReader(bytes);

        int iteration = 0;
        while (quantizationTables[iteration][table.destination].has_value()) {
            iteration++;
            if (static_cast<int>(quantizationTables.size()) <= iteration) {
                quantizationTables.emplace_back();
            }
        }
        quantizationTables[iteration][table.destination] = table;
    }
}

void FileParser::Jpeg::JpegImage::readHuffmanTables() {
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
        while (tables[iteration][tableId].has_value()) {
            iteration++;
            if (static_cast<int>(tables.size()) <= iteration) {
                tables.emplace_back();
            }
        }
        tables[iteration][tableId] = *HuffmanBuilder::readFromFile(file);
        length -= (static_cast<uint16_t>(file.tellg()) - static_cast<uint16_t>(dataStartIndex));
    }
}

void FileParser::Jpeg::JpegImage::readDefineNumberOfLines() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&info.height), 2);
    info.height = SwapBytes(info.height);
}

void FileParser::Jpeg::JpegImage::readDefineRestartIntervals() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    file.read(reinterpret_cast<char*>(&currentRestartInterval), 2);
    currentRestartInterval = SwapBytes(currentRestartInterval);
}

void FileParser::Jpeg::JpegImage::readComments() {
    uint16_t length = readLengthBytes();
    comment = std::string(length + 1, '\0');
    file.read(comment.data(), length);
}

void FileParser::Jpeg::JpegImage::processQuantizationQueue(const std::vector<ScanHeaderComponentSpecification>& scanComps) {
    static double totalTime = 0.0f;
    static int mcusCount = 0;
    mcusCount = 0;
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
                dequantize(*mcu, this, scanComp);
            }
            mcusCount++;
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

void FileParser::Jpeg::JpegImage::processIdctQueue() {
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
            inverseDCT(*mcu);
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

void FileParser::Jpeg::JpegImage::processColorConversionQueue() {
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
            // TODO: Get rid of this thread
            // mcu->generateColorBlocks();
            clock_t end = clock();
            totalTime += (end - begin);
        }
        if (colorConversionQueue.allProductsAdded && colorConversionQueue.queue.empty()) {
            break;
        }
    }
    std::cout << "Total time on color: " << (totalTime) / CLOCKS_PER_SEC << " seconds\n";
}

std::shared_ptr<FileParser::Jpeg::ScanHeader> FileParser::Jpeg::JpegImage::readScanHeader() {
    file.seekg(2, std::ios::cur); // Skip past the length bytes
    std::shared_ptr<ScanHeader> scanHeader = std::make_shared<ScanHeader>(this, file.tellg());
    scanHeaders.emplace_back(scanHeader);
    return scanHeader;
}

void FileParser::Jpeg::JpegImage::readBaselineStartOfScan() {
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

void FileParser::Jpeg::JpegImage::pollCurrentScan(const int mcuIndex, const int scanNumber) {
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

void FileParser::Jpeg::JpegImage::pushToNextScan(const int mcuIndex, const int scanNumber) {
    std::shared_ptr<AtomicCondition> nextScanData;
    {
        std::unique_lock scanLock(scanIndicesMutex);
        nextScanData = scanIndices[scanNumber + 1];
    }
    std::unique_lock lock(nextScanData->mutex);
    nextScanData->value = mcuIndex;
    nextScanData->condition.notify_one();
}

void FileParser::Jpeg::JpegImage::processProgressiveStartOfScan(const std::shared_ptr<ScanHeader>& scan, const int scanNumber) {
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
                    decodeProgressiveComponent(mcus[mcuIndex]->Y[luminanceIndex], *scan, scanSpec, prevDc, componentsToSkip);

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
                    decodeProgressiveComponent(mcus[mcuIndex]->Cb, *scan, scanSpec, prevDc, componentsToSkip);
                } else if (scanSpec.componentId == 3) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cr, *scan, scanSpec, prevDc, componentsToSkip);
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
                        decodeProgressiveComponent(mcus[mcuIndex]->Y[i], *scan, componentInfo, prevDc, componentsToSkip);
                    }
                } else if (componentInfo.componentId == 2) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cb, *scan, componentInfo, prevDc, componentsToSkip);
                } else if (componentInfo.componentId == 3) {
                    decodeProgressiveComponent(mcus[mcuIndex]->Cr, *scan, componentInfo, prevDc, componentsToSkip);
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

void FileParser::Jpeg::JpegImage::readProgressiveStartOfScan() {
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

void FileParser::Jpeg::JpegImage::readStartOfScan() {
    scanIndices.emplace_back(std::make_shared<AtomicCondition>());
    if (info.encodingProcess == SOF0) {
        readBaselineStartOfScan();
    } else if (info.encodingProcess == SOF2) {
        readProgressiveStartOfScan();
    }
}

void FileParser::Jpeg::JpegImage::decodeBaseLine() {
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

void FileParser::Jpeg::JpegImage::decodeProgressive() {
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

void FileParser::Jpeg::JpegImage::decode() {
    if (info.encodingProcess == SOF0) {
        decodeBaseLine();
    } else if (info.encodingProcess == SOF2) {
        decodeProgressive();
    }
}

void FileParser::Jpeg::JpegImage::readFile() {
    uint8_t byte;
    while (!file.eof()) {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (byte == 0xFF) {
            uint8_t marker;
            file.read(reinterpret_cast<char*>(&marker), 1);
            if (marker == 0x00) continue;
            if (marker == DHT) {
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

auto FileParser::Jpeg::convertMcusToColorBlocks(
    const std::vector<std::shared_ptr<Mcu>>& mcus, const size_t pixelWidth, const size_t pixelHeight
) -> std::vector<ColorBlock> {
    auto ceilDivide = [](auto a, auto b) {
        return (a + b - 1) / b;
    };

    if (mcus.empty()) {
        return {};
    }

    constexpr size_t blockSideLength = 8;
    const size_t blockWidth  = ceilDivide(pixelWidth, blockSideLength);
    const size_t blockHeight = ceilDivide(pixelHeight, blockSideLength);

    const int horizontal =  mcus[0]->horizontalSampleSize;
    const int vertical   =  mcus[0]->verticalSampleSize;

    const size_t mcuTotalWidth  = ceilDivide(blockWidth, horizontal);
    std::vector result(blockWidth * blockHeight , ColorBlock());

    for (size_t mcuIndex = 0; mcuIndex < mcus.size(); mcuIndex++) {
        const size_t mcuRow = mcuIndex / mcuTotalWidth;
        const size_t mcuCol = mcuIndex % mcuTotalWidth;

        auto colors = generateColorBlocks(*mcus[mcuIndex]);
        for (size_t colorIndex = 0; colorIndex < colors.size(); colorIndex++) {
            const size_t colorRow = colorIndex / horizontal;
            const size_t colorCol = colorIndex % horizontal;

            const size_t blockRow = mcuRow * vertical   + colorRow;
            const size_t blockCol = mcuCol * horizontal + colorCol;

            if (blockRow < blockHeight && blockCol < blockWidth) {
                result[blockRow * blockWidth + blockCol] = colors[colorIndex];
            }
        }
    }
    return result;
}

auto FileParser::Jpeg::convertMcusToColorBlocks(const std::vector<Mcu>& mcus, size_t pixelWidth,
    size_t pixelHeight) -> std::vector<ColorBlock> {
    auto ceilDivide = [](auto a, auto b) {
        return (a + b - 1) / b;
    };

    if (mcus.empty()) {
        return {};
    }

    constexpr size_t blockSideLength = 8;
    const size_t blockWidth  = ceilDivide(pixelWidth, blockSideLength);
    const size_t blockHeight = ceilDivide(pixelHeight, blockSideLength);

    const int horizontal =  mcus[0].horizontalSampleSize;
    const int vertical   =  mcus[0].verticalSampleSize;

    const size_t mcuTotalWidth  = ceilDivide(blockWidth, horizontal);
    std::vector result(blockWidth * blockHeight , ColorBlock());

    for (size_t mcuIndex = 0; mcuIndex < mcus.size(); mcuIndex++) {
        const size_t mcuRow = mcuIndex / mcuTotalWidth;
        const size_t mcuCol = mcuIndex % mcuTotalWidth;

        auto colors = generateColorBlocks(mcus[mcuIndex]);
        for (size_t colorIndex = 0; colorIndex < colors.size(); colorIndex++) {
            const size_t colorRow = colorIndex / horizontal;
            const size_t colorCol = colorIndex % horizontal;

            const size_t blockRow = mcuRow * vertical   + colorRow;
            const size_t blockCol = mcuCol * horizontal + colorCol;

            if (blockRow < blockHeight && blockCol < blockWidth) {
                result[blockRow * blockWidth + blockCol] = colors[colorIndex];
            }
        }
    }
    return result;
}

FileParser::Jpeg::JpegImage::JpegImage(const std::string& path) {
    file = std::ifstream(path, std::ifstream::binary);
    readFile();
}
