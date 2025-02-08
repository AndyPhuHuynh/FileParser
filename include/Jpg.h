// ReSharper disable All
#pragma once
#include <array>
#include <fstream>
#include <map>
#include <numbers>
#include <unordered_map>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>

#include "BitManipulationUtil.h"

// Start of Frame markers, non-differential, Huffman coding
constexpr uint8_t SOF0 = 0xC0; // Baseline DCT
constexpr uint8_t SOF1 = 0xC1; // Extended sequential DCT
constexpr uint8_t SOF2 = 0xC2; // Progressive DCT
constexpr uint8_t SOF3 = 0xC3; // Lossless (sequential)

// Start of Frame markers, differential, Huffman coding
constexpr uint8_t SOF5 = 0xC5; // Differential sequential DCT
constexpr uint8_t SOF6 = 0xC6; // Differential progressive DCT
constexpr uint8_t SOF7 = 0xC7; // Differential lossless (sequential)

// Start of Frame markers, non-differential, arithmetic coding
constexpr uint8_t SOF9 = 0xC9; // Extended sequential DCT
constexpr uint8_t SOF10 = 0xCA; // Progressive DCT
constexpr uint8_t SOF11 = 0xCB; // Lossless (sequential)

// Start of Frame markers, differential, arithmetic coding
constexpr uint8_t SOF13 = 0xCD; // Differential sequential DCT
constexpr uint8_t SOF14 = 0xCE; // Differential progressive DCT
constexpr uint8_t SOF15 = 0xCF; // Differential lossless (sequential)

// Define Huffman Table(s)
constexpr uint8_t DHT = 0xC4;

// JPEG extensions
constexpr uint8_t JPG = 0xC8;

// Define Arithmetic Coding Conditioning(s)
constexpr uint8_t DAC = 0xCC;

// Restart interval Markers
constexpr uint8_t RST0 = 0xD0;
constexpr uint8_t RST1 = 0xD1;
constexpr uint8_t RST2 = 0xD2;
constexpr uint8_t RST3 = 0xD3;
constexpr uint8_t RST4 = 0xD4;
constexpr uint8_t RST5 = 0xD5;
constexpr uint8_t RST6 = 0xD6;
constexpr uint8_t RST7 = 0xD7;

// Other Markers
constexpr uint8_t SOI = 0xD8; // Start of Image
constexpr uint8_t EOI = 0xD9; // End of Image
constexpr uint8_t SOS = 0xDA; // Start of Scan
constexpr uint8_t DQT = 0xDB; // Define Quantization Table(s)
constexpr uint8_t DNL = 0xDC; // Define Number of Lines
constexpr uint8_t DRI = 0xDD; // Define Restart Interval
constexpr uint8_t DHP = 0xDE; // Define Hierarchical Progression
constexpr uint8_t EXP = 0xDF; // Expand Reference Component(s)

// APPN Markers
constexpr uint8_t APP0 = 0xE0;
constexpr uint8_t APP1 = 0xE1;
constexpr uint8_t APP2 = 0xE2;
constexpr uint8_t APP3 = 0xE3;
constexpr uint8_t APP4 = 0xE4;
constexpr uint8_t APP5 = 0xE5;
constexpr uint8_t APP6 = 0xE6;
constexpr uint8_t APP7 = 0xE7;
constexpr uint8_t APP8 = 0xE8;
constexpr uint8_t APP9 = 0xE9;
constexpr uint8_t APP10 = 0xEA;
constexpr uint8_t APP11 = 0xEB;
constexpr uint8_t APP12 = 0xEC;
constexpr uint8_t APP13 = 0xED;
constexpr uint8_t APP14 = 0xEE;
constexpr uint8_t APP15 = 0xEF;

// Misc Markers
constexpr uint8_t JPG0 = 0xF0;
constexpr uint8_t JPG1 = 0xF1;
constexpr uint8_t JPG2 = 0xF2;
constexpr uint8_t JPG3 = 0xF3;
constexpr uint8_t JPG4 = 0xF4;
constexpr uint8_t JPG5 = 0xF5;
constexpr uint8_t JPG6 = 0xF6;
constexpr uint8_t JPG7 = 0xF7;
constexpr uint8_t JPG8 = 0xF8;
constexpr uint8_t JPG9 = 0xF9;
constexpr uint8_t JPG10 = 0xFA;
constexpr uint8_t JPG11 = 0xFB;
constexpr uint8_t JPG12 = 0xFC;
constexpr uint8_t JPG13 = 0xFD;
constexpr uint8_t COM = 0xFE;
constexpr uint8_t TEM = 0x01;

const unsigned char zigZagMap[] = {
    0,   1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

// IDCT scaling factors
const float m0 = static_cast<float>(2.0 * std::cos(1.0 / 16.0 * 2.0 * std::numbers::pi));
const float m1 = static_cast<float>(2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi));
const float m3 = static_cast<float>(2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi));
const float m5 = static_cast<float>(2.0 * std::cos(3.0 / 16.0 * 2.0 * std::numbers::pi));
const float m2 = m0 - m5;
const float m4 = m0 + m5;

const float s0 = static_cast<float>(std::cos(0.0 / 16.0 * std::numbers::pi) / std::sqrt(8));
const float s1 = static_cast<float>(std::cos(1.0 / 16.0 * std::numbers::pi) / 2.0);
const float s2 = static_cast<float>(std::cos(2.0 / 16.0 * std::numbers::pi) / 2.0);
const float s3 = static_cast<float>(std::cos(3.0 / 16.0 * std::numbers::pi) / 2.0);
const float s4 = static_cast<float>(std::cos(4.0 / 16.0 * std::numbers::pi) / 2.0);
const float s5 = static_cast<float>(std::cos(5.0 / 16.0 * std::numbers::pi) / 2.0);
const float s6 = static_cast<float>(std::cos(6.0 / 16.0 * std::numbers::pi) / 2.0);
const float s7 = static_cast<float>(std::cos(7.0 / 16.0 * std::numbers::pi) / 2.0);

class Jpg;

enum Subsampling {
    None,
    HorizontalHalved,
    VerticalHalved,
    Quartered
};

class FrameHeaderComponentSpecification {
public:
    uint8_t identifier;
    uint8_t horizontalSamplingFactor;
    uint8_t verticalSamplingFactor;
    uint8_t quantizationTableSelector;

    FrameHeaderComponentSpecification() = default;
    FrameHeaderComponentSpecification(const uint8_t identifier, const uint8_t horizontalSamplingFactor, const uint8_t verticalSamplingFactor, const uint8_t quantizationTableSelector)
        : identifier(identifier), horizontalSamplingFactor(horizontalSamplingFactor), verticalSamplingFactor(verticalSamplingFactor), quantizationTableSelector(quantizationTableSelector) {}
    void print() const;
};

class FrameHeader {
public:
    uint8_t encodingProcess;
    uint8_t precision;
    uint16_t height;
    uint16_t width;
    uint8_t numOfChannels;
    std::map<uint8_t, FrameHeaderComponentSpecification> componentSpecifications;
    Subsampling subsampling = None;
    int luminanceComponentsPerMcu = 1;
    int maxHorizontalSample = 1;
    int maxVerticalSample = 1;
    int mcuPixelWidth;
    int mcuPixelHeight;
    int mcuImageWidth;
    int mcuImageHeight;

    FrameHeader() = default;
    FrameHeader(uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex);
    void print() const;
};

class QuantizationTable {
public:
    static constexpr int tableLength = 64;
    std::array<float, tableLength> table{0};
    bool isSet = false;

    QuantizationTable() = default;
    QuantizationTable(std::ifstream& file, const std::streampos& dataStartIndex, bool is8Bit);
    void print() const;
};

class HuffmanEncoding {
public:
    uint16_t encoding;
    uint8_t bitLength;
    uint8_t value;
    HuffmanEncoding(const uint16_t encoding, const uint8_t bitLength, const uint8_t value)
        : encoding(encoding), bitLength(bitLength), value(value) {}
};

struct HuffmanTableEntry {
    uint8_t bitLength;
    uint8_t value;
    std::unique_ptr<std::array<HuffmanTableEntry, 256>> table;

    HuffmanTableEntry() : bitLength(0), value(0), table(nullptr) {};
    HuffmanTableEntry(const uint8_t bitLength, const uint8_t value) : bitLength(bitLength), value(value) {}
    HuffmanTableEntry(const uint8_t bitLength, const uint8_t value, std::unique_ptr<std::array<HuffmanTableEntry, 256>> table)
    : bitLength(bitLength), value(value), table(std::move(table)) {}
};

class HuffmanTable {
public:
    static constexpr int maxEncodingLength = 16;
    std::vector<HuffmanEncoding> encodings;
    std::unique_ptr<std::array<HuffmanTableEntry, 256>> table;
    bool isInitialized = false;
    
    HuffmanTable() = default;
    HuffmanTable(std::ifstream& file, const std::streampos& dataStartIndex);
    void generateLookupTable();
    uint8_t decodeNextValue(BitReader& bitReader) const;
    void print() const;
};

class ScanHeaderComponentSpecification {
public:
    uint8_t componentId;
    uint8_t dcTableSelector;
    uint8_t acTableSelector;

    uint8_t quantizationTableIteration;
    uint8_t dcTableIteration;
    uint8_t acTableIteration;

    ScanHeaderComponentSpecification() = default;
    ScanHeaderComponentSpecification(const uint8_t componentId, const uint8_t dcTableSelector, const uint8_t acTableSelector,
        const uint8_t quantizationTableIteration, const uint8_t dcTableIteration, const uint8_t acTableIteration)
        : componentId(componentId), dcTableSelector(dcTableSelector), acTableSelector(acTableSelector), quantizationTableIteration(quantizationTableIteration),
        dcTableIteration(dcTableIteration), acTableIteration(acTableIteration) {}
    void print() const;
};

class ScanHeader {
public:
    std::vector<ScanHeaderComponentSpecification> componentSpecifications;
    uint8_t spectralSelectionStart;
    uint8_t spectralSelectionEnd;
    uint8_t successiveApproximationHigh;
    uint8_t successiveApproximationLow;
    BitReader bitReader;
    
    ScanHeader() = default;
    ScanHeader(Jpg* jpg, const std::streampos& dataStartIndex);
    void print() const;
};

class ColorBlock {
public:
    static constexpr int colorBlockLength = 64;
    std::array<float, colorBlockLength> R {0};
    std::array<float, colorBlockLength> G {0};
    std::array<float, colorBlockLength> B {0};
    void print() const;
};

class Mcu {
public:
    static constexpr int dataUnitLength = 64;
    bool postDctMode = true; // True = After FDCT, IDCT needs to be performed
    // Component 1
    std::vector<std::shared_ptr<std::array<float, dataUnitLength>>> Y;
    // Component 2
    std::shared_ptr<std::array<float, dataUnitLength>> Cb;
    // Component 3
    std::shared_ptr<std::array<float, dataUnitLength>> Cr;
    int horizontalSampleSize = 1;
    int verticalSampleSize = 1;
    std::vector<ColorBlock> colorBlocks;

    Mcu();
    Mcu(const int luminanceComponents, const int horizontalSampleSize, const int verticalSampleSize);
    void print() const;
    std::array<int, dataUnitLength>* getComponent(int id);
    static int getColorIndex(const int blockIndex, const int pixelIndex, const int horizontalFactor, const int verticalFactor);
    void generateColorBlocks();
    std::tuple<uint8_t, uint8_t, uint8_t> getColor(int index);
    static void performInverseDCT(std::array<float, 64>& array);
    void performInverseDCT();
    static void dequantize(std::array<float, 64>& array, const QuantizationTable& quantizationTable);
    void dequantize(Jpg* jpg, const ScanHeaderComponentSpecification& scanComp);
};

class EntropyDecoder {
public:
    static int decodeSSSS(BitReader& bitReader, const int SSSS);
    static int decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable);
    static std::pair<int, int> decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable);
    static std::array<float, 64>* decodeComponent(Jpg* jpg, BitReader& bitReader, const ScanHeaderComponentSpecification& scanComp, int (&prevDc)[3]);
    static Mcu* decodeMcu(Jpg* jpg, ScanHeader& scanHeader, int (&prevDc)[3]);

    static void skipZeros(BitReader& bitReader, std::array<float, 64>*& component, int numToSkip, int& index, int approximationLow, int spectralEnd);
    static int decodeProgressiveDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable);
    static void decodeProgressiveComponent(Jpg* jpg, std::array<float, 64>* component,
        ScanHeader& scanHeader, const ScanHeaderComponentSpecification& componentInfo, int (&prevDc)[3], int& numBlocksToSkip);
};

struct ConsumerQueue {
    std::mutex mutex;
    std::condition_variable condition;
    std::queue<std::shared_ptr<Mcu>> queue;
    bool allProductsAdded = false;
};

struct AtomicCondition {
    std::mutex mutex;
    int value = -1;
    std::condition_variable condition;
};

/* TODO: Arithmetic Coding */
class Jpg {
public:
    std::ifstream file;
    FrameHeader frameHeader;
    std::string comment;
    // Index in the vector is the iteration, index in the array is the table number
    std::vector<std::array<QuantizationTable, 4>> quantizationTables =  std::vector<std::array<QuantizationTable, 4>>(1);
    std::vector<std::array<HuffmanTable, 4>> dcHuffmanTables = std::vector<std::array<HuffmanTable, 4>>(1);
    std::vector<std::array<HuffmanTable, 4>> acHuffmanTables = std::vector<std::array<HuffmanTable, 4>>(1);
    uint16_t restartInterval = 0;
    std::vector<std::shared_ptr<Mcu>> mcus;
private:   
    ConsumerQueue quantizationQueue;
    ConsumerQueue idctQuantizationQueue;
    ConsumerQueue colorConversionQueue;

    uint8_t currentQuantizationTableIteration = -1;
    uint8_t currentAcHuffmanTableIteration = -1;
    uint8_t currentDcHuffmanTableIteration = -1;
    
    std::vector<std::shared_ptr<ScanHeader>> scanHeaders;
    int currentScan = 0;
    // the value of scanIndicies[i] indicates the mcuIndex that scan #i can read up to
    std::mutex scanIndiciesMutex;
    std::vector<std::shared_ptr<AtomicCondition>> scanIndices;
    std::vector<std::unique_ptr<std::thread>> scanThreads;
    
public:
    explicit Jpg(const std::string& path);
private:
    uint16_t readLengthBytes();
    void readFrameHeader(uint8_t frameMarker);
    void readQuantizationTables();
    void readHuffmanTables();
    void readDefineNumberOfLines();
    void readDefineRestartIntervals();
    void readComments();
    void processQuantizationQueue(const std::vector<ScanHeaderComponentSpecification>& scanComps);
    void processIdctQuantizationQueue();
    void processColorConversionQueue();
    std::shared_ptr<ScanHeader> readScanHeader();
    void readBaselineStartOfScan();
    void createMcus();
    void processProgressiveStartOfScan(std::shared_ptr<ScanHeader>& scan, int scanNumber);
    void readProgressiveStartOfScan();
    void readFile();
public:
    void writeBmp(const std::string& filename) const;
    void printInfo() const;
};

