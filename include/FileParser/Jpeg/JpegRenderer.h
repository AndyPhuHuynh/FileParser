#pragma once

#include <cstdint>
#include <vector>

#include "Mcu.hpp"

namespace FileParser::Jpeg::Renderer {
    void renderJpeg(const std::vector<Mcu>& mcus, uint16_t width, uint16_t height);
}
