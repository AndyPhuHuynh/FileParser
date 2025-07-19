#include <fstream>
#include <iostream>
#include <cmath>
#include <memory>

#include "BitManipulationUtil.h"
#include "Bmp/BmpImage.h"
#include "Gui/Renderer.h"
#include "Gui/RenderWindow.h"

ImageProcessing::Bmp::BmpHeader ImageProcessing::Bmp::BmpHeader::getHeaderFromFile(std::ifstream& file) {
    BmpHeader header;
    file.seekg(fileHeaderOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&header.signature), 2);
    file.read(reinterpret_cast<char*>(&header.fileSize), 4);
    file.read(reinterpret_cast<char*>(&header.reserved), 4);
    file.read(reinterpret_cast<char*>(&header.dataOffset), 4);
    return header;
}

ImageProcessing::Bmp::BmpInfo ImageProcessing::Bmp::BmpInfo::getInfoFromFile(std::ifstream& file) {
    auto info = BmpInfo();
    file.seekg(fileInfoHeaderPos, std::ios::beg);
    file.read(reinterpret_cast<char*>(&info.size), 4);
    if (info.size == 12) {
        std::cout << "File uses BITMAPCOREHEADER\n";
        file.read(reinterpret_cast<char*>(&info.width), 2);
        file.read(reinterpret_cast<char*>(&info.height), 2);
        file.read(reinterpret_cast<char*>(&info.planes), 2);
        file.read(reinterpret_cast<char*>(&info.bitCount), 2);
        return info;
    }
    if (info.size == 40) {
        std::cout << "File uses BITMAPINFOHEADER\n";
    } else {
        std::cout << "Info header size of " << info.size << " bytes is not fully supported\n";
        std::cout << "Attempting to read file with BITMAPINFOHEADER format\n";
    }
    file.read(reinterpret_cast<char*>(&info.width), 4);
    file.read(reinterpret_cast<char*>(&info.height), 4);
    file.read(reinterpret_cast<char*>(&info.planes), 2);
    file.read(reinterpret_cast<char*>(&info.bitCount), 2);
    file.read(reinterpret_cast<char*>(&info.compression), 4);
    file.read(reinterpret_cast<char*>(&info.imageSize), 4);
    file.read(reinterpret_cast<char*>(&info.xPixelsPerMeter), 4);
    file.read(reinterpret_cast<char*>(&info.yPixelsPerMeter), 4);
    file.read(reinterpret_cast<char*>(&info.colorsUsed), 4);
    file.read(reinterpret_cast<char*>(&info.importantColors), 4);
    return info;
}

ImageProcessing::Bmp::BmpRasterEncoding ImageProcessing::Bmp::BmpInfo::getRasterEncoding() const {
    if (bitCount == 1 && compression == 0) { return BmpRasterEncoding::Monochrome; }
    if (bitCount == 4 && compression == 0) { return BmpRasterEncoding::FourBitNoCompression; }
    if (bitCount == 8 && compression == 0) { return BmpRasterEncoding::EightBitNoCompression; }
    if (bitCount == 16 && compression == 0) { return BmpRasterEncoding::SixteenBitNoCompression; }
    if (bitCount == 24 && compression == 0) { return BmpRasterEncoding::TwentyFourBitNoCompression; }
    if (bitCount == 4 && compression == 2) { return BmpRasterEncoding::FourBit2Compression; }
    if (bitCount == 8 && compression == 1) { return BmpRasterEncoding::EightBit1Compression; }
    return BmpRasterEncoding::None;
}

void ImageProcessing::Bmp::BmpInfo::print() const {
    std::cout << "Size: " << size << '\n';
    std::cout << "Width: " << width << '\n';
    std::cout << "Height: " << height << '\n';
    std::cout << "Planes: " << planes << '\n';
    std::cout << "Bits per pixel: " << bitCount << '\n';
    std::cout << "Compression: " << compression << '\n';
    if (size == 12) return;
    std::cout << "Image size: " << imageSize << '\n';
    std::cout << "xPixelsPerMeter: " << xPixelsPerMeter << '\n';
    std::cout << "yPixelsPerMeter: " << yPixelsPerMeter << '\n';
    std::cout << "Colors used: " << colorsUsed << '\n';
    std::cout << "Important colors: " << importantColors << '\n';
}

int ImageProcessing::Bmp::BmpInfo::getNumColors() const {
    if (bitCount == 1) {
        return 2;
    }
    if (bitCount == 4) {
        return 16;
    }
    if (bitCount == 8) {
        return 256;
    }
    return -1;
}

void ImageProcessing::Bmp::BmpImage::initColorTable() {
    const int colorCount = info.getNumColors();
    colorTable = std::vector<Color>(colorCount);
    file.seekg(fileColorTableOffset, std::ios::beg);

    for (int i = 0; i < colorCount; i++) {
        colorTable[i] = Color::readFromFile(file);
    }
}

std::shared_ptr<std::vector<Point>> ImageProcessing::Bmp::BmpImage::getPoints() {
    auto points = std::make_shared<std::vector<Point>>();
    points->reserve(static_cast<int>(info.width * info.height));
    file.seekg(header.dataOffset, std::ios::beg);
    
    for (int y = static_cast<int>(info.height) - 1; y >= 0; y--) {
        if (rasterEncoding == BmpRasterEncoding::TwentyFourBitNoCompression) {
            ParseRow24BitNoCompression(points, y);
        } 
        else {
            ParseRowByteOrLessNoCompression(points, y);
        }
    }
    return points;
}

void ImageProcessing::Bmp::BmpImage::ParseRowByteOrLessNoCompression(const std::shared_ptr<std::vector<Point>>& points, const int y) {
    uint32_t pixelsInRowRead = 0;
    bool allNonPaddingBitsRead = false;
    for (uint32_t byteInRow = 0; byteInRow < rowSize; byteInRow++) {
        unsigned char byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
            
        if (allNonPaddingBitsRead) {
            continue;
        }

        const int pixelsPerByte = 8 / info.bitCount;
        // Process bits
        for (int i = 0; i < pixelsPerByte; i++) {
            if (pixelsInRowRead >= info.width) {
                allNonPaddingBitsRead = true;
                break;
            }

            const int x = static_cast<int>(byteInRow) * pixelsPerByte + i;
            unsigned char index;
            if (rasterEncoding == BmpRasterEncoding::Monochrome) {
                index = GetBitFromLeft(byte, i);
            } else if (rasterEncoding == BmpRasterEncoding::FourBitNoCompression) {
                index = GetNibble(byte, i);
            } else {
                index = byte;
            }
            Color color = colorTable[index];

            points->emplace_back(static_cast<float>(x), static_cast<float>(y), color);
                
            pixelsInRowRead++;
        }
    }
}

void ImageProcessing::Bmp::BmpImage::ParseRow24BitNoCompression(const std::shared_ptr<std::vector<Point>>& points, const int y) {
    for (uint32_t x = 0; x < rowSize / 3; x++) {
        unsigned char byte;
        Color color;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.b = byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.g = byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.r = byte;
        color.a = 255.0f;

        points->emplace_back(static_cast<float>(x), static_cast<float>(y), color);
    }
    const uint32_t padding = rowSize % 3;
    file.seekg(padding, std::ios::cur);
}

ImageProcessing::Bmp::BmpImage::BmpImage(const std::string& path) {
    file = std::ifstream(path, std::ios::binary);
    header = BmpHeader::getHeaderFromFile(file);
    info = BmpInfo::getInfoFromFile(file);
    rasterEncoding = info.getRasterEncoding();
    if (rasterEncoding == BmpRasterEncoding::Monochrome ||
        rasterEncoding == BmpRasterEncoding::FourBitNoCompression ||
        rasterEncoding == BmpRasterEncoding::EightBitNoCompression) {
        initColorTable();
    }
    rowSize = static_cast<uint32_t>(floor(((info.bitCount * info.width + 31) / 32))) * 4;
}