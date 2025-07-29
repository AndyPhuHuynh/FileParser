#pragma once

#include <array>
#include <fstream>
#include <map>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <expected>
#include <optional>

#include "Mcu.hpp"
#include "FileParser/BitManipulationUtil.h"
#include "FileParser/Huffman/Table.hpp"
#include "FileParser/Jpeg/QuantizationTable.hpp"

namespace FileParser::Jpeg {
    class JpegImage;

    struct FrameHeaderComponentSpecification {
        uint8_t identifier = 0;
        uint8_t horizontalSamplingFactor = 0;
        uint8_t verticalSamplingFactor = 0;
        uint8_t quantizationTableSelector = 0;

        FrameHeaderComponentSpecification() = default;
        FrameHeaderComponentSpecification(const uint8_t identifier, const uint8_t horizontalSamplingFactor, const uint8_t verticalSamplingFactor, const uint8_t quantizationTableSelector)
            : identifier(identifier), horizontalSamplingFactor(horizontalSamplingFactor), verticalSamplingFactor(verticalSamplingFactor), quantizationTableSelector(quantizationTableSelector) {}
        void print() const;
    };

    struct FrameHeader {
        uint8_t encodingProcess = 0;
        uint8_t precision = 0;
        uint16_t height = 0;
        uint16_t width = 0;
        uint8_t numOfChannels = 0;
        std::map<uint8_t, FrameHeaderComponentSpecification> componentSpecifications;
        int luminanceComponentsPerMcu = 1;
        int maxHorizontalSample = 1;
        int maxVerticalSample = 1;
        int mcuPixelWidth = 0;
        int mcuPixelHeight =0;
        int mcuImageWidth = 0;
        int mcuImageHeight = 0;

        FrameHeader() = default;
        FrameHeader(uint8_t encodingProcess, std::ifstream& file, const std::streampos& dataStartIndex);
        FrameHeader(uint8_t SOF, uint8_t precision, uint16_t height, uint16_t width,
                    const std::vector<FrameHeaderComponentSpecification>& components);
        void print() const;
    private:
        void initializeValuesFromComponents();
    };

    class ScanHeaderComponentSpecification {
    public:
        uint8_t componentId;
        uint8_t dcTableSelector;
        uint8_t acTableSelector;

        uint8_t quantizationTableIteration;
        uint8_t dcTableIteration;
        uint8_t acTableIteration;

        ScanHeaderComponentSpecification() = default;
        ScanHeaderComponentSpecification(const uint8_t componentId, const uint8_t dcTableSelector, const uint8_t acTableSelector,
            const uint8_t quantizationTableIteration, const uint8_t dcTableIteration, const uint8_t acTableIteration)
            : componentId(componentId), dcTableSelector(dcTableSelector), acTableSelector(acTableSelector), quantizationTableIteration(quantizationTableIteration),
            dcTableIteration(dcTableIteration), acTableIteration(acTableIteration) {}
        void print() const;
    };

    class ScanHeader {
    public:
        std::vector<ScanHeaderComponentSpecification> componentSpecifications;
        uint8_t spectralSelectionStart = 0;
        uint8_t spectralSelectionEnd = 0;
        uint8_t successiveApproximationHigh = 0;
        uint8_t successiveApproximationLow = 0;
        BitReader bitReader = BitReader();
        uint16_t restartInterval = 0;
    
        ScanHeader() = default;
        ScanHeader(JpegImage* jpeg, const std::streampos& dataStartIndex);
        ScanHeader(const std::vector<ScanHeaderComponentSpecification>& components,
            const uint8_t ss, const uint8_t se, const uint8_t ah, const uint8_t al)
            : componentSpecifications(components), spectralSelectionStart(ss), spectralSelectionEnd(se),
            successiveApproximationHigh(ah), successiveApproximationLow(al) {}
        bool containsComponentId(int id);
        ScanHeaderComponentSpecification& getComponent(int id);
        void print() const;
    };

    class ColorBlock {
    public:
        static constexpr int colorBlockLength = 64;
        std::array<float, colorBlockLength> R {0};
        std::array<float, colorBlockLength> G {0};
        std::array<float, colorBlockLength> B {0};
        void print() const;
        void rgbToYCbCr();
    };

    struct ConsumerQueue {
        std::mutex mutex;
        std::condition_variable condition;
        std::queue<std::shared_ptr<Mcu>> queue;
        bool allProductsAdded = false;
    };

    struct AtomicCondition {
        std::mutex mutex;
        int value = -1;
        std::condition_variable condition;
    };

    /* TODO: Arithmetic Coding */
    class JpegImage {
    public:
        std::vector<std::shared_ptr<Mcu>> mcus;
        std::ifstream file;
        FrameHeader info;
        std::string comment;
        uint16_t currentRestartInterval = 0;
        // Index in the vector is the iteration, index in the array is the table number
        std::vector<std::array<std::optional<QuantizationTable>, 4>> quantizationTables =
            std::vector<std::array<std::optional<QuantizationTable>, 4>>(1);
        std::vector<std::array<std::optional<HuffmanTable>, 4>> dcHuffmanTables =
            std::vector<std::array<std::optional<HuffmanTable>, 4>>(1);
        std::vector<std::array<std::optional<HuffmanTable>, 4>> acHuffmanTables =
            std::vector<std::array<std::optional<HuffmanTable>, 4>>(1);
    private:
        ConsumerQueue quantizationQueue;
        ConsumerQueue idctQueue;
        ConsumerQueue colorConversionQueue;
    
        // The value of scanIndices[i] indicates the mcuIndex that scan the index can read up to
        std::mutex scanIndicesMutex;
        std::vector<std::shared_ptr<AtomicCondition>> scanIndices;
        std::vector<std::shared_ptr<ScanHeader>> scanHeaders;
    
    public:
        explicit JpegImage(const std::string& path);
        // Methods to decode entropy encoded data.
    private:
        static int decodeSSSS(BitReader& bitReader, int SSSS);
        static int decodeDcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable);
        static std::pair<int, int> decodeAcCoefficient(BitReader& bitReader, const HuffmanTable& huffmanTable);

        Component decodeComponent(BitReader& bitReader, const ScanHeaderComponentSpecification& scanComp,
                                  int (&prevDc)[3]);
        void decodeMcu(Mcu *mcu, ScanHeader& scanHeader, int (&prevDc)[3]);
        static void skipZeros(BitReader& bitReader, Component& component, int numToSkip, int& index, int approximationLow);
        void decodeProgressiveComponent(Component& component, ScanHeader& scanHeader,
                                        const ScanHeaderComponentSpecification& componentInfo, int (&prevDc)[3], int& numBlocksToSkip);
    private:
        uint16_t readLengthBytes();
        void readFrameHeader(uint8_t frameMarker);
        void readQuantizationTables();
        void readHuffmanTables();
        void readDefineNumberOfLines();
        void readDefineRestartIntervals();
        void readComments();
        void processQuantizationQueue(const std::vector<ScanHeaderComponentSpecification>& scanComps);
        void processIdctQueue();
        void processColorConversionQueue();
        std::shared_ptr<ScanHeader> readScanHeader();
        void readBaselineStartOfScan();
        // Used in progressive jpegs to test if the scan is ready to process an mcu
        void pollCurrentScan(int mcuIndex, int scanNumber);
        // Used in progressive jpegs when a scan is done processing an mcu
        void pushToNextScan(int mcuIndex, int scanNumber);
        void processProgressiveStartOfScan(const std::shared_ptr<ScanHeader>& scan, int scanNumber);
        void readProgressiveStartOfScan();
        void readStartOfScan();
        void decodeBaseLine();
        void decodeProgressive();
        void decode();
        void readFile();
    };

    auto convertMcusToColorBlocks(const std::vector<std::shared_ptr<Mcu>>& mcus, size_t pixelWidth, size_t pixelHeight) -> std::vector<ColorBlock>;
}