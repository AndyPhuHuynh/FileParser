#pragma once

#include <cstdint>
#include <filesystem>
#include <span>
#include <type_traits>
#include <vector>

namespace FileParser::IO {
    template <typename T>
    concept is_one_byte_char =
        sizeof(T) == 1 &&
        (std::is_same_v<T, char> ||
         std::is_same_v<T, signed char> ||
         std::is_same_v<T, unsigned char> ||
         std::is_same_v<T, std::byte>);

    class ByteSpanReader {
        std::span<const uint8_t> m_bytes;
        size_t m_pos = 0;
        bool m_failed = false;

        explicit ByteSpanReader(std::span<const uint8_t> span);
    public:
        ByteSpanReader() = default;

        [[nodiscard]] auto get_buffer_length() const -> size_t;
        [[nodiscard]] auto get_pos() const -> size_t;
        auto set_pos(size_t newPos) -> void;
        auto rewind_pos(size_t amount) -> void;
        auto forward_pos(size_t amount) -> void;
        auto reset() -> void;

        template <typename T> requires(is_one_byte_char<T>)
        auto set_view(std::span<const T> span) -> void {
            auto ptr = reinterpret_cast<const uint8_t *>(span.data());
            m_bytes = std::span(ptr, span.size());
            reset();
        }

        template <typename T> requires(is_one_byte_char<T>)
        static auto from_bytes(std::span<const T> span) -> ByteSpanReader {
            const auto ptr = reinterpret_cast<const uint8_t *>(span.data());
            return ByteSpanReader{std::span<const uint8_t>(ptr, span.size())};
        }

        [[nodiscard]] auto has_failed() const -> bool;

        auto read_into(uint8_t *buffer, size_t len) -> void;

        template <typename T>
        auto read_le() -> T {
            T result = 0;
            uint8_t bytes[sizeof(T)];
            read_into(bytes, sizeof(T));
            if (has_failed()) {
                return 0;
            }
            for (size_t i = 0; i < sizeof(T); i++) {
                result |= static_cast<T>(bytes[i]) << (i * 8);
            }
            return result;
        }

        template <typename T>
        auto read_be() -> T {
            T result = 0;
            uint8_t bytes[sizeof(T)];
            read_into(bytes, sizeof(T));
            if (has_failed()) {
                return 0;
            }
            for (size_t i = 0; i < sizeof(T); i++) {
                result |= static_cast<T>(bytes[i]) << ((sizeof(T) - 1 - i) * 8);
            }
            return result;
        }

        auto read_u8() -> uint8_t;
    };

    class ByteStream {
        std::vector<uint8_t> m_bytes{};
        bool m_failed = false;
    public:
        ByteSpanReader reader{};
        explicit ByteStream(const std::vector<uint8_t>& bytes);
        explicit ByteStream(std::vector<uint8_t>&& bytes);
        explicit ByteStream(const std::filesystem::path& path);

        [[nodiscard]] auto has_failed() const -> bool;
    };
}
