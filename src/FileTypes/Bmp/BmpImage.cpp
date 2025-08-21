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

auto FileParser::Bmp::parseHeader(IO::ByteSpanReader& reader) -> std::expected<BmpHeader, std::string> {
    BmpHeader header;
    uint8_t signature[2] = { 0, 0 };
    BYTEREADER_CALL_VOID_OR_RETURN(reader, read_into(signature, 2), std::unexpected("Unable to parse BMP signature"));
    if (!FileUtils::matchesSignature(FileUtils::bmpSig, signature)) {
        return std::unexpected("File signature did not match BMP signature");
    }
    BYTEREADER_READ_OR_RETURN(                 const, fileSize,   reader, read_le<uint32_t>(), std::unexpected("Unable to parse file"));
    BYTEREADER_READ_OR_RETURN([[maybe_unused]] const, reserved,   reader, read_le<uint32_t>(), std::unexpected("Unable to parse reserved bytes"));
    BYTEREADER_READ_OR_RETURN(                 const, dataOffset, reader, read_le<uint32_t>(), std::unexpected("Unable to parse data offset"));

    header.fileSize   = fileSize;
    header.dataOffset = dataOffset;

    return header;
}

auto FileParser::Bmp::parseInfo(IO::ByteSpanReader& reader) -> std::expected<BmpInfo, std::string> {
    BYTEREADER_READ_OR_RETURN(const, size,     reader, read_le<uint32_t>(), std::unexpected("Unable to parse file size"));
    BYTEREADER_READ_OR_RETURN(const, width,    reader, read_le<uint16_t>(), std::unexpected("Unable to parse width"));
    BYTEREADER_READ_OR_RETURN(const, height,   reader, read_le<uint16_t>(), std::unexpected("Unable to parse height"));
    BYTEREADER_READ_OR_RETURN(const, planes,   reader, read_le<uint16_t>(), std::unexpected("Unable to parse planes"));
    BYTEREADER_READ_OR_RETURN(const, bitCount, reader, read_le<uint16_t>(), std::unexpected("Unable to parse bit count"));

    BmpInfo info;
    info.size     = size;
    info.width    = width;
    info.height   = height;
    info.planes   = planes;
    info.bitCount = bitCount;

    if (info.size == 12) {
        return info;
    }

    BYTEREADER_READ_OR_RETURN(const, compression,     reader, read_le<uint32_t>(), std::unexpected("Unable to parse compression"));
    BYTEREADER_READ_OR_RETURN(const, imageSize,       reader, read_le<uint32_t>(), std::unexpected("Unable to parse image size"));
    BYTEREADER_READ_OR_RETURN(const, xPixelsPerMeter, reader, read_le<uint32_t>(), std::unexpected("Unable to parse x pixels per meter"));
    BYTEREADER_READ_OR_RETURN(const, yPixelsPerMeter, reader, read_le<uint32_t>(), std::unexpected("Unable to parse y pixels per meter"));
    BYTEREADER_READ_OR_RETURN(const, colorsUsed,      reader, read_le<uint32_t>(), std::unexpected("Unable to parse colors used"));
    BYTEREADER_READ_OR_RETURN(const, importantColors, reader, read_le<uint32_t>(), std::unexpected("Unable to parse important colors"));

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
    IO::ByteSpanReader reader, const size_t numColors
) -> std::expected<std::vector<Color>, std::string> {
    static constexpr size_t colorTableOffset = 0x36;
    reader.set_pos(colorTableOffset);
    if (reader.has_failed()) {
        return std::unexpected("Unable to parse BMP color table");
    }

    auto colorTable = std::vector<Color>(numColors);
    for (size_t i = 0; i < numColors; i++) {
        uint8_t bgr[3];
        BYTEREADER_CALL_VOID_OR_RETURN(reader, read_into(bgr, std::size(bgr)), std::unexpected("Unable to parse color table"));
        colorTable[i] = {
            .r = bgr[2],
            .g = bgr[1],
            .b = bgr[0],
        };
    }

    return colorTable;
}

auto FileParser::Bmp::parseImageData(
    IO::ByteSpanReader& reader, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    reader.set_pos(bmpData.header.dataOffset);
    if (reader.has_failed()) {
        return std::unexpected(std::format("Unable to jump to data offset position: {:#X}", bmpData.header.dataOffset));
    }

    switch (bmpData.info.getRasterEncoding()) {
        case BmpRasterEncoding::Monochrome:
            return parseImageDataMonochrome(reader, bmpData);
        case BmpRasterEncoding::FourBitNoCompression:
            return parseImageData4BitNoCompression(reader, bmpData);
        case BmpRasterEncoding::EightBitNoCompression:
            return parseImageData8BitNoCompression(reader, bmpData);
        case BmpRasterEncoding::SixteenBitNoCompression:
            return std::unexpected("16-bit BMP not yet supported");
        case BmpRasterEncoding::TwentyFourBitNoCompression:
            return parseImageData24Bit(reader, bmpData);
        case BmpRasterEncoding::FourBit2Compression:
        case BmpRasterEncoding::EightBit1Compression:
            return std::unexpected("Raster encoding with compression no yet supported");
        case BmpRasterEncoding::None:
            return std::unexpected("No raster encoding detected");
    }
    return std::unexpected("Unexpected error");
}

auto FileParser::Bmp::parseImageDataMonochrome(
    IO::ByteSpanReader& reader, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;

        for (uint32_t byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            BYTEREADER_READ_OR_RETURN(const, byte, reader, read_u8(), std::unexpected("Unable to parse monochrome color data"));
            for (int bit = 7; bit >= 0 && pixelsInRowRead < bmpData.info.width; bit--) {
                const auto colorIndex = static_cast<size_t>((byte >> bit) & 1);
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
    IO::ByteSpanReader& reader, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;
        for (size_t byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            BYTEREADER_READ_OR_RETURN(const, byte, reader, read_u8(), std::unexpected("Unable to parse 4-bit color data"));
            for (int nibble = 1; nibble >= 0 && pixelsInRowRead < bmpData.info.width; nibble--) {
                const auto colorIndex = static_cast<size_t>((byte >> (nibble * 4)) & 0x0F);
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
    IO::ByteSpanReader reader, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string> {
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> data;
    data.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        uint32_t pixelsInRowRead = 0;
        for (size_t byteInRow = 0; byteInRow < rowSize && pixelsInRowRead < bmpData.info.width; byteInRow++) {
            BYTEREADER_READ_OR_RETURN(const, byte, reader, read_u8(), std::unexpected("Unable to parse 8-bit color data"));
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
    IO::ByteSpanReader reader, const BmpData& bmpData
) -> std::expected<std::vector<uint8_t>, std::string>{
    const auto rowSize = calculateRowSize(bmpData.info.bitCount, bmpData.info.width);
    const auto pixelCount = bmpData.info.width * bmpData.info.height;

    std::vector<uint8_t> rgbData;
    rgbData.reserve(pixelCount * 3);

    for (uint32_t y = 0; y < bmpData.info.height; y++) {
        for (uint32_t x = 0; x < bmpData.info.width; x++) {
            uint8_t bgr[3];
            BYTEREADER_CALL_VOID_OR_RETURN(reader, read_into(bgr, std::size(bgr)), std::unexpected("Unable to parse color table"));
            // Convert bgr to rgb
            rgbData.push_back(static_cast<uint8_t>(bgr[2]));
            rgbData.push_back(static_cast<uint8_t>(bgr[1]));
            rgbData.push_back(static_cast<uint8_t>(bgr[0]));
        }

        // Skip row padding
        const auto padding = rowSize - (bmpData.info.width * 3);
        reader.forward_pos(padding);
        if (reader.has_failed()) {
            return std::unexpected(std::format("Unable to skip forwarding padding {} bytes", padding));
        }
    }

    return rgbData;
}

auto FileParser::Bmp::decode(IO::ByteSpanReader& reader) -> std::expected<Image, std::string> {
    BmpData bmpData;

    ASSIGN_OR_RETURN(header, parseHeader(reader), "Unable to parse header");
    ASSIGN_OR_RETURN(info,   parseInfo(reader),   "Unable to parse info");

    bmpData.header = header;
    bmpData.info   = info;

    if (const auto numColors = info.getNumColors(); numColors != -1) {
        ASSIGN_OR_RETURN(colorTable, parseColorTable(reader, static_cast<size_t>(numColors)), "Unable to parse color table");
        bmpData.colorTable = colorTable;
    }

    ASSIGN_OR_RETURN_MUT(rgbData, parseImageData(reader, bmpData), "Unable to parse rgb data");
    auto img = Image(bmpData.info.width, bmpData.info.height, std::move(rgbData));
    flipVertically(img);
    return img;
}