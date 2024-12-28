#pragma once
#include <fstream>
#include <iostream>
#include <vector>
#include "Color.h"

enum class BmpRasterEncoding : std::uint8_t {
    None,
    Monochrome,
    FourBitNoCompression,
    EightBitNoCompression,
    SixteenBitNoCompression,
    TwentyFourBitNoCompression,
    FourBit2Compression,
    EightBit1Compression
};

struct BmpHeader {
    static constexpr int fileHeaderOffset = 0x0;
    static constexpr int fileHeaderSize = 14;
    uint16_t signature;
    uint32_t fileSize;
    uint32_t reserved;
    uint32_t dataOffset;

    static BmpHeader getHeaderFromFile(std::ifstream& file);
};

struct BmpInfo {
    static constexpr int fileInfoHeaderPos = 0x0e;
    static constexpr int fileInfoHeaderSize = 40;
    std::uint32_t size;
    std::uint32_t width;
    std::uint32_t height;
    std::uint16_t planes;
    std::uint16_t bitCount;
    std::uint32_t compression;
    std::uint32_t imageSize;
    std::uint32_t xPixelsPerMeter;
    std::uint32_t yPixelsPerMeter;
    std::uint32_t colorsUsed;
    std::uint32_t importantColors;

    static BmpInfo getInfoFromFile(std::ifstream& file);
    BmpRasterEncoding getRasterEncoding() const;
    void print() const;
    int getNumColors() const;
};

struct BmpPoint {
    float x;
    float y;
    Color color;

    BmpPoint();
    BmpPoint(const float x, const float y, const Color& color) : x(x), y(y), color(color) {}
};

class Bmp {
    static constexpr int fileColorTableOffset = 0x36;

public:
    std::ifstream file;
    BmpHeader header;
    BmpInfo info;
    BmpRasterEncoding rasterEncoding;
    std::vector<Color> colorTable;
    uint32_t rowSize;
    
    explicit Bmp(const std::string& path);
    int render();
    
private:
    std::vector<BmpPoint> getPoints();
    void ParseRowByteOrLessNoCompression(std::vector<BmpPoint>& points, float normalizedY);
    void ParseRow24BitNoCompression(std::vector<BmpPoint>& points, float normalizedY);
    void initColorTable();
};
