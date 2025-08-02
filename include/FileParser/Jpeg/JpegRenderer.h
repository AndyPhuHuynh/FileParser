#pragma once
#include "FileParser/Jpeg/JpegImage.h"

namespace FileParser::Jpeg::Renderer {
    void renderJpeg(const JpegImage& jpeg);
    void renderJpeg(const std::vector<Mcu>& mcus, uint16_t width, uint16_t height);
}
