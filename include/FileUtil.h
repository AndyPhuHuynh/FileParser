﻿#pragma once
#include <cstdint>
#include <string>

namespace FileUtils {
    inline constexpr uint8_t jpegSig[] = {0xFF, 0xD8, 0xFF};
    inline constexpr uint8_t pngSig[] = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};
    inline constexpr uint8_t bmpSig[] = {0x42, 0x4D};
    
    enum class FileType : uint8_t {
        None = 0,
        Bmp,
        Jpeg,
    };

    FileType getFileType(const std::string& filePath);
    FileType stringToFileType(const std::string& str);
}
