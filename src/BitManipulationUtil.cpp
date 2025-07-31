#include "FileParser/BitManipulationUtil.h"

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

auto getUpperNibble(const uint8_t byte) -> uint8_t {
    return static_cast<uint8_t>(byte >> 4);
}

auto getLowerNibble(const uint8_t byte) -> uint8_t {
    return static_cast<uint8_t>(byte & 0x0F);
}

static auto getReadBytesErrorMsg(const std::streamsize n) -> std::unexpected<std::string> {
    return std::unexpected(std::format("Unable to read {} bytes from file (EOF or read error)", n));
}

auto read_uint8(std::ifstream& file) -> std::expected<uint8_t, std::string> {
    uint8_t byte{};
    file.read(reinterpret_cast<char*>(&byte), 1);
    if (!file) {
        return getReadBytesErrorMsg(1);
    }
    return byte;
}

auto read_uint8(std::ifstream& file, const std::streamsize n) -> std::expected<std::vector<uint8_t>, std::string> {
    if (n < 1) {
        return std::unexpected("A size of at least one must be specified for reading bytes");
    }
    auto bytes = std::vector<uint8_t>(n);
    if (!file.read(reinterpret_cast<char*>(bytes.data()), n)) {
        return getReadBytesErrorMsg(n);
    }
    return bytes;
}

auto read_uint16_be(std::ifstream& file) -> std::expected<uint16_t, std::string> {
    uint8_t bytes[2];
    file.read(reinterpret_cast<char*>(bytes), 2);
    if (!file) {
        return getReadBytesErrorMsg(2);
    }
    return static_cast<uint16_t>(bytes[0] << 8 | bytes[1]);
}

auto read_uint16_be(std::ifstream& file, const std::streamsize n) -> std::expected<std::vector<uint16_t>, std::string> {
    auto bytes = read_uint8(file, n * static_cast<std::streamsize>(sizeof(uint16_t)));
    if (!bytes) {
        return std::unexpected(bytes.error());
    }
    auto words = std::vector<uint16_t>(n);
    for (std::streamsize i = 0; i < n; i++) {
        words[i] = static_cast<uint16_t>((*bytes)[2 * i] << 8 | (*bytes)[2 * i + 1]);
    }
    return words;
}

auto read_string(std::ifstream& file, const std::streamsize n) -> std::expected<std::string, std::string> {
    if (n < 1) {
        return std::unexpected("A size of at least one must be specified for reading bytes");
    }
    std::string bytes(n, '\0');
    if (!file.read(bytes.data(), n)) {
        return getReadBytesErrorMsg(n);
    }
    return bytes;
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
    this->m_bytes = bytes;
    m_bitPosition = 0;
    m_byteIndex = 0;
}

uint8_t BitReader::getBit() {
    const uint8_t bit = GetBitFromLeft(m_bytes[m_byteIndex], m_bitPosition++);
    if (m_bitPosition >= 8) {
        m_bitPosition = 0;
        m_byteIndex++;
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

    if (m_bitPosition > 0) {
        const int bitsToReadFromCurrentByte = std::min(bitsLeftToRead, 8 - m_bitPosition);
        result |= (m_bytes[m_byteIndex] >> (8 - m_bitPosition - bitsToReadFromCurrentByte)) & ((1 << bitsToReadFromCurrentByte) - 1);
        bitsLeftToRead -= bitsToReadFromCurrentByte;
        skipBits(bitsToReadFromCurrentByte);
    }
    
    while (bitsLeftToRead >= 8) {
        result <<= 8;
        result |= m_byteIndex < static_cast<int>(m_bytes.size()) ?  m_bytes[m_byteIndex++] & 0xFF : 0;
        bitsLeftToRead -= 8;
    }
    
    if (bitsLeftToRead > 0) {
        result <<= bitsLeftToRead;
        result |= m_byteIndex < static_cast<int>(m_bytes.size()) ? m_bytes[m_byteIndex] >> (8 - bitsLeftToRead) : 0;
        skipBits(bitsLeftToRead);
    }
    return result;
}

uint8_t BitReader::getLastByteRead() const {
    return m_bytes[m_byteIndex];
}

void BitReader::alignToByte() {
    if (m_bitPosition == 0) return;
    m_bitPosition = 0;
    m_byteIndex++;
}

uint8_t BitReader::getByteConstant() const {
    const int bitsLeftInCurrentByte = 8 - m_bitPosition;

    const auto bitsInCurrent = static_cast<uint8_t>(m_bytes[m_byteIndex] << (8 + m_bitPosition));
    const uint8_t nextByte = m_byteIndex + 1 < static_cast<int>(m_bytes.size()) ? (m_bytes[m_byteIndex + 1] >> bitsLeftInCurrentByte) : 0;
    return bitsInCurrent | nextByte;
}

uint16_t BitReader::getWordConstant() const {
    const int bitsLeftInCurrentByte = 8 - m_bitPosition;

    const auto bitsInCurrent = static_cast<uint16_t>(m_bytes[m_byteIndex] << (8 + m_bitPosition));
    const uint16_t nextByte = m_byteIndex + 1 < static_cast<int>(m_bytes.size()) ? static_cast<uint16_t>(m_bytes[m_byteIndex + 1] << m_bitPosition) : 0;
    const uint16_t lastBits = m_byteIndex + 2 < static_cast<int>(m_bytes.size()) ? m_bytes[m_byteIndex + 2] >> bitsLeftInCurrentByte : 0;

    return bitsInCurrent | nextByte | lastBits;
}

void BitReader::skipBits(const int numBits) {
    m_bitPosition += static_cast<uint8_t>(numBits);
    m_byteIndex += m_bitPosition / 8;
    m_bitPosition %= 8;
}

bool BitReader::reachedEnd() const {
    return m_byteIndex >= static_cast<int>(m_bytes.size());
}

void BitReader::addByte(const uint8_t byte) {
    m_bytes.push_back(byte);
}

NewBitReader::NewBitReader(const std::vector<uint8_t>& bytes) {
    m_bytes = bytes;
}

NewBitReader::NewBitReader(std::vector<uint8_t>&& bytes) {
    m_bytes = std::move(bytes);
}

auto NewBitReader::getBit() -> uint8_t {
    const auto result = getUInt8();
    skipBits(1);
    return result;
}

auto NewBitReader::getNBits(const size_t numBits) -> uint64_t {
    const auto result = peekNBits(numBits);
    skipBits(numBits);
    return result;
}

auto NewBitReader::getUInt8() -> uint8_t {
    const auto result = peekUInt8();
    skipBits(8);
    return result;
}

auto NewBitReader::getUInt16() -> uint16_t {
    const auto result = peekUInt16();
    skipBits(16);
    return result;
}

auto NewBitReader::getUInt32() -> uint32_t {
    const auto result = peekUInt32();
    skipBits(32);
    return result;
}

auto NewBitReader::getUInt64() -> uint64_t {
    const auto result = peekUInt64();
    skipBits(64);
    return result;
}

auto NewBitReader::peekBit() const -> uint8_t {
    return static_cast<uint8_t>((m_bytes[m_byteIndex] >> (7 - m_bitPosition)) & 1);
}

auto NewBitReader::peekNBits(const size_t numBits) const -> uint64_t {
    if (numBits == 0) return 0;
    if (numBits > 64) {
        throw std::invalid_argument("Number of bits to read must be in the range [0, 64]");
    }

    constexpr size_t bitsInByte = 8;
    uint64_t result = 0;
    size_t bitsRead = 0;

    size_t byteIndex = m_byteIndex;
    size_t bitOffset = m_bitPosition;

    while (bitsRead < numBits) {
        const uint8_t currentByte = (byteIndex < m_bytes.size()) ? m_bytes[byteIndex] : 0;

        const size_t bitsAvailable = bitsInByte - bitOffset;
        const size_t bitsToRead = std::min(numBits - bitsRead, bitsAvailable);

        const uint8_t shifted = currentByte >> (bitsInByte - bitOffset - bitsToRead);
        const uint8_t mask = (1 << bitsToRead) - 1;
        const uint8_t extracted = shifted & mask;

        result = (result << bitsToRead) | extracted;

        bitsRead += bitsToRead;
        bitOffset += bitsToRead;

        if (bitOffset == bitsInByte) {
            bitOffset = 0;
            ++byteIndex;
        }
    }

    return result;
}

auto NewBitReader::peekUInt8() const -> uint8_t {
    return static_cast<uint8_t>(peekNBits(8));
}

auto NewBitReader::peekUInt16() const -> uint16_t {
    return static_cast<uint16_t>(peekNBits(16));
}

auto NewBitReader::peekUInt32() const -> uint32_t {
    return static_cast<uint32_t>(peekNBits(32));
}

auto NewBitReader::peekUInt64() const -> uint64_t {
    return peekNBits(64);
}

auto NewBitReader::skipBits(const size_t numBits) -> void {
    constexpr size_t bitsInByte = 8;
    m_bitPosition += numBits;
    m_byteIndex   += m_bitPosition / bitsInByte;
    m_bitPosition  = m_bitPosition % bitsInByte;
}

auto NewBitReader::alignToByte() -> void {
    if (m_bitPosition == 0) return;
    m_bitPosition = 0;
    m_byteIndex++;
}

auto NewBitReader::addByte(const uint8_t byte) -> void {
    m_bytes.push_back(byte);
}

BitWriter::BitWriter(const std::string& filepath, int bufferSize) : m_bufferSize(bufferSize), m_buffer(bufferSize), m_filepath(filepath) {
    m_file = std::ofstream(m_filepath, std::ios::out | std::ios::binary);
    if (!m_file.is_open()) {
        throw std::ios_base::failure("Bit writer failed to open file: " + filepath);
    }
}

BitWriter::BitWriter(BitWriter&& other) noexcept
    : m_byte(other.m_byte),
      m_bitPosition(other.m_bitPosition),
      m_bufferPos(other.m_bufferPos),
      m_bufferSize(other.m_bufferSize),
      m_buffer(std::move(other.m_buffer)),
      m_filepath(std::move(other.m_filepath)),
      m_file(std::move(other.m_file)) {
    other.m_byte = 0;
    other.m_bitPosition = 0;
    other.m_bufferPos = 0;
    other.m_bufferSize = 0;
    
    if (other.m_file.is_open()) {
        other.m_file.close();
    }
}

BitWriter::~BitWriter() {
    if (m_bufferPos != 0) flushBuffer();
    if (m_file.is_open()) {
        m_file.close();
    }
}

void BitWriter::writeZero() {
    incrementBitPosition();
}

void BitWriter::writeOne() {
    m_byte |= 1 << (7 - m_bitPosition);
    incrementBitPosition();
}

void BitWriter::writeBit(const bool isOne) {
    isOne ? writeOne() : writeZero();
}

void BitWriter::flushByte(const bool padWithOnes) {
    if (m_bufferPos >= m_bufferSize) {
        flushBuffer();
    }
    if (padWithOnes) {
        m_byte |= (1 << (8 - m_bitPosition)) - 1;
    }
    m_buffer.at(m_bufferPos++) = m_byte;
    m_prevByte = m_byte;
    m_bitPosition = 0;
    m_byte = 0;
}

void BitWriter::flushBuffer() {
    if (m_bufferPos == 0) return;
    
    m_file.write(reinterpret_cast<const char*>(m_buffer.data()), m_bufferPos);
    if (!m_file) {
        std:: ostringstream errorMsg;
        errorMsg << "BitWriter failed to write to file: " << m_filepath << ", " << m_file.tellp() << "bytes written";
        throw std::ios_base::failure(errorMsg.str());
    }
    m_bufferPos = 0;
}

void BitWriter::incrementBitPosition() {
    m_bitPosition += 1;
    if (m_bitPosition >= 8) {
        flushByte(false);
    }
}
