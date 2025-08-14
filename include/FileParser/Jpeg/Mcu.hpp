#pragma once

#include <array>
#include <vector>

namespace FileParser::Jpeg {
    struct Component {
        static constexpr size_t length = 64;
        std::array<float, length> data{};

        auto operator[](size_t index) -> float&;
        auto operator[](size_t index) const -> const float&;
    };

    struct RGBBlock {
        Component R{};
        Component G{};
        Component B{};
        void rgbToYCbCr();
    };

    struct Mcu {
        std::vector<Component> Y;
        Component Cb{};
        Component Cr{};
        int horizontalSampleSize = 1;
        int verticalSampleSize = 1;

        Mcu();
        Mcu(int horizontalSampleSize_, int verticalSampleSize_);
        explicit Mcu(const RGBBlock& colorBlock);

        // Given the index of the luminance block and the pixel index (0-63) within that block, get the index to the
        // corresponding pixel in Cb and Cr
        [[nodiscard]] size_t getColorIndex(size_t blockIndex, size_t pixelIndex) const;
    };
}
