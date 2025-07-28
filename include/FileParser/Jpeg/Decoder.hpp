#pragma once

#include <expected>
#include <filesystem>

namespace FileParser::Jpeg {
    struct NewFrameComponent {
        uint8_t identifier = 0;
        uint8_t horizontalSamplingFactor = 0;
        uint8_t verticalSamplingFactor = 0;
        uint8_t quantizationTableSelector = 0;
    };

    struct NewFrameHeader {
        uint8_t  precision = 0; // Precision in bits for the samples for the components
        uint16_t numberOfLines = 0;
        uint16_t numberOfSamplesPerLine = 0;
        uint8_t  numberOfComponents = 0;
        std::vector<NewFrameComponent> components;
    };

    class NewScanHeader {

    };

    struct JpegData {
        NewFrameHeader frameHeader;
        NewScanHeader scanHeader;
        std::vector<uint8_t> compressedData;
    };

    class JpegParser {
        static auto parseFrameComponent(std::ifstream& file) -> std::expected<NewFrameComponent, std::string>;
        static auto parseFrameHeader(std::ifstream& file, uint8_t SOF) -> std::expected<NewFrameHeader, std::string>;
        static auto parseDefineNumberOfLines(std::ifstream& file) -> void;
        static auto parseFile(const std::filesystem::path& filePath) -> std::expected<void, std::string>;
    };
}
