#pragma once
#include "Mcu.hpp"

namespace FileParser::Jpeg {
    // DCT

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
