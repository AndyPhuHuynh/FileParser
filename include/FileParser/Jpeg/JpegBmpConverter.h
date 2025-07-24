#pragma once

#include <FileParser/Jpeg/JpegImage.h>

namespace FileParser::Jpeg::Converter {
    void writeJpegAsBmp(const JpegImage& jpeg, const std::string& filename);
}
 