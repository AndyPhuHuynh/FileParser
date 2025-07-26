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

