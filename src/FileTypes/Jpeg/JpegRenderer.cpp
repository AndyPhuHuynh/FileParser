#include "Jpeg/JpegRenderer.h"

#include <vector>

#include "Gui/Renderer.h"
#include "Gui/RenderWindow.h"
#include "Jpeg/JpegImage.h"
#include "Point.h"
#include "ShaderUtil.h"

void ImageProcessing::Jpeg::Renderer::RenderJpeg(const JpegImage& jpeg) {
    std::shared_ptr<std::vector<Point>> points = std::make_shared<std::vector<Point>>();
    points->reserve(static_cast<unsigned int>(jpeg.info.width * jpeg.info.height));
    // For every mcu
    for (int mcuIndex = 0; mcuIndex < static_cast<int>(jpeg.mcus.size()); mcuIndex++) {
        int mcuX = mcuIndex % jpeg.info.mcuImageWidth;
        int mcuY = mcuIndex / jpeg.info.mcuImageWidth;
        
        // For every color block in the Mcu
        for (int blockY = 0; blockY < jpeg.info.maxVerticalSample; blockY++) {
            for (int blockX = 0; blockX < jpeg.info.maxHorizontalSample; blockX++) {
                constexpr int blockSideLength = 8;
                auto& [R, G, B] = jpeg.mcus[mcuIndex]->colorBlocks[blockY * jpeg.info.maxHorizontalSample + blockX];
        
                // For every color in the color block
                for (int colorY = 0; colorY < blockSideLength; colorY++) {
                    int pointY = (mcuY * jpeg.info.mcuPixelHeight) + (blockY * blockSideLength) + colorY;
                    if (pointY >= jpeg.info.height) {
                        break;
                    }
                    for (int colorX = 0; colorX < blockSideLength; colorX++) {
                        int pointX = (mcuX * jpeg.info.mcuPixelWidth) + (blockX * blockSideLength) + colorX;
                        if (pointX >= jpeg.info.width) {
                            break;
                        }
                        Color color = Color(R[colorY * blockSideLength + colorX],
                            G[colorY * blockSideLength + colorX],
                            B[colorY * blockSideLength + colorX]);
                        color.normalizeColor();
                        
                        float normalX = Shaders::Util::NormalizeToNdc(static_cast<float>(pointX), jpeg.info.width);
                        float normalY = Shaders::Util::NormalizeToNdc(static_cast<float>(pointY), jpeg.info.height) * -1;
                        
                        points->emplace_back(normalX, normalY, color);
                    }
                }
            }
        }
    }

    auto windowFuture = Gui::Renderer::GetInstance()->createWindowAsync(jpeg.info.width, jpeg.info.height, "Jpeg", Gui::RenderMode::Point);

    if (auto window = windowFuture.get().lock()) {
        window->setBufferDataPointsAsync(points);
        window->showWindowAsync();
    }
}

