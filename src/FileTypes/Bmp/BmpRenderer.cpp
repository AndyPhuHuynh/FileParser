#include "Bmp/BmpRenderer.h"

#include <sstream>

#include "Gui/Renderer.h"
#include "Gui/RenderWindow.h"

void FileParser::Bmp::Renderer::renderBmp(BmpImage& bmp) {
    if (bmp.rasterEncoding != BmpRasterEncoding::Monochrome &&
        bmp.rasterEncoding != BmpRasterEncoding::FourBitNoCompression &&
        bmp.rasterEncoding != BmpRasterEncoding::EightBitNoCompression &&
        bmp.rasterEncoding != BmpRasterEncoding::TwentyFourBitNoCompression) {
        std::stringstream msg;
        msg << "Raster encoding not supported: " <<
            "\n\tBitCount: "<<  bmp.info.bitCount <<
            "\n\tCompression: " << bmp.info.compression << '\n';
        throw std::runtime_error(msg.str());
    }
    
    auto windowFuture = Gui::Renderer::GetInstance()->
        createWindowAsync(static_cast<int>(bmp.info.width), static_cast<int>(bmp.info.height), "Bmp", Gui::RenderMode::Point);
    
    if (const auto window = windowFuture.get().lock()) {
        const auto points = bmp.getPoints();
        for (auto& point : *points) {
            point.color.normalizeColor();
        }
        window->setBufferDataPointsAsync(points);
        window->showWindowAsync();
    }
}
