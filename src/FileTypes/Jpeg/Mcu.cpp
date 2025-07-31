#include "FileParser/Jpeg/Mcu.hpp"

#include <cmath>
#include <ranges>

#include <simde/x86/avx512.h>

#include "FileParser/Jpeg/JpegImage.h"

auto FileParser::Jpeg::Component::operator[](const size_t index) -> float& {
    return data[index];
}

auto FileParser::Jpeg::Component::operator[](const size_t index) const -> const float& {
    return data[index];
}

int FileParser::Jpeg::Mcu::getColorIndex(const int blockIndex, const int pixelIndex) const {
    const int blockRow = blockIndex / horizontalSampleSize;
    const int blockCol = blockIndex % horizontalSampleSize;

    constexpr int blockHeight = 8;
    const int pixelRow = pixelIndex / blockHeight;
    const int pixelCol = pixelIndex % blockHeight;

    const int chromaRow = (blockRow * blockHeight + pixelRow) / verticalSampleSize;
    const int chromaCol = (blockCol * blockHeight + pixelCol) / horizontalSampleSize;

    return chromaRow * blockHeight + chromaCol;
}

FileParser::Jpeg::Mcu::Mcu() {
    Y.push_back({});
}

FileParser::Jpeg::Mcu::Mcu(const int luminanceComponents, const int horizontalSampleSize, const int verticalSampleSize) {
    for (int i = 0; i < luminanceComponents; i++) {
        Y.push_back({});
    }
    this->horizontalSampleSize = horizontalSampleSize;
    this->verticalSampleSize = verticalSampleSize;
}

FileParser::Jpeg::Mcu::Mcu(const ColorBlock& colorBlock) {
    ColorBlock copy = colorBlock;
    copy.rgbToYCbCr();
    Y.push_back({});
    for (int i = 0; i < DataUnitLength; i++) {
        Y[0][i] = copy.R[i];
        Cb[i]   = copy.G[i];
        Cr[i]   = copy.B[i];
    }
}

auto FileParser::Jpeg::generateColorBlocks(const Mcu& mcu) -> std::vector<ColorBlock> {
    simde__m256 one_two_eight = simde_mm256_set1_ps(128);
    simde__m256 red_cr_scaler = simde_mm256_set1_ps(1.402f);
    simde__m256 green_cb_scaler = simde_mm256_set1_ps(-0.344f);
    simde__m256 green_cr_scaler = simde_mm256_set1_ps(-0.714f);
    simde__m256 blue_cb_scaler = simde_mm256_set1_ps(1.772f);

    std::vector colorBlocks(mcu.Y.size(), ColorBlock());
    for (size_t i = 0; i < mcu.Y.size(); i++) {
        auto& [R, G, B] = colorBlocks[i];
        constexpr int simdIncrement = 8;
        for (int j = 0; j < Mcu::DataUnitLength; j += simdIncrement) {
            int colorIndices[simdIncrement];
            for (int k = 0; k < simdIncrement; k++) {
                colorIndices[k] = mcu.getColorIndex(static_cast<int>(i), j + k);
            }

            // Load Y values (assumes contiguous memory layout)
            simde__m256 y = simde_mm256_loadu_ps(&mcu.Y[i][j]);

            // Load Cb values using gathered indices
            simde__m256 cb = simde_mm256_set_ps(
                mcu.Cb[colorIndices[7]], mcu.Cb[colorIndices[6]], mcu.Cb[colorIndices[5]], mcu.Cb[colorIndices[4]],
                mcu.Cb[colorIndices[3]], mcu.Cb[colorIndices[2]], mcu.Cb[colorIndices[1]], mcu.Cb[colorIndices[0]]
            );

            // Load Cr values using gathered indices
            simde__m256 cr = simde_mm256_set_ps(
                mcu.Cr[colorIndices[7]], mcu.Cr[colorIndices[6]], mcu.Cr[colorIndices[5]], mcu.Cr[colorIndices[4]],
                mcu.Cr[colorIndices[3]], mcu.Cr[colorIndices[2]], mcu.Cr[colorIndices[1]], mcu.Cr[colorIndices[0]]
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
    return colorBlocks;
}

