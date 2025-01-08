#include "BitManipulationUtil.h"

#include <cstdint>
#include <iostream>
#include <sstream>

unsigned char GetBitFromLeft(const unsigned char byte, const int pos) {
    if (pos < 0 || pos >= 8) {
        std::ostringstream message;
        message << "Error in GetBit: position" << pos << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    return (byte >> (7 - pos)) & 1;
}

unsigned char GetBitFromRight(const uint16_t word, const int pos) {
    if (pos < 0 || pos >= 16) {
        std::ostringstream message;
        message << "Error in GetBit: position" << pos << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    return static_cast<uint8_t>(word >> pos) & 1;
}

unsigned char GetNibble(const unsigned char byte, const int pos) {
    if (pos < 0 || pos > 1) {
        std::ostringstream message;
        message << "Error in GetBit: position" << pos << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    if (pos == 1) {
        return byte & 0x0F;
    }
    return (byte >> 4) & 0x0F;
}

uint16_t SwapBytes(const uint16_t value) {
    return static_cast<uint16_t>(value << 8) | static_cast<uint16_t>(value >> 8);
}

BitReader::BitReader(const std::vector<uint8_t>& bytes) {
    this->bytes = bytes;
    bitPosition = 0;
    byteIndex = 0;
}

uint8_t BitReader::getBit() {
    if (bitPosition >= 8) {
        bitPosition = 0;
        byteIndex++;
    }
    if (static_cast<size_t>(byteIndex) >= bytes.size()) {
        static bool flag = false;
        if (!flag) {
            std::cout << "Error - No more bytes to read in bit reader\n";
            flag = true;
            return 0;
        }
    }
    uint8_t bit = GetBitFromLeft(bytes[byteIndex], bitPosition++);
    return bit;
}

uint8_t BitReader::getLastByteRead() const {
    return bytes[byteIndex];
}

void BitReader::alignToByte() {
    if (bitPosition == 0) return;
    bitPosition = 0;
    byteIndex++;
}


