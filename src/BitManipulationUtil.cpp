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
    if (static_cast<size_t>(byteIndex) >= bytes.size()) {
        static bool flag = false;
        if (!flag) {
            std::cout << "Error - No more bytes to read in bit reader\n";
            flag = true;
            return 0;
        }
    }
    uint8_t bit = GetBitFromLeft(bytes[byteIndex], bitPosition++);
    if (bitPosition >= 8) {
        bitPosition = 0;
        byteIndex++;
    }
    return bit;
}

uint32_t BitReader::getNBits(const int numBits) {
    if (numBits == 0) return 0;
    if (numBits < 1 || numBits > 32) {
        std::ostringstream message;
        message << "Error in getNBits: Cannot get" << numBits << " bits";
        throw std::invalid_argument(message.str());
    }
    
    int bitsLeftToRead = numBits;
    int result = 0;

    if (bitPosition > 0) {
        int bitsToReadFromCurrentByte = std::min(bitsLeftToRead, 8 - bitPosition);
        result |= (bytes[byteIndex] >> (8 - bitPosition - bitsToReadFromCurrentByte)) & ((1 << bitsToReadFromCurrentByte) - 1);
        bitsLeftToRead -= bitsToReadFromCurrentByte;
        skipBits(bitsToReadFromCurrentByte);
    }
    
    while (bitsLeftToRead >= 8) {
        result <<= 8;
        result |= byteIndex < static_cast<int>(bytes.size()) ?  bytes[byteIndex++] & 0xFF : 0;
        bitsLeftToRead -= 8;
    }
    
    if (bitsLeftToRead > 0) {
        result <<= bitsLeftToRead;
        result |= byteIndex < static_cast<int>(bytes.size()) ? bytes[byteIndex] >> (8 - bitsLeftToRead) : 0;
        skipBits(bitsLeftToRead);
    }
    return result;
}

uint8_t BitReader::getLastByteRead() const {
    return bytes[byteIndex];
}

void BitReader::alignToByte() {
    if (bitPosition == 0) return;
    bitPosition = 0;
    byteIndex++;
}

uint8_t BitReader::getByteConstant() const {
    int bitsLeftInCurrentByte = 8 - bitPosition;

    uint8_t bitsInCurrent = static_cast<uint8_t>(bytes[byteIndex] << (8 + bitPosition));
    uint8_t nextByte = byteIndex + 1 < static_cast<int>(bytes.size()) ? (bytes[byteIndex + 1] >> bitsLeftInCurrentByte) : 0;
    return bitsInCurrent | nextByte;
}

uint16_t BitReader::getWordConstant() const {
    int bitsLeftInCurrentByte = 8 - bitPosition;

    uint16_t bitsInCurrent = static_cast<uint16_t>(bytes[byteIndex] << (8 + bitPosition));
    uint16_t nextByte = byteIndex + 1 < static_cast<int>(bytes.size()) ? static_cast<uint16_t>(bytes[byteIndex + 1] << bitPosition) : 0;
    uint16_t lastBits = byteIndex + 2 < static_cast<int>(bytes.size()) ? bytes[byteIndex + 2] >> bitsLeftInCurrentByte : 0;

    return bitsInCurrent | nextByte | lastBits;
}

void BitReader::skipBits(const int numBits) {
    bitPosition += numBits;
    byteIndex += bitPosition / 8;
    bitPosition %= 8;
}
