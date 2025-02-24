#include "Jpeg/JpegBmpConverter.h"

#include <cstdint>
#include <iostream>

#include "BitManipulationUtil.h"

// TODO: Make writing to bmp faster

void ImageProcessing::Jpeg::Converter::WriteJpegAsBmp(const JpegImage& jpeg, const std::string& filename) {
    clock_t begin = clock();
    // open file
    std::cout << "Writing " << filename << "...\n";
    std::ofstream outFile(filename, std::ios::out | std::ios::binary);
    if (!outFile.is_open()) {
        std::cout << "Error - Error opening output file\n";
        return;
    }
    
    const int paddingSize = jpeg.info.width % 4;
    const int size = 14 + 12 + jpeg.info.height * jpeg.info.width * 3 + paddingSize * jpeg.info.height;

    uint8_t *buffer = new (std::nothrow) uint8_t[size];
    if (buffer == nullptr) {
        std::cout << "Error - Memory error\n";
        outFile.close();
        return;
    }
    uint8_t *bufferPos = buffer;

    *bufferPos++ = 'B';
    *bufferPos++ = 'M';
    PutInt(bufferPos, size);
    PutInt(bufferPos, 0);
    PutInt(bufferPos, 0x1A);
    PutInt(bufferPos, 12);
    PutShort(bufferPos, jpeg.info.width);
    PutShort(bufferPos, jpeg.info.height);
    PutShort(bufferPos, 1);
    PutShort(bufferPos, 24);
    
    for (int y = jpeg.info.height - 1; y>= 0; y--) {
        const int blockRow = y / jpeg.info.mcuPixelHeight;
        const int pixelRow = y % jpeg.info.mcuPixelHeight;
        for (int x = 0; x < jpeg.info.width; x++) {
            const int blockColumn = x / jpeg.info.mcuPixelWidth;
            const int pixelColumn = x % jpeg.info.mcuPixelWidth;
            const int blockIndex = blockRow * jpeg.info.mcuImageWidth + blockColumn;
            const int pixelIndex = pixelRow * jpeg.info.mcuPixelWidth + pixelColumn;
            auto [R, G, B] = jpeg.mcus[blockIndex]->getColor(pixelIndex);

            *bufferPos++ = B;
            *bufferPos++ = G;
            *bufferPos++ = R;
        }
        for (int i = 0; i < paddingSize; i++) {
            *bufferPos++ = 0;
        }
    }

    outFile.write(reinterpret_cast<char*>(buffer), size);
    outFile.close();
    delete[] buffer;
    clock_t end = clock();
    std::cout << "Time to write: " << static_cast<double>(end - begin) / CLOCKS_PER_SEC << " seconds\n";
}
