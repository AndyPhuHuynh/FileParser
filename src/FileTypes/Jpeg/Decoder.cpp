#include "FileParser/Jpeg/Decoder.hpp"

#include <format>
#include <fstream>
#include <unordered_set>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/Markers.hpp"

auto FileParser::Jpeg::JpegParser::parseFrameComponent(
    std::ifstream& file
) -> std::expected<NewFrameComponent, std::string> {
    NewFrameComponent component;

    const auto identifier = read_uint8(file);
    if (!identifier) {
        return getUnexpected(identifier, "Unable to read component identifier");
    }
    component.identifier = identifier.value();

    const auto samplingFactor = read_uint8(file);
    if (!samplingFactor) {
        return getUnexpected(samplingFactor, "Unable to read sampling factor");
    }
    component.horizontalSamplingFactor  = getUpperNibble(samplingFactor.value());
    component.verticalSamplingFactor    = getLowerNibble(samplingFactor.value());

    const auto qTableSelector = read_uint8(file);
    if (!qTableSelector) {
        return getUnexpected(qTableSelector, "Unable to read quantization table selector");
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
        return getUnexpected(length, "Unable to read frame length");
    }

    const auto precision = read_uint8(file);
    if (!precision) {
        return getUnexpected(precision, "Unable to read frame precision");
    }
    frame.precision = precision.value();
    if (frame.precision != 8) {
        return std::unexpected(
            std::format("Unsupported precision in frame header: {}, precision must be 8 bits", frame.precision));
    }

    const auto numberOfLines = read_uint16_be(file);
    if (!numberOfLines) {
        return getUnexpected(numberOfLines, "Unable to read number of lines");
    }
    frame.numberOfLines = numberOfLines.value();

    const auto numberOfSamplesPerLine = read_uint16_be(file);
    if (!numberOfSamplesPerLine) {
        return getUnexpected(numberOfSamplesPerLine, "Unable to read number of samples per line");
    }
    frame.numberOfSamplesPerLine = numberOfSamplesPerLine.value();

    const auto numberOfComponents = read_uint8(file);
    if (!numberOfComponents) {
        return getUnexpected(numberOfComponents, "Unable to read number of components");
    }
    frame.numberOfComponents = numberOfComponents.value();

    const uint16_t expectedLength = 8 + 3 * frame.numberOfComponents;
    if (*length != expectedLength) {
        return std::unexpected(
            std::format(R"(Specified length of FrameHeader "{}" does not match expected length of "{}")",
                *length, expectedLength));
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

auto FileParser::Jpeg::JpegParser::parseDefineNumberOfLines(
    std::ifstream& file
) -> std::expected<uint16_t, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }
    constexpr uint16_t expectedLength = 4;
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    const auto numberOfLines = read_uint16_be(file);
    if (!numberOfLines) {
        return getUnexpected(numberOfLines, "Unable to parse number of lines");
    }
    return *numberOfLines;
}

auto FileParser::Jpeg::JpegParser::parseDefineRestartInterval(
    std::ifstream& file
) -> std::expected<uint16_t, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }
    constexpr uint16_t expectedLength = 4;
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    const auto restartInterval = read_uint16_be(file);
    if (!restartInterval) {
        return getUnexpected(restartInterval, "Unable to parse restart interval");
    }
    return *restartInterval;
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

    std::unordered_set<uint8_t> encounteredMarkers;
    JpegData data;

    uint8_t byte;
    while (!file.eof()) {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (byte == 0xFF) {
            uint8_t marker;
            file.read(reinterpret_cast<char*>(&marker), 1);
            if (marker == 0x00) continue; // Byte stuffing
            if (marker == DHT) {
                // readHuffmanTables();
            } else if (marker == DQT) {
                // readQuantizationTables();
            } else if (isSOF(marker)) {
                if (std::ranges::any_of(encounteredMarkers, [](const uint8_t m) { return isSOF(m); })) {
                    return std::unexpected("Multiple SOF markers encountered. Only one SOF marker is allowed");
                }
                auto frameHeader = parseFrameHeader(file, marker);
                if (!frameHeader) {
                    return std::unexpected(std::format("Unable to parser frame header: {}", frameHeader.error()));
                }
                data.frameHeader = *frameHeader;
            } else if (marker == DNL) {
                if (encounteredMarkers.contains(DNL)) {
                    return std::unexpected("Multiple DNL markers encountered. Only one DNL marker is allowed");
                }
                auto numberOfLines = parseDefineNumberOfLines(file);
                if (!numberOfLines) {
                    return std::unexpected(std::format("Unable to parser DNL: {}", numberOfLines.error()));
                }
                data.frameHeader.numberOfLines = *numberOfLines;
            } else if (marker == DRI) {
                if (encounteredMarkers.contains(DRI)) {
                    return std::unexpected("Multiple DRI markers encountered. Only one DRI marker is allowed");
                }
                auto restartInterval = parseDefineRestartInterval(file);
                if (!restartInterval) {
                    return std::unexpected(std::format("Unable parser DRI: {}", restartInterval.error()));
                }
                data.restartInterval = *restartInterval;
            } else if (marker == COM) {
                // readComments();
            } else if (marker == SOS) {
                // readStartOfScan();
            } else if (marker == EOI) {
                // decode();
                break;
            }
            encounteredMarkers.insert(marker);
        }
    }

    // Check if number of lines in frame == 0, error

    return {};
}
