#include "BitManipulationUtil.h"

#include <cstdint>
#include <sstream>

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

bool AreFloatsEqual(const float a, const float b, const float epsilon) {
    return std::abs(a - b) < epsilon;
}

void PutInt(uint8_t*& bufferPos, const int value) {
    *bufferPos++ = static_cast<uint8_t>(value >>  0);
    *bufferPos++ = static_cast<uint8_t>(value >>  8);
    *bufferPos++ = static_cast<uint8_t>(value >> 16);
    *bufferPos++ = static_cast<uint8_t>(value >> 24);
}

void PutShort(uint8_t*& bufferPos, const int value) {
    *bufferPos++ = static_cast<unsigned char>(value >> 0);
    *bufferPos++ = static_cast<unsigned char>(value >> 8);
}

BitReader::BitReader(const std::vector<uint8_t>& bytes) {
    this->bytes = bytes;
    bitPosition = 0;
    byteIndex = 0;
}

uint8_t BitReader::getBit() {
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
    bitPosition += static_cast<uint8_t>(numBits);
    byteIndex += bitPosition / 8;
    bitPosition %= 8;
}

bool BitReader::reachedEnd() const {
    return byteIndex >= static_cast<int>(bytes.size());
}

void BitReader::addByte(const uint8_t byte) {
    bytes.push_back(byte);
}
