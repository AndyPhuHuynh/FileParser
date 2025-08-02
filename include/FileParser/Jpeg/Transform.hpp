#pragma once

#include <cmath>
#include <numbers>

#include "Decoder.hpp"
#include "Mcu.hpp"

namespace FileParser::Jpeg {
    // DCT

    // IDCT scaling factors
    const float m0 = static_cast<float>(2.0 * std::cos(1.0 / 16.0 * 2.0 * std::numbers::pi));
    const float m1 = static_cast<float>(2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi));
    const float m3 = static_cast<float>(2.0 * std::cos(2.0 / 16.0 * 2.0 * std::numbers::pi));
    const float m5 = static_cast<float>(2.0 * std::cos(3.0 / 16.0 * 2.0 * std::numbers::pi));
    const float m2 = m0 - m5;
    const float m4 = m0 + m5;

    const float s0 = static_cast<float>(std::cos(0.0 / 16.0 * std::numbers::pi) / std::sqrt(8));
    const float s1 = static_cast<float>(std::cos(1.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s2 = static_cast<float>(std::cos(2.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s3 = static_cast<float>(std::cos(3.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s4 = static_cast<float>(std::cos(4.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s5 = static_cast<float>(std::cos(5.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s6 = static_cast<float>(std::cos(6.0 / 16.0 * std::numbers::pi) / 2.0);
    const float s7 = static_cast<float>(std::cos(7.0 / 16.0 * std::numbers::pi) / 2.0);

    void inverseDCT(Component& array);
    void inverseDCT(Mcu& mcu);

    void forwardDCT(Component& component);
    void forwardDCT(Mcu& mcu);
    void forwardDCT(std::vector<Mcu>& mcus);

    // Quantization

    void dequantize(Component& component, const QuantizationTable& quantizationTable);
    void dequantize(Mcu& mcu, const FrameInfo& frame, const NewScanHeader& scanHeader,
        const TableIterations& iterations, const std::array<std::vector<QuantizationTable>, 4>& quantizationTables);

    void quantize(Component& component, const QuantizationTable& quantizationTable);
    void quantize(Mcu& mcu, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);
    void quantize(std::vector<Mcu>& mcus, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);

    inline auto convertMcusToColorBlocks(const std::vector<Mcu>& mcus, const size_t pixelWidth,
                                         const size_t pixelHeight) -> std::vector<ColorBlock> {
        auto ceilDivide = [](auto a, auto b) {
            return (a + b - 1) / b;
        };

        if (mcus.empty()) {
            return {};
        }

        constexpr size_t blockSideLength = 8;
        const size_t blockWidth  = ceilDivide(pixelWidth, blockSideLength);
        const size_t blockHeight = ceilDivide(pixelHeight, blockSideLength);

        const int horizontal =  mcus[0].horizontalSampleSize;
        const int vertical   =  mcus[0].verticalSampleSize;

        const size_t mcuTotalWidth  = ceilDivide(blockWidth, horizontal);
        std::vector result(blockWidth * blockHeight , ColorBlock());

        for (size_t mcuIndex = 0; mcuIndex < mcus.size(); mcuIndex++) {
            const size_t mcuRow = mcuIndex / mcuTotalWidth;
            const size_t mcuCol = mcuIndex % mcuTotalWidth;

            auto colors = generateColorBlocks(mcus[mcuIndex]);
            for (size_t colorIndex = 0; colorIndex < colors.size(); colorIndex++) {
                const size_t colorRow = colorIndex / horizontal;
                const size_t colorCol = colorIndex % horizontal;

                const size_t blockRow = mcuRow * vertical   + colorRow;
                const size_t blockCol = mcuCol * horizontal + colorCol;

                if (blockRow < blockHeight && blockCol < blockWidth) {
                    result[blockRow * blockWidth + blockCol] = colors[colorIndex];
                }
            }
        }
        return result;
    }
}
