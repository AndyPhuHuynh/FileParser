#pragma once

#include <array>
#include <expected>
#include <filesystem>
#include <vector>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Huffman/Table.hpp"
#include "FileParser/Image.hpp"
#include "FileParser/Jpeg/Mcu.hpp"
#include "FileParser/Jpeg/QuantizationTable.hpp"

namespace FileParser::Jpeg {
    struct NewFrameComponent {
        uint8_t identifier = 0;
        uint8_t horizontalSamplingFactor = 0;
        uint8_t verticalSamplingFactor = 0;
        uint8_t quantizationTableSelector = 0;
    };

    // Represents the raw data for the frame header
    struct NewFrameHeader {
        uint8_t  precision = 0; // Precision in bits for the samples for the components
        uint16_t numberOfLines = 0;
        uint16_t numberOfSamplesPerLine = 0;
        uint8_t  numberOfComponents = 0;
        std::vector<NewFrameComponent> components;
    };

    // Stores computed information about the frame header
    struct FrameInfo {
        NewFrameHeader header;
        uint8_t frameMarker = 0;

        constexpr static uint8_t unassignedID = -1;
        uint8_t luminanceID            = unassignedID;
        uint8_t chrominanceBlueID      = unassignedID;
        uint8_t chrominanceRedID       = unassignedID;

        uint16_t luminanceHorizontalSamplingFactor = 0;
        uint16_t luminanceVerticalSamplingFactor   = 0;

        uint32_t mcuWidth  = 0;
        uint32_t mcuHeight = 0;
    };

    struct TableIterations {
        std::array<size_t, 4> quantization{};
        std::array<size_t, 4> dc{};
        std::array<size_t, 4> ac{};
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
        uint16_t restartInterval = 0;
        TableIterations iterations;
        std::vector<std::vector<uint8_t>> dataSections; // Sections of data separated at restart markers
    };

    struct HuffmanParseResult {
        uint8_t tableClass = 0; // 0 = DC table, 1 = AC table
        uint8_t tableDestination = 0;
        HuffmanTable table;
    };

    struct HuffmanTables {
        std::array<std::vector<HuffmanTable>, 4> dc;
        std::array<std::vector<HuffmanTable>, 4> ac;
    };

    struct ACCoefficientResult {
        int r;
        int s;
    };

    struct JpegData {
        FrameInfo frameInfo;
        uint16_t lastSetRestartInterval = 0;
        std::vector<Scan> scans;
        std::vector<std::string> comments;

        std::array<std::vector<QuantizationTable>, 4> quantizationTables;
        HuffmanTables huffmanTables;
    };

    using PreviousDC = std::array<int, 3>;

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
        [[nodiscard]] static auto parseEOI(std::ifstream& file) -> std::expected<void, std::string>;

        [[nodiscard]] static auto analyzeFrameHeader(const NewFrameHeader& header, uint8_t SOF) -> std::expected<FrameInfo, std::string>;
    public:
        [[nodiscard]] static auto parseFile(const std::filesystem::path& filePath) -> std::expected<JpegData, std::string>;
    };

    class JpegDecoder {
        [[nodiscard]] static auto isEOB(int r, int s) -> bool;
        [[nodiscard]] static auto isZRL(int r, int s) -> bool;

        // Given the SSSS category, read that many bits from the BitReader and decode its value
        static auto decodeSSSS(NewBitReader& bitReader, int SSSS) -> int;
        static auto decodeNextValue(NewBitReader& bitReader, const HuffmanTable& table) -> uint8_t;
        static auto decodeDcCoefficient(NewBitReader& bitReader, const HuffmanTable& huffmanTable) -> int;
        static auto decodeAcCoefficient(NewBitReader& bitReader, const HuffmanTable& huffmanTable) -> ACCoefficientResult;

        [[nodiscard]] static auto decodeComponent(
            NewBitReader& bitReader,
            const NewScanHeaderComponent& scanComp,
            const TableIterations& iterations,
            const HuffmanTables& huffmanTables,
            PreviousDC& prevDc) -> std::expected<Component, std::string>;

        [[nodiscard]] static auto decodeMcu(
            NewBitReader& bitReader,
            const FrameInfo& frame,
            const NewScanHeader& scanHeader,
            const TableIterations& iterations,
            const HuffmanTables& huffmanTables,
            PreviousDC& prevDc) -> std::expected<Mcu, std::string>;

        [[nodiscard]] static auto decodeRSTSegment(
            const FrameInfo& frame,
            const NewScanHeader& scanHeader,
            const TableIterations& iterations,
            const HuffmanTables& huffmanTables,
            uint16_t restartInterval,
            const std::vector<uint8_t>& rstData) -> std::expected<std::vector<Mcu>, std::string>;

        [[nodiscard]] static auto decodeScan(
            const FrameInfo& frame,
            const Scan& scan,
            const HuffmanTables& huffmanTables) -> std::expected<std::vector<Mcu>, std::string>;
    public:
        [[nodiscard]] static auto decode(const std::filesystem::path& filePath, uint16_t *outWidth, uint16_t *outHeight) -> std::expected<std::vector<Mcu>, std::string>;
    };
}
