#pragma once

#include <array>
#include <vector>

namespace FileParser::Jpeg {
    class JpegImage;
    class ScanHeaderComponentSpecification;
    class ColorBlock;
    struct QuantizationTable;

    struct Component {
        static constexpr size_t length = 64;
        std::array<float, length> data{};

        auto operator[](size_t index) -> float&;
        auto operator[](size_t index) const -> const float&;
    };

    class Mcu {
    public:
        static constexpr int DataUnitLength = 64;
        std::vector<Component> Y; // Component 1
        Component Cb{}; // Component 2
        Component Cr{}; // Component 3
        int horizontalSampleSize = 1;
        int verticalSampleSize = 1;
        std::vector<ColorBlock> colorBlocks;

        Mcu();
        Mcu(int luminanceComponents, int horizontalSampleSize, int verticalSampleSize);
        explicit Mcu(const ColorBlock& colorBlock);
        void print() const;
        static int getColorIndex(int blockIndex, int pixelIndex, int horizontalFactor, int verticalFactor);
        void generateColorBlocks();
        [[nodiscard]] std::tuple<uint8_t, uint8_t, uint8_t> getColor(int index) const;
    };
}
