#pragma once
#include <vector>

namespace FileParser {
    struct Image {
        uint32_t width = 0;
        uint32_t height = 0;
        std::vector<unsigned char> data;
    };
}
