#pragma once
#include <expected>
#include <filesystem>

#include "FileParser/Image.hpp"

namespace FileParser::Bmp {
    [[nodiscard]] auto encode(const Image& image, const std::filesystem::path& savePath) -> std::expected<void, std::string>;
}
