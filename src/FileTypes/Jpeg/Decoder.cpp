#include "FileParser/Jpeg/Decoder.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Image.hpp"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
#include "FileParser/Jpeg/Markers.hpp"
#include "FileParser/Utils.hpp"
#include "FileParser/Jpeg/Transform.hpp"

auto FileParser::Jpeg::JpegParser::parseFrameComponent(
    std::ifstream& file
) -> std::expected<FrameComponent, std::string> {
    FrameComponent component;

    const auto identifier = read_uint8(file);
    if (!identifier) {
        return utils::getUnexpected(identifier, "Unable to read component identifier");
    }
    component.identifier = identifier.value();

    const auto samplingFactor = read_uint8(file);
    if (!samplingFactor) {
        return utils::getUnexpected(samplingFactor, "Unable to read sampling factor");
    }
    component.horizontalSamplingFactor  = getUpperNibble(samplingFactor.value());
    component.verticalSamplingFactor    = getLowerNibble(samplingFactor.value());

    const auto qTableSelector = read_uint8(file);
    if (!qTableSelector) {
        return utils::getUnexpected(qTableSelector, "Unable to read quantization table selector");
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

auto FileParser::Jpeg::JpegParser::parseFrameHeader(std::ifstream& file, const uint8_t SOF) -> std::expected<FrameHeader, std::string> {
    if (SOF != SOF0) {
        return std::unexpected(std::format(R"(Unsupported start of frame marker: "{}")", SOF));
    }

    FrameHeader frame;
    const auto length = read_uint16_be(file);
    if (!length) {
        return utils::getUnexpected(length, "Unable to read frame length");
    }

    const auto precision = read_uint8(file);
    if (!precision) {
        return utils::getUnexpected(precision, "Unable to read frame precision");
    }
    frame.precision = precision.value();
    if (frame.precision != 8) {
        return std::unexpected(
            std::format("Unsupported precision in frame header: {}, precision must be 8 bits", frame.precision));
    }

    const auto numberOfLines = read_uint16_be(file);
    if (!numberOfLines) {
        return utils::getUnexpected(numberOfLines, "Unable to read number of lines");
    }
    frame.numberOfLines = numberOfLines.value();

    const auto numberOfSamplesPerLine = read_uint16_be(file);
    if (!numberOfSamplesPerLine) {
        return utils::getUnexpected(numberOfSamplesPerLine, "Unable to read number of samples per line");
    }
    frame.numberOfSamplesPerLine = numberOfSamplesPerLine.value();

    const auto numberOfComponents = read_uint8(file);
    if (!numberOfComponents) {
        return utils::getUnexpected(numberOfComponents, "Unable to read number of components");
    }

    const uint16_t expectedLength = 8 + 3 * *numberOfComponents;
    if (*length != expectedLength) {
        return std::unexpected(
            std::format(R"(Specified length of FrameHeader "{}" does not match expected length of "{}")",
                *length, expectedLength));
    }

    constexpr uint16_t minComponents = 1, maxComponents = 4;
    if (*numberOfComponents < minComponents || *numberOfComponents > maxComponents) {
        return std::unexpected(std::format(R"(Number of components must be between {} and {}, got {})",
            minComponents, maxComponents, *numberOfComponents));
    }


    for (size_t i = 0; i < *numberOfComponents; ++i) {
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
        return utils::getUnexpected(length, "Unable to parse length");
    }
    constexpr uint16_t expectedLength = 4;
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    const auto numberOfLines = read_uint16_be(file);
    if (!numberOfLines) {
        return utils::getUnexpected(numberOfLines, "Unable to parse number of lines");
    }
    return numberOfLines;
}

auto FileParser::Jpeg::JpegParser::parseDRI(
    std::ifstream& file
) -> std::expected<uint16_t, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return utils::getUnexpected(length, "Unable to parse length");
    }
    constexpr uint16_t expectedLength = 4;
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    const auto restartInterval = read_uint16_be(file);
    if (!restartInterval) {
        return utils::getUnexpected(restartInterval, "Unable to parse restart interval");
    }
    return restartInterval;
}

auto FileParser::Jpeg::JpegParser::parseComment(std::ifstream& file) -> std::expected<std::string, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return utils::getUnexpected(length, "Unable to parse length");
    }
    auto comment = read_string(file, *length - 2);
    if (!comment) {
        return utils::getUnexpected(comment, "Unable to parse comment");
    }
    return comment;
}

auto FileParser::Jpeg::JpegParser::parseDQT(
    std::ifstream& file
) -> std::expected<std::vector<QuantizationTable>, std::string> {
    const std::streampos filePosBefore = file.tellg();

    const auto length = read_uint16_be(file);
    if (!length) {
        return utils::getUnexpected(length, "Unable to parse length");
    }

    std::vector<QuantizationTable> tables;
    while (file.tellg() - filePosBefore < *length && file) {
        const auto precisionAndDestination = read_uint8(file);
        if (!precisionAndDestination) {
            return utils::getUnexpected(precisionAndDestination, "Unable to parse precision and id");
        }

        QuantizationTable& table = tables.emplace_back();
        // Precision (1 = 16-bit, 0 = 8-bit) in upper nibble, Destination ID in lower nibble
        table.precision   = getUpperNibble(*precisionAndDestination);
        table.destination = getLowerNibble(*precisionAndDestination);

        if (table.precision == 0) {
            const auto elements = read_uint8(file, QuantizationTable::length);
            if (!elements) {
                return utils::getUnexpected(elements, "Unable to parse quantization table elements");
            }
            for (const auto i : zigZagMap) {
                table[i] = static_cast<float>((*elements)[i]);
            }
        } else {
            const auto elements = read_uint16_be(file, QuantizationTable::length);
            if (!elements) {
                return utils::getUnexpected(elements, "Unable to parse quantization table elements");
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
        return utils::getUnexpected(length, "Unable to parse length");
    }

    std::vector<HuffmanParseResult> tables;
    while (file.tellg() - filePosBefore < *length && file) {
        const auto tableClassAndDestination = read_uint8(file);
        if (!tableClassAndDestination) {
            return utils::getUnexpected(tableClassAndDestination, "Unable to parse table class and destination");
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
            return utils::getUnexpected(tableExpected, "Unable to parse Huffman table");
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
) -> std::expected<ScanComponent, std::string> {
    const auto selector = read_uint8(file);
    if (!selector) {
        return utils::getUnexpected(selector, "Unable to parse scan header component");
    }
    const auto destination = read_uint8(file);
    if (!destination) {
        return utils::getUnexpected(destination, "Unable to parse scan header component");
    }

    ScanComponent component {
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

auto FileParser::Jpeg::JpegParser::parseScanHeader(std::ifstream& file) -> std::expected<ScanHeader, std::string> {
    const auto length = read_uint16_be(file);
    if (!length) {
        return utils::getUnexpected(length, "Unable to parse length");
    }

    const auto numberOfComponents = read_uint8(file);
    if (!numberOfComponents) {
        return utils::getUnexpected(numberOfComponents, "Unable to parse number of components");
    }

    const uint16_t expectedLength = 6 + 2 * numberOfComponents.value();
    if (*length != expectedLength) {
        return std::unexpected(std::format("Expected length {}, got {}", expectedLength, *length));
    }

    ScanHeader scanHeader;
    for (uint16_t i = 0; i < numberOfComponents.value(); ++i) {
        const auto component = parseScanHeaderComponent(file);
        if (!component) {
            return utils::getUnexpected(component, "Unable to parse scan header component");
        }
        scanHeader.components.push_back(*component);
    }

    const auto ss = read_uint8(file);
    if (!ss) {
        return utils::getUnexpected(ss, "Unable to parse spectral selection start");
    }
    scanHeader.spectralSelectionStart = *ss;

    const auto se = read_uint8(file);
    if (!se) {
        return utils::getUnexpected(se, "Unable to parse spectral selection end");
    }
    scanHeader.spectralSelectionEnd = *se;

    const auto approximation = read_uint8(file);
    if (!approximation) {
        return utils::getUnexpected(approximation, "Unable to parse approximation");
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
                pair[0] = pair[1];
                const auto nextByte = read_uint8(file);
                if (!nextByte) {
                    return std::unexpected("Unable to parse ECS");
                }
                pair[1] = *nextByte;
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
            return utils::getUnexpected(nextByte, "Unable to parse ECS");
        }
        pair[1] = *nextByte;
    }
}

auto FileParser::Jpeg::JpegParser::parseSOS(std::ifstream& file) -> std::expected<Scan, std::string> {
    Scan scan;
    const auto header = parseScanHeader(file);
    if (!header) {
        return utils::getUnexpected(header, "Unable to parse scan header");
    }
    scan.header = *header;

    auto ecs = parseECS(file);
    if (!ecs) {
        return utils::getUnexpected(ecs, "Unable to parse ECS");
    }
    scan.dataSections = std::move(*ecs);
    return scan;
}

auto FileParser::Jpeg::JpegParser::parseEOI(std::ifstream& file) -> std::expected<void, std::string> {
    char dummy;
    if (file.read(&dummy, 1)) {
        return std::unexpected("Unexpected bytes encountered after EOI marker");
    }
    return {};
}

auto FileParser::Jpeg::JpegParser::analyzeFrameHeader(const FrameHeader& header, const uint8_t SOF) -> std::expected<FrameInfo, std::string> {
    FrameInfo info;
    info.frameMarker = SOF;
    info.header      = header;

    if (header.components.size() != 3) {
        return std::unexpected("Number of components must be equal to 3. Only YCbCr Jpegs are supported");
    }

    for (const auto& comp : header.components) {
        if (comp.horizontalSamplingFactor != 1 || comp.verticalSamplingFactor != 1) {
            if (info.luminanceID != FrameInfo::unassignedID) {
                return std::unexpected("Multiple components with sampling factors greater than one detected");
            }
            info.luminanceID = comp.identifier;
            info.luminanceHorizontalSamplingFactor = comp.horizontalSamplingFactor;
            info.luminanceVerticalSamplingFactor   = comp.verticalSamplingFactor;
        }
    }

    for (const auto& comp : header.components) {
        if (info.luminanceID == FrameInfo::unassignedID) {
            info.luminanceID = comp.identifier;
            info.luminanceHorizontalSamplingFactor = 1;
            info.luminanceVerticalSamplingFactor   = 1;
        } else if (info.chrominanceBlueID == FrameInfo::unassignedID) {
            info.chrominanceBlueID = comp.identifier;
        } else if (info.chrominanceRedID == FrameInfo::unassignedID) {
            info.chrominanceRedID = comp.identifier;
        }
    }

    constexpr size_t componentSideLength = 8;
    info.mcuWidth  = utils::ceilDivide<uint32_t>(info.header.numberOfSamplesPerLine, componentSideLength * info.luminanceHorizontalSamplingFactor);
    info.mcuHeight = utils::ceilDivide<uint32_t>(info.header.numberOfLines         , componentSideLength * info.luminanceVerticalSamplingFactor);
    return info;
}

auto FileParser::Jpeg::JpegParser::parseFile(
    const std::filesystem::path& filePath
) -> std::expected<JpegData, std::string> {
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

    uint8_t soiBytes[2];
    auto soiExpected = read_bytes(reinterpret_cast<char *>(soiBytes), file, 2);
    if (!soiExpected) {
        return utils::getUnexpected(soiExpected, "Unable to parse SOI");
    }
    if (soiBytes[0] != MarkerHeader || soiBytes[1] != SOI) {
        return std::unexpected("File must start with SOI marker");
    }

    std::unordered_set encounteredMarkers{SOI};
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
                        return utils::getUnexpected(huffmanParseResults, "Unable to parse DHT data");
                    }
                    for (const auto& [tableClass, tableDestination, table] : *huffmanParseResults) {
                        auto& tableVec = tableClass == 0 ? data.huffmanTables.dc : data.huffmanTables.ac;
                        tableVec[tableDestination].push_back(table);
                    }
                    break;
                }
                case DQT: {
                    auto quantizationTables = parseDQT(file);
                    if (!quantizationTables) {
                        return utils::getUnexpected(quantizationTables, "Unable to parse DQT data");
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
                        return utils::getUnexpected(numberOfLines, "Unable to parse DNL");
                    }
                    data.frameInfo.header.numberOfLines = *numberOfLines;
                    break;
                }
                case DRI: {
                    auto restartInterval = parseDRI(file);
                    if (!restartInterval) {
                        return utils::getUnexpected(restartInterval, "Unable to parse restart interval");
                    }
                    data.lastSetRestartInterval = *restartInterval;
                    break;
                }
                case COM: {
                    auto comment = parseComment(file);
                    if (!comment) {
                        return utils::getUnexpected(comment, "Unable to parse comment");
                    }
                    data.comments.push_back(std::move(*comment));
                    break;
                }
                case SOS: {
                    auto scan = parseSOS(file);
                    if (!scan) {
                        return utils::getUnexpected(scan, "Unable to parse SOS");
                    }
                    scan->restartInterval = data.lastSetRestartInterval;
                    for (size_t i = 0; i < 4; i++) {
                        scan->iterations.quantization[i] = data.quantizationTables[i].size() - 1;
                        scan->iterations.dc[i] = data.huffmanTables.dc[i].size() - 1;
                        scan->iterations.ac[i] = data.huffmanTables.ac[i].size() - 1;
                    }
                    data.scans.push_back(std::move(*scan));
                    break;
                }
                case EOI: {
                    const auto eoi = parseEOI(file);
                    if (!eoi) {
                        return std::unexpected(eoi.error());
                    }
                    break;
                }
                default: {
                    if (isSOF(marker)) {
                        if (std::ranges::any_of(encounteredMarkers, [](const uint8_t m) { return isSOF(m); })) {
                            return std::unexpected("Multiple SOF markers encountered. Only one SOF marker is allowed");
                        }
                        auto frameHeader = parseFrameHeader(file, marker);
                        if (!frameHeader) {
                            return utils::getUnexpected(frameHeader, "Unable to parse frame header");
                        }
                        auto frameInfo = analyzeFrameHeader(*frameHeader, marker);
                        if (!frameInfo) {
                            return utils::getUnexpected(frameHeader, "Unable to analyze frame header");
                        }
                        data.frameInfo = std::move(*frameInfo);
                    } else {
                        std::cout << std::hex << "Other byte encountered: " << static_cast<int>(marker) << std::endl;
                    }
                }
            }
            encounteredMarkers.insert(marker);
        }
    }

    // Check required markers
    if (!std::ranges::contains(encounteredMarkers, SOI)) {
        return std::unexpected("Missing SOI (Start of Image) marker");
    }
    if (!std::ranges::any_of(encounteredMarkers, [](const uint8_t m) { return isSOF(m); })) {
        return std::unexpected("Missing SOF (Start of Frame) marker");
    }
    if (!std::ranges::contains(encounteredMarkers, SOS)) {
        return std::unexpected("No SOS (Start of Scan) marker found");
    }
    if (!std::ranges::contains(encounteredMarkers, EOI)) {
        return std::unexpected("Missing EOI (End of Image) marker");
    }

    // Validate data
    if (data.frameInfo.header.numberOfLines == 0) {
        return std::unexpected("Number of lines was never specified");
    }
    if (data.frameInfo.header.numberOfSamplesPerLine == 0) {
        return std::unexpected("Number of samples per line is 0");
    }
    // Ensure scan components reference valid frame components
    for (const auto& scan : data.scans) {
        for (const auto& scanComp : scan.header.components) {
            if (!std::ranges::any_of(data.frameInfo.header.components, [&scanComp](const auto frameComp) {
                return frameComp.identifier == scanComp.componentSelector;
            })) {
                return std::unexpected("Scan references undefined component");
            }
        }
    }

    return data;
}

auto FileParser::Jpeg::JpegDecoder::isEOB(const int r, const int s) -> bool {
    return r == 0x0 && s == 0x0;
}

auto FileParser::Jpeg::JpegDecoder::isZRL(const int r, const int s) -> bool {
    return r == 0xF && s == 0x0;
}

int FileParser::Jpeg::JpegDecoder::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = static_cast<int>(bitReader.getNBits(SSSS));
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

auto FileParser::Jpeg::JpegDecoder::decodeNextValue(BitReader& bitReader, const HuffmanTable& table) -> uint8_t {
    auto [bitLength, value] = table.decode(bitReader.peekUInt16());
    bitReader.skipBits(bitLength);
    return value;
}

int FileParser::Jpeg::JpegDecoder::decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    const int sCategory = decodeNextValue(bitReader, huffmanTable);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

auto FileParser::Jpeg::JpegDecoder::decodeAcCoefficient(
    BitReader& bitReader, const HuffmanTable& huffmanTable
) -> ACCoefficientResult {
    const uint8_t rs = decodeNextValue(bitReader, huffmanTable);
    return {getUpperNibble(rs), getLowerNibble(rs)};
}

auto FileParser::Jpeg::JpegDecoder::decodeComponent(
    BitReader& bitReader, const ScanComponent& scanComp, const TableIterations& iterations,
    const HuffmanTables& huffmanTables, PreviousDC& prevDc
) -> std::expected<Component, std::string> {
    Component result;
    // DC Coefficient
    const auto& dcTable = huffmanTables.dc[scanComp.dcTableSelector][iterations.dc[scanComp.dcTableSelector]];
    const int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[scanComp.componentSelector - 1];
    result[0] = static_cast<float>(dcCoefficient);
    prevDc[scanComp.componentSelector - 1] = dcCoefficient;

    // AC Coefficients
    size_t index = 1;
    while (index < Component::length) {
        const auto& acTable = huffmanTables.ac[scanComp.acTableSelector][iterations.ac[scanComp.acTableSelector]];
        auto [r, s] = decodeAcCoefficient(bitReader, acTable);
        if (static_cast<size_t>(r) > Component::length - index) {
            return std::unexpected("Run length would exceed component bounds");
        }
        if (s < 0 || s > 10) {  // Typical JPEG SSSS range
            return std::unexpected(std::format("Invalid SSSS value in AC coefficient: {}", s));
        }
        if (isEOB(r, s)) {
            // Remaining coefficients are 0
            break;
        }
        if (isZRL(r, s)) {
            // 16 zeros in a row
            index += 16;
            continue;
        }
        index += r;
        const int coefficient = decodeSSSS(bitReader, s);
        result[zigZagMap[index]] = static_cast<float>(coefficient);
        index++;
    }
    return result;
}

auto FileParser::Jpeg::JpegDecoder::decodeMcu(
    BitReader& bitReader, const FrameInfo& frame, const ScanHeader& scanHeader,
    const TableIterations& iterations, const HuffmanTables& huffmanTables, PreviousDC& prevDc
) -> std::expected<Mcu, std::string> {
    auto mcu = Mcu(frame.luminanceHorizontalSamplingFactor, frame.luminanceVerticalSamplingFactor);
    for (const auto& scanComp : scanHeader.components) {
        if (scanComp.componentSelector == frame.luminanceID) {
            const auto numLuminanceComponents = frame.luminanceHorizontalSamplingFactor * frame.luminanceVerticalSamplingFactor;
            for (int i = 0; i < numLuminanceComponents; i++) {
                const auto comp = decodeComponent(bitReader, scanComp, iterations, huffmanTables, prevDc);
                if (!comp) {
                    return utils::getUnexpected(comp, "Unable to parse luminance component");
                }
                mcu.Y[i] = *comp;
            }
        } else if (scanComp.componentSelector == frame.chrominanceBlueID) {
            const auto comp = decodeComponent(bitReader, scanComp, iterations, huffmanTables, prevDc);
            if (!comp) {
                return utils::getUnexpected(comp, "Unable to parse chrominance blue component");
            }
            mcu.Cb = *comp;
        } else if (scanComp.componentSelector == frame.chrominanceRedID) {
            const auto comp = decodeComponent(bitReader, scanComp, iterations, huffmanTables, prevDc);
            if (!comp) {
                return utils::getUnexpected(comp, "Unable to parse chrominance red component");
            }
            mcu.Cr = *comp;
        }
    }
    return mcu;
}

auto FileParser::Jpeg::JpegDecoder::decodeRSTSegment(
    const FrameInfo& frame, const ScanHeader& scanHeader, const TableIterations& iterations,
    const HuffmanTables& huffmanTables, const uint16_t restartInterval, const std::vector<uint8_t>& rstData
) -> std::expected<std::vector<Mcu>, std::string> {
    BitReader bitReader{rstData};
    std::vector<Mcu> mcus;
    PreviousDC prevDc{};

    const size_t mcusToRead = restartInterval != 0 ? restartInterval : frame.mcuWidth * frame.mcuHeight;
    for (size_t i = 0; i < mcusToRead; i++) {
        const auto mcu = decodeMcu(bitReader, frame, scanHeader, iterations, huffmanTables, prevDc);
        if (!mcu) {
            return utils::getUnexpected(mcu, "Unable to decode MCU");
        }
        mcus.push_back(*mcu);
    }

    bitReader.alignToByte();
    if (!bitReader.reachedEnd()) {
        return std::unexpected("Extra unused data found before the end of RST marker");
    }
    return mcus;
}

auto FileParser::Jpeg::JpegDecoder::decodeScan(
    const FrameInfo& frame, const Scan& scan, const HuffmanTables& huffmanTables
) -> std::expected<std::vector<Mcu>, std::string> {
    std::vector<Mcu> mcus;

    for (const auto& section : scan.dataSections) {
        const auto decodedSection =
            decodeRSTSegment(frame, scan.header, scan.iterations, huffmanTables, scan.restartInterval, section);
        if (!decodedSection) {
            return utils::getUnexpected(decodedSection, "Unable to decode RST segment");
        }
        mcus.insert(mcus.end(), decodedSection->begin(), decodedSection->end());
    }

    return mcus;
}

auto FileParser::Jpeg::JpegDecoder::decode(
    const std::filesystem::path& filePath
) -> std::expected<Image, std::string> {
    const auto data = JpegParser::parseFile(filePath);
    if (!data) {
        return utils::getUnexpected(data, "Unable to parse file");
    }

    const size_t width  = data->frameInfo.header.numberOfSamplesPerLine;
    const size_t height = data->frameInfo.header.numberOfLines;

    auto mcus = decodeScan(data->frameInfo, data->scans[0], data->huffmanTables);
    if (!mcus) {
        return utils::getUnexpected(mcus, "Unable to decode scan");
    }

    for (auto& mcu: *mcus) {
        dequantize(mcu, data->frameInfo, data->scans[0].header, data->scans[0].iterations, data->quantizationTables);
        inverseDCT(mcu);
    }

    const std::vector<ColorBlock> colorBlocks = convertMcusToColorBlocks(*mcus, width, height);
    std::vector<uint8_t> rgbData = getRawRGBData(colorBlocks, width, height);
    return Image(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::move(rgbData));
}
