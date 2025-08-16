#pragma once

#include <array>
#include <expected>
#include <filesystem>
#include <vector>

#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Image.hpp"
#include "FileParser/Huffman/Table.hpp"
#include "FileParser/Jpeg/Mcu.hpp"
#include "FileParser/Jpeg/Structures.hpp"

namespace FileParser::Jpeg {
    // Stores computed information about the frame header
    struct FrameInfo {
        FrameHeader header;
        uint8_t frameMarker = 0;

        constexpr static uint8_t unassignedID = 0xFF;
        uint8_t luminanceID            = unassignedID;
        uint8_t chrominanceBlueID      = unassignedID;
        uint8_t chrominanceRedID       = unassignedID;

        uint16_t luminanceHorizontalSamplingFactor = 0;
        uint16_t luminanceVerticalSamplingFactor   = 0;

        uint32_t mcuWidth  = 0;
        uint32_t mcuHeight = 0;
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

    using QuantizationTablePtrs = std::array<const QuantizationTable *, MaxTableId>;
    using HuffmanTablePtrs      = std::array<const HuffmanTable *,      MaxTableId>;

    struct JpegData {
        FrameInfo frameInfo;
        uint16_t lastSetRestartInterval = 0;
        std::vector<Scan> scans;
        std::vector<std::string> comments;

        std::array<std::vector<QuantizationTable>, 4> quantizationTables;
        HuffmanTables huffmanTables;
    };

    using PreviousDC = std::map<int, int>;

    class Parser {
        [[nodiscard]] static auto parseFrameComponent(std::ifstream& file) -> std::expected<FrameComponent, std::string>;
        [[nodiscard]] static auto parseFrameHeader(std::ifstream& file, uint8_t SOF) -> std::expected<FrameHeader, std::string>;
        [[nodiscard]] static auto parseDNL(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseDRI(std::ifstream& file) -> std::expected<uint16_t, std::string>;
        [[nodiscard]] static auto parseComment(std::ifstream& file) -> std::expected<std::string, std::string>;
        [[nodiscard]] static auto parseDQT(std::ifstream& file) -> std::expected<std::vector<QuantizationTable>, std::string>;
        [[nodiscard]] static auto parseDHT(std::ifstream& file) -> std::expected<std::vector<HuffmanParseResult>, std::string>;
        [[nodiscard]] static auto parseScanHeaderComponent(std::ifstream& file) -> std::expected<ScanComponent, std::string>;
        [[nodiscard]] static auto parseScanHeader(std::ifstream& file) -> std::expected<ScanHeader, std::string>;
        [[nodiscard]] static auto parseECS(std::ifstream& file) -> std::expected<std::vector<std::vector<uint8_t>>, std::string>;
        [[nodiscard]] static auto parseSOS(std::ifstream& file) -> std::expected<Scan, std::string>;
        [[nodiscard]] static auto parseEOI(std::ifstream& file) -> std::expected<void, std::string>;

        [[nodiscard]] static auto analyzeFrameHeader(const FrameHeader& header, uint8_t SOF) -> std::expected<FrameInfo, std::string>;
    public:
        [[nodiscard]] static auto parseFile(const std::filesystem::path& filePath) -> std::expected<JpegData, std::string>;
    };

    class Decoder {
        [[nodiscard]] static auto isEOB(int r, int s) -> bool;
        [[nodiscard]] static auto isZRL(int r, int s) -> bool;

        // Given the SSSS category, read that many bits from the BitReader and decode its value
        [[nodiscard]] static auto decodeSSSS         (BitReader& bitReader, int SSSS) -> int;
        [[nodiscard]] static auto decodeNextValue    (BitReader& bitReader, const HuffmanTable& huffmanTable) -> uint8_t;
        [[nodiscard]] static auto decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) -> int;
        [[nodiscard]] static auto decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable) -> ACCoefficientResult;

        [[nodiscard]] static auto decodeComponent(
            Component& out,
            BitReader& bitReader,
            const ScanComponent& scanComp,
            const HuffmanTablePtrs& dcTables,
            const HuffmanTablePtrs& acTables, PreviousDC& prevDc) -> std::expected<Component, std::string>;

        [[nodiscard]] static auto decodeMcu(
            BitReader& bitReader,
            const FrameInfo& frame,
            const ScanHeader& scanHeader,
            const HuffmanTablePtrs& dcTables,
            const HuffmanTablePtrs& acTables,
            PreviousDC& prevDc) -> std::expected<Mcu, std::string>;

        [[nodiscard]] static auto decodeRSTSegment(
            const FrameInfo& frame,
            const ScanHeader& scanHeader,
            const HuffmanTablePtrs& dcTables,
            const HuffmanTablePtrs& acTables,
            uint16_t restartInterval,
            const std::vector<uint8_t>& rstData) -> std::expected<std::vector<Mcu>, std::string>;

        [[nodiscard]] static auto decodeScan(
            const FrameInfo& frame,
            const Scan& scan,
            const HuffmanTablePtrs& dcTables,
            const HuffmanTablePtrs& acTables) -> std::expected<std::vector<Mcu>, std::string>;
    public:
        [[nodiscard]] static auto decode(const std::filesystem::path& filePath) -> std::expected<Image, std::string>;
    };
}
