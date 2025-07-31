#include "FileParser/Jpeg/Decoder.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
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
        frame.components.push_back(*component);
    }

    return frame;
}

auto FileParser::Jpeg::JpegParser::parseDNL(
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
    return numberOfLines;
}

auto FileParser::Jpeg::JpegParser::parseDRI(
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
    return restartInterval;
}

auto FileParser::Jpeg::JpegParser::parseComment(std::ifstream& file) -> std::expected<std::string, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }
    auto comment = read_string(file, *length - 2);
    if (!comment) {
        return getUnexpected(comment, "Unable to parse comment");
    }
    return comment;
}

auto FileParser::Jpeg::JpegParser::parseDQT(
    std::ifstream& file
) -> std::expected<std::vector<QuantizationTable>, std::string> {
    const std::streampos filePosBefore = file.tellg();

    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }

    std::vector<QuantizationTable> tables;
    while (file.tellg() - filePosBefore < *length && file) {
        const auto precisionAndDestination = read_uint8(file);
        if (!precisionAndDestination) {
            return getUnexpected(precisionAndDestination, "Unable to parse precision and id");
        }

        QuantizationTable& table = tables.emplace_back();
        // Precision (1 = 16-bit, 0 = 8-bit) in upper nibble, Destination ID in lower nibble
        table.precision   = getUpperNibble(*precisionAndDestination);
        table.destination = getLowerNibble(*precisionAndDestination);

        if (table.precision == 0) {
            const auto elements = read_uint8(file, QuantizationTable::length);
            if (!elements) {
                return getUnexpected(elements, "Unable to parse quantization table elements");
            }
            for (const auto i : zigZagMap) {
                table[i] = static_cast<float>((*elements)[i]);
            }
        } else {
            const auto elements = read_uint16_be(file, QuantizationTable::length);
            if (!elements) {
                return getUnexpected(elements, "Unable to parse quantization table elements");
            }
            for (const auto i : zigZagMap) {
                table[i] = static_cast<float>((*elements)[i]);
            }
        }
    }
    const std::streampos filePosAfter = file.tellg();
    if (const auto bytesRead = filePosAfter - filePosBefore; bytesRead != *length) {
        return std::unexpected(std::format("Length mismatch. Length was {}, however {} bytes was read", *length, bytesRead));
    }
    return tables;
}

auto FileParser::Jpeg::JpegParser::parseDHT(
    std::ifstream& file
) -> std::expected<std::vector<HuffmanParseResult>, std::string> {
    const std::streampos filePosBefore = file.tellg();

    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }

    std::vector<HuffmanParseResult> tables;
    while (file.tellg() - filePosBefore < *length && file) {
        const auto tableClassAndDestination = read_uint8(file);
        if (!tableClassAndDestination) {
            return getUnexpected(tableClassAndDestination, "Unable to parse table class and destination");
        }

        auto& [tableClass, tableDestination, table] = tables.emplace_back();
        tableClass       = getUpperNibble(*tableClassAndDestination);
        tableDestination = getLowerNibble(*tableClassAndDestination);

        if (tableClass != 0 && tableClass != 1) {
            return std::unexpected(std::format("Table class must be 0 or 1, got {}", tableClass));
        }
        constexpr uint8_t maxDestination = 3;
        if (tableDestination > maxDestination) {
            return std::unexpected(std::format("Table destination must be between 0 and 3, got {}", tableDestination));
        }

        auto tableExpected = HuffmanBuilder::readFromFile(file);
        if (!tableExpected) {
            return getUnexpected(tableExpected, "Unable to parse Huffman table");
        }
        table = std::move(*tableExpected);
    }
    const std::streampos filePosAfter = file.tellg();
    if (const auto bytesRead = filePosAfter - filePosBefore; bytesRead != *length) {
        return std::unexpected(std::format("Length mismatch. Length was {}, however {} bytes was read", *length, bytesRead));
    }
    return tables;
}

auto FileParser::Jpeg::JpegParser::parseScanHeaderComponent(
    std::ifstream& file
) -> std::expected<NewScanHeaderComponent, std::string> {
    const auto selector = read_uint8(file);
    if (!selector) {
        return getUnexpected(selector, "Unable to parse scan header component");
    }
    const auto destination = read_uint8(file);
    if (!destination) {
        return getUnexpected(destination, "Unable to parse scan header component");
    }

    NewScanHeaderComponent component {
        .componentSelector = *selector,
        .dcTableSelector = getUpperNibble(*destination),
        .acTableSelector = getLowerNibble(*destination),
    };

    constexpr uint8_t minTable = 0, maxTable = 3;
    if (component.dcTableSelector < minTable || component.dcTableSelector > maxTable) {
        return std::unexpected(std::format("Dc table selector must be between {} and {}, got {}",
            minTable, maxTable, component.dcTableSelector));
    }
    if (component.acTableSelector < minTable || component.acTableSelector > maxTable) {
        return std::unexpected(std::format("Ac table selector must be between {} and {}, got {}",
            minTable, maxTable, component.acTableSelector));
    }

    return component;
}

auto FileParser::Jpeg::JpegParser::parseScanHeader(std::ifstream& file) -> std::expected<NewScanHeader, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return getUnexpected(length, "Unable to parse length");
    }

    const auto numberOfComponents = read_uint8(file);
    if (!numberOfComponents) {
        return getUnexpected(numberOfComponents, "Unable to parse number of components");
    }

    const uint16_t expectedLength = 6 + 2 * numberOfComponents.value();
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    NewScanHeader scanHeader;
    for (uint16_t i = 0; i < numberOfComponents.value(); ++i) {
        const auto component = parseScanHeaderComponent(file);
        if (!component) {
            return getUnexpected(component, "Unable to parse scan header component");
        }
        scanHeader.components.push_back(*component);
    }

    const auto ss = read_uint8(file);
    if (!ss) {
        return getUnexpected(ss, "Unable to parse spectral selection start");
    }
    scanHeader.spectralSelectionStart = *ss;

    const auto se = read_uint8(file);
    if (!se) {
        return getUnexpected(se, "Unable to parse spectral selection end");
    }
    scanHeader.spectralSelectionEnd = *se;

    const auto approximation = read_uint8(file);
    if (!approximation) {
        return getUnexpected(approximation, "Unable to parse approximation");
    }
    scanHeader.successiveApproximationHigh = getUpperNibble(*approximation);
    scanHeader.successiveApproximationLow  = getLowerNibble(*approximation);

    return scanHeader;
}

auto FileParser::Jpeg::JpegParser::parseECS(std::ifstream& file) -> std::expected<std::vector<std::vector<uint8_t>>, std::string> {
    uint8_t pair[2];
    if (!file.read(reinterpret_cast<char *>(pair), 2)) {
        return std::unexpected("Unable to parse ECS");
    }

    size_t currentSection = 0;
    std::vector sections(1, std::vector<uint8_t>());
    uint8_t prevRST = RST7; // Init to the last RST

    while (true) {
        if (pair[0] == MarkerHeader) {
            if (pair[1] == ByteStuffing) {
                // If we encounter 0xFF00 for byte stuffing, it is treated as a literal 0xFF
                sections[currentSection].push_back(pair[0]);
            } else if (isRST(pair[1])) {
                if (getNextRST(prevRST) != pair[1]) {
                    return std::unexpected("RST markers were not encountered in the correct order");
                }
                currentSection++;
                sections.emplace_back();
                prevRST = pair[1];
            } else {
                // Encountered different marker, noting the end of the ECS
                file.seekg(-2, std::ios::cur);
                return sections;
            }
        } else {
            sections[currentSection].push_back(pair[0]);
        }
        pair[0] = pair[1];
        const auto nextByte = read_uint8(file);
        if (!nextByte) {
            return getUnexpected(nextByte, "Unable to parse ECS");
        }
        pair[1] = *nextByte;
    }
}

auto FileParser::Jpeg::JpegParser::parseSOS(std::ifstream& file) -> std::expected<Scan, std::string> {
    Scan scan;
    const auto header = parseScanHeader(file);
    if (!header) {
        return getUnexpected(header, "Unable to parse scan header");
    }
    scan.header = *header;

    auto ecs = parseECS(file);
    if (!ecs) {
        return getUnexpected(ecs, "Unable to parse ECS");
    }
    scan.dataSections = std::move(*ecs);
    return scan;
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
    while (file.read(reinterpret_cast<char*>(&byte), 1)) {
        if (byte == 0xFF) {
            uint8_t marker;
            file.read(reinterpret_cast<char*>(&marker), 1);
            switch (marker) {
                case DHT: {
                    auto huffmanParseResults = parseDHT(file);
                    if (!huffmanParseResults) {
                        return getUnexpected(huffmanParseResults, "Unable to parse DHT data");
                    }
                    for (const auto& [tableClass, tableDestination, table] : *huffmanParseResults) {
                        auto& tableVec = tableClass == 0 ? data.dcHuffmanTables : data.acHuffmanTables;
                        tableVec[tableDestination].push_back(table);
                    }
                    break;
                }
                case DQT: {
                    auto quantizationTables = parseDQT(file);
                    if (!quantizationTables) {
                        return getUnexpected(quantizationTables, "Unable to parse DQT data");
                    }
                    for (const auto& table : *quantizationTables) {
                        data.quantizationTables[table.destination].push_back(table);
                    }
                    break;
                }
                case DNL: {
                    if (encounteredMarkers.contains(DNL)) {
                        return std::unexpected("Multiple DNL markers encountered. Only one DNL marker is allowed");
                    }
                    auto numberOfLines = parseDNL(file);
                    if (!numberOfLines) {
                        return getUnexpected(numberOfLines, "Unable to parse DNL");
                    }
                    data.frameHeader.numberOfLines = *numberOfLines;
                    break;
                }
                case DRI: {
                    if (encounteredMarkers.contains(DRI)) {
                        return std::unexpected("Multiple DRI markers encountered. Only one DRI marker is allowed");
                    }
                    auto restartInterval = parseDRI(file);
                    if (!restartInterval) {
                        return getUnexpected(restartInterval, "Unable to parse restart interval");
                    }
                    data.restartInterval = *restartInterval;
                    break;
                }
                case COM: {
                    auto comment = parseComment(file);
                    if (!comment) {
                        return getUnexpected(comment, "Unable to parse comment");
                    }
                    data.comments.push_back(std::move(*comment));
                    break;
                }
                case SOS: {
                    auto scan = parseSOS(file);
                    if (!scan) {
                        return getUnexpected(scan, "Unable to parse SOS");
                    }
                    data.scan = *scan;
                    break;
                }
                case EOI: {
                    // decode();
                    break;
                }
                default: {
                    if (isSOF(marker)) {
                        if (std::ranges::any_of(encounteredMarkers, [](const uint8_t m) { return isSOF(m); })) {
                            return std::unexpected("Multiple SOF markers encountered. Only one SOF marker is allowed");
                        }
                        auto frameHeader = parseFrameHeader(file, marker);
                        if (!frameHeader) {
                            return getUnexpected(frameHeader, "Unable to parse frame header");
                        }
                        data.frameHeader = *frameHeader;
                    } else {
                        std::cout << std::hex << "Other byte encountered: " << static_cast<int>(marker) << std::endl;
                    }
                }
            }
            encounteredMarkers.insert(marker);
        }
    }

    // Check if number of lines in frame == 0, error
    // Handle non-optional markers such as SOS and SOF
    // File must start with SOI and end with EOI

    return {};
}
