#pragma once
#include "Mcu.hpp"

namespace FileParser::Jpeg {
    // Quantization

    void dequantize(Component& component, const QuantizationTable& quantizationTable);
    void dequantize(Mcu& mcu, JpegImage* jpeg, const ScanHeaderComponentSpecification& scanComp);
}
