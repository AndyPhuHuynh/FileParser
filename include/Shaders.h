#pragma once
#include <string>

namespace Shaders {
    inline std::string PointVertexShader = R"(
        #version 330 core

        uniform float xOffset;
        uniform float yOffset;

        layout(location = 0) in vec2 position;
        layout(location = 1) in vec4 colorToDraw;

        out vec4 colorToSend;

        void main() {
            float x = position.x + float(xOffset);
            float y = position.y + float(yOffset);
            gl_Position = vec4(x, y, 0.0, 1.0);
            colorToSend = colorToDraw;
        }
    )";

    inline std::string PointFragmentShader = R"(
        #version 330 core

        in vec4 colorToSend;

        layout(location = 0) out vec4 color;

        void main() {
            color = colorToSend;
        }
    )";
}
