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
        uint16_t restartInterval;
        NewScanHeader scanHeader;
        std::vector<uint8_t> compressedData;
    };

    template <typename T>
    auto getUnexpected(
        const std::expected<T, std::string>& expected, std::string_view errorMsg
    ) -> std::unexpected<std::string> {
        return std::unexpected(std::format("{}: {}", errorMsg, expected.error()));
    }

    class JpegParser {
    public:
        [[nodiscard]] static auto parseFrameComponent(std::ifstream& file) -> std::expected<NewFrameComponent, std::string>;
        [[nodiscard]] static auto parseFrameHeader(std::ifstream& file, uint8_t SOF) -> std::expected<NewFrameHeader, std::string>;
        [[nodiscard]] static auto parseDefineNumberOfLines(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseDefineRestartInterval(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseFile(const std::filesystem::path& filePath) -> std::expected<void, std::string>;
    };
}
