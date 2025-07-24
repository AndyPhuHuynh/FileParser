#include "FileParser/FileUtil.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>

static bool CheckSignature(std::ifstream& file, const uint8_t* signature, const int signatureSize) {
    file.clear();
    file.seekg(0, std::ios::beg);
    uint8_t byte = 0;
    for (int i = 0; i < signatureSize; i++) {
        file.read(reinterpret_cast<char*>(&byte), 1);
        if (byte != signature[i]) {
            return false;
        }
    }
    return true;
}

static std::string ToLower(const std::string& str) {
    std::string result = str;
    std::ranges::transform(result, result.begin(), 
                           [](const unsigned char c) { return std::tolower(c); });
    return result;
}

FileUtils::FileType FileUtils::getFileType(const std::string& filePath) {
    auto file = std::ifstream(filePath, std::ios::binary);
    if (!file.is_open()) {
        // Debug breakpoint or print error
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return FileType::None;
    }

    if (CheckSignature(file, bmpSig, 2)) {
        return FileType::Bmp;
    }
    if (CheckSignature(file, jpegSig, 3)) {
        return FileType::Jpeg;
    }

    file.close();
    return FileType::None;
}

FileUtils::FileType FileUtils::stringToFileType(const std::string& str) {
    const std::string copy = ToLower(str);
    if (copy == "jpeg" || copy == "jpg") {
        return FileType::Jpeg;
    }
    if (copy == "bmp") {
        return FileType::Bmp;
    }
    return FileType::None;
}
