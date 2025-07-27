#pragma once

#include <cmath>
#include <numbers>

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
    void dequantize(Mcu& mcu, JpegImage* jpeg, const ScanHeaderComponentSpecification& scanComp);

    void quantize(Component& component, const QuantizationTable& quantizationTable);
    void quantize(Mcu& mcu, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);
    void quantize(std::vector<Mcu>& mcus, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable);
}
