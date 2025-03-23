#pragma once

#include <Bmp/BmpImage.h>
#include "Jpeg/JpegImage.h"

namespace ImageProcessing::Bmp::Converter {
    std::vector<Jpeg::Mcu> getMcus(BmpImage& bmp);
    void writeBmpAsJpeg(BmpImage& bmp, const std::string& filename);
}
