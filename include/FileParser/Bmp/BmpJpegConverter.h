#pragma once

#include "FileParser/Bmp/BmpImage.h"
#include "FileParser/Jpeg/JpegEncoder.h"

namespace FileParser::Bmp::Converter {
    std::vector<Jpeg::Mcu> getMcus(BmpImage& bmp);
    void writeBmpAsJpeg(BmpImage& bmp, const std::string& filename, const Jpeg::Encoder::EncodingSettings& encodingSettings);
}
