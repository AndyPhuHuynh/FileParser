#pragma once

#include <expected>
#include <filesystem>
#include <fstream>
#include <vector>

#include "FileParser/Image.hpp"

namespace FileParser::Bmp {
    struct Color {
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
    };

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
        uint32_t fileSize   = 0;
        uint32_t dataOffset = 0;
    };

    struct BmpInfo {
        std::uint32_t size            = 0;
        std::uint32_t width           = 0;
        std::uint32_t height          = 0;
        std::uint16_t planes          = 0;
        std::uint16_t bitCount        = 0;
        std::uint32_t compression     = 0;
        std::uint32_t imageSize       = 0;
        std::uint32_t xPixelsPerMeter = 0;
        std::uint32_t yPixelsPerMeter = 0;
        std::uint32_t colorsUsed      = 0;
        std::uint32_t importantColors = 0;

        BmpInfo() = default;
        [[nodiscard]] auto getNumColors() const -> int;
        [[nodiscard]] auto getRasterEncoding() const -> BmpRasterEncoding;
    };

    struct BmpData {
        BmpHeader header;
        BmpInfo info;
        std::vector<Color> colorTable;
    };

    auto parseHeader(std::ifstream& file) -> std::expected<BmpHeader, std::string>;
    auto parseInfo(std::ifstream& file) -> std::expected<BmpInfo, std::string>;
    auto parseColorTable(std::ifstream& file, int numColors) -> std::expected<std::vector<Color>, std::string>;
    auto parseImageData(std::ifstream& file, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;

    auto parseRow24BitNoCompression(std::ifstream& file, std::vector<uint8_t>& data, int rowSize) -> std::expected<void, std::string>;
    auto parseRowByteOrLessNoCompression(std::ifstream& file, std::vector<uint8_t>& data, const BmpData& bmpData, int rowSize) -> std::expected<void, std::string>;

    auto decode(const std::filesystem::path& filePath) -> std::expected<Image, std::string>;
}
