#include <iostream>
#include <vector>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "FileParser/FileUtil.h"
#include "FileParser/Utils.hpp"
#include "FileParser/Bmp/BmpImage.h"
#include "FileParser/Jpeg/Decoder.hpp"

// Vertex and fragment shaders for rendering a textured quad
auto vertexShaderSrc = R"(
#version 330 core
layout(location = 0) in vec2 pos;
layout(location = 1) in vec2 texCoord;
out vec2 TexCoord;
void main() {
    TexCoord = texCoord;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

auto fragmentShaderSrc = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D imageTexture;
void main() {
    FragColor = texture(imageTexture, TexCoord);
}
)";

// Create a texture from your image
GLuint createTexture(const FileParser::Image& img) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload data to GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // Set to 1 for RGB data
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,
        static_cast<int>(img.getWidth()),
        static_cast<int>(img.getHeight()),
        0, GL_RGB, GL_UNSIGNED_BYTE, img.getData().data());

    return tex;
}

// Compile shader and link program
GLuint compileShaders(const char* vertSrc, const char* fragSrc) {
    const GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertSrc, nullptr);
    glCompileShader(vertexShader);

    const GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragSrc, nullptr);
    glCompileShader(fragmentShader);

    const GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Cleanup
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

auto decodeImage(const std::filesystem::path& filePath) -> std::expected<FileParser::Image, std::string> {
    using namespace FileUtils;
    const FileType fileType = getFileType(filePath.string());
    switch (fileType) {
        case FileType::Bmp: {
            const auto img = FileParser::Bmp::decode(filePath);
            if (!img) {
                return FileParser::utils::getUnexpected(img, "Unable to parse bmp");
            }
            return img;
        }
        case FileType::Jpeg: {
            const auto img = FileParser::Jpeg::Decoder::decode(filePath);
            if (!img) {
                return FileParser::utils::getUnexpected(img, "Unable to parse jpeg");
            }
            return img;
        }
        case FileType::None:
            return std::unexpected<std::string>("File type not recognised");
    }
    return std::unexpected<std::string>("File type not recognised");
}

#include <fstream>
#include <vector>
#include <iostream>
#include <cstring>

int main(const int argc, const char** argv) {
    if (!glfwInit()) {
        std::cerr << "Error: Failed to initialize GLFW." << std::endl;
        return -1;
    }

    if (argc < 2) {
        std::cerr << "Provide one arg\n";
        return -1;
    }

    auto image = decodeImage(argv[1]);
    if (!image) {
        std::cout << "Error: " << image.error() << '\n';
        return 0;
    }

    // Create window
    GLFWwindow* window = glfwCreateWindow(static_cast<int>(image->getWidth()), static_cast<int>(image->getHeight()), "Image Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glewInit();

    const GLuint tex = createTexture(*image);
    const GLuint shaderProgram = compileShaders(vertexShaderSrc, fragmentShaderSrc);

    // Fullscreen quad (two triangles)
    constexpr float vertices[] = {
        // positions   // texcoords
        -1.0f, -1.0f,   0.0f, 0.0f,
         1.0f, -1.0f,   1.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 1.0f
    };
    constexpr unsigned int indices[] = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    // tex-coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<void *>(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Main loop
    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);
        glBindTexture(GL_TEXTURE_2D, tex);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
