// ReSharper disable All
#pragma once
#include <array>
#include <fstream>
#include <map>
#include <numbers>
#include <unordered_map>
#include <vector>

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
const float m0 = 2.0 * std::cos(1.0 / 16.0 * 2.0 * std::numbers::pi);
const float m1 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi);
const float m3 = 2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi);
const float m5 = 2.0 * std::cos(3.0 / 16.0 * 2.0 * std::numbers::pi);
const float m2 = m0 - m5;
const float m4 = m0 + m5;

const float s0 = std::cos(0.0 / 16.0 * std::numbers::pi) / std::sqrt(8);
const float s1 = std::cos(1.0 / 16.0 * std::numbers::pi) / 2.0;
const float s2 = std::cos(2.0 / 16.0 * std::numbers::pi) / 2.0;
const float s3 = std::cos(3.0 / 16.0 * std::numbers::pi) / 2.0;
const float s4 = std::cos(4.0 / 16.0 * std::numbers::pi) / 2.0;
const float s5 = std::cos(5.0 / 16.0 * std::numbers::pi) / 2.0;
const float s6 = std::cos(6.0 / 16.0 * std::numbers::pi) / 2.0;
const float s7 = std::cos(7.0 / 16.0 * std::numbers::pi) / 2.0;

class Jpg;

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

    FrameHeader() = default;
    FrameHeader(uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex);
    void print() const;
};

class QuantizationTable {
public:
    static constexpr int tableLength = 64;
    std::array<uint8_t, tableLength> table8;
    std::array<uint16_t, tableLength> table16;
    bool is8Bit;

    QuantizationTable() = default;
    QuantizationTable(std::ifstream& file, const std::streampos& dataStartIndex, bool is8Bit);
    void printTable() const;
};

class HuffmanEncoding {
public:
    uint16_t encoding;
    uint8_t bitLength;
    uint8_t value;
    HuffmanEncoding(const uint16_t encoding, const uint8_t bitLength, const uint8_t value)
        : encoding(encoding), bitLength(bitLength), value(value) {}
};

class HuffmanNode {
public:
    std::unique_ptr<HuffmanNode> left;
    std::unique_ptr<HuffmanNode> right;
private:
    bool isNodeSet;
    uint8_t value;

public:
    HuffmanNode() : isNodeSet(false), value(0), left(nullptr), right(nullptr) {}
    explicit HuffmanNode(const uint8_t value)
    : value(value), left(nullptr), right(nullptr) {}
    bool isLeaf() const;
    uint8_t getValue() const;
    void setValue(const uint8_t value);
    bool isSet() const;
};

class HuffmanTree {
public:
    std::unique_ptr<HuffmanNode> root;

    HuffmanTree();
    explicit HuffmanTree(const std::vector<HuffmanEncoding>& encodings);
    void addEncoding(const HuffmanEncoding& encoding);
    void printTree() const;
    static void printTree(const HuffmanNode&, const std::string& currentCode);
    uint8_t decodeNextValue(BitReader& bitReader);
};

class HuffmanTable {
public:
    static constexpr int maxEncodingLength = 16;
    std::vector<HuffmanEncoding> encodings;
    HuffmanTree tree;
    
    HuffmanTable() = default;
    HuffmanTable(std::ifstream& file, const std::streampos& dataStartIndex);
};

class ScanHeaderComponentSpecification {
public:
    uint8_t componentId;
    uint8_t dcTableSelector;
    uint8_t acTableSelector;

    ScanHeaderComponentSpecification() = default;
    ScanHeaderComponentSpecification(const uint8_t componentId, const uint8_t dcTableSelector, const uint8_t acTableSelector)
        : componentId(componentId), dcTableSelector(dcTableSelector), acTableSelector(acTableSelector) {}
    void print() const;
};

class ScanHeader {
public:
    std::vector<ScanHeaderComponentSpecification> componentSpecifications;
    uint8_t spectralSelectionStart;
    uint8_t spectralSelectionEnd;
    uint8_t successiveApproximationHigh;
    uint8_t successiveApproximationLow;

    ScanHeader() = default;
    ScanHeader(std::ifstream& file, const std::streampos& dataStartIndex);
    void print() const;
};

class DataUnit {
public:
    static constexpr int dataUnitLength = 64;
    bool rgbMode = false;
    bool postDctMode = true; // True = After FDCT, IDCT needs to be performed
    // Component 1
    union {
        std::array<int, dataUnitLength> Y = {0};
        std::array<int, dataUnitLength> R;
    };
    // Component 2
    union {
        std::array<int, dataUnitLength> Cb = {0};
        std::array<int, dataUnitLength> G;
    };
    // Component 3
    union {
        std::array<int, dataUnitLength> Cr = {0};
        std::array<int, dataUnitLength> B;
    };

    void print() const;
    std::array<int, dataUnitLength>* getComponent(int id);
    void convertToRGB();
    void performInverseDCT();
    void dequantize(const std::vector<QuantizationTable>& quantizationTables);
};

class EntropyDecoder {
public:
    static int decodeSSSS(BitReader& bitReader, const int SSSS);
    static int decodeDcCoefficient(BitReader& bitReader, HuffmanTable& huffmanTable);
    static std::pair<int, int> decodeAcCoefficient(BitReader& bitReader, HuffmanTable& huffmanTable);
    static DataUnit decodeMcu(Jpg* jpg, BitReader& bitReader, int (&prevDc)[3]);
};

/* TODO: Arithmetic Coding */
// TODO: Hierarchical mode 
class Jpg {
public:
    std::ifstream file;
    FrameHeader frameHeader;
    ScanHeader scanHeader;
    std::string comment;
    std::array<QuantizationTable, 4> quantizationTables;
    std::array<HuffmanTable, 4> dcHuffmanTables;
    std::array<HuffmanTable, 4> acHuffmanTables;
    uint16_t restartInterval = 0;
    std::vector<DataUnit> mcus;
    
    explicit Jpg(const std::string& path);
    uint16_t readLengthBytes();
    void readFrameHeader(uint8_t frameMarker);
    void readQuantizationTables();
    void readHuffmanTables();
    void readDefineNumberOfLines();
    void readDefineRestartIntervals();
    void readComments();
    void readScanHeader();
    void readStartOfScan();
    void readFile();
    // int render() const;
    void writeBmp(FrameHeader* image, const std::string& filename, std::vector<DataUnit>& mcu);
    void printInfo() const;
};

