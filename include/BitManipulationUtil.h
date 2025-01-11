#pragma once

#include <cstdint>
#include <vector>

unsigned char GetBitFromLeft(unsigned char byte, int pos);
unsigned char GetBitFromRight(uint16_t word, int pos);
unsigned char GetNibble(unsigned char byte, int pos);
uint16_t SwapBytes(uint16_t value);

class BitReader {
private:
    std::vector<uint8_t> bytes;
    int byteIndex = 0;
    uint8_t bitPosition;                                                                
                                                                                                                                     
public:
    explicit BitReader(const std::vector<uint8_t>& bytes);
    uint8_t getBit();
    uint32_t getNBits(int numBits);
    uint8_t getLastByteRead() const;
    void alignToByte();
    uint8_t getByteConstant() const;
    uint16_t getWordConstant() const;
    void skipBits(int numBits);
};