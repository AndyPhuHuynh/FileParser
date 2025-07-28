#include "FileParser/Jpeg/Decoder.hpp"

#include <fstream>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/Markers.hpp"

auto FileParser::Jpeg::JpegParser::parseFrameComponent(
    std::ifstream& file
) -> std::expected<NewFrameComponent, std::string> {
    NewFrameComponent component;

    const auto identifier = read_uint8(file);
    if (!identifier) {
        return std::unexpected(std::format("Unable to read component identifier: {}", identifier.error()));
    }
    component.identifier = identifier.value();

    const auto samplingFactor = read_uint8(file);
    if (!samplingFactor) {
        return std::unexpected(std::format("Unable to read sampling factor: {}", samplingFactor.error()));
    }
    component.horizontalSamplingFactor  = getUpperNibble(samplingFactor.value());
    component.verticalSamplingFactor    = getLowerNibble(samplingFactor.value());

    const auto qTableSelector = read_uint8(file);
    if (!qTableSelector) {
        return std::unexpected(std::format("Unable to read quantization table selector: {}", qTableSelector.error()));
    }
    component.quantizationTableSelector = qTableSelector.value();

    constexpr uint8_t minHorizontalFactor = 1, maxHorizontalFactor = 4;
    if (component.horizontalSamplingFactor < minHorizontalFactor || component.horizontalSamplingFactor > maxHorizontalFactor) {
        return std::unexpected(std::format(R"(Horizontal sampling factor must be between {} and {}, got "{}")",
            minHorizontalFactor, maxHorizontalFactor, component.horizontalSamplingFactor));
    }
    constexpr uint8_t minVerticalFactor   = 1, maxVerticalFactor   = 4;
    if (component.verticalSamplingFactor < minVerticalFactor || component.verticalSamplingFactor > maxVerticalFactor) {
        return std::unexpected(std::format(R"(Vertical sampling factor must be between {} and {}, got "{}")",
            minVerticalFactor, maxVerticalFactor, component.verticalSamplingFactor));
    }
    constexpr uint8_t maxQuantizationTableSelector = 3;
    if (component.quantizationTableSelector > maxQuantizationTableSelector) {
        return std::unexpected(std::format(R"(Quantization destination selector must be less than {}, got "{}")",
            maxQuantizationTableSelector, component.quantizationTableSelector));
    }

    return component;
}

auto FileParser::Jpeg::JpegParser::parseFrameHeader(std::ifstream& file, const uint8_t SOF) -> std::expected<NewFrameHeader, std::string> {
    if (SOF != SOF0 && SOF != SOF2) {
        return std::unexpected(std::format(R"(Unsupported start of frame marker: "{}")", SOF));
    }

    NewFrameHeader frame;
    const auto length = read_uint16_be(file);
    if (!length) {
        return std::unexpected(std::format("Unable to read frame length: {}", length.error()));
    }

    const auto precision = read_uint8(file);
    if (!precision) {
        return std::unexpected(std::format("Unable to read precision: {}", precision.error()));
    }
    frame.precision = precision.value();
    if (frame.precision != 8) {
        return std::unexpected(
            std::format("Unsupported precision in frame header: {}, precision must be 8 bits", frame.precision));
    }

    const auto numberOfLines = read_uint16_be(file);
    if (!numberOfLines) {
        return std::unexpected(std::format("Unable to read number of lines: {}", numberOfLines.error()));
    }
    frame.numberOfLines = numberOfLines.value();

    const auto numberOfSamplesPerLine = read_uint16_be(file);
    if (!numberOfSamplesPerLine) {
        return std::unexpected(std::format("Unable to read number of samples per line: {}", numberOfSamplesPerLine.error()));
    }
    frame.numberOfSamplesPerLine = numberOfSamplesPerLine.value();

    const auto numberOfComponents = read_uint8(file);
    if (!numberOfComponents) {
        return std::unexpected(std::format("Unable to read number of components: {}", numberOfComponents.error()));
    }
    frame.numberOfComponents = numberOfComponents.value();

    const uint16_t expectedLength = 8 + 3 * frame.numberOfComponents;
    if (length.value() != expectedLength) {
        return std::unexpected(
            std::format(R"(Specified length of FrameHeader "{}" does not match expected length of "{}")",
                length, expectedLength));
    }

    constexpr uint16_t minComponents = 1, maxComponents = 4;
    if (frame.numberOfComponents < minComponents || frame.numberOfComponents > maxComponents) {
        return std::unexpected(std::format(R"(Number of components must be between {} and {}, got {})",
            minComponents, maxComponents, frame.numberOfComponents));
    }


    for (size_t i = 0; i < frame.numberOfComponents; ++i) {
        auto component = parseFrameComponent(file);
        if (!component) {
            return std::unexpected(std::format("Error parsing frame component #{}: {}", i, component.error()));
        }
        frame.components.push_back(std::move(component.value()));
    }

    return frame;
}

auto FileParser::Jpeg::JpegParser::parseDefineNumberOfLines(std::ifstream& file) -> void {

}

auto FileParser::Jpeg::JpegParser::parseFile(
    const std::filesystem::path& filePath
) -> std::expected<void, std::string> {
    if (!std::filesystem::exists(filePath)) {
        return std::unexpected("File does not exist: " + filePath.string());
    }
    if (!std::filesystem::is_regular_file(filePath)) {
        return std::unexpected("File is not a regular file: " + filePath.string());
    }
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        return std::unexpected("File could not be opened: " + filePath.string());
    }

    JpegData data;

    uint8_t byte;
    while (!file.eof()) {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (byte == 0xFF) {
            uint8_t marker;
            file.read(reinterpret_cast<char*>(&marker), 1);
            if (marker == 0x00) continue;
            if (marker == DHT) {
                // readHuffmanTables();
            } else if (marker == DQT) {
                // readQuantizationTables();
            } else if (marker >= SOF0 && marker <= SOF15 && !(marker == DHT || marker == JPG || marker == DAC)) {
                auto frameHeader = parseFrameHeader(file, marker);
                if (!frameHeader.has_value()) {
                    return std::unexpected(std::format("Unable to parser frame header: {}", frameHeader.error()));
                }
                data.frameHeader = frameHeader.value();
            } else if (marker == DNL) {
                // readDefineNumberOfLines();
            } else if (marker == DRI) {
                // readDefineRestartIntervals();
            } else if (marker == COM) {
                // readComments();
            } else if (marker == SOS) {
                // readStartOfScan();
            } else if (marker == EOI) {
                // decode();
                break;
            }
        }
    }
    return {};
}
