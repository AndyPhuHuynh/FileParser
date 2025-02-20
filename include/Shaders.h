﻿#pragma once
#include <string>

namespace shaders {
    inline std::string PointVertexShader = R"(
        #version 330 core

        layout(location = 0) in vec2 position;
        layout(location = 1) in vec4 colorToDraw;

        out vec4 colorToSend;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
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
