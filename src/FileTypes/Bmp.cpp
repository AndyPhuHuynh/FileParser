#include <fstream>
#include <iostream>

#include <Gl/glew.h>
#include <GLFW/glfw3.h>

#include "BitManipulationUtil.h"
#include "Bmp.h"
#include "ShaderUtil.h"

BmpPoint::BmpPoint() {
    x = 0;
    y = 0;
    color = Color();
}

BmpHeader BmpHeader::getHeaderFromFile(std::ifstream& file) {
    BmpHeader header;
    file.seekg(fileHeaderOffset, std::ios::beg);
    file.read(reinterpret_cast<char*>(&header.signature), 2);
    file.read(reinterpret_cast<char*>(&header.fileSize), 4);
    file.read(reinterpret_cast<char*>(&header.reserved), 4);
    file.read(reinterpret_cast<char*>(&header.dataOffset), 4);
    return header;
}

BmpInfo BmpInfo::getInfoFromFile(std::ifstream& file) {
    BmpInfo info;
    file.seekg(fileInfoHeaderPos, std::ios::beg);
    file.read(reinterpret_cast<char*>(&info.size), 4);
    file.read(reinterpret_cast<char*>(&info.width), 4);
    file.read(reinterpret_cast<char*>(&info.height), 4);
    file.read(reinterpret_cast<char*>(&info.planes), 2);
    file.read(reinterpret_cast<char*>(&info.bitCount), 2);
    file.read(reinterpret_cast<char*>(&info.compression), 4);
    file.read(reinterpret_cast<char*>(&info.imageSize), 4);
    file.read(reinterpret_cast<char*>(&info.xPixelsPerMeter), 4);
    file.read(reinterpret_cast<char*>(&info.yPixelsPerMeter), 4);
    file.read(reinterpret_cast<char*>(&info.colorsUsed), 4);
    file.read(reinterpret_cast<char*>(&info.importantColors), 4);
    return info;
}

BmpRasterEncoding BmpInfo::getRasterEncoding() const {
    if (bitCount == 1 && compression == 0) { return BmpRasterEncoding::Monochrome; }
    if (bitCount == 4 && compression == 0) { return BmpRasterEncoding::FourBitNoCompression; }
    if (bitCount == 8 && compression == 0) { return BmpRasterEncoding::EightBitNoCompression; }
    if (bitCount == 16 && compression == 0) { return BmpRasterEncoding::SixteenBitNoCompression; }
    if (bitCount == 24 && compression == 0) { return BmpRasterEncoding::TwentyFourBitNoCompression; }
    if (bitCount == 4 && compression == 2) { return BmpRasterEncoding::FourBit2Compression; }
    if (bitCount == 8 && compression == 1) { return BmpRasterEncoding::EightBit1Compression; }
    return BmpRasterEncoding::None;
}

void BmpInfo::print() const {
    std::cout << "Size: " << size << '\n';
    std::cout << "Width: " << width << '\n';
    std::cout << "Height: " << height << '\n';
    std::cout << "Planes: " << planes << '\n';
    std::cout << "Bits per pixel: " << bitCount << '\n';
    std::cout << "Compression: " << compression << '\n';
    std::cout << "Image size: " << imageSize << '\n';
    std::cout << "xPixelsPerMeter: " << xPixelsPerMeter << '\n';
    std::cout << "yPixelsPerMeter: " << yPixelsPerMeter << '\n';
    std::cout << "Colors used: " << colorsUsed << '\n';
    std::cout << "Important colors: " << importantColors << '\n';
}

int BmpInfo::getNumColors() const {
    if (bitCount == 1) {
        return 2;
    }
    if (bitCount == 4) {
        return 16;
    }
    if (bitCount == 8) {
        return 256;
    }
    return -1;
}

void Bmp::initColorTable() {
    int colorCount = info.getNumColors();
    colorTable = std::vector<Color>(colorCount);
    file.seekg(fileColorTableOffset, std::ios::beg);

    for (int i = 0; i < colorCount; i++) {
        colorTable[i] = Color::readFromFile(file);
    }
}

std::vector<BmpPoint> Bmp::getPoints() {
    std::vector<BmpPoint> points;
    points.reserve(info.width * info.height);
    file.seekg(header.dataOffset, std::ios::beg);
    
    for (uint32_t y = 0; y < info.height; y++) {
        float normalizedY = NormalizeToNdc(static_cast<float>(y), static_cast<int>(info.height));
        if (rasterEncoding == BmpRasterEncoding::TwentyFourBitNoCompression) {
            ParseRow24BitNoCompression(points, normalizedY);
        }
        else {
            ParseRowByteOrLessNoCompression(points, normalizedY);
        }
    }
    return points;
}

void Bmp::ParseRowByteOrLessNoCompression(std::vector<BmpPoint>& points, float normalizedY) {
    uint32_t pixelsInRowRead = 0;
    bool allNonPaddingBitsRead = false;
    for (uint32_t byteInRow = 0; byteInRow < rowSize; byteInRow++) {
        unsigned char byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
            
        if (allNonPaddingBitsRead) {
            continue;
        }

        int pixelsPerByte = 8 / info.bitCount;
        // Process bits
        for (int i = 0; i < pixelsPerByte; i++) {
            if (pixelsInRowRead >= info.width) {
                allNonPaddingBitsRead = true;
                break;
            }
                
            int x = static_cast<int>(byteInRow) * pixelsPerByte + i;
            float normalizedX = NormalizeToNdc(static_cast<float>(x), static_cast<int>(info.width));

            unsigned char index;
            if (rasterEncoding == BmpRasterEncoding::Monochrome) {
                index = GetBitFromLeft(byte, i);
            } else if (rasterEncoding == BmpRasterEncoding::FourBitNoCompression) {
                index = GetNibble(byte, i);
            } else {
                index = byte;
            }
            Color color = colorTable[index];

            points.emplace_back(normalizedX, normalizedY, color);
                
            pixelsInRowRead++;
        }
    }
}

void Bmp::ParseRow24BitNoCompression(std::vector<BmpPoint>& points, float normalizedY) {
    int pixelsInRowRead = 0;
    for (uint32_t x = 0; x < rowSize / 3; x++) {
        unsigned char byte;
        float normalizedX = NormalizeToNdc(static_cast<float>(x), static_cast<int>(info.width));
        Color color;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.b = byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.g = byte;
        file.read(reinterpret_cast<char*>(&byte), 1);
        color.r = byte;
        color.a = 255.0f;
        color.normalizeColor();
        
        points.emplace_back(normalizedX, normalizedY, color);
        pixelsInRowRead++;
    }
}

int Bmp::render() {
    if (rasterEncoding != BmpRasterEncoding::Monochrome &&
        rasterEncoding != BmpRasterEncoding::FourBitNoCompression &&
        rasterEncoding != BmpRasterEncoding::EightBitNoCompression &&
        rasterEncoding != BmpRasterEncoding::TwentyFourBitNoCompression) {
        std::cout << "Raster encoding not supported: " <<
            "\n\tBitCount: "<<  info.bitCount <<
            "\n\tCompression: " << info.compression << '\n';
        return -1;
    }
    
    std::cout << "Rendering..." << '\n';
    if (!glfwInit()) {
        return -1;
    }

    // Create a window
    GLFWwindow *window = glfwCreateWindow(static_cast<int>(info.width), static_cast<int>(info.height), "Hello World!",nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }

    // Make window current context
    glfwMakeContextCurrent(window);

    // Initialize glew
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << '\n';
        return -1;
    }

    std::cout << "OpenGL version: " << glGetString(GL_VERSION) << '\n';
    // std::ifstream vertexShaderFile("shaders/Basic.vert");
    // std::ifstream fragShaderFile("shaders/Basic.frag");
    // std::string vertexShader{std::istreambuf_iterator<char>(vertexShaderFile), std::istreambuf_iterator<char>()};
    // std::string fragShader{std::istreambuf_iterator<char>(fragShaderFile), std::istreambuf_iterator<char>()};

    std::string vertexShader = R"(
        #version 330 core

        layout(location = 0) in vec2 position;
        layout(location = 1) in vec4 colorToDraw;

        out vec4 colorToSend;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            colorToSend = colorToDraw;
        }
    )";

    std::string fragShader = R"(
        #version 330 core

        in vec4 colorToSend;

        layout(location = 0) out vec4 color;

        void main() {
            color = colorToSend;
        }
    )";

    const unsigned int shaderProgram = CreateShader(vertexShader, fragShader);
    glUseProgram(shaderProgram);
    glViewport(0, 0, static_cast<int>(info.width), static_cast<int>(info.height));
    
    std::vector<BmpPoint> points = getPoints();
    
    int pointsSize = static_cast<int>(sizeof(BmpPoint) * points.size());
    unsigned int buffer;
    glGenBuffers(1, &buffer);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glBufferData(GL_ARRAY_BUFFER, pointsSize, points.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(BmpPoint), nullptr);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(BmpPoint), reinterpret_cast<void*>(2 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Rendering code goes here
        glDrawArrays(GL_POINTS, 0, static_cast<int>(points.size()));
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}

Bmp::Bmp(const std::string& path) {
    file = std::ifstream(path, std::ios::binary);
    header = BmpHeader::getHeaderFromFile(file);
    info = BmpInfo::getInfoFromFile(file);
    rasterEncoding = info.getRasterEncoding();
    if (rasterEncoding == BmpRasterEncoding::Monochrome ||
        rasterEncoding == BmpRasterEncoding::FourBitNoCompression ||
        rasterEncoding == BmpRasterEncoding::EightBitNoCompression) {
        initColorTable();
    }
    rowSize = static_cast<uint32_t>(floor(((info.bitCount * info.width + 31) / 32))) * 4;
}