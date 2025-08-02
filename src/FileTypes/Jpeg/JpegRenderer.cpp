#include "FileParser/Jpeg/JpegRenderer.h"

#include <vector>

#include "FileParser/Gui/Renderer.h"
#include "FileParser/Gui/RenderWindow.h"
#include "FileParser/Point.h"
#include "FileParser/Jpeg/Mcu.hpp"
#include "FileParser/Jpeg/Transform.hpp"

void FileParser::Jpeg::Renderer::renderJpeg(const std::vector<Mcu>& mcus, uint16_t width, uint16_t height) {
    const auto points = std::make_shared<std::vector<Point>>();
    points->reserve(static_cast<unsigned int>(width * height));
    auto colorBlocks = convertMcusToColorBlocks(mcus, width, height);
    const size_t blockWidth = (width + 8 - 1) / 8;
    for (size_t y = 0; y < height; y++) {
        for (size_t x = 0; x < width; x++) {
            const size_t blockRow = y / 8;
            const size_t blockCol = x / 8;

            const size_t colorRow = y % 8;
            const size_t colorCol = x % 8;

            const auto& [R, G, B] = colorBlocks[blockRow * blockWidth + blockCol];
            Color color(R[colorRow * 8 + colorCol], G[colorRow * 8 + colorCol], B[colorRow * 8 + colorCol]);
            color.normalizeColor();

            points->emplace_back(static_cast<float>(x), static_cast<float>(y), color);
        }
    }

    auto windowFuture = Gui::Renderer::GetInstance()->createWindowAsync(width, height, "Jpeg", Gui::RenderMode::Point);

    if (const auto window = windowFuture.get().lock()) {
        window->setBufferDataPointsAsync(points);
        window->showWindowAsync();
    }
}

