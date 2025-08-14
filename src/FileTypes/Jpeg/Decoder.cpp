#include "FileParser/Jpeg/Decoder.hpp"

#include <algorithm>
#include <format>
#include <fstream>
#include <iostream>
#include <unordered_set>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/FileUtil.h"
#include "FileParser/Image.hpp"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
#include "FileParser/Jpeg/Markers.hpp"
#include "FileParser/Jpeg/Transform.hpp"
#include "FileParser/Macros.hpp"
#include "FileParser/Utils.hpp"

#define READ_LENGTH() ASSIGN_OR_RETURN(length, read_uint16_be(file), "Unable to read length");

#define REQUIRE_LENGTH(actual, expected) \
    if ((actual) != (expected)) { \
        return std::unexpected(std::format("Expected length {}, got {}", expected, actual)); \
    }

#define READ_AND_REQUIRE_LENGTH(expected) \
    READ_LENGTH() \
    REQUIRE_LENGTH(length, expected)


auto FileParser::Jpeg::Parser::parseFrameComponent(
    std::ifstream& file
) -> std::expected<FrameComponent, std::string> {
    ASSIGN_OR_RETURN(identifier,     read_uint8(file), "Unable to read component identifier");
    ASSIGN_OR_RETURN(samplingFactor, read_uint8(file), "Unable to read sampling factor");
    ASSIGN_OR_RETURN(qTableSelector, read_uint8(file), "Unable to read quantization table selector");

    FrameComponent component {
        .identifier                = identifier,
        .horizontalSamplingFactor  = getUpperNibble(samplingFactor),
        .verticalSamplingFactor    = getLowerNibble(samplingFactor),
        .quantizationTableSelector = qTableSelector
    };

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

auto FileParser::Jpeg::Parser::parseFrameHeader(std::ifstream& file, const uint8_t SOF) -> std::expected<FrameHeader, std::string> {
    if (SOF != SOF0) {
        return std::unexpected(std::format(R"(Unsupported start of frame marker: "{}")", SOF));
    }

    FrameHeader frame;
    READ_LENGTH();
    ASSIGN_OR_RETURN(precision, read_uint8(file), "Unable to read frame precision");
    frame.precision = precision;
    if (frame.precision != 8) {
        return std::unexpected(
            std::format("Unsupported precision in frame header: {}, precision must be 8 bits", frame.precision));
    }

    ASSIGN_OR_RETURN(numberOfLines,          read_uint16_be(file), "Unable to read number of lines");
    ASSIGN_OR_RETURN(numberOfSamplesPerLine, read_uint16_be(file), "Unable to read number of samples per line");
    ASSIGN_OR_RETURN(numberOfComponents,     read_uint8(file),     "Unable to read number of components");
    frame.numberOfLines          = numberOfLines;
    frame.numberOfSamplesPerLine = numberOfSamplesPerLine;

    const uint16_t expectedLength = 8 + 3 * numberOfComponents;
    REQUIRE_LENGTH(length, expectedLength);

    constexpr uint16_t minComponents = 1, maxComponents = 4;
    if (numberOfComponents < minComponents || numberOfComponents > maxComponents) {
        return std::unexpected(std::format(R"(Number of components must be between {} and {}, got {})",
            minComponents, maxComponents, numberOfComponents));
    }

    for (size_t i = 0; i < numberOfComponents; ++i) {
        auto component = parseFrameComponent(file);
        if (!component) {
            return std::unexpected(std::format("Error parsing frame component #{}: {}", i, component.error()));
        }
        frame.components.push_back(*component);
    }

    return frame;
}

auto FileParser::Jpeg::Parser::parseDNL(
    std::ifstream& file
) -> std::expected<uint16_t, std::string> {
    READ_AND_REQUIRE_LENGTH(4);
    ASSIGN_OR_RETURN(numberOfLines, read_uint16_be(file), "Unable to read number of lines");
    return numberOfLines;
}

auto FileParser::Jpeg::Parser::parseDRI(
    std::ifstream& file
) -> std::expected<uint16_t, std::string> {
    READ_AND_REQUIRE_LENGTH(4);
    ASSIGN_OR_RETURN(restartInterval, read_uint16_be(file), "Unable to read restart interval");
    return restartInterval;
}

auto FileParser::Jpeg::Parser::parseComment(std::ifstream& file) -> std::expected<std::string, std::string> {
    READ_LENGTH();
    ASSIGN_OR_RETURN(comment, read_string(file, length - 2), "Unable to read comment");
    return comment;
}

auto FileParser::Jpeg::Parser::parseDQT(
    std::ifstream& file
) -> std::expected<std::vector<QuantizationTable>, std::string> {
    const std::streampos filePosBefore = file.tellg();
    READ_LENGTH();

    std::vector<QuantizationTable> tables;
    while (file.tellg() - filePosBefore < length && file) {
        ASSIGN_OR_RETURN(precisionAndDestination, read_uint8(file), "Unable to read precision and id");

        QuantizationTable& table = tables.emplace_back();
        // Precision (1 = 16-bit, 0 = 8-bit) in upper nibble, Destination ID in lower nibble
        table.precision   = getUpperNibble(precisionAndDestination);
        table.destination = getLowerNibble(precisionAndDestination);

        if (table.precision == 0) {
            ASSIGN_OR_RETURN(elements, read_uint8(file, QuantizationTable::length), "Unable to read quantization table elements");
            for (const auto i : zigZagMap) {
                table[i] = static_cast<float>((elements)[i]);
            }
        } else {
            ASSIGN_OR_RETURN(elements, read_uint16_be(file, QuantizationTable::length), "Unable to read quantization table elements");
            for (const auto i : zigZagMap) {
                table[i] = static_cast<float>((elements)[i]);
            }
        }
    }
    const std::streampos filePosAfter = file.tellg();
    if (const auto bytesRead = filePosAfter - filePosBefore; bytesRead != length) {
        return std::unexpected(std::format("Length mismatch. Length was {}, however {} bytes was read", length, bytesRead));
    }
    return tables;
}

auto FileParser::Jpeg::Parser::parseDHT(
    std::ifstream& file
) -> std::expected<std::vector<HuffmanParseResult>, std::string> {
    const std::streampos filePosBefore = file.tellg();
    READ_LENGTH()
    std::vector<HuffmanParseResult> tables;
    while (file.tellg() - filePosBefore < length && file) {
        ASSIGN_OR_RETURN(tableClassAndDestination, read_uint8(file), "Unable to parse table class and destination");
        auto& [tableClass, tableDestination, table] = tables.emplace_back();
        tableClass       = getUpperNibble(tableClassAndDestination);
        tableDestination = getLowerNibble(tableClassAndDestination);

        if (tableClass != 0 && tableClass != 1) {
            return std::unexpected(std::format("Table class must be 0 or 1, got {}", tableClass));
        }
        constexpr uint8_t maxDestination = 3;
        if (tableDestination > maxDestination) {
            return std::unexpected(std::format("Table destination must be between 0 and 3, got {}", tableDestination));
        }

        ASSIGN_OR_RETURN_MUT(constructedTable, HuffmanBuilder::readFromFile(file), "Unable to parse Huffman table");
        table = std::move(constructedTable);
    }
    const std::streampos filePosAfter = file.tellg();
    if (const auto bytesRead = filePosAfter - filePosBefore; bytesRead != length) {
        return std::unexpected(std::format("Length mismatch. Length was {}, however {} bytes was read", length, bytesRead));
    }
    return tables;
}

auto FileParser::Jpeg::Parser::parseScanHeaderComponent(
    std::ifstream& file
) -> std::expected<ScanComponent, std::string> {
    ASSIGN_OR_RETURN(selector,    read_uint8(file), "Unable to read component selector");
    ASSIGN_OR_RETURN(destination, read_uint8(file), "Unable to read table destination");

    ScanComponent component {
        .componentSelector = selector,
        .dcTableSelector = getUpperNibble(destination),
        .acTableSelector = getLowerNibble(destination),
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

auto FileParser::Jpeg::Parser::parseScanHeader(std::ifstream& file) -> std::expected<ScanHeader, std::string> {
    READ_LENGTH();
    ASSIGN_OR_RETURN(numberOfComponents, read_uint8(file), "Unable to read number of components");
    const uint16_t expectedLength = 6 + 2 * numberOfComponents;
    REQUIRE_LENGTH(length, expectedLength);

    ScanHeader scanHeader;
    for (uint8_t i = 0; i < numberOfComponents; ++i) {
        ASSIGN_OR_RETURN(component, parseScanHeaderComponent(file), "Unable to read scan header component");
        scanHeader.components.push_back(component);
    }

    ASSIGN_OR_RETURN(ss, read_uint8(file), "Unable to read spectral selection start");
    ASSIGN_OR_RETURN(se, read_uint8(file), "Unable to read spectral selection end");
    ASSIGN_OR_RETURN(approximation, read_uint8(file), "Unable to read approximation");

    scanHeader.spectralSelectionStart = ss;
    scanHeader.spectralSelectionEnd   = se;
    scanHeader.successiveApproximationHigh = getUpperNibble(approximation);
    scanHeader.successiveApproximationLow  = getLowerNibble(approximation);
    return scanHeader;
}

auto FileParser::Jpeg::Parser::parseECS(std::ifstream& file) -> std::expected<std::vector<std::vector<uint8_t>>, std::string> {
    uint8_t pair[2];
    if (!file.read(reinterpret_cast<char *>(pair), 2)) {
        return std::unexpected("Unable to parse ECS");
    }

    size_t currentSection = 0;
    std::vector sections(1, std::vector<uint8_t>());
    uint8_t prevRST = RST7; // Init to the last RST

#define shiftByte()  \
    pair[0] = pair[1]; \
    const auto nextByte = read_uint8(file); \
    if (!nextByte) { \
        return utils::getUnexpected(nextByte, "Unable to parse ECS"); \
    } \
    pair[1] = *nextByte; \

    while (true) {
        if (pair[0] == MarkerHeader) {
            if (pair[1] == ByteStuffing) {
                // If we encounter 0xFF00 for byte stuffing, it is treated as a literal 0xFF
                sections[currentSection].push_back(pair[0]);
                shiftByte();
            } else if (isRST(pair[1])) {
                if (getNextRST(prevRST) != pair[1]) {
                    return std::unexpected("RST markers were not encountered in the correct order");
                }
                currentSection++;
                sections.emplace_back();
                prevRST = pair[1];
                shiftByte();
            } else {
                // Encountered different marker, noting the end of the ECS
                file.seekg(-2, std::ios::cur);
                return sections;
            }
        } else {
            sections[currentSection].push_back(pair[0]);
        }
        shiftByte();
    }
#undef shiftByte
}

auto FileParser::Jpeg::Parser::parseSOS(std::ifstream& file) -> std::expected<Scan, std::string> {
    ASSIGN_OR_RETURN(header, parseScanHeader(file), "Unable to read scan header");
    ASSIGN_OR_RETURN_MUT(ecs, parseECS(file), "Unable to read ecs");
    return Scan { .header = header, .dataSections = std::move(ecs) };
}

auto FileParser::Jpeg::Parser::parseEOI(std::ifstream& file) -> std::expected<void, std::string> {
    char dummy;
    if (file.read(&dummy, 1)) {
        return std::unexpected("Unexpected bytes encountered after EOI marker");
    }
    return {};
}

auto FileParser::Jpeg::Parser::analyzeFrameHeader(const FrameHeader& header, const uint8_t SOF) -> std::expected<FrameInfo, std::string> {
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

auto FileParser::Jpeg::Parser::parseFile(
    const std::filesystem::path& filePath
) -> std::expected<JpegData, std::string> {
    ASSIGN_OR_PROPAGATE_MUT(file, FileUtils::openRegularFile(filePath, std::ios::binary));

    uint8_t soiBytes[2];
    CHECK_VOID_AND_RETURN(read_bytes(reinterpret_cast<char *>(soiBytes), file, 2), "Unable to parse SOI");
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
                    ASSIGN_OR_RETURN_MUT(huffmanParseResults, parseDHT(file), "Unable to parse DHT data");
                    for (auto& [tableClass, tableDestination, table] : huffmanParseResults) {
                        auto& tableVec = tableClass == 0 ? data.huffmanTables.dc : data.huffmanTables.ac;
                        tableVec[tableDestination].push_back(std::move(table));
                    }
                    break;
                }
                case DQT: {
                    ASSIGN_OR_RETURN_MUT(quantizationTables, parseDQT(file), "Unable to parse DQT data");
                    for (const auto& table : quantizationTables) {
                        data.quantizationTables[table.destination].push_back(table);
                    }
                    break;
                }
                case DNL: {
                    if (encounteredMarkers.contains(DNL)) {
                        return std::unexpected("Multiple DNL markers encountered. Only one DNL marker is allowed");
                    }
                    ASSIGN_OR_RETURN(numberOfLines, parseDNL(file), "Unable to parse DNL");
                    data.frameInfo.header.numberOfLines = numberOfLines;
                    break;
                }
                case DRI: {
                    ASSIGN_OR_RETURN(restartInterval, parseDRI(file), "Unable to parse restart interval");
                    data.lastSetRestartInterval = restartInterval;
                    break;
                }
                case COM: {
                    ASSIGN_OR_RETURN_MUT(comment, parseComment(file), "Unable to parse comment");
                    data.comments.push_back(std::move(comment));
                    break;
                }
                case SOS: {
                    ASSIGN_OR_RETURN_MUT(scan, parseSOS(file), "Unable to parse SOS");
                    scan.restartInterval = data.lastSetRestartInterval;
                    for (size_t i = 0; i < 4; i++) {
                        scan.iterations.quantization[i] = data.quantizationTables[i].size() - 1;
                        scan.iterations.dc[i] = data.huffmanTables.dc[i].size() - 1;
                        scan.iterations.ac[i] = data.huffmanTables.ac[i].size() - 1;
                    }
                    // Ensure scan header references tables that have already been specified
                    for (const auto& [componentSelector, dcTableSelector, acTableSelector] : scan.header.components) {
                        const auto frameComp = data.frameInfo.header.getComponent(componentSelector);
                        if (!frameComp) {
                            return std::unexpected(std::format("Scan header references undefined component in frame header: \"{}\"", componentSelector));
                        }
                        if (data.quantizationTables[frameComp->quantizationTableSelector].empty()) {
                            return std::unexpected(std::format("Scan header references undefined quantization table: \"{}\"", frameComp->quantizationTableSelector));
                        }
                        if (data.huffmanTables.dc[dcTableSelector].empty()) {
                            return std::unexpected(std::format("Scan header references undefined DC Huffman Table: \"{}\"", dcTableSelector));
                        }
                        if (data.huffmanTables.ac[acTableSelector].empty()) {
                            return std::unexpected(std::format("Scan header references undefined AC Huffman Table: \"{}\"", acTableSelector));
                        }
                    }
                    data.scans.push_back(std::move(scan));
                    break;
                }
                case EOI: {
                    CHECK_VOID_OR_PROPAGATE(parseEOI(file));
                    break;
                }
                default: {
                    if (isSOF(marker)) {
                        if (std::ranges::any_of(encounteredMarkers, [](const uint8_t m) { return isSOF(m); })) {
                            return std::unexpected("Multiple SOF markers encountered. Only one SOF marker is allowed");
                        }
                        ASSIGN_OR_RETURN(frameHeader, parseFrameHeader(file, marker), "Unable to parse frame header");
                        ASSIGN_OR_RETURN_MUT(frameInfo, analyzeFrameHeader(frameHeader, marker), "Unable to analyze frame header");
                        data.frameInfo = std::move(frameInfo);
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

    return data;
}

auto FileParser::Jpeg::Decoder::isEOB(const int r, const int s) -> bool {
    return r == 0x0 && s == 0x0;
}

auto FileParser::Jpeg::Decoder::isZRL(const int r, const int s) -> bool {
    return r == 0xF && s == 0x0;
}

int FileParser::Jpeg::Decoder::decodeSSSS(BitReader& bitReader, const int SSSS) {
    int coefficient = static_cast<int>(bitReader.getNBits(SSSS));
    if (coefficient < 1 << (SSSS - 1)) {
        coefficient -= (1 << SSSS) - 1;
    }
    return coefficient;
}

auto FileParser::Jpeg::Decoder::decodeNextValue(BitReader& bitReader, const HuffmanTable& huffmanTable) -> uint8_t {
    auto [bitLength, value] = huffmanTable.decode(bitReader.peekUInt16());
    bitReader.skipBits(bitLength);
    return value;
}

int FileParser::Jpeg::Decoder::decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) {
    const int sCategory = decodeNextValue(bitReader, huffmanTable);
    return sCategory == 0 ? 0 : decodeSSSS(bitReader, sCategory);
}

auto FileParser::Jpeg::Decoder::decodeAcCoefficient(
    BitReader& bitReader, const HuffmanTable& huffmanTable
) -> ACCoefficientResult {
    const uint8_t rs = decodeNextValue(bitReader, huffmanTable);
    return {getUpperNibble(rs), getLowerNibble(rs)};
}

auto FileParser::Jpeg::Decoder::decodeComponent(
    Component& out,
    BitReader& bitReader,
    const ScanComponent& scanComp,
    const HuffmanTablePtrs& dcTables,
    const HuffmanTablePtrs& acTables, PreviousDC& prevDc
) -> std::expected<Component, std::string>  {
    // DC Coefficient
    const auto& dcTable = *dcTables[scanComp.dcTableSelector];
    const int dcCoefficient = decodeDcCoefficient(bitReader, dcTable) + prevDc[scanComp.componentSelector];
    out[0] = static_cast<float>(dcCoefficient);
    prevDc[scanComp.componentSelector] = dcCoefficient;

    // AC Coefficients
    size_t index = 1;
    while (index < Component::length) {
        const auto& acTable = *acTables[scanComp.acTableSelector];
        auto [r, s] = decodeAcCoefficient(bitReader, acTable);
        if (static_cast<size_t>(r) > Component::length - index) {
            return std::unexpected("Run length would exceed component bounds");
        }
        if (s < 0 || s > 10) {  // Typical JPEG SSSS range
            return std::unexpected(std::format("Invalid SSSS value in AC coefficient: {}", s));
        }
        if (isEOB(r, s)) { // Remaining coefficients are 0
            break;
        }
        if (isZRL(r, s)) { // 16 zeros in a row
            index += 16;
            continue;
        }
        index += r;
        const int coefficient = decodeSSSS(bitReader, s);
        out[zigZagMap[index]] = static_cast<float>(coefficient);
        index++;
    }
    return out;
}

auto FileParser::Jpeg::Decoder::decodeMcu(
    BitReader& bitReader,
    const FrameInfo& frame,
    const ScanHeader& scanHeader,
    const HuffmanTablePtrs& dcTables,
    const HuffmanTablePtrs& acTables,
    PreviousDC& prevDc
) -> std::expected<Mcu, std::string> {
    auto mcu = Mcu(frame.luminanceHorizontalSamplingFactor, frame.luminanceVerticalSamplingFactor);
    for (const auto& scanComp : scanHeader.components) {
        if (scanComp.componentSelector == frame.luminanceID) {
            const auto numLuminanceComponents = frame.luminanceHorizontalSamplingFactor * frame.luminanceVerticalSamplingFactor;
            for (int i = 0; i < numLuminanceComponents; i++) {
                CHECK_VOID_AND_RETURN(
                    decodeComponent(mcu.Y[i], bitReader, scanComp, dcTables, acTables, prevDc),
                    "Unable to parse luminance component");
            }
        } else if (scanComp.componentSelector == frame.chrominanceBlueID) {
            CHECK_VOID_AND_RETURN(
                decodeComponent(mcu.Cb, bitReader, scanComp, dcTables, acTables, prevDc),
                "Unable to parse chrominance blue component");
        } else if (scanComp.componentSelector == frame.chrominanceRedID) {
            CHECK_VOID_AND_RETURN(
                decodeComponent(mcu.Cr, bitReader, scanComp, dcTables, acTables, prevDc),
                "Unable to parse chrominance red component");
        }
    }
    return mcu;
}

auto FileParser::Jpeg::Decoder::decodeRSTSegment(
    const FrameInfo& frame,
    const ScanHeader& scanHeader,
    const HuffmanTablePtrs& dcTables,
    const HuffmanTablePtrs& acTables,
    const uint16_t restartInterval,
    const std::vector<uint8_t>& rstData
) -> std::expected<std::vector<Mcu>, std::string> {
    BitReader bitReader{rstData};
    PreviousDC prevDc{};

    const size_t mcusToRead = restartInterval != 0 ? restartInterval : frame.mcuWidth * frame.mcuHeight;
    std::vector<Mcu> mcus;
    for (size_t i = 0; i < mcusToRead; i++) {
        ASSIGN_OR_RETURN(mcu, decodeMcu(bitReader, frame, scanHeader, dcTables, acTables, prevDc), "Unable to decode MCU");
        mcus.push_back(mcu);
    }

    bitReader.alignToByte();
    if (!bitReader.reachedEnd()) {
        return std::unexpected("Extra unused data found before the end of RST marker");
    }
    return mcus;
}

auto FileParser::Jpeg::Decoder::decodeScan(
    const FrameInfo& frame, const Scan& scan, const HuffmanTablePtrs& dcTables, const HuffmanTablePtrs& acTables
) -> std::expected<std::vector<Mcu>, std::string> {
    std::vector<Mcu> mcus;

    for (const auto& section : scan.dataSections) {
        ASSIGN_OR_RETURN(decodedSection,
            decodeRSTSegment(frame, scan.header, dcTables, acTables, scan.restartInterval, section),
            "Unable to decode RST segment");
        mcus.insert(mcus.end(), decodedSection.begin(), decodedSection.end());
    }

    return mcus;
}

namespace {
    struct ResolvedIterations {
        FileParser::Jpeg::QuantizationTablePtrs quantizationTables{};
        FileParser::Jpeg::HuffmanTablePtrs acTables{};
        FileParser::Jpeg::HuffmanTablePtrs dcTables{};
    };

    auto resolveTableIterations(
        const FileParser::Jpeg::TableIterations& iterations,
        const std::array<std::vector<FileParser::Jpeg::QuantizationTable>, FileParser::Jpeg::MaxTableId>& quantizationTables,
        const FileParser::Jpeg::HuffmanTables& huffmanTables
    ) -> ResolvedIterations {
        ResolvedIterations result;
        for (size_t i = 0; i < FileParser::Jpeg::MaxTableId; i++) {
            if (quantizationTables[i].size() >= iterations.quantization[i]) {
                result.quantizationTables[i] = &quantizationTables[i][iterations.quantization[i]];
            }
            if (huffmanTables.dc[i].size() >= iterations.dc[i]) {
                result.dcTables[i] = &huffmanTables.dc[i][iterations.dc[i]];
            }
            if (huffmanTables.ac[i].size() >= iterations.ac[i]) {
                result.acTables[i] = &huffmanTables.ac[i][iterations.ac[i]];
            }
        }
        return result;
    }
}

auto FileParser::Jpeg::Decoder::decode(
    const std::filesystem::path& filePath
) -> std::expected<Image, std::string> {
    ASSIGN_OR_PROPAGATE(data, Parser::parseFile(filePath));
    const size_t width  = data.frameInfo.header.numberOfSamplesPerLine;
    const size_t height = data.frameInfo.header.numberOfLines;

    const auto [quantizationTables, acTables, dcTables] = resolveTableIterations(
        data.scans[0].iterations, data.quantizationTables, data.huffmanTables);

    ASSIGN_OR_RETURN_MUT(mcus, decodeScan(data.frameInfo, data.scans[0], dcTables, acTables), "Unable to decode scan");
    for (auto& mcu: mcus) {
        CHECK_VOID_AND_RETURN(dequantize(mcu, data.frameInfo, data.scans[0].header, quantizationTables), "Unable to dequantize scan");
        inverseDCT(mcu);
    }

    std::vector<uint8_t> rgbData = getRawRGBData(convertMcusToColorBlocks(mcus, width, height), width, height);
    return Image(static_cast<uint32_t>(width), static_cast<uint32_t>(height), std::move(rgbData));
}
