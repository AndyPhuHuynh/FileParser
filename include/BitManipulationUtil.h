#pragma once

#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <vector>

/**
 * @param value The value from which to extract a bit
 * @param pos Position from the leftmost bit
 * @return The bit located pos bits from the left
 */
template<typename T>
unsigned char GetBitFromLeft(const T& value, const int pos) {
    int maxPos = sizeof(T) * 8 - 1;
    if (pos < 0 || pos > maxPos) {
        std::ostringstream message;
        message << "Error in GetBit: position" << pos << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    return (value >> (maxPos - pos)) & 1;
}

/**
 * @param value The value from which to extract a bit
 * @param pos Position from the rightmost bit
 * @return The bit located pos bits from the right
 */
template <typename T>
unsigned char GetBitFromRight(const T& value, int pos) {
    int maxPos = sizeof(T) * 8 - 1;
    if (pos < 0 || pos > maxPos) {
        std::ostringstream message;
        message << "Error in GetBit: position" << pos << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    return static_cast<uint8_t>(value >> pos) & 1;
}

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

/**
 * @brief Calculates the minimum number of bits needed to store an integer
 */
int GetMinNumBits(int value);

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
    std::vector<uint8_t> m_bytes;
    int m_byteIndex = 0;
    uint8_t m_bitPosition = 0;
};

class BitWriter {
public:
    explicit BitWriter(const std::string& filepath, int bufferSize = 16);
    ~BitWriter();
    void writeZero();
    void writeOne();
    void writeBit(bool isOne);
    void flushByte(bool padWithOnes = false);
    void flushBuffer();
    
    /**
     * @brief Takes numBits rightmost bits from value and writes them to the bitstream stream
     * @param value The value whose bits will be written to the output stream
     * @param numBits The amount of rightmost bits to read from value
     */
    template<typename T>
    void writeBits(T value, int numBits);

    /**
     * @brief Writes all the bits from value into the bitstream
     */
    template<typename T>
    void writeValue(T value);
private:
    uint8_t m_byte = 0;
    int m_bitPosition = 0;
    
    int m_bufferPos = 0;
    const int m_bufferSize;
    std::vector<uint8_t> m_buffer;

    std::string m_filepath;
    std::ofstream m_file;
    
    void incrementBitPosition();
};

template <typename T>
void BitWriter::writeBits(T value, const int numBits) {
    int maxBits = sizeof(T) * 8;
    if (numBits < 0 || numBits > maxBits) {
        std::ostringstream message;
        message << "Error in BitWriter::writeBits: numBits" << numBits << " is out of bounds";
        throw std::invalid_argument(message.str());
    }
    for (int i = numBits - 1; i >= 0; i--) {
        int bit = GetBitFromRight(value, i);
        writeBit(bit);
    }
}

template <typename T>
void BitWriter::writeValue(T value) {
    writeBits(value, sizeof(T) * 8);
}
