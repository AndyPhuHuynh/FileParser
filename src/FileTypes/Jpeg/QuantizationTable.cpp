#include "FileParser/Jpeg/QuantizationTable.hpp"

#include <iomanip>
#include <iostream>

#include "FileParser/Jpeg/JpegImage.h"

FileParser::Jpeg::QuantizationTable::QuantizationTable(
    std::ifstream& file, const std::streampos& dataStartIndex,
    const bool is8Bit, const uint8_t tableDestination
) : is8Bit(is8Bit), tableDestination(tableDestination) {
    file.seekg(dataStartIndex, std::ios::beg);
    for (const unsigned char i : zigZagMap) {
        if (is8Bit) {
            uint8_t byte;
            file.read(reinterpret_cast<char*>(&byte), 1);
            table[i] = static_cast<float>(byte);
        } else {
            uint16_t word;
            file.read(reinterpret_cast<char*>(&word), 2);
            word = static_cast<uint16_t>((word >> 8) | (word << 8));
            table[i] = static_cast<float>(word);
        }
    }
}

void FileParser::Jpeg::QuantizationTable::print() const {
    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            std::cout << std::setw(6) << static_cast<int>(table[i * 8 + j]) << " ";
        }
        std::cout << "\n";
    }
}