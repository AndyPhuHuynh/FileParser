#include "FileUtil.h"

#include <filesystem>
#include <fstream>

static bool CheckSignature(std::ifstream& file, const uint8_t* signature, const int signatureSize) {
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

fileUtils::FileType fileUtils::GetFileType(const std::string& filePath) {
    std::ifstream file = std::ifstream(filePath, std::ios::binary);

    if (CheckSignature(file, bmpSig, 2)) {
        return FileType::Bmp;
    }
    if (CheckSignature(file, jpegSig, 3)) {
        return FileType::Jpeg;
    }

    file.close();
    return FileType::None;
}
