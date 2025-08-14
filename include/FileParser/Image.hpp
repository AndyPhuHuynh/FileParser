#pragma once
#include <vector>

namespace FileParser {
    struct Image {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> data;

        Image(const uint32_t width, const uint32_t height, std::vector<uint8_t> data)
            : width(width), height(height), data(std::move(data)) {}
    };

    uint8_t& getPixel(Image& image, uint32_t x, uint32_t y, uint32_t channel);
    void flipVertically(Image& image);
    void flipHorizontally(Image& image);
}
