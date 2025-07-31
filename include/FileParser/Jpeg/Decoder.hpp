#pragma once

#include <array>
#include <expected>
#include <filesystem>
#include <vector>

#include "QuantizationTable.hpp"
#include "FileParser/Huffman/Table.hpp"

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

    struct TableIterations {
        uint8_t quantization = 0;
        uint8_t dc = 0;
        uint8_t ac = 0;
    };

    struct NewScanHeaderComponent {
        uint8_t componentSelector = 0;
        uint8_t dcTableSelector = 0;
        uint8_t acTableSelector = 0;
    };

    struct NewScanHeader {
        std::vector<NewScanHeaderComponent> components;
        uint8_t spectralSelectionStart = 0;
        uint8_t spectralSelectionEnd = 0;
        uint8_t successiveApproximationHigh = 0;
        uint8_t successiveApproximationLow = 0;
    };

    struct Scan {
        NewScanHeader header;
        uint8_t restartInterval = 0;
        TableIterations iterations;
        std::vector<std::vector<uint8_t>> dataSections; // Sections of data separated at restart markers
    };

    struct HuffmanParseResult {
        uint8_t tableClass = 0; // 0 = DC table, 1 = AC table
        uint8_t tableDestination = 0;
        HuffmanTable table;
    };

    struct JpegData {
        NewFrameHeader frameHeader;
        uint16_t restartInterval;
        Scan scan;
        std::vector<uint8_t> compressedData;
        std::vector<std::string> comments;

        std::array<std::vector<QuantizationTable>, 4> quantizationTables;
        std::array<std::vector<HuffmanTable>, 4> dcHuffmanTables;
        std::array<std::vector<HuffmanTable>, 4> acHuffmanTables;
    };

    class JpegParser {
        [[nodiscard]] static auto parseFrameComponent(std::ifstream& file) -> std::expected<NewFrameComponent, std::string>;
        [[nodiscard]] static auto parseFrameHeader(std::ifstream& file, uint8_t SOF) -> std::expected<NewFrameHeader, std::string>;
        [[nodiscard]] static auto parseDNL(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseDRI(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseComment(std::ifstream& file) -> std::expected<std::string, std::string>;
        [[nodiscard]] static auto parseDQT(std::ifstream& file) -> std::expected<std::vector<QuantizationTable>, std::string>;
        [[nodiscard]] static auto parseDHT(std::ifstream& file) -> std::expected<std::vector<HuffmanParseResult>, std::string>;
        [[nodiscard]] static auto parseScanHeaderComponent(std::ifstream& file) -> std::expected<NewScanHeaderComponent, std::string>;
        [[nodiscard]] static auto parseScanHeader(std::ifstream& file) -> std::expected<NewScanHeader, std::string>;
        [[nodiscard]] static auto parseECS(std::ifstream& file) -> std::expected<std::vector<std::vector<uint8_t>>, std::string>;
        [[nodiscard]] static auto parseSOS(std::ifstream& file) -> std::expected<Scan, std::string>;
    public:
        [[nodiscard]] static auto parseFile(const std::filesystem::path& filePath) -> std::expected<void, std::string>;
    };
}
