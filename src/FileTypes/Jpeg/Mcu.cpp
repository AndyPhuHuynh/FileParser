#include "FileParser/Jpeg/Mcu.hpp"

#include <algorithm>
#include <cmath>
#include <ranges>

#include "FileParser/Jpeg/Transform.hpp"

auto FileParser::Jpeg::Component::operator[](const size_t index) -> float& {
    return data[index];
}

auto FileParser::Jpeg::Component::operator[](const size_t index) const -> const float& {
    return data[index];
}

void FileParser::Jpeg::RGBBlock::rgbToYCbCr() {
    for (size_t i = 0; i < Component::length; i++) {
        auto [y, cb, cr] = RGBToYCbCr(R[i], G[i], B[i]);
        R[i] = y;
        G[i] = cb;
        B[i] = cr;
    }
}

size_t FileParser::Jpeg::Mcu::getColorIndex(const size_t blockIndex, const size_t pixelIndex) const {
    const size_t blockRow = blockIndex / horizontalSampleSize;
    const size_t blockCol = blockIndex % horizontalSampleSize;

    constexpr size_t blockWidth  = 8;
    constexpr size_t blockHeight = 8;
    const size_t pixelRow = pixelIndex / blockWidth;
    const size_t pixelCol = pixelIndex % blockWidth;

    const size_t chromaRow = (blockRow * blockHeight + pixelRow) / verticalSampleSize;
    const size_t chromaCol = (blockCol * blockWidth + pixelCol) / horizontalSampleSize;

    return chromaRow * blockHeight + chromaCol;
}

FileParser::Jpeg::Mcu::Mcu() {
    Y.push_back({});
}

FileParser::Jpeg::Mcu::Mcu(const int horizontalSampleSize, const int verticalSampleSize) {
    for (int i = 0; i < horizontalSampleSize * verticalSampleSize; i++) {
        Y.push_back({});
    }
    this->horizontalSampleSize = horizontalSampleSize;
    this->verticalSampleSize = verticalSampleSize;
}

FileParser::Jpeg::Mcu::Mcu(const RGBBlock& colorBlock) {
    RGBBlock copy = colorBlock;
    copy.rgbToYCbCr();
    Y.push_back({});
    for (size_t i = 0; i < Component::length; i++) {
        Y[0][i] = copy.R[i];
        Cb[i]   = copy.G[i];
        Cr[i]   = copy.B[i];
    }
}

