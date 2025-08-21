#pragma once

#include <expected>
#include <filesystem>
#include <vector>

#include "FileParser/ByteReader.hpp"
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

    auto calculateRowSize(uint16_t bitCount, uint32_t width) -> uint32_t;

    auto parseHeader(IO::ByteSpanReader& reader) -> std::expected<BmpHeader, std::string>;
    auto parseInfo(IO::ByteSpanReader& reader) -> std::expected<BmpInfo, std::string>;
    auto parseColorTable(IO::ByteSpanReader reader, size_t numColors) -> std::expected<std::vector<Color>, std::string>;
    auto parseImageData(IO::ByteSpanReader& reader, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;

    auto parseImageDataMonochrome(IO::ByteSpanReader& reader, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;
    auto parseImageData4BitNoCompression(IO::ByteSpanReader& reader, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;
    auto parseImageData8BitNoCompression(IO::ByteSpanReader reader, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;
    auto parseImageData24Bit(IO::ByteSpanReader reader, const BmpData& bmpData) -> std::expected<std::vector<uint8_t>, std::string>;

    auto decode(IO::ByteSpanReader& reader) -> std::expected<Image, std::string>;
}
