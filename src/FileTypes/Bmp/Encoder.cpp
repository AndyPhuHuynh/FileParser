#include "FileParser/FileUtil.h"
#include "FileParser/Bmp/Encoder.hpp"
#include "FileParser/Macros.hpp"

auto FileParser::Bmp::encode(const Image& image, const std::filesystem::path& savePath) -> std::expected<void, std::string> {
    ASSIGN_OR_PROPAGATE_MUT(file, FileUtils::openRegularFileForWrite(savePath, std::ios::binary));
    FileUtils::writeSignatureToFile(file, FileUtils::bmpSig);

    // Write BMP file header
    constexpr uint32_t reserved   = 0;
    constexpr uint32_t dataOffset = 0x1a;
    const uint32_t size = dataOffset + static_cast<uint32_t>(image.data.size());
    file.write(reinterpret_cast<const char *>(&size), sizeof(size));
    file.write(reinterpret_cast<const char *>(&reserved), sizeof(reserved));
    file.write(reinterpret_cast<const char *>(&dataOffset), sizeof(dataOffset));

    // Write BitMapCoreHeader
    constexpr int32_t infoSize      = 12;
    const auto width                = static_cast<uint16_t>(image.width);
    const auto height               = static_cast<uint16_t>(image.height);
    constexpr uint16_t planes       = 1;
    constexpr uint16_t bitsPerPixel = 24;
    file.write(reinterpret_cast<const char*>(&infoSize), sizeof(infoSize));
    file.write(reinterpret_cast<const char*>(&width), sizeof(width));
    file.write(reinterpret_cast<const char*>(&height), sizeof(height));
    file.write(reinterpret_cast<const char*>(&planes), sizeof(planes));
    file.write(reinterpret_cast<const char*>(&bitsPerPixel), sizeof(bitsPerPixel));

    file.write(reinterpret_cast<const char*>(image.data.data()), static_cast<std::streamsize>(image.data.size()));
    if (!file) {
        std::unexpected(std::format("Unable to write to file: {}", savePath.string()));
    };
    return {};
}
