#pragma once

#include <Bmp/BmpImage.h>

#include "Jpeg/JpegEncoder.h"
#include "Jpeg/JpegImage.h"

namespace FileParser::Bmp::Converter {
    std::vector<Jpeg::Mcu> getMcus(BmpImage& bmp);
    void writeBmpAsJpeg(BmpImage& bmp, const std::string& filename, const Jpeg::Encoder::EncodingSettings& encodingSettings);
}
