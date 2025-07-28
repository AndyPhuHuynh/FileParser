#pragma once

#include <cstdint>

namespace FileParser::Jpeg {
    constexpr uint8_t MarkerHeader = 0xFF;
    // Start of Frame markers, non-differential, Huffman coding
    constexpr uint8_t SOF0 = 0xC0; // Baseline DCT
    constexpr uint8_t SOF1 = 0xC1; // Extended sequential DCT
    constexpr uint8_t SOF2 = 0xC2; // Progressive DCT
    constexpr uint8_t SOF3 = 0xC3; // Lossless (sequential)

    // Start of Frame markers, differential, Huffman coding
    constexpr uint8_t SOF5 = 0xC5; // Differential sequential DCT
    constexpr uint8_t SOF6 = 0xC6; // Differential progressive DCT
    constexpr uint8_t SOF7 = 0xC7; // Differential lossless (sequential)

    // Start of Frame markers, non-differential, arithmetic coding
    constexpr uint8_t SOF9 = 0xC9; // Extended sequential DCT
    constexpr uint8_t SOF10 = 0xCA; // Progressive DCT
    constexpr uint8_t SOF11 = 0xCB; // Lossless (sequential)

    // Start of Frame markers, differential, arithmetic coding
    constexpr uint8_t SOF13 = 0xCD; // Differential sequential DCT
    constexpr uint8_t SOF14 = 0xCE; // Differential progressive DCT
    constexpr uint8_t SOF15 = 0xCF; // Differential lossless (sequential)

    // Define Huffman Table(s)
    constexpr uint8_t DHT = 0xC4;

    // JPEG extensions
    constexpr uint8_t JPG = 0xC8;

    // Define Arithmetic Coding Conditioning(s)
    constexpr uint8_t DAC = 0xCC;

    // Restart interval Markers
    constexpr uint8_t RST0 = 0xD0;
    constexpr uint8_t RST1 = 0xD1;
    constexpr uint8_t RST2 = 0xD2;
    constexpr uint8_t RST3 = 0xD3;
    constexpr uint8_t RST4 = 0xD4;
    constexpr uint8_t RST5 = 0xD5;
    constexpr uint8_t RST6 = 0xD6;
    constexpr uint8_t RST7 = 0xD7;

    // Other Markers
    constexpr uint8_t SOI = 0xD8; // Start of Image
    constexpr uint8_t EOI = 0xD9; // End of Image
    constexpr uint8_t SOS = 0xDA; // Start of Scan
    constexpr uint8_t DQT = 0xDB; // Define Quantization Table(s)
    constexpr uint8_t DNL = 0xDC; // Define Number of Lines
    constexpr uint8_t DRI = 0xDD; // Define Restart Interval
    constexpr uint8_t DHP = 0xDE; // Define Hierarchical Progression
    constexpr uint8_t EXP = 0xDF; // Expand Reference Component(s)

    // APPN Markers
    constexpr uint8_t APP0 = 0xE0;
    constexpr uint8_t APP1 = 0xE1;
    constexpr uint8_t APP2 = 0xE2;
    constexpr uint8_t APP3 = 0xE3;
    constexpr uint8_t APP4 = 0xE4;
    constexpr uint8_t APP5 = 0xE5;
    constexpr uint8_t APP6 = 0xE6;
    constexpr uint8_t APP7 = 0xE7;
    constexpr uint8_t APP8 = 0xE8;
    constexpr uint8_t APP9 = 0xE9;
    constexpr uint8_t APP10 = 0xEA;
    constexpr uint8_t APP11 = 0xEB;
    constexpr uint8_t APP12 = 0xEC;
    constexpr uint8_t APP13 = 0xED;
    constexpr uint8_t APP14 = 0xEE;
    constexpr uint8_t APP15 = 0xEF;

    // Misc Markers
    constexpr uint8_t JPG0 = 0xF0;
    constexpr uint8_t JPG1 = 0xF1;
    constexpr uint8_t JPG2 = 0xF2;
    constexpr uint8_t JPG3 = 0xF3;
    constexpr uint8_t JPG4 = 0xF4;
    constexpr uint8_t JPG5 = 0xF5;
    constexpr uint8_t JPG6 = 0xF6;
    constexpr uint8_t JPG7 = 0xF7;
    constexpr uint8_t JPG8 = 0xF8;
    constexpr uint8_t JPG9 = 0xF9;
    constexpr uint8_t JPG10 = 0xFA;
    constexpr uint8_t JPG11 = 0xFB;
    constexpr uint8_t JPG12 = 0xFC;
    constexpr uint8_t JPG13 = 0xFD;
    constexpr uint8_t COM = 0xFE;
    constexpr uint8_t TEM = 0x01;

    inline bool isSOF(const uint8_t marker) {
        return marker >= SOF0 && marker <= SOF15 &&
            !(marker == DHT || marker == JPG || marker == DAC);
    }
}