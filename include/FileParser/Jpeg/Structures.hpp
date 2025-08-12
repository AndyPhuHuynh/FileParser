#pragma once

#include <cstdint>
#include <vector>

namespace FileParser::Jpeg {
    constexpr size_t MaxTableId = 4;

    struct FrameComponent {
        uint8_t identifier = 0;
        uint8_t horizontalSamplingFactor = 0;
        uint8_t verticalSamplingFactor = 0;
        uint8_t quantizationTableSelector = 0;
    };

    // Represents the raw data for the frame header
    struct FrameHeader {
        uint8_t  precision = 0; // Precision in bits for the samples for the components
        uint16_t numberOfLines = 0;
        uint16_t numberOfSamplesPerLine = 0;
        std::vector<FrameComponent> components;

        [[nodiscard]] auto getComponent(uint8_t id) const -> std::expected<FrameComponent, std::string> {
            const auto it = std::ranges::find_if(components, [id](const FrameComponent& c) { return c.identifier == id; });
            if (it == components.end()) return std::unexpected(std::format("Unable to find frame component with id {}", id));
            return *it;
        }
    };

    struct ScanComponent {
        uint8_t componentSelector = 0;
        uint8_t dcTableSelector = 0;
        uint8_t acTableSelector = 0;
    };

    struct ScanHeader {
        std::vector<ScanComponent> components;
        uint8_t spectralSelectionStart = 0;
        uint8_t spectralSelectionEnd = 0;
        uint8_t successiveApproximationHigh = 0;
        uint8_t successiveApproximationLow = 0;
    };

    // Tables can be overridden in progressive Jpegs. This struct tells you which iteration of each table to use
    struct TableIterations {
        std::array<size_t, MaxTableId> quantization{};
        std::array<size_t, MaxTableId> dc{};
        std::array<size_t, MaxTableId> ac{};
    };

    struct Scan {
        ScanHeader header;
        uint16_t restartInterval = 0;
        TableIterations iterations;
        std::vector<std::vector<uint8_t>> dataSections; // Sections of data separated at restart markers
    };

    const unsigned char zigZagMap[] = {
        0,   1,  8, 16,  9,  2,  3, 10,
        17, 24, 32, 25, 18, 11,  4,  5,
        12, 19, 26, 33, 40, 48, 41, 34,
        27, 20, 13,  6,  7, 14, 21, 28,
        35, 42, 49, 56, 57, 50, 43, 36,
        29, 22, 15, 23, 30, 37, 44, 51,
        58, 59, 52, 45, 38, 31, 39, 46,
        53, 60, 61, 54, 47, 55, 62, 63
    };

    struct QuantizationTable {
        static constexpr size_t length = 64;

        uint8_t precision = 0; // 0 => 8-bit, 1 => 16-bit
        uint8_t destination = 0;
        std::array<float, length> table{}; // De-zigzagged values

        auto operator[](const size_t index) -> float& { return table[index]; }
        auto operator[](const size_t index) const -> const float& { return table[index]; }
    };
}