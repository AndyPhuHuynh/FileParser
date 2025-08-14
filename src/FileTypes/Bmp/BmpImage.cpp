#include <filesystem>
#include <fstream>
#include <cmath>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Bmp/BmpImage.h"

#include "FileParser/FileUtil.h"
#include "FileParser/Macros.hpp"
#include "FileParser/Utils.hpp"

int FileParser::Bmp::BmpInfo::getNumColors() const {
    if (bitCount == 1) return 2;
    if (bitCount == 4) return 16;
    if (bitCount == 8) return 256;
    return -1;
}

FileParser::Bmp::BmpRasterEncoding FileParser::Bmp::BmpInfo::getRasterEncoding() const {
    if (bitCount == 1  && compression == 0) { return BmpRasterEncoding::Monochrome; }
    if (bitCount == 4  && compression == 0) { return BmpRasterEncoding::FourBitNoCompression; }
    if (bitCount == 8  && compression == 0) { return BmpRasterEncoding::EightBitNoCompression; }
    if (bitCount == 16 && compression == 0) { return BmpRasterEncoding::SixteenBitNoCompression; }
    if (bitCount == 24 && compression == 0) { return BmpRasterEncoding::TwentyFourBitNoCompression; }
    if (bitCount == 4  && compression == 2) { return BmpRasterEncoding::FourBit2Compression; }
    if (bitCount == 8  && compression == 1) { return BmpRasterEncoding::EightBit1Compression; }
    return BmpRasterEncoding::None;
}

auto FileParser::Bmp::calculateRowSize(const uint16_t bitCount, const uint32_t width) -> uint32_t {
    return static_cast<uint32_t>(floor((bitCount * width + 31) / 32)) * 4;
}

auto FileParser::Bmp::parseHeader(std::ifstream& file) -> std::expected<BmpHeader, std::string> {
    BmpHeader header;
    char signature[2] = { 0, 0 };
    CHECK_VOID_AND_RETURN(read_bytes(signature, file, 2), "Unable to parse BMP signature");
    if (signature[0] != FileUtils::bmpSig[0] || signature[1] != FileUtils::bmpSig[1]) {
        return std::unexpected("File signature did not match BMP signature");
    }

    ASSIGN_OR_RETURN(fileSize,   read_uint32_le(file), "Unable to parse file");
    ASSIGN_OR_RETURN(reserved,   read_uint32_le(file), "Unable to parse reserved bytes");
    ASSIGN_OR_RETURN(dataOffset, read_uint32_le(file), "Unable to parse data offset");

    header.fileSize   = fileSize;
    header.dataOffset = dataOffset;

    return header;
}

auto FileParser::Bmp::parseInfo(std::ifstream& file) -> std::expected<BmpInfo, std::string> {

    ASSIGN_OR_RETURN(size,     read_uint32_le(file), "Unable to parse file size");
    ASSIGN_OR_RETURN(width,    read_uint16_le(file), "Unable to parse width");
    ASSIGN_OR_RETURN(height,   read_uint16_le(file), "Unable to parse height");
    ASSIGN_OR_RETURN(planes,   read_uint16_le(file), "Unable to parse planes");
    ASSIGN_OR_RETURN(bitCount, read_uint16_le(file), "Unable to parse bit count");


    BmpInfo info;
    info.size     = size;
    info.width    = width;
    info.height   = height;
    info.planes   = planes;
    info.bitCount = bitCount;

    if (info.size == 12) {
        return info;
    }

    ASSIGN_OR_RETURN(compression,     read_uint32_le(file), "Unable to parse compression");
    ASSIGN_OR_RETURN(imageSize,       read_uint32_le(file), "Unable to parse image size");
    ASSIGN_OR_RETURN(xPixelsPerMeter, read_uint32_le(file), "Unable to parse x pixels per meter");
    ASSIGN_OR_RETURN(yPixelsPerMeter, read_uint32_le(file), "Unable to parse y pixels per meter");
    ASSIGN_OR_RETURN(colorsUsed,      read_uint32_le(file), "Unable to parse colors used");
    ASSIGN_OR_RETURN(importantColors, read_uint32_le(file), "Unable to parse important colors");

    info.compression     = compression;
    info.imageSize       = imageSize;
    info.xPixelsPerMeter = xPixelsPerMeter;
    info.yPixelsPerMeter = yPixelsPerMeter;
    info.colorsUsed      = colorsUsed;
    info.importantColors = importantColors;

    if (info.size == 40) {
        return info;
    }
    return std::unexpected(std::format("Info header size of {} bytes is not fully supported\n", info.size));
}

auto FileParser::Bmp::parseColorTable(
    std::ifstream& file, const int numColors
) -> std::expected<std::vector<Color>, std::string> {
    static constexpr int colorTableOffset = 0x36;
    file.seekg(colorTableOffset, std::ios::beg);

    auto colorTable = std::vector<Color>(numColors);
    for (int i = 0; i < numColors; i++) {
        char bgr[3];
        CHECK_VOID_AND_RETURN(read_bytes(bgr, file, std::size(bgr)), "Unable to parse color table");
        colorTable[i] = {
            .r = static_cast<uint8_t>(bgr[2]),
            .g = static_cast<uint8_t>(bgr[1]),
            .b = static_cast<uint8_t>(bgr[0]),
        };
    }

    return colorTable;
}

auto FileParser::Bmp::parseImageData(
    std::ifstream& file, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    file.seekg(bmpData.header.dataOffset, std::ios::beg);

    switch (bmpData.info.getRasterEncoding()) {
        case BmpRasterEncoding::Monochrome:
            return parseImageDataMonochrome(file, bmpData);
        case BmpRasterEncoding::FourBitNoCompression:
            return parseImageData4BitNoCompression(file, bmpData);
        case BmpRasterEncoding::EightBitNoCompression:
            return parseImageData8BitNoCompression(file, bmpData);
        case BmpRasterEncoding::SixteenBitNoCompression:
            return std::unexpected("16-bit BMP not yet supported");
        case BmpRasterEncoding::TwentyFourBitNoCompression:
            return parseImageData24Bit(file, bmpData);
        case BmpRasterEncoding::FourBit2Compression:
        case BmpRasterEncoding::EightBit1Compression:
            return std::unexpected("Raster encoding with compression no yet supported");
        case BmpRasterEncoding::None:
            return std::unexpected("No raster encoding detected");
    }
    return std::unexpected("Unexpected error");
}

auto FileParser::Bmp::parseImageDataMonochrome(
    std::ifstream& file, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;

        for (int byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            ASSIGN_OR_RETURN(byte, read_uint8(file), "Unable to parse monochrome color data");
            for (int bit = 7; bit >= 0 && pixelsInRowRead < bmpData.info.width; bit--) {
                const auto colorIndex = (byte >> bit) & 1;
                const auto [r, g, b] = bmpData.colorTable[colorIndex];

                data.push_back(r);
                data.push_back(g);
                data.push_back(b);
                pixelsInRowRead++;
            }
        }
    }

    return data;
}

auto FileParser::Bmp::parseImageData4BitNoCompression(
    std::ifstream& file, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;
        for (int byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            ASSIGN_OR_RETURN(byte, read_uint8(file), "Unable to parse 4-bit color data");
            for (int nibble = 1; nibble >= 0 && pixelsInRowRead < bmpData.info.width; nibble--) {
                const auto colorIndex = (byte >> (nibble * 4)) & 0x0F;
                const auto [r, g, b] = bmpData.colorTable[colorIndex];

                data.push_back(r);
                data.push_back(g);
                data.push_back(b);
                pixelsInRowRead++;
            }
        }
    }
    return data;
}

auto FileParser::Bmp::parseImageData8BitNoCompression(
    std::ifstream& file, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;
        for (int byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            ASSIGN_OR_RETURN(byte, read_uint8(file), "Unable to parse 8-bit color data");
            const auto [r, g, b] = bmpData.colorTable[byte];
            data.push_back(r);
            data.push_back(g);
            data.push_back(b);
            pixelsInRowRead++;
        }
    }
    return data;
}


auto FileParser::Bmp::parseImageData24Bit(
    std::ifstream& file, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string>{
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> rgbData;
    rgbData.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        for (uint32_t x = 0; x < bmpData.info.width; x++) {
            char bgr[3];
            CHECK_VOID_AND_RETURN(read_bytes(bgr, file, 3), "Unable to parse 24-bit pixel");
            // Convert bgr to rgb
            rgbData.push_back(static_cast<uint8_t>(bgr[2]));
            rgbData.push_back(static_cast<uint8_t>(bgr[1]));
            rgbData.push_back(static_cast<uint8_t>(bgr[0]));
        }

        // Skip row padding
        const auto padding = rowSize - (bmpData.info.width * 3);
        file.seekg(padding, std::ios::cur);
    }

    return rgbData;
}

auto FileParser::Bmp::decode(const std::filesystem::path& filePath) -> std::expected<Image, std::string> {
    std::ifstream file(filePath, std::ios::binary);
    BmpData bmpData;

    ASSIGN_OR_RETURN(header, parseHeader(file), "Unable to parse header");
    ASSIGN_OR_RETURN(info,   parseInfo(file),   "Unable to parse info");

    bmpData.header = header;
    bmpData.info   = info;

    if (const auto numColors = info.getNumColors(); numColors != -1) {
        ASSIGN_OR_RETURN(colorTable, parseColorTable(file, numColors), "Unable to parse color table");
        bmpData.colorTable = colorTable;
    }

    ASSIGN_OR_RETURN_MUT(rgbData, parseImageData(file, bmpData), "Unable to parse rgb data");

    return Image(bmpData.info.width, bmpData.info.height, std::move(rgbData));
}