#pragma once

#include <Jpeg/JpegImage.h>

namespace ImageProcessing::Jpeg::Converter {
    void WriteJpegAsBmp(const JpegImage& jpeg, const std::string& filename);
}
