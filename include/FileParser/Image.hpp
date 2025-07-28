#pragma once
#include <vector>

namespace FileParser {
    class Image {
        uint32_t m_width = 0;
        uint32_t m_height = 0;
        std::vector<uint8_t> m_data;

    public:
        Image(const uint32_t width, const uint32_t height, std::vector<uint8_t> data)
            : m_width(width), m_height(height), m_data(std::move(data)) {};

        [[nodiscard]] auto getWidth() const -> uint32_t { return m_width; }
        [[nodiscard]] auto getHeight() const -> uint32_t { return m_height; }
        [[nodiscard]] auto getData() const -> const std::vector<uint8_t>& { return m_data; }
    };
}
