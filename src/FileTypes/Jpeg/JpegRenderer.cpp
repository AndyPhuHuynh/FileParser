#include "FileParser/Jpeg/JpegRenderer.h"

#include <vector>

#include "FileParser/Gui/Renderer.h"
#include "FileParser/Gui/RenderWindow.h"
#include "FileParser/Jpeg/JpegImage.h"
#include "FileParser/Point.h"

void FileParser::Jpeg::Renderer::renderJpeg(const JpegImage& jpeg) {
    const auto points = std::make_shared<std::vector<Point>>();
    points->reserve(static_cast<unsigned int>(jpeg.info.width * jpeg.info.height));
    auto colorBlocks = convertMcusToColorBlocks(jpeg.mcus, jpeg.info.width, jpeg.info.height);
    const size_t blockWidth = (jpeg.info.width + 8 - 1) / 8;
    for (size_t y = 0; y < jpeg.info.height; y++) {
        for (size_t x = 0; x < jpeg.info.width; x++) {
            const size_t blockRow = y / 8;
            const size_t blockCol = x / 8;

            const size_t colorRow = y % 8;
            const size_t colorCol = x % 8;

            const auto& [R, G, B] = colorBlocks[blockRow * blockWidth + blockCol];
            Color color(R[colorRow * 8 + colorCol], G[colorRow * 8 + colorCol], B[colorRow * 8 + colorCol]);
            color.normalizeColor();

            points->emplace_back(x, y, color);
        }
    }

    auto windowFuture = Gui::Renderer::GetInstance()->createWindowAsync(jpeg.info.width, jpeg.info.height, "Jpeg", Gui::RenderMode::Point);

    if (const auto window = windowFuture.get().lock()) {
        window->setBufferDataPointsAsync(points);
        window->showWindowAsync();
    }
}

