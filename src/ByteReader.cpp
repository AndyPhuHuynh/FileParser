#include "FileParser/ByteReader.hpp"

#include <cstring>
#include <fstream>

FileParser::IO::ByteSpanReader::ByteSpanReader(const std::span<const uint8_t> span)
    : m_bytes(span) {}

auto FileParser::IO::ByteSpanReader::get_buffer_length() const -> size_t {
    return m_bytes.size();
}

auto FileParser::IO::ByteSpanReader::get_pos() const -> size_t {
    return m_pos;
}

auto FileParser::IO::ByteSpanReader::set_pos(const size_t newPos) -> void {
    if (newPos >= m_bytes.size()) {
        m_failed = true;
        return;
    }
    m_pos = newPos;
}

auto FileParser::IO::ByteSpanReader::rewind_pos(const size_t amount) -> void {
    if (amount > m_pos) {
        m_failed = true;
        return;
    }
    set_pos(m_pos - amount);
}

auto FileParser::IO::ByteSpanReader::forward_pos(const size_t amount) -> void {
    if (amount == 0) return;
    set_pos(m_pos + amount);
}

auto FileParser::IO::ByteSpanReader::reset() -> void {
    set_pos(0);
    m_failed = false;
}

auto FileParser::IO::ByteSpanReader::has_failed() const -> bool {
    return m_failed;
}

auto FileParser::IO::ByteSpanReader::read_into(uint8_t *buffer, const size_t len) -> void {
    if (const size_t remaining = m_bytes.size() - m_pos; len > remaining) {
        m_failed = true;
        return;
    }
    std::memcpy(buffer, m_bytes.data() + m_pos, len);
    m_pos += len;
}

auto FileParser::IO::ByteSpanReader::read_u8() -> uint8_t {
    return read_le<uint8_t>();
}

FileParser::IO::ByteStream::ByteStream(const std::vector<uint8_t>& bytes)
    : m_bytes(bytes), reader(ByteSpanReader::from_bytes(std::span<const uint8_t>(m_bytes))) {}

FileParser::IO::ByteStream::ByteStream(std::vector<uint8_t>&& bytes)
    : m_bytes(std::move(bytes)), reader(ByteSpanReader::from_bytes(std::span<const uint8_t>(m_bytes))) {}

FileParser::IO::ByteStream::ByteStream(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        m_failed = true;
        return;
    }

    file.seekg(0, std::ios::end);
    const std::streampos size = file.tellg();
    if (size < 0) {
        m_failed = true;
        return;
    }

    file.seekg(0, std::ios::beg);
    m_bytes.resize(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char *>(m_bytes.data()), size)) {
        m_failed = true;
        return;
    }
    reader.set_view(std::span<const uint8_t>(m_bytes));
}

auto FileParser::IO::ByteStream::has_failed() const -> bool {
    return m_failed || reader.has_failed();
}
