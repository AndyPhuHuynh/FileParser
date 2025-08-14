#pragma once
#include <cstdint>
#include <vector>

namespace FileParser {
    struct Image {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<uint8_t> data;

        Image(const uint32_t width_, const uint32_t height_, std::vector<uint8_t> data_)
            : width(width_), height(height_), data(std::move(data_)) {}
    };

    uint8_t& getPixel(Image& image, uint32_t x, uint32_t y, uint32_t channel);
    void flipVertically(Image& image);
    void flipHorizontally(Image& image);
}
