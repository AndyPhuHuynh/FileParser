#pragma once

#include <cstdint>
#include <limits>
#include <vector>

/**
 * @param value The value from which to extract a bit
 * @param pos Position from the leftmost bit
 * @return The bit located pos bits from the left
 */
template<typename T>
unsigned char GetBitFromLeft(const T& value, int pos);

/**
 * @param value The value from which to extract a bit
 * @param pos Position from the leftmost bit
 * @return The bit located pos bits from the right
 */
template<typename T>
unsigned char GetBitFromRight(const T& value, int pos);

/**
 * @brief Extracts a nibble from a byte.
 * @param byte The byte from which a nibble will be extracted.
 * @param pos Indicates which nibble will be extracted.
 * - 0 = Leftmost nibble
 * - 1 = Rightmost nibble
 * @return The extracted nibble from the byte
 */
unsigned char GetNibble(unsigned char byte, int pos);

/**
 * @brief Swaps the bytes of a 16-bit integer
 * @param value The 16-bit integer whose bytes will be swapped
 * @return The value of the swapped 16-bit integer
 */
uint16_t SwapBytes(uint16_t value);

/**
 * @brief Compares two floats to see if they are equal.
 * @param a Float 1.
 * @param b Float 2.
 * @param epsilon Minimum acceptable distance between floats for them to be considered equal.
 * @return True if a and b are within epsilon distance of each other
 */
bool AreFloatsEqual(float a, float b, float epsilon = std::numeric_limits<float>::epsilon());

/**
 * @brief Writes an integer in little endian into a byte buffer.
 * @param bufferPos Pointer into a position in a byte buffer where the integer will be written.
 * @param value Integer to be written into the buffer.
 */
void PutInt(uint8_t*& bufferPos, int value);

/**
 * @brief Writes a short in little endian into a byte buffer.
 * @param bufferPos Pointer into a position in a byte buffer where the short will be written.
 * @param value Short to be written into the buffer.
 */
void PutShort(uint8_t*& bufferPos, int value);

class BitReader {
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
private:
    std::vector<uint8_t> bytes;
    int byteIndex = 0;
    uint8_t bitPosition = 0;
};