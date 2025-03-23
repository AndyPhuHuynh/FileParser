#include "Bmp/BmpJpegConverter.h"

#include "Jpeg/JpegEncoder.h"

std::vector<ImageProcessing::Jpeg::Mcu> ImageProcessing::Bmp::Converter::getMcus(BmpImage& bmp) {
    auto points = bmp.getPoints();
    
    int columnCount = static_cast<int>(bmp.info.width + 7) / 8;
    int rowCount = static_cast<int>(bmp.info.height + 7) / 8;
    int height = static_cast<int>(bmp.info.height);
    int width = static_cast<int>(bmp.info.width);
    
    std::vector rows(rowCount, std::vector<Jpeg::ColorBlock>(columnCount));
    
    for (auto& point: *points) {
        int blockRow = (static_cast<int>(point.y) / 8);
        int pixelRow = (static_cast<int>(point.y) % 8);
        int blockCol = (static_cast<int>(point.x) / 8);
        int pixelCol = (static_cast<int>(point.x) % 8);
        rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = point.color.r;
        rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = point.color.g;
        rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = point.color.b;
    }

    // Pad points outside the height of the image with same color as the last color within the column
    if (height % 8 != 0) {
        for (int y = height; y < rowCount * 8; y++) {
            int blockRow = height / 8;
            int pixelRow = y % 8;
            for (int x = 0; x < width; x++) {
                int blockCol = x / 8;
                int pixelCol = x % 8;
                float prevRed = rows[blockRow][blockCol].R[(pixelRow - 1) * 8 + pixelCol];
                float prevGreen = rows[blockRow][blockCol].G[(pixelRow - 1) * 8 + pixelCol];
                float prevBlue = rows[blockRow][blockCol].B[(pixelRow - 1) * 8 + pixelCol];
                rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = prevRed;
                rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = prevGreen;
                rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = prevBlue;
            }
        }
    }

    // Pad points outside the width of the image with same color as the last color within the row
    if (width % 8 != 0) {
        for (int y = 0; y < rowCount * 8; y++) {
            int blockRow = y / 8;
            int pixelRow = y % 8;
            for (int x = width; x < columnCount * 8; x++) {
                int blockCol = x / 8;
                int pixelCol = x % 8;
                float prevRed = rows[blockRow][blockCol].R[pixelRow * 8 + (pixelCol - 1)];
                float prevGreen = rows[blockRow][blockCol].G[pixelRow * 8 + (pixelCol - 1)];
                float prevBlue = rows[blockRow][blockCol].B[pixelRow * 8 + (pixelCol - 1)];
                rows[blockRow][blockCol].R[pixelRow * 8 + pixelCol] = prevRed;
                rows[blockRow][blockCol].G[pixelRow * 8 + pixelCol] = prevGreen;
                rows[blockRow][blockCol].B[pixelRow * 8 + pixelCol] = prevBlue;
            }
        }
    }

    std::vector<Jpeg::Mcu> mcus;
    for (const auto& row : rows) {
        for (const auto& block : row) {
            mcus.emplace_back(block);
        }
    }
    return mcus;
}

void ImageProcessing::Bmp::Converter::writeBmpAsJpeg(BmpImage& bmp, const std::string& filename) {
    std::vector<Jpeg::Mcu> mcus = getMcus(bmp);
    Jpeg::Encoder::writeJpeg(filename, mcus, static_cast<uint16_t>(bmp.info.height), static_cast<uint16_t>(bmp.info.width));
}
