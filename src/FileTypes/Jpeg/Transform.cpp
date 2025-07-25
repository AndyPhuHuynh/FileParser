#include "FileParser/Jpeg/Transform.hpp"

#include <cmath> // NOLINT (needed for simde)
#include "simde/x86/avx512.h"

#include "FileParser/Jpeg/JpegImage.h"

void FileParser::Jpeg::dequantize(Component& component, const QuantizationTable& quantizationTable) {
    for (size_t i = 0; i < Component::length; i += 16) {
        const simde__m512 arrayVec = simde_mm512_loadu_ps(&component[i]);
        const simde__m512 quantTableVec = simde_mm512_loadu_ps(&quantizationTable.table[i]);
        const simde__m512 resultVec = simde_mm512_mul_ps(arrayVec, quantTableVec);
        simde_mm512_storeu_ps(&component[i], resultVec);
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