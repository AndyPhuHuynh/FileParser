#pragma once

#include <cstdint>
#include <limits>
#include <vector>

unsigned char GetBitFromLeft(unsigned char byte, int pos);
unsigned char GetBitFromRight(uint16_t word, int pos);
unsigned char GetNibble(unsigned char byte, int pos);
uint16_t SwapBytes(uint16_t value);
bool AreFloatsEqual(float a, float b, float epsilon = std::numeric_limits<float>::epsilon() * 1000);

class BitReader {
private:
    std::vector<uint8_t> bytes;
    int byteIndex = 0;
    uint8_t bitPosition = 0;
public:
    BitReader() = default;
    explicit BitReader(const std::vector<uint8_t>& bytes);
    uint8_t getBit();
    uint32_t getNBits(int numBits);
    uint8_t getLastByteRead() const;
    void alignToByte();
    uint8_t getByteConstant() const;
    uint16_t getWordConstant() const;
    void skipBits(int numBits);
    bool reachedEnd() const;
    void addByte(uint8_t byte);
};