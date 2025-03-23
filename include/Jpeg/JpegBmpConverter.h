#pragma once

#include <Jpeg/JpegImage.h>

namespace ImageProcessing::Jpeg::Converter {
    void writeJpegAsBmp(const JpegImage& jpeg, const std::string& filename);
}
 