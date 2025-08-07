#include "FileParser/Jpeg/JpegEncoder.h"

#include <algorithm>
#include <iostream>
#include <ranges>

#include "FileParser/Jpeg/JpegBitWriter.h"
#include "FileParser/Jpeg/HuffmanBuilder.hpp"
#include "FileParser/Jpeg/HuffmanEncoder.hpp"
#include "FileParser/Jpeg/Markers.hpp"
#include "FileParser/Jpeg/Transform.hpp"

const FileParser::HuffmanTable& FileParser::Jpeg::Encoder::getDefaultLuminanceDcTable() {
    static const HuffmanTable table{
        {
            HuffmanEncoding(0b00,        2, 0x0),
            HuffmanEncoding(0b010,       3, 0x1),
            HuffmanEncoding(0b011,       3, 0x2),
            HuffmanEncoding(0b100,       3, 0x3),
            HuffmanEncoding(0b101,       3, 0x4),
            HuffmanEncoding(0b110,       3, 0x5),
            HuffmanEncoding(0b1110,      4, 0x6),
            HuffmanEncoding(0b11110,     5, 0x7),
            HuffmanEncoding(0b111110,    6, 0x8),
            HuffmanEncoding(0b1111110,   7, 0x9),
            HuffmanEncoding(0b11111110,  8, 0x0A),
            HuffmanEncoding(0b111111110, 9, 0x0B)
        }
    };
    return table;
}

const FileParser::HuffmanTable& FileParser::Jpeg::Encoder::getDefaultLuminanceAcTable() {
    static const HuffmanTable table{
        {
            // Luminance ac table sheet 1
            HuffmanEncoding(0b1010,             4,  0x00),
            HuffmanEncoding(0b00,               2,  0x01),
            HuffmanEncoding(0b01,               2,  0x02),
            HuffmanEncoding(0b100,              3,  0x03),
            HuffmanEncoding(0b1011,             4,  0x04),
            HuffmanEncoding(0b11010,            5,  0x05),
            HuffmanEncoding(0b1111000,          7,  0x06),
            HuffmanEncoding(0b11111000,         8,  0x07),
            HuffmanEncoding(0b1111110110,       10, 0x08),
            HuffmanEncoding(0b1111111110000010, 16, 0x09),
            HuffmanEncoding(0b1111111110000011, 16, 0x0A),
            HuffmanEncoding(0b1100,             4,  0x11),
            HuffmanEncoding(0b11011,            5,  0x12),
            HuffmanEncoding(0b1111001,          7,  0x13),
            HuffmanEncoding(0b111110110,        9,  0x14),
            HuffmanEncoding(0b11111110110,      11, 0x15),
            HuffmanEncoding(0b1111111110000100, 16, 0x16),
            HuffmanEncoding(0b1111111110000101, 16, 0x17),
            HuffmanEncoding(0b1111111110000110, 16, 0x18),
            HuffmanEncoding(0b1111111110000111, 16, 0x19),
            HuffmanEncoding(0b1111111110001000, 16, 0x1A),
            HuffmanEncoding(0b11100,            5,  0x21),
            HuffmanEncoding(0b11111001,         8,  0x22),
            HuffmanEncoding(0b1111110111,       10, 0x23),
            HuffmanEncoding(0b111111110100,     12, 0x24),
            HuffmanEncoding(0b1111111110001001, 16, 0x25),
            HuffmanEncoding(0b1111111110001010, 16, 0x26),
            HuffmanEncoding(0b1111111110001011, 16, 0x27),
            HuffmanEncoding(0b1111111110001100, 16, 0x28),
            HuffmanEncoding(0b1111111110001101, 16, 0x29),
            HuffmanEncoding(0b1111111110001110, 16, 0x2A),
            HuffmanEncoding(0b111010,           6,  0x31),
            HuffmanEncoding(0b111110111,        9,  0x32),
            HuffmanEncoding(0b111111110101,     12, 0x33),
            HuffmanEncoding(0b1111111110001111, 16, 0x34),
            HuffmanEncoding(0b1111111110010000, 16, 0x35),
            HuffmanEncoding(0b1111111110010001, 16, 0x36),
            HuffmanEncoding(0b1111111110010010, 16, 0x37),
            HuffmanEncoding(0b1111111110010011, 16, 0x38),
            HuffmanEncoding(0b1111111110010100, 16, 0x39),
            HuffmanEncoding(0b1111111110010101, 16, 0x3A),

            // Luminance ac table sheet 2
            HuffmanEncoding(0b111011,           6 , 0x41),
            HuffmanEncoding(0b1111111000,       10, 0x42),
            HuffmanEncoding(0b1111111110010110, 16, 0x43),
            HuffmanEncoding(0b1111111110010111, 16, 0x44),
            HuffmanEncoding(0b1111111110011000, 16, 0x45),
            HuffmanEncoding(0b1111111110011001, 16, 0x46),
            HuffmanEncoding(0b1111111110011010, 16, 0x47),
            HuffmanEncoding(0b1111111110011011, 16, 0x48),
            HuffmanEncoding(0b1111111110011100, 16, 0x49),
            HuffmanEncoding(0b1111111110011101, 16, 0x4A),
            HuffmanEncoding(0b1111010,          7 , 0x51),
            HuffmanEncoding(0b11111110111,      11, 0x52),
            HuffmanEncoding(0b1111111110011110, 16, 0x53),
            HuffmanEncoding(0b1111111110011111, 16, 0x54),
            HuffmanEncoding(0b1111111110100000, 16, 0x55),
            HuffmanEncoding(0b1111111110100001, 16, 0x56),
            HuffmanEncoding(0b1111111110100010, 16, 0x57),
            HuffmanEncoding(0b1111111110100011, 16, 0x58),
            HuffmanEncoding(0b1111111110100100, 16, 0x59),
            HuffmanEncoding(0b1111111110100101, 16, 0x5A),
            HuffmanEncoding(0b1111011,          7 , 0x61),
            HuffmanEncoding(0b111111110110,     12, 0x62),
            HuffmanEncoding(0b1111111110100110, 16, 0x63),
            HuffmanEncoding(0b1111111110100111, 16, 0x64),
            HuffmanEncoding(0b1111111110101000, 16, 0x65),
            HuffmanEncoding(0b1111111110101001, 16, 0x66),
            HuffmanEncoding(0b1111111110101010, 16, 0x67),
            HuffmanEncoding(0b1111111110101011, 16, 0x68),
            HuffmanEncoding(0b1111111110101100, 16, 0x69),
            HuffmanEncoding(0b1111111110101101, 16, 0x6A),
            HuffmanEncoding(0b11111010,         8 , 0x71),
            HuffmanEncoding(0b111111110111,     12, 0x72),
            HuffmanEncoding(0b1111111110101110, 16, 0x73),
            HuffmanEncoding(0b1111111110101111, 16, 0x74),
            HuffmanEncoding(0b1111111110110000, 16, 0x75),
            HuffmanEncoding(0b1111111110110001, 16, 0x76),
            HuffmanEncoding(0b1111111110110010, 16, 0x77),
            HuffmanEncoding(0b1111111110110011, 16, 0x78),
            HuffmanEncoding(0b1111111110110100, 16, 0x79),
            HuffmanEncoding(0b1111111110110101, 16, 0x7A),
            HuffmanEncoding(0b111111000,        9 , 0x81),
            HuffmanEncoding(0b111111111000000,  15, 0x82),

            // Luminance ac table sheet 3
            HuffmanEncoding(0b1111111110110110, 16, 0x83),
            HuffmanEncoding(0b1111111110110111, 16, 0x84),
            HuffmanEncoding(0b1111111110111000, 16, 0x85),
            HuffmanEncoding(0b1111111110111001, 16, 0x86),
            HuffmanEncoding(0b1111111110111010, 16, 0x87),
            HuffmanEncoding(0b1111111110111011, 16, 0x88),
            HuffmanEncoding(0b1111111110111100, 16, 0x89),
            HuffmanEncoding(0b1111111110111101, 16, 0x8A),
            HuffmanEncoding(0b111111001,        9 , 0x91),
            HuffmanEncoding(0b1111111110111110, 16, 0x92),
            HuffmanEncoding(0b1111111110111111, 16, 0x93),
            HuffmanEncoding(0b1111111111000000, 16, 0x94),
            HuffmanEncoding(0b1111111111000001, 16, 0x95),
            HuffmanEncoding(0b1111111111000010, 16, 0x96),
            HuffmanEncoding(0b1111111111000011, 16, 0x97),
            HuffmanEncoding(0b1111111111000100, 16, 0x98),
            HuffmanEncoding(0b1111111111000101, 16, 0x99),
            HuffmanEncoding(0b1111111111000110, 16, 0x9A),
            HuffmanEncoding(0b111111010,        9 , 0xA1),
            HuffmanEncoding(0b1111111111000111, 16, 0xA2),
            HuffmanEncoding(0b1111111111001000, 16, 0xA3),
            HuffmanEncoding(0b1111111111001001, 16, 0xA4),
            HuffmanEncoding(0b1111111111001010, 16, 0xA5),
            HuffmanEncoding(0b1111111111001011, 16, 0xA6),
            HuffmanEncoding(0b1111111111001100, 16, 0xA7),
            HuffmanEncoding(0b1111111111001101, 16, 0xA8),
            HuffmanEncoding(0b1111111111001110, 16, 0xA9),
            HuffmanEncoding(0b1111111111001111, 16, 0xAA),
            HuffmanEncoding(0b1111111001,       10, 0xB1),
            HuffmanEncoding(0b1111111111010000, 16, 0xB2),
            HuffmanEncoding(0b1111111111010001, 16, 0xB3),
            HuffmanEncoding(0b1111111111010010, 16, 0xB4),
            HuffmanEncoding(0b1111111111010011, 16, 0xB5),
            HuffmanEncoding(0b1111111111010100, 16, 0xB6),
            HuffmanEncoding(0b1111111111010101, 16, 0xB7),
            HuffmanEncoding(0b1111111111010110, 16, 0xB8),
            HuffmanEncoding(0b1111111111010111, 16, 0xB9),
            HuffmanEncoding(0b1111111111011000, 16, 0xBA),
            HuffmanEncoding(0b1111111010,       10, 0xC1),
            HuffmanEncoding(0b1111111111011001, 16, 0xC2),
            HuffmanEncoding(0b1111111111011010, 16, 0xC3),
            HuffmanEncoding(0b1111111111011011, 16, 0xC4),

            // Luminance ac table sheet 4
            HuffmanEncoding(0b1111111111011100, 16, 0xC5),
            HuffmanEncoding(0b1111111111011101, 16, 0xC6),
            HuffmanEncoding(0b1111111111011110, 16, 0xC7),
            HuffmanEncoding(0b1111111111011111, 16, 0xC8),
            HuffmanEncoding(0b1111111111100000, 16, 0xC9),
            HuffmanEncoding(0b1111111111100001, 16, 0xCA),
            HuffmanEncoding(0b11111111000,      11, 0xD1),
            HuffmanEncoding(0b1111111111100010, 16, 0xD2),
            HuffmanEncoding(0b1111111111100011, 16, 0xD3),
            HuffmanEncoding(0b1111111111100100, 16, 0xD4),
            HuffmanEncoding(0b1111111111100101, 16, 0xD5),
            HuffmanEncoding(0b1111111111100110, 16, 0xD6),
            HuffmanEncoding(0b1111111111100111, 16, 0xD7),
            HuffmanEncoding(0b1111111111101000, 16, 0xD8),
            HuffmanEncoding(0b1111111111101001, 16, 0xD9),
            HuffmanEncoding(0b1111111111101010, 16, 0xDA),
            HuffmanEncoding(0b1111111111101011, 16, 0xE1),
            HuffmanEncoding(0b1111111111101100, 16, 0xE2),
            HuffmanEncoding(0b1111111111101101, 16, 0xE3),
            HuffmanEncoding(0b1111111111101110, 16, 0xE4),
            HuffmanEncoding(0b1111111111101111, 16, 0xE5),
            HuffmanEncoding(0b1111111111110000, 16, 0xE6),
            HuffmanEncoding(0b1111111111110001, 16, 0xE7),
            HuffmanEncoding(0b1111111111110010, 16, 0xE8),
            HuffmanEncoding(0b1111111111110011, 16, 0xE9),
            HuffmanEncoding(0b1111111111110100, 16, 0xEA),
            HuffmanEncoding(0b11111111001,      11, 0xF0),
            HuffmanEncoding(0b1111111111110101, 16, 0xF1),
            HuffmanEncoding(0b1111111111110110, 16, 0xF2),
            HuffmanEncoding(0b1111111111110111, 16, 0xF3),
            HuffmanEncoding(0b1111111111111000, 16, 0xF4),
            HuffmanEncoding(0b1111111111111001, 16, 0xF5),
            HuffmanEncoding(0b1111111111111010, 16, 0xF6),
            HuffmanEncoding(0b1111111111111011, 16, 0xF7),
            HuffmanEncoding(0b1111111111111100, 16, 0xF8),
            HuffmanEncoding(0b1111111111111101, 16, 0xF9),
            HuffmanEncoding(0b1111111111111110, 16, 0xFA),
        }
    };
    return table;
}

const FileParser::HuffmanTable& FileParser::Jpeg::Encoder::getDefaultChrominanceDcTable() {
    static const HuffmanTable table {
        {
            HuffmanEncoding(0b00,          2,   0x0),
            HuffmanEncoding(0b01,          2,   0x1),
            HuffmanEncoding(0b10,          2,   0x2),
            HuffmanEncoding(0b110,         3,   0x3),
            HuffmanEncoding(0b1110,        4,   0x4),
            HuffmanEncoding(0b11110,       5,   0x5),
            HuffmanEncoding(0b111110,      6,   0x6),
            HuffmanEncoding(0b1111110,     7,   0x7),
            HuffmanEncoding(0b11111110,    8,   0x8),
            HuffmanEncoding(0b111111110,   9,   0x9),
            HuffmanEncoding(0b1111111110,  10,  0x0A),
            HuffmanEncoding(0b11111111110, 11,  0x0B)
        }
    };
    return table;
}

const FileParser::HuffmanTable& FileParser::Jpeg::Encoder::getDefaultChrominanceAcTable() {
    static const HuffmanTable table {
        {
            // Chrominance ac table sheet 1
            HuffmanEncoding(0b00,               2,  0x00),
            HuffmanEncoding(0b01,               2,  0x01),
            HuffmanEncoding(0b100,              3,  0x02),
            HuffmanEncoding(0b1010,             4,  0x03),
            HuffmanEncoding(0b11000,            5,  0x04),
            HuffmanEncoding(0b11001,            5,  0x05),
            HuffmanEncoding(0b111000,           6,  0x06),
            HuffmanEncoding(0b1111000,          7,  0x07),
            HuffmanEncoding(0b111110100,        9,  0x08),
            HuffmanEncoding(0b1111110110,       10, 0x09),
            HuffmanEncoding(0b111111110100,     12, 0x0A),
            HuffmanEncoding(0b1011,             4,  0x11),
            HuffmanEncoding(0b111001,           6,  0x12),
            HuffmanEncoding(0b11110110,         8,  0x13),
            HuffmanEncoding(0b111110101,        9,  0x14),
            HuffmanEncoding(0b11111110110,      11, 0x15),
            HuffmanEncoding(0b111111110101,     12, 0x16),
            HuffmanEncoding(0b1111111110001000, 16, 0x17),
            HuffmanEncoding(0b1111111110001001, 16, 0x18),
            HuffmanEncoding(0b1111111110001010, 16, 0x19),
            HuffmanEncoding(0b1111111110001011, 16, 0x1A),
            HuffmanEncoding(0b11010,            5,  0x21),
            HuffmanEncoding(0b11110111,         8,  0x22),
            HuffmanEncoding(0b1111110111,       10, 0x23),
            HuffmanEncoding(0b111111110110,     12, 0x24),
            HuffmanEncoding(0b111111111000010,  15, 0x25),
            HuffmanEncoding(0b1111111110001100, 16, 0x26),
            HuffmanEncoding(0b1111111110001101, 16, 0x27),
            HuffmanEncoding(0b1111111110001110, 16, 0x28),
            HuffmanEncoding(0b1111111110001111, 16, 0x29),
            HuffmanEncoding(0b1111111110010000, 16, 0x2A),
            HuffmanEncoding(0b11011,            5,  0x31),
            HuffmanEncoding(0b11111000,         8,  0x32),
            HuffmanEncoding(0b1111111000,       10, 0x33),
            HuffmanEncoding(0b111111110111,     12, 0x34),
            HuffmanEncoding(0b1111111110010001, 16, 0x35),
            HuffmanEncoding(0b1111111110010010, 16, 0x36),
            HuffmanEncoding(0b1111111110010011, 16, 0x37),
            HuffmanEncoding(0b1111111110010100, 16, 0x38),
            HuffmanEncoding(0b1111111110010101, 16, 0x39),
            HuffmanEncoding(0b1111111110010110, 16, 0x3A),
            HuffmanEncoding(0b111010,           6,  0x41),

            // Chrominance ac table sheet 2
            HuffmanEncoding(0b111110110,        9,  0x42),
            HuffmanEncoding(0b1111111110010111, 16, 0x43),
            HuffmanEncoding(0b1111111110011000, 16, 0x44),
            HuffmanEncoding(0b1111111110011001, 16, 0x45),
            HuffmanEncoding(0b1111111110011010, 16, 0x46),
            HuffmanEncoding(0b1111111110011011, 16, 0x47),
            HuffmanEncoding(0b1111111110011100, 16, 0x48),
            HuffmanEncoding(0b1111111110011101, 16, 0x49),
            HuffmanEncoding(0b1111111110011110, 16, 0x4A),
            HuffmanEncoding(0b111011,           6,  0x51),
            HuffmanEncoding(0b1111111001,       10, 0x52),
            HuffmanEncoding(0b1111111110011111, 16, 0x53),
            HuffmanEncoding(0b1111111110100000, 16, 0x54),
            HuffmanEncoding(0b1111111110100001, 16, 0x55),
            HuffmanEncoding(0b1111111110100010, 16, 0x56),
            HuffmanEncoding(0b1111111110100011, 16, 0x57),
            HuffmanEncoding(0b1111111110100100, 16, 0x58),
            HuffmanEncoding(0b1111111110100101, 16, 0x59),
            HuffmanEncoding(0b1111111110100110, 16, 0x5A),
            HuffmanEncoding(0b1111001,          7,  0x61),
            HuffmanEncoding(0b11111110111,      11, 0x62),
            HuffmanEncoding(0b1111111110100111, 16, 0x63),
            HuffmanEncoding(0b1111111110101000, 16, 0x64),
            HuffmanEncoding(0b1111111110101001, 16, 0x65),
            HuffmanEncoding(0b1111111110101010, 16, 0x66),
            HuffmanEncoding(0b1111111110101011, 16, 0x67),
            HuffmanEncoding(0b1111111110101100, 16, 0x68),
            HuffmanEncoding(0b1111111110101101, 16, 0x69),
            HuffmanEncoding(0b1111111110101110, 16, 0x6A),
            HuffmanEncoding(0b1111010,          7,  0x71),
            HuffmanEncoding(0b11111111000,      11, 0x72),
            HuffmanEncoding(0b1111111110101111, 16, 0x73),
            HuffmanEncoding(0b1111111110110000, 16, 0x74),
            HuffmanEncoding(0b1111111110110001, 16, 0x75),
            HuffmanEncoding(0b1111111110110010, 16, 0x76),
            HuffmanEncoding(0b1111111110110011, 16, 0x77),
            HuffmanEncoding(0b1111111110110100, 16, 0x78),
            HuffmanEncoding(0b1111111110110101, 16, 0x79),
            HuffmanEncoding(0b1111111110110110, 16, 0x7A),
            HuffmanEncoding(0b11111001,         8,  0x81),
            HuffmanEncoding(0b1111111110110111, 16, 0x82),
            HuffmanEncoding(0b1111111110111000, 16, 0x83),

            // Chrominance ac table sheet 3
            HuffmanEncoding(0b1111111110111001, 16, 0x84),
            HuffmanEncoding(0b1111111110111010, 16, 0x85),
            HuffmanEncoding(0b1111111110111011, 16, 0x86),
            HuffmanEncoding(0b1111111110111100, 16, 0x87),
            HuffmanEncoding(0b1111111110111101, 16, 0x88),
            HuffmanEncoding(0b1111111110111110, 16, 0x89),
            HuffmanEncoding(0b1111111110111111, 16, 0x8A),
            HuffmanEncoding(0b111110111,        9,  0x91),
            HuffmanEncoding(0b1111111111000000, 16, 0x92),
            HuffmanEncoding(0b1111111111000001, 16, 0x93),
            HuffmanEncoding(0b1111111111000010, 16, 0x94),
            HuffmanEncoding(0b1111111111000011, 16, 0x95),
            HuffmanEncoding(0b1111111111000100, 16, 0x96),
            HuffmanEncoding(0b1111111111000101, 16, 0x97),
            HuffmanEncoding(0b1111111111000110, 16, 0x98),
            HuffmanEncoding(0b1111111111000111, 16, 0x99),
            HuffmanEncoding(0b1111111111001000, 16, 0x9A),
            HuffmanEncoding(0b111111000,        9,  0xA1),
            HuffmanEncoding(0b1111111111001001, 16, 0xA2),
            HuffmanEncoding(0b1111111111001010, 16, 0xA3),
            HuffmanEncoding(0b1111111111001011, 16, 0xA4),
            HuffmanEncoding(0b1111111111001100, 16, 0xA5),
            HuffmanEncoding(0b1111111111001101, 16, 0xA6),
            HuffmanEncoding(0b1111111111001110, 16, 0xA7),
            HuffmanEncoding(0b1111111111001111, 16, 0xA8),
            HuffmanEncoding(0b1111111111010000, 16, 0xA9),
            HuffmanEncoding(0b1111111111010001, 16, 0xAA),
            HuffmanEncoding(0b111111001,        9,  0xB1),
            HuffmanEncoding(0b1111111111010010, 16, 0xB2),
            HuffmanEncoding(0b1111111111010011, 16, 0xB3),
            HuffmanEncoding(0b1111111111010100, 16, 0xB4),
            HuffmanEncoding(0b1111111111010101, 16, 0xB5),
            HuffmanEncoding(0b1111111111010110, 16, 0xB6),
            HuffmanEncoding(0b1111111111010111, 16, 0xB7),
            HuffmanEncoding(0b1111111111011000, 16, 0xB8),
            HuffmanEncoding(0b1111111111011001, 16, 0xB9),
            HuffmanEncoding(0b1111111111011010, 16, 0xBA),
            HuffmanEncoding(0b111111010,        9,  0xC1),
            HuffmanEncoding(0b1111111111011011, 16, 0xC2),
            HuffmanEncoding(0b1111111111011100, 16, 0xC3),
            HuffmanEncoding(0b1111111111011101, 16, 0xC4),
            HuffmanEncoding(0b1111111111011110, 16, 0xC5),

            // Chrominance ac table sheet 4
            HuffmanEncoding(0b1111111111011111, 16, 0xC6),
            HuffmanEncoding(0b1111111111100000, 16, 0xC7),
            HuffmanEncoding(0b1111111111100001, 16, 0xC8),
            HuffmanEncoding(0b1111111111100010, 16, 0xC9),
            HuffmanEncoding(0b1111111111100011, 16, 0xCA),
            HuffmanEncoding(0b11111111001,      11, 0xD1),
            HuffmanEncoding(0b1111111111100100, 16, 0xD2),
            HuffmanEncoding(0b1111111111100101, 16, 0xD3),
            HuffmanEncoding(0b1111111111100110, 16, 0xD4),
            HuffmanEncoding(0b1111111111100111, 16, 0xD5),
            HuffmanEncoding(0b1111111111101000, 16, 0xD6),
            HuffmanEncoding(0b1111111111101001, 16, 0xD7),
            HuffmanEncoding(0b1111111111101010, 16, 0xD8),
            HuffmanEncoding(0b1111111111101011, 16, 0xD9),
            HuffmanEncoding(0b1111111111101100, 16, 0xDA),
            HuffmanEncoding(0b11111111100000,   14, 0xE1),
            HuffmanEncoding(0b1111111111101101, 16, 0xE2),
            HuffmanEncoding(0b1111111111101110, 16, 0xE3),
            HuffmanEncoding(0b1111111111101111, 16, 0xE4),
            HuffmanEncoding(0b1111111111110000, 16, 0xE5),
            HuffmanEncoding(0b1111111111110001, 16, 0xE6),
            HuffmanEncoding(0b1111111111110010, 16, 0xE7),
            HuffmanEncoding(0b1111111111110011, 16, 0xE8),
            HuffmanEncoding(0b1111111111110100, 16, 0xE9),
            HuffmanEncoding(0b1111111111110101, 16, 0xEA),
            HuffmanEncoding(0b1111111010,       10, 0xF0),
            HuffmanEncoding(0b111111111000011,  15, 0xF1),
            HuffmanEncoding(0b1111111111110110, 16, 0xF2),
            HuffmanEncoding(0b1111111111110111, 16, 0xF3),
            HuffmanEncoding(0b1111111111111000, 16, 0xF4),
            HuffmanEncoding(0b1111111111111001, 16, 0xF5),
            HuffmanEncoding(0b1111111111111010, 16, 0xF6),
            HuffmanEncoding(0b1111111111111011, 16, 0xF7),
            HuffmanEncoding(0b1111111111111100, 16, 0xF8),
            HuffmanEncoding(0b1111111111111101, 16, 0xF9),
            HuffmanEncoding(0b1111111111111110, 16, 0xFA),
        }
    };

    return table;
}

void FileParser::Jpeg::Encoder::encodeCoefficients(const Component& component, std::vector<Coefficient>& outCoefficients,
                                                   std::vector<Coefficient>& dcCoefficients, std::vector<Coefficient>& acCoefficients, int& prevDc) {

    // Encode the dc coefficient
    int dcCoefficient = static_cast<int>(component[0]) - prevDc;
    uint8_t dcCode = dcCoefficient == 0 ? 0 : static_cast<uint8_t>(GetMinNumBits(dcCoefficient));
    dcCoefficients.emplace_back(dcCode, dcCoefficient);
    outCoefficients.emplace_back(dcCode, dcCoefficient);
    prevDc = static_cast<int>(component[0]);

    // Find the index of the EOB
    bool eob = true;
    size_t eobIndex = Component::length - 1;
    while (eobIndex >= 1) {
        const int coefficient = static_cast<int>(std::round(component[zigZagMap[eobIndex]]));
        if (coefficient != 0) {
            // If the last coefficient is non-zero, there is no eob
            if (eobIndex == Component::length - 1) {
                eob = false;
            }
            // eobIndex is currently index of last non-zero
            // Add one to make it index of last zero
            eobIndex++;
            break;
        }
        eobIndex--;
    }
    eobIndex = std::max<size_t>(1, eobIndex);

    // Look through all the ac coefficients and find their encodings
    uint8_t r = 0;
    for (int i = 1; i < eobIndex; i++) {
        int ac = static_cast<int>(std::round(component[zigZagMap[i]]));
        if (ac == 0) {
            if (r == 15) {
                r = 0;
                acCoefficients.emplace_back(static_cast<uint8_t>(0xF0), 0x00);
                outCoefficients.emplace_back(static_cast<uint8_t>(0xF0), 0x00);
            } else {
                r++;
            }
        } else {
            const auto s = static_cast<uint8_t>(GetMinNumBits(ac));
            auto code = static_cast<uint8_t>((r << 4) | s);
            acCoefficients.emplace_back(code, ac);
            outCoefficients.emplace_back(code, ac);
            r = 0;
        }
    }

    if (eob) {
        constexpr uint8_t EOBMarker = 0x00;
        acCoefficients.emplace_back(EOBMarker, 0x00);
        outCoefficients.emplace_back(EOBMarker, 0x00);
    }
}

void FileParser::Jpeg::Encoder::encodeCoefficients(const Mcu& mcu, std::vector<EncodedMcu>& outEncodedMcus, std::array<int, 3>& prevDc,
    std::vector<Coefficient>& outLuminanceDcCoefficients, std::vector<Coefficient>& outLuminanceAcCoefficients,
    std::vector<Coefficient>& outChromaDcCoefficients, std::vector<Coefficient>& outChromaAcCoefficients) {
    outEncodedMcus.emplace_back();
    auto& [Y, Cb, Cr] = outEncodedMcus.back();
    Y.reserve(mcu.Y.size());
    for (const auto& y : mcu.Y) {
        Y.emplace_back();
        std::vector<Coefficient>& coefficients = Y.back();
        encodeCoefficients(y, coefficients, outLuminanceDcCoefficients, outLuminanceAcCoefficients, prevDc[0]);
    }
    encodeCoefficients(mcu.Cb, Cb, outChromaDcCoefficients, outChromaAcCoefficients, prevDc[1]);
    encodeCoefficients(mcu.Cr, Cr, outChromaDcCoefficients, outChromaAcCoefficients, prevDc[2]);
}

void FileParser::Jpeg::Encoder::encodeCoefficients(const std::vector<Mcu>& mcus, std::vector<EncodedMcu>& outEncodedMcus,
    std::vector<Coefficient>& outLuminanceDcCoefficients, std::vector<Coefficient>& outLuminanceAcCoefficients,
    std::vector<Coefficient>& outChromaDcCoefficients, std::vector<Coefficient>& outChromaAcCoefficients) {
    std::array prevDc = {0, 0, 0};
    for (auto& mcu : mcus) {
        encodeCoefficients(mcu, outEncodedMcus, prevDc,
            outLuminanceDcCoefficients, outLuminanceAcCoefficients,
            outChromaDcCoefficients, outChromaAcCoefficients);
    }
}

void FileParser::Jpeg::Encoder::writeMarker(const uint8_t marker, JpegBitWriter& bitWriter) {
    bitWriter << MarkerHeader << marker;
}

void FileParser::Jpeg::Encoder::writeFrameHeaderComponentSpecification(
    const FrameComponent& component, JpegBitWriter& bitWriter) {

    const uint8_t identifier = component.identifier;
    const uint8_t quantizationTableSelector = component.quantizationTableSelector;
    const auto samplingFactor = static_cast<uint8_t>((component.horizontalSamplingFactor << 4) | component.verticalSamplingFactor);

    bitWriter << identifier << samplingFactor << quantizationTableSelector;
}

void FileParser::Jpeg::Encoder::writeFrameHeader(const uint8_t SOF, const FrameHeader& frameHeader, JpegBitWriter& bitWriter) {
    const uint8_t frameType = SOF;
    const auto length = static_cast<uint16_t>(8 + frameHeader.components.size() * 3);

    writeMarker(frameType, bitWriter);
    bitWriter << length << frameHeader.precision << frameHeader.numberOfLines <<
        frameHeader.numberOfSamplesPerLine << static_cast<uint8_t>(frameHeader.components.size());

    for (const auto& component : frameHeader.components) {
        writeFrameHeaderComponentSpecification(component, bitWriter);
    }
}

FileParser::Jpeg::QuantizationTable FileParser::Jpeg::Encoder::createQuantizationTable(
    const std::array<float, QuantizationTable::length>& table, int quality, const bool is8Bit, const uint8_t tableDestination) {
    if (quality < 1 || quality > 100) {
        std::cout << "Quality must be between 1 and 100\n";
        quality = std::clamp(quality, 1, 100);
    }

    const int scale = quality < 50 ? (5000 / quality) : (200 - 2 * quality);
    std::array<float, QuantizationTable::length> scaledTable{};
    for (size_t i = 0; i < QuantizationTable::length; i++) {
        scaledTable[i] = std::round(std::clamp(table[i] * static_cast<float>(scale) / 100, 1.0f, 255.0f));
    }

    return QuantizationTable {
        .precision = static_cast<uint8_t>(is8Bit ? 0 : 1),
        .destination = tableDestination,
        .table = scaledTable,
    };
}

void FileParser::Jpeg::Encoder::writeQuantizationTableNoMarker(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter) {
    const uint8_t precision = quantizationTable.precision;
    const auto precisionAndTableId = static_cast<uint8_t>((precision << 4) | quantizationTable.destination);
    bitWriter << precisionAndTableId;

    if (quantizationTable.precision == 0) {
        for (const unsigned char i : zigZagMap) {
            auto byte = static_cast<uint8_t>(quantizationTable.table.at(i));
            bitWriter << byte;
        }
    } else {
        for (const unsigned char i : zigZagMap) {
            auto word = static_cast<uint16_t>(quantizationTable.table.at(i));
            bitWriter << word;
        }
    }
}

void FileParser::Jpeg::Encoder::writeQuantizationTable(const QuantizationTable& quantizationTable, JpegBitWriter& bitWriter) {
    writeMarker(DQT, bitWriter);

    const uint8_t bytesPerEntry = quantizationTable.precision == 0 ? 1 : 2;
    const uint16_t length = 2 + 1 + (64 * bytesPerEntry);
    bitWriter << length;

    writeQuantizationTableNoMarker(quantizationTable, bitWriter);
}

void FileParser::Jpeg::Encoder::writeScanHeaderComponentSpecification(
    const ScanComponent& component, JpegBitWriter& bitWriter) {
    const auto tableDestination =  static_cast<uint8_t>((component.dcTableSelector << 4) | component.acTableSelector);
    bitWriter << component.componentSelector << tableDestination;
}

void FileParser::Jpeg::Encoder::writeScanHeader(const ScanHeader& scanHeader, JpegBitWriter& bitWriter) {
    writeMarker(SOS, bitWriter);

    const auto numComponents = static_cast<uint8_t>(scanHeader.components.size());
    const uint16_t length = 6 + 2 * numComponents;
    bitWriter << length << numComponents;

    for (auto component : scanHeader.components) {
        writeScanHeaderComponentSpecification(component, bitWriter);
    }

    const auto successiveApproximation = static_cast<uint8_t>(scanHeader.successiveApproximationHigh << 4 | scanHeader.successiveApproximationLow);
    bitWriter << scanHeader.spectralSelectionStart << scanHeader.spectralSelectionEnd << successiveApproximation;
}

int FileParser::Jpeg::Encoder::encodeSSSS(const uint8_t SSSS, const int value) {
    if (value > 0) return value;
    return value - 1 + (1 << SSSS);
}

void FileParser::Jpeg::Encoder::writeCoefficients(const std::vector<Coefficient>& coefficients, const HuffmanTable& dcTable,
    const HuffmanTable& acTable, JpegBitWriter& bitWriter) {
    // DC coefficient
    // Write the RLE encoded value
    const HuffmanEncoding dcEncoding = dcTable.encode(coefficients[0].encoding);
    bitWriter << BitField(dcEncoding.encoding, dcEncoding.bitLength);
    // Write the SSSS rightmost bits of the symbol
    const uint8_t dcSSSS = coefficients[0].encoding & 0x0F;
    const int dcValue = encodeSSSS(dcSSSS, coefficients[0].value);
    bitWriter << BitField(dcValue, dcSSSS);

    // AC coefficient
    for (size_t i = 1; i < coefficients.size(); i++) {
        // Write the RLE encoded value
        const HuffmanEncoding acEncoding = acTable.encode(coefficients[i].encoding);
        bitWriter << BitField(acEncoding.encoding, acEncoding.bitLength);
        // Write the SSSS rightmost bits of the symbol
        const uint8_t acSSSS = coefficients[i].encoding & 0x0F;
        const int acValue = encodeSSSS(acSSSS, coefficients[i].value);
        bitWriter << BitField(acValue, acSSSS);
    }
}

void FileParser::Jpeg::Encoder::writeEncodedMcu(const EncodedMcu& mcu,
    const HuffmanTable& luminanceDcTable, const HuffmanTable& luminanceAcTable,
    const HuffmanTable& chrominanceDcTable, const HuffmanTable& chrominanceAcTable, JpegBitWriter& bitWriter) {
    // Encoded luminance
    for (const auto& y : mcu.Y) {
        writeCoefficients(y, luminanceDcTable, luminanceAcTable, bitWriter);
    }
    // Encode chrominance
    writeCoefficients(mcu.Cb, chrominanceDcTable, chrominanceAcTable, bitWriter);
    writeCoefficients(mcu.Cr, chrominanceDcTable, chrominanceAcTable, bitWriter);
}

void FileParser::Jpeg::Encoder::writeEncodedMcu(const std::vector<EncodedMcu>& mcus,
    const HuffmanTable& luminanceDcTable, const HuffmanTable& luminanceAcTable,
    const HuffmanTable& chrominanceDcTable, const HuffmanTable& chrominanceAcTable, JpegBitWriter& bitWriter) {
    bitWriter.setByteStuffing(true);
    for (const auto& block : mcus) {
        writeEncodedMcu(block, luminanceDcTable, luminanceAcTable, chrominanceDcTable, chrominanceAcTable, bitWriter);
    }
    bitWriter.flushByte(true);
    bitWriter.setByteStuffing(false);
}

auto FileParser::Jpeg::Encoder::writeJpeg(
    const std::string& filepath, std::vector<Mcu>& mcus, const EncodingSettings& settings,
    uint16_t pixelHeight, uint16_t pixelWidth
) -> std::expected<void, std::string> {
    JpegBitWriter bitWriter(filepath);
    // SOI
    writeMarker(SOI, bitWriter);
    // Tables/Misc
    // Quantization table
    QuantizationTable qTableLuminance = createQuantizationTable(LuminanceTable, settings.luminanceQuality, true, 0);
    QuantizationTable qTableChrominance = createQuantizationTable(ChrominanceTable, settings.chrominanceQuality, true, 1);
    writeQuantizationTable(qTableLuminance, bitWriter);
    writeQuantizationTable(qTableChrominance, bitWriter);
    // Huffman table
    forwardDCT(mcus);
    quantize(mcus, qTableLuminance, qTableChrominance);

    constexpr size_t luminDcIndex = 0, luminAcIndex = 1, chromaDcIndex = 2, chromaAcIndex = 3;
    auto indexToString = [](const size_t index) {
        switch (index) {
            case 0:  return "Luminance DC";
            case 1:  return "Luminance AC";
            case 2:  return "Chrominance DC";
            case 3:  return "Chrominance AC";
            default: return "";
        }
    };

    std::array<std::vector<Coefficient>, 4> coefficients{};

    std::vector<EncodedMcu> encodedMcus;
    encodeCoefficients(mcus, encodedMcus,
        coefficients[luminDcIndex], coefficients[luminAcIndex],
        coefficients[chromaDcIndex], coefficients[chromaAcIndex]);

    std::vector<HuffmanEncoder> huffmanEncoders;
    if (settings.optimizeHuffmanTables) {
        for (size_t i = 0; i < coefficients.size(); i++) {
            auto encoder = HuffmanEncoder::create(coefficients[i]);
            if (!encoder) {
                return std::unexpected(
                    std::format("Unable to create {} Huffman table: {}",indexToString(i), encoder.error()));
            }
            huffmanEncoders.emplace_back(*encoder);
        }

        huffmanEncoders[luminDcIndex].writeToFile(bitWriter, TableDescription::LuminanceDC);
        huffmanEncoders[luminAcIndex].writeToFile(bitWriter, TableDescription::LuminanceAC);
        huffmanEncoders[chromaDcIndex].writeToFile(bitWriter, TableDescription::ChrominanceDC);
        huffmanEncoders[chromaAcIndex].writeToFile(bitWriter, TableDescription::ChrominanceAC);
    } else {
        // writeHuffmanTable(0, 0, getDefaultLuminanceDcTable(), bitWriter);
        // writeHuffmanTable(0, 1, getDefaultChrominanceDcTable(), bitWriter);
        // writeHuffmanTable(1, 0, getDefaultLuminanceAcTable(), bitWriter);
        // writeHuffmanTable(1, 1, getDefaultChrominanceAcTable(), bitWriter);
    }

    const HuffmanTable& luminanceDcTable = settings.optimizeHuffmanTables ? huffmanEncoders[0].getTable() : getDefaultLuminanceDcTable();
    const HuffmanTable& luminanceAcTable = settings.optimizeHuffmanTables ? huffmanEncoders[1].getTable() : getDefaultLuminanceAcTable();
    const HuffmanTable& chrominanceDcTable = settings.optimizeHuffmanTables ? huffmanEncoders[2].getTable() : getDefaultChrominanceDcTable();
    const HuffmanTable& chrominanceAcTable = settings.optimizeHuffmanTables ? huffmanEncoders[3].getTable() : getDefaultChrominanceAcTable();

    // Restart Interval
    // Comment
    // App data
    // Number of lines (height of image)
    // Frame header
    FrameComponent frameCompY(1, 1, 1, 0);
    FrameComponent frameCompCb(2, 1, 1, 1);
    FrameComponent frameCompCr(3, 1, 1, 1);
    std::vector frameComponents{frameCompY, frameCompCb, frameCompCr};
    FrameHeader frameHeader(8, pixelHeight, pixelWidth, frameComponents);
    writeFrameHeader(SOF0, frameHeader, bitWriter);
    // Scan
    ScanComponent component1(1, 0, 0);
    ScanComponent component2(2, 1, 1);
    ScanComponent component3(3, 1, 1);
    std::vector scanComponents{component1, component2, component3};
    ScanHeader scanHeader(scanComponents, 0, 63, 0, 0);
    writeScanHeader(scanHeader, bitWriter);
    // Entropy Data
    writeEncodedMcu(encodedMcus, luminanceDcTable, luminanceAcTable, chrominanceDcTable, chrominanceAcTable, bitWriter);
    //EOI
    writeMarker(EOI, bitWriter);
    return {};
}
