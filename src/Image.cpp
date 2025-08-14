#include "FileParser/Image.hpp"

#include <algorithm>

uint8_t& FileParser::getPixel(Image& image, const uint32_t x, const uint32_t y, const uint32_t channel) {
    constexpr size_t numChannels = 3;
    return image.data[y * image.width * numChannels + x * numChannels + channel];
}

void swapPixel(
    FileParser::Image& image,
    const uint32_t x1, const uint32_t y1,
    const uint32_t x2, const uint32_t y2,
    const uint32_t numChannels) {
    for (uint32_t i = 0; i < numChannels; i++) {
        std::swap(getPixel(image, x1, y1, i), getPixel(image, x2, y2, i));
    }
}

void FileParser::flipVertically(Image& image) {
    const size_t bytesPerRow = image.width * 3;
    for (uint32_t y = 0; y < image.height / 2; y++) {
        const auto topRow = image.data.begin() + static_cast<std::ptrdiff_t>(y * bytesPerRow);
        const auto bottomRow = image.data.begin() + static_cast<std::ptrdiff_t>((image.height - y - 1) * bytesPerRow);
        std::swap_ranges(topRow, topRow + static_cast<std::ptrdiff_t>(bytesPerRow), bottomRow);
    }
}

void FileParser::flipHorizontally(Image& image) {
    constexpr size_t numChannels = 3;
    for (uint32_t x = 0; x < image.width / 2; x++) {
        for (uint32_t y = 0; y < image.height; y++) {
            swapPixel(image, x, y, image.width - 1 - x, y, numChannels);
        }
    }
}
