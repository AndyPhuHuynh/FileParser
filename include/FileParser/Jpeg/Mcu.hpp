#pragma once

#include <array>
#include <vector>

namespace FileParser::Jpeg {
    class JpegImage;
    struct QuantizationTable;

    struct Component {
        static constexpr size_t length = 64;
        std::array<float, length> data{};

        auto operator[](size_t index) -> float&;
        auto operator[](size_t index) const -> const float&;
    };

    class ColorBlock {
    public:
        static constexpr int colorBlockLength = 64;
        std::array<float, colorBlockLength> R{};
        std::array<float, colorBlockLength> G{};
        std::array<float, colorBlockLength> B{};
        void rgbToYCbCr();
    };

    class Mcu {
    public:
        static constexpr int DataUnitLength = 64;
        std::vector<Component> Y; // Component 1
        Component Cb{}; // Component 2
        Component Cr{}; // Component 3
        int horizontalSampleSize = 1;
        int verticalSampleSize = 1;

        Mcu();
        Mcu(int horizontalSampleSize, int verticalSampleSize);
        explicit Mcu(const ColorBlock& colorBlock);

        // Given the index of the luminance block and the pixel index (0-63) within that block, get the index to the
        // corresponding pixel in Cb and Cr
        [[nodiscard]] int getColorIndex(int blockIndex, int pixelIndex) const;
    };

    auto generateColorBlocks(const Mcu& mcu) -> std::vector<ColorBlock>;
}
