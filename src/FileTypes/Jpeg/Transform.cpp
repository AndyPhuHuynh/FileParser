#include "FileParser/Jpeg/Transform.hpp"

#include <cmath> // NOLINT (needed for simde)
#include "simde/x86/avx512.h"

#include "FileParser/Jpeg/JpegImage.h"

// Uses AAN DCT
void FileParser::Jpeg::inverseDCT(Component& array) {
    float results[64];
    // Calculates the rows
    for (int i = 0; i < 8; i++) {
         const float g0 = array[0 * 8 + i] * s0;
         const float g1 = array[4 * 8 + i] * s4;
         const float g2 = array[2 * 8 + i] * s2;
         const float g3 = array[6 * 8 + i] * s6;
         const float g4 = array[5 * 8 + i] * s5;
         const float g5 = array[1 * 8 + i] * s1;
         const float g6 = array[7 * 8 + i] * s7;
         const float g7 = array[3 * 8 + i] * s3;

         const float f0 = g0;
         const float f1 = g1;
         const float f2 = g2;
         const float f3 = g3;
         const float f4 = g4 - g7;
         const float f5 = g5 + g6;
         const float f6 = g5 - g6;
         const float f7 = g4 + g7;

         const float e0 = f0;
         const float e1 = f1;
         const float e2 = f2 - f3;
         const float e3 = f2 + f3;
         const float e4 = f4;
         const float e5 = f5 - f7;
         const float e6 = f6;
         const float e7 = f5 + f7;
         const float e8 = f4 + f6;

         const float d0 = e0;
         const float d1 = e1;
         const float d2 = e2 * m1;
         const float d3 = e3;
         const float d4 = e4 * m2;
         const float d5 = e5 * m3;
         const float d6 = e6 * m4;
         const float d7 = e7;
         const float d8 = e8 * m5;

         const float c0 = d0 + d1;
         const float c1 = d0 - d1;
         const float c2 = d2 - d3;
         const float c3 = d3;
         const float c4 = d4 + d8;
         const float c5 = d5 + d7;
         const float c6 = d6 - d8;
         const float c7 = d7;
         const float c8 = c5 - c6;

         const float b0 = c0 + c3;
         const float b1 = c1 + c2;
         const float b2 = c1 - c2;
         const float b3 = c0 - c3;
         const float b4 = c4 - c8;
         const float b5 = c8;
         const float b6 = c6 - c7;
         const float b7 = c7;

         results[0 * 8 + i] = b0 + b7;
         results[1 * 8 + i] = b1 + b6;
         results[2 * 8 + i] = b2 + b5;
         results[3 * 8 + i] = b3 + b4;
         results[4 * 8 + i] = b3 - b4;
         results[5 * 8 + i] = b2 - b5;
         results[6 * 8 + i] = b1 - b6;
         results[7 * 8 + i] = b0 - b7;
     }
    // Calculates the columns
    for (int i = 0; i < 8; i++) {
        const float g0 = results[i * 8 + 0] * s0;
        const float g1 = results[i * 8 + 4] * s4;
        const float g2 = results[i * 8 + 2] * s2;
        const float g3 = results[i * 8 + 6] * s6;
        const float g4 = results[i * 8 + 5] * s5;
        const float g5 = results[i * 8 + 1] * s1;
        const float g6 = results[i * 8 + 7] * s7;
        const float g7 = results[i * 8 + 3] * s3;

        const float f0 = g0;
        const float f1 = g1;
        const float f2 = g2;
        const float f3 = g3;
        const float f4 = g4 - g7;
        const float f5 = g5 + g6;
        const float f6 = g5 - g6;
        const float f7 = g4 + g7;

        const float e0 = f0;
        const float e1 = f1;
        const float e2 = f2 - f3;
        const float e3 = f2 + f3;
        const float e4 = f4;
        const float e5 = f5 - f7;
        const float e6 = f6;
        const float e7 = f5 + f7;
        const float e8 = f4 + f6;

        const float d0 = e0;
        const float d1 = e1;
        const float d2 = e2 * m1;
        const float d3 = e3;
        const float d4 = e4 * m2;
        const float d5 = e5 * m3;
        const float d6 = e6 * m4;
        const float d7 = e7;
        const float d8 = e8 * m5;

        const float c0 = d0 + d1;
        const float c1 = d0 - d1;
        const float c2 = d2 - d3;
        const float c3 = d3;
        const float c4 = d4 + d8;
        const float c5 = d5 + d7;
        const float c6 = d6 - d8;
        const float c7 = d7;
        const float c8 = c5 - c6;

        const float b0 = c0 + c3;
        const float b1 = c1 + c2;
        const float b2 = c1 - c2;
        const float b3 = c0 - c3;
        const float b4 = c4 - c8;
        const float b5 = c8;
        const float b6 = c6 - c7;
        const float b7 = c7;

        array[i * 8 + 0] = b0 + b7;
        array[i * 8 + 1] = b1 + b6;
        array[i * 8 + 2] = b2 + b5;
        array[i * 8 + 3] = b3 + b4;
        array[i * 8 + 4] = b3 - b4;
        array[i * 8 + 5] = b2 - b5;
        array[i * 8 + 6] = b1 - b6;
        array[i * 8 + 7] = b0 - b7;
    }
}

void FileParser::Jpeg::inverseDCT(Mcu& mcu) {
    for (auto& y : mcu.Y) {
        inverseDCT(y);
    }
    inverseDCT(mcu.Cb);
    inverseDCT(mcu.Cr);
}

void FileParser::Jpeg::dequantize(Component& component, const QuantizationTable& quantizationTable) {
    for (size_t i = 0; i < Component::length; i += 16) {
        const simde__m512 arrayVec = simde_mm512_loadu_ps(&component[i]);
        const simde__m512 quantTableVec = simde_mm512_loadu_ps(&quantizationTable.table[i]);
        const simde__m512 resultVec = simde_mm512_mul_ps(arrayVec, quantTableVec);
        simde_mm512_storeu_ps(&component[i], resultVec);
    }
}

auto FileParser::Jpeg::dequantize(
    Mcu& mcu, const FrameInfo& frame, const NewScanHeader& scanHeader,
    const TableIterations& iterations, const std::array<std::vector<QuantizationTable>, 4>& quantizationTables
) -> void {
    for (const auto& scanComp : scanHeader.components) {
        const auto& qTable = quantizationTables[scanComp.dcTableSelector][iterations.quantization[scanComp.dcTableSelector]];
        if (scanComp.componentSelector == frame.luminanceID) {
            for (auto& y : mcu.Y) {
                dequantize(y, qTable);
            }
        } else if (scanComp.componentSelector == frame.chrominanceBlueID) {
            dequantize(mcu.Cb, qTable);
        } else if (scanComp.componentSelector == frame.chrominanceRedID) {
            dequantize(mcu.Cr, qTable);
        }
    }
}

void FileParser::Jpeg::forwardDCT(Component& component) {
    for (int i = 0; i < 8; ++i) {
        const float a0 = component[0 * 8 + i];
        const float a1 = component[1 * 8 + i];
        const float a2 = component[2 * 8 + i];
        const float a3 = component[3 * 8 + i];
        const float a4 = component[4 * 8 + i];
        const float a5 = component[5 * 8 + i];
        const float a6 = component[6 * 8 + i];
        const float a7 = component[7 * 8 + i];

        const float b0 = a0 + a7;
        const float b1 = a1 + a6;
        const float b2 = a2 + a5;
        const float b3 = a3 + a4;
        const float b4 = a3 - a4;
        const float b5 = a2 - a5;
        const float b6 = a1 - a6;
        const float b7 = a0 - a7;

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 = b1 - b2;
        const float c3 = b0 - b3;
        const float c4 = b4;
        const float c5 = b5 - b4;
        const float c6 = b6 - c5;
        const float c7 = b7 - b6;

        const float d0 = c0 + c1;
        const float d1 = c0 - c1;
        const float d2 = c2;
        const float d3 = c3 - c2;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c5 + c7;
        const float d8 = c4 - c6;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * m1;
        const float e3 = d3;
        const float e4 = d4 * m2;
        const float e5 = d5 * m3;
        const float e6 = d6 * m4;
        const float e7 = d7;
        const float e8 = d8 * m5;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4 + e8;
        const float f5 = e5 + e7;
        const float f6 = e6 + e8;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = f5 - f6;
        const float g7 = f7 - f4;

        component[0 * 8 + i] = g0 * s0;
        component[4 * 8 + i] = g1 * s4;
        component[2 * 8 + i] = g2 * s2;
        component[6 * 8 + i] = g3 * s6;
        component[5 * 8 + i] = g4 * s5;
        component[1 * 8 + i] = g5 * s1;
        component[7 * 8 + i] = g6 * s7;
        component[3 * 8 + i] = g7 * s3;
    }
    for (int i = 0; i < 8; ++i) {
        const float a0 = component[i * 8 + 0];
        const float a1 = component[i * 8 + 1];
        const float a2 = component[i * 8 + 2];
        const float a3 = component[i * 8 + 3];
        const float a4 = component[i * 8 + 4];
        const float a5 = component[i * 8 + 5];
        const float a6 = component[i * 8 + 6];
        const float a7 = component[i * 8 + 7];

        const float b0 = a0 + a7;
        const float b1 = a1 + a6;
        const float b2 = a2 + a5;
        const float b3 = a3 + a4;
        const float b4 = a3 - a4;
        const float b5 = a2 - a5;
        const float b6 = a1 - a6;
        const float b7 = a0 - a7;

        const float c0 = b0 + b3;
        const float c1 = b1 + b2;
        const float c2 = b1 - b2;
        const float c3 = b0 - b3;
        const float c4 = b4;
        const float c5 = b5 - b4;
        const float c6 = b6 - c5;
        const float c7 = b7 - b6;

        const float d0 = c0 + c1;
        const float d1 = c0 - c1;
        const float d2 = c2;
        const float d3 = c3 - c2;
        const float d4 = c4;
        const float d5 = c5;
        const float d6 = c6;
        const float d7 = c5 + c7;
        const float d8 = c4 - c6;

        const float e0 = d0;
        const float e1 = d1;
        const float e2 = d2 * m1;
        const float e3 = d3;
        const float e4 = d4 * m2;
        const float e5 = d5 * m3;
        const float e6 = d6 * m4;
        const float e7 = d7;
        const float e8 = d8 * m5;

        const float f0 = e0;
        const float f1 = e1;
        const float f2 = e2 + e3;
        const float f3 = e3 - e2;
        const float f4 = e4 + e8;
        const float f5 = e5 + e7;
        const float f6 = e6 + e8;
        const float f7 = e7 - e5;

        const float g0 = f0;
        const float g1 = f1;
        const float g2 = f2;
        const float g3 = f3;
        const float g4 = f4 + f7;
        const float g5 = f5 + f6;
        const float g6 = f5 - f6;
        const float g7 = f7 - f4;

        component[i * 8 + 0] = g0 * s0;
        component[i * 8 + 4] = g1 * s4;
        component[i * 8 + 2] = g2 * s2;
        component[i * 8 + 6] = g3 * s6;
        component[i * 8 + 5] = g4 * s5;
        component[i * 8 + 1] = g5 * s1;
        component[i * 8 + 7] = g6 * s7;
        component[i * 8 + 3] = g7 * s3;
    }
}

void FileParser::Jpeg::forwardDCT(Mcu& mcu) {
    for (auto& y : mcu.Y) {
        forwardDCT(y);
    }
    forwardDCT(mcu.Cb);
    forwardDCT(mcu.Cr);
}

void FileParser::Jpeg::forwardDCT(std::vector<Mcu>& mcus) {
    for (auto& mcu : mcus) {
        forwardDCT(mcu);
    }
}

void FileParser::Jpeg::dequantize(Mcu& mcu, JpegImage* jpeg, const ScanHeaderComponentSpecification& scanComp) {
    // TODO: Add error messages for getting values from these optionals
    if (scanComp.componentId == 1) {
        for (auto& y : mcu.Y) {
            dequantize(y, *jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[1].quantizationTableSelector]);
        }
    } else if (scanComp.componentId == 2){
        dequantize(mcu.Cb, *jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[2].quantizationTableSelector]);
    } else if (scanComp.componentId == 3){
        dequantize(mcu.Cr, *jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[3].quantizationTableSelector]);
    }
}

void FileParser::Jpeg::quantize(Component& component, const QuantizationTable& quantizationTable) {
    for (int i = 0; i < Mcu::DataUnitLength; i++) {
        component[i] = std::round(component[i] / quantizationTable.table.at(i));
    }
}

void FileParser::Jpeg::quantize(Mcu& mcu, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable) {
    for (auto& y : mcu.Y) {
        quantize(y, luminanceTable);
    }
    quantize(mcu.Cb, chrominanceTable);
    quantize(mcu.Cr, chrominanceTable);
}

void FileParser::Jpeg::quantize(std::vector<Mcu>& mcus, const QuantizationTable& luminanceTable, const QuantizationTable& chrominanceTable) {
    for (auto& mcu : mcus) {
        quantize(mcu, luminanceTable, chrominanceTable);
    }
}