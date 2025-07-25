#include "FileParser/Jpeg/Mcu.hpp"

#include <iomanip>
#include <iostream>
#include <ranges>

#include <simde/x86/avx512.h>

#include "FileParser/Jpeg/JpegImage.h"

auto FileParser::Jpeg::Component::operator[](const size_t index) -> float& {
    return data[index];
}

auto FileParser::Jpeg::Component::operator[](const size_t index) const -> const float& {
    return data[index];
}

void FileParser::Jpeg::Mcu::print() const {
    std::cout << std::dec;
    std::cout << "Luminance (Y)(x" << Y.size() << ")        | Blue Chrominance (Cb)        | Red Chrominance (Cr)" << '\n';
    std::cout << "----------------------------------------------------------------------------------\n";

    for (int y = 0; y < 8; y++) {
        for (const auto& i : Y) {
            for (int x = 0; x < 8; x++) {
                // Print Luminance
                int luminanceIndex = y * 8 + x;
                std::cout << std::setw(5) << static_cast<int>(i[luminanceIndex]) << " ";
            }
            std::cout << "| ";
        }

        for (int x = 0; x < 8; x++) {
            // Print Blue Chrominance
            int blueIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(Cb[blueIndex]) << " ";
        }
        std::cout << "| ";

        for (int x = 0; x < 8; x++) {
            // Print Red Chrominance
            int redIndex = y * 8 + x;
            std::cout << std::setw(5) << static_cast<int>(Cr[redIndex]) << " ";
        }
        std::cout << '\n';
    }
}

int FileParser::Jpeg::Mcu::getColorIndex(const int blockIndex, const int pixelIndex, const int horizontalFactor, const int verticalFactor) {
    int blockRow = blockIndex / horizontalFactor;
    int blockCol = blockIndex % horizontalFactor;

    int pixelRow = pixelIndex / 8;
    int pixelCol = pixelIndex % 8;

    int chromaRow = (blockRow * 8 + pixelRow) / verticalFactor;
    int chromaCol = (blockCol * 8 + pixelCol) / horizontalFactor;

    return chromaRow * 8 + chromaCol;
}

void FileParser::Jpeg::Mcu::generateColorBlocks() {
    simde__m256 one_two_eight = simde_mm256_set1_ps(128);
    simde__m256 red_cr_scaler = simde_mm256_set1_ps(1.402f);
    simde__m256 green_cb_scaler = simde_mm256_set1_ps(-0.344f);
    simde__m256 green_cr_scaler = simde_mm256_set1_ps(-0.714f);
    simde__m256 blue_cb_scaler = simde_mm256_set1_ps(1.772f);

    for (int i = 0; i < static_cast<int>(Y.size()); i++) {
        auto& [R, G, B] = colorBlocks[i];
        constexpr int simdIncrement = 8;
        for (int j = 0; j < DataUnitLength; j += simdIncrement) {
            int colorIndices[simdIncrement];
            for (int k = 0; k < simdIncrement; k++) {
                colorIndices[k] = getColorIndex(i, j + k, horizontalSampleSize, verticalSampleSize);
            }

            // Load Y values (assumes contiguous memory layout)
            simde__m256 y = simde_mm256_loadu_ps(&Y[i][j]);

            // Load Cb values using gathered indices
            simde__m256 cb = simde_mm256_set_ps(
                Cb[colorIndices[7]], Cb[colorIndices[6]], Cb[colorIndices[5]], Cb[colorIndices[4]],
                Cb[colorIndices[3]], Cb[colorIndices[2]], Cb[colorIndices[1]], Cb[colorIndices[0]]
            );

            // Load Cr values using gathered indices
            simde__m256 cr = simde_mm256_set_ps(
                Cr[colorIndices[7]], Cr[colorIndices[6]], Cr[colorIndices[5]], Cr[colorIndices[4]],
                Cr[colorIndices[3]], Cr[colorIndices[2]], Cr[colorIndices[1]], Cr[colorIndices[0]]
            );


            y = simde_mm256_add_ps(y, one_two_eight);

            simde__m256 r = simde_mm256_add_ps(y, simde_mm256_mul_ps(red_cr_scaler, cr));
            simde__m256 g = simde_mm256_add_ps(y, simde_mm256_add_ps(
                                simde_mm256_mul_ps(green_cb_scaler, cb),
                                simde_mm256_mul_ps(green_cr_scaler, cr)));
            simde__m256 b = simde_mm256_add_ps(y, simde_mm256_mul_ps(blue_cb_scaler, cb));

            simde__m256 r_clamped = simde_mm256_max_ps(simde_mm256_min_ps(r, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));
            simde__m256 g_clamped = simde_mm256_max_ps(simde_mm256_min_ps(g, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));
            simde__m256 b_clamped = simde_mm256_max_ps(simde_mm256_min_ps(b, simde_mm256_set1_ps(255.0f)), simde_mm256_set1_ps(0.0f));

            simde_mm256_storeu_ps(&R[j], r_clamped);
            simde_mm256_storeu_ps(&G[j], g_clamped);
            simde_mm256_storeu_ps(&B[j], b_clamped);
        }
    }
}

std::tuple<uint8_t, uint8_t, uint8_t> FileParser::Jpeg::Mcu::getColor(const int index) const {
    int blockRow = index / (horizontalSampleSize * 8) / 8;
    int blockCol = index % (horizontalSampleSize * 8) / 8;

    int pixelRow = index / (horizontalSampleSize * 8) % 8;
    int pixelColumn = index % 8;

    int blockIndex = blockRow * horizontalSampleSize + blockCol;
    int pixelIndex = pixelRow * 8 + pixelColumn;

    return {static_cast<uint8_t>(colorBlocks[blockIndex].R[pixelIndex]),
        static_cast<uint8_t>(colorBlocks[blockIndex].G[pixelIndex]) ,
        static_cast<uint8_t>(colorBlocks[blockIndex].B[pixelIndex])};
}

// Uses AAN DCT
void FileParser::Jpeg::Mcu::performInverseDCT(Component& array) {
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

void FileParser::Jpeg::Mcu::performInverseDCT() {
    for (auto& y : Y) {
        performInverseDCT(y);
    }
    performInverseDCT(Cb);
    performInverseDCT(Cr);
}

void FileParser::Jpeg::Mcu::dequantize(Component& array, const QuantizationTable& quantizationTable) {
    for (size_t i = 0; i < DataUnitLength; i += 16) {
        simde__m512 arrayVec = simde_mm512_loadu_ps(&array[i]);
        simde__m512 quantTableVec = simde_mm512_loadu_ps(&quantizationTable.table[i]);
        simde__m512 resultVec = simde_mm512_mul_ps(arrayVec, quantTableVec);
        simde_mm512_storeu_ps(&array[i], resultVec);
    }
}

void FileParser::Jpeg::Mcu::dequantize(JpegImage* jpeg, const ScanHeaderComponentSpecification& scanComp) {
    if (scanComp.componentId == 1) {
        for (auto& y : Y) {
            dequantize(y, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[1].quantizationTableSelector]);
        }
    } else if (scanComp.componentId == 2){
        dequantize(Cb, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[2].quantizationTableSelector]);
    } else if (scanComp.componentId == 3){
        dequantize(Cr, jpeg->quantizationTables[scanComp.quantizationTableIteration][jpeg->info.componentSpecifications[3].quantizationTableSelector]);
    }
}

FileParser::Jpeg::Mcu::Mcu() {
    Y.push_back({});
    colorBlocks = std::vector(1, ColorBlock());
}

FileParser::Jpeg::Mcu::Mcu(const int luminanceComponents, const int horizontalSampleSize, const int verticalSampleSize) {
    for (const int i : std::ranges::views::iota(0, luminanceComponents)) {
        Y.push_back({});
    }
    colorBlocks = std::vector(luminanceComponents, ColorBlock());
    this->horizontalSampleSize = horizontalSampleSize;
    this->verticalSampleSize = verticalSampleSize;
}

FileParser::Jpeg::Mcu::Mcu(const ColorBlock& colorBlock) {
    colorBlocks.push_back(colorBlock);
    ColorBlock copy = colorBlock;
    copy.rgbToYCbCr();
    Y.push_back({});
    for (int i = 0; i < DataUnitLength; i++) {
        Y[0][i] = copy.R[i];
        Cb[i]   = copy.G[i];
        Cr[i]   = copy.B[i];
    }
}

