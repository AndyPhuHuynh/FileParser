#include <fstream>
#include <iostream>
#include <cmath>
#include <memory>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Bmp/BmpImage.h"

#include <filesystem>

#include "FileParser/FileUtil.h"
#include "FileParser/Utils.hpp"

auto FileParser::Bmp::parseHeader(std::ifstream& file) -> std::expected<BmpHeader, std::string> {
    BmpHeader header;
    char signature[2] = { 0, 0 };
    const auto signatureExpected = read_bytes(signature, file, 2);
    if (!signatureExpected) {
        return utils::getUnexpected(signatureExpected, "Unable to parse BMP file signature");
    }
    if (signature[0] != FileUtils::bmpSig[0] || signature[1] != FileUtils::bmpSig[1]) {
        return std::unexpected("File signature did not match BMP signature");
    }

    const auto fileSize = read_uint32_le(file);
    if (!fileSize) {
        return utils::getUnexpected(fileSize, "Unable to parse file size");
    }
    header.fileSize = *fileSize;

    const auto reserved = read_uint32_le(file);
    if (!reserved) {
        return utils::getUnexpected(reserved, "Unable to parse reserved bytes");
    }

    const auto dataOffset = read_uint32_le(file);
    if (!dataOffset) {
        return utils::getUnexpected(dataOffset, "Unable to parse file offset");
    }
    header.dataOffset = *dataOffset;

    return header;
}

auto FileParser::Bmp::parseInfo(std::ifstream& file) -> std::expected<BmpInfo, std::string> {
    BmpInfo info;

    const auto size     = read_uint32_le(file);
    const auto width    = read_uint16_le(file);
    const auto height   = read_uint16_le(file);
    const auto planes   = read_uint16_le(file);
    const auto bitCount = read_uint16_le(file);

    if (!size) {
        return utils::getUnexpected(size, "Unable to parse file size");
    }
    if (!width) {
        return utils::getUnexpected(width, "Unable to parse file width");
    }
    if (!height) {
        return utils::getUnexpected(height, "Unable to parse file height");
    }
    if (!planes) {
        return utils::getUnexpected(planes, "Unable to parse file planes");
    }
    if (!bitCount) {
        return utils::getUnexpected(bitCount, "Unable to parse file bitCount");
    }

    info.size     = *size;
    info.width    = *width;
    info.height   = *height;
    info.planes   = *planes;
    info.bitCount = *bitCount;

    if (info.size == 12) {
        return info;
    }

    const auto compression     = read_uint32_le(file);
    const auto imageSize       = read_uint32_le(file);
    const auto xPixelsPerMeter = read_uint32_le(file);
    const auto yPixelsPerMeter = read_uint32_le(file);
    const auto colorsUsed      = read_uint32_le(file);
    const auto importantColors = read_uint32_le(file);

    if (!compression) {
        return utils::getUnexpected(compression, "Unable to parse compression");
    }
    if (!imageSize) {
        return utils::getUnexpected(imageSize, "Unable to parse image size");
    }
    if (!xPixelsPerMeter) {
        return utils::getUnexpected(xPixelsPerMeter, "Unable to parse xPixelsPerMeter");
    }
    if (!yPixelsPerMeter) {
        return utils::getUnexpected(yPixelsPerMeter, "Unable to parse yPixelsPerMeter");
    }
    if (!colorsUsed) {
        return utils::getUnexpected(colorsUsed, "Unable to parse colors used");
    }
    if (!importantColors) {
        return utils::getUnexpected(importantColors, "Unable to parse importantColors");
    }

    info.compression     = *compression;
    info.imageSize       = *imageSize;
    info.xPixelsPerMeter = *xPixelsPerMeter;
    info.yPixelsPerMeter = *yPixelsPerMeter;
    info.colorsUsed      = *colorsUsed;
    info.importantColors = *importantColors;

    if (info.size == 40) {
        return info;
    }
    return std::unexpected(std::format("Info header size of {} bytes is not fully supported\n", info.size));
}

auto FileParser::Bmp::parseColorTable(
    std::ifstream& file, const int numColors
) -> std::expected<std::vector<Color>, std::string> {
    static constexpr int fileColorTableOffset = 0x36;
    file.seekg(fileColorTableOffset, std::ios::beg);

    auto colorTable = std::vector<Color>(numColors);
    for (int i = 0; i < numColors; i++) {
        char bgr[3];
        const auto rgbExpected = read_bytes(bgr, file, std::size(bgr));
        if (!rgbExpected) {
            return utils::getUnexpected(rgbExpected, "Unable to parse color table");
        }
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

    const auto rowSize = static_cast<int>(floor((bmpData.info.bitCount * bmpData.info.width + 31) / 32)) * 4;
    std::vector<uint8_t> data;
    data.reserve((bmpData.info.width * bmpData.info.height * 3));
    const BmpRasterEncoding encoding = bmpData.info.getRasterEncoding();

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        if (encoding == BmpRasterEncoding::TwentyFourBitNoCompression) {
            const auto result = parseRow24BitNoCompression(file, data, rowSize);
            if (!result) {
                return utils::getUnexpected(result, "Unable to parse row");
            }
        }
        else {
            const auto result = parseRowByteOrLessNoCompression(file, data, bmpData, rowSize);
        }
    }
    return data;
}

auto FileParser::Bmp::parseRow24BitNoCompression(
    std::ifstream& file, std::vector<uint8_t>& data, const int rowSize
) -> std::expected<void, std::string>  {
    for (int x = 0; x < rowSize / 3; x++) {
        char bgr[3];
        const auto rgbExpected = read_bytes(bgr, file, std::size(bgr));
        if (!rgbExpected) {
            return utils::getUnexpected(rgbExpected, "Unable to parse image data");
        }
        data.insert(data.end(), std::rbegin(bgr), std::rend(bgr));
    }
    const uint32_t padding = rowSize % 3;
    file.seekg(padding, std::ios::cur);
    return {};
}

auto FileParser::Bmp::parseRowByteOrLessNoCompression(
    std::ifstream& file, std::vector<uint8_t>& data, const BmpData& bmpData, const int rowSize
) -> std::expected<void, std::string> {
    uint32_t pixelsInRowRead = 0;
    for (int byteInRow = 0; byteInRow < rowSize; byteInRow++) {
        const auto byte = read_uint8(file);
        if (!byte) {
            return utils::getUnexpected(byte, "Unable to parse row");
        }

        const int pixelsPerByte = 8 / bmpData.info.bitCount;
        // Process bits
        for (int i = 0; i < pixelsPerByte; i++) {
            if (pixelsInRowRead >= bmpData.info.width) {
                return {};
            }

            unsigned char index;
            switch (bmpData.info.getRasterEncoding()) {
                case BmpRasterEncoding::Monochrome: {
                    index = GetBitFromLeft(*byte, i);
                    break;
                }
                case BmpRasterEncoding::FourBitNoCompression: {
                    index = GetNibble(*byte, i);
                    break;
                }
                default: {
                    index = *byte;
                    break;
                }
            }
            auto [r, g, b] = bmpData.colorTable[index];
            data.push_back(r);
            data.push_back(g);
            data.push_back(b);
            pixelsInRowRead++;
        }
    }
    return {};
}

auto FileParser::Bmp::decode(const std::filesystem::path& filePath) -> std::expected<Image, std::string> {
    std::ifstream file(filePath, std::ios::binary);
    BmpData bmpData;

    const auto header = parseHeader(file);
    if (!header) {
        return utils::getUnexpected(header, "Unable to parse header");
    }
    bmpData.header = *header;

    const auto info = parseInfo(file);
    if (!info) {
        return utils::getUnexpected(info, "Unable to parse info");
    }
    bmpData.info = *info;

    if (const auto numColors = info->getNumColors(); numColors != -1) {
        const auto colorTable = parseColorTable(file, numColors);
        if (!colorTable) {
            return utils::getUnexpected(colorTable, "Unable to parse color table");
        }
        bmpData.colorTable = *colorTable;
    }


    const auto rgbData = parseImageData(file, bmpData);
    if (!rgbData) {
        return utils::getUnexpected(rgbData, "Unable to parse image data");
    }

    return Image(bmpData.info.width, bmpData.info.height, *rgbData);
}

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