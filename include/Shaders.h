#pragma once
#include <string>

namespace Shaders {
    inline std::string PointVertexShader = R"(
        #version 330 core

        layout(location = 0) in vec2 position;
        layout(location = 1) in vec4 colorToDraw;

        out vec4 vertColor;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            vertColor = colorToDraw;
        }
    )";

    inline std::string PointGeometryShader = R"(
        #version 330 core

        layout (points) in;
        layout (triangle_strip, max_vertices = 4) out;

        uniform mat4 u_mvp;
        
        in vec4 vertColor[];
        out vec4 geoColor;

        void main() {
            vec4 pos = gl_in[0].gl_Position;

            gl_Position = u_mvp * vec4(pos.x, pos.y, pos.zw);
            geoColor = vertColor[0];
            EmitVertex();

            gl_Position = u_mvp * vec4(pos.x + 1, pos.y , pos.zw);
            geoColor = vertColor[0];
            EmitVertex();

            gl_Position = u_mvp * vec4(pos.x, pos.y + 1, pos.zw);
            geoColor = vertColor[0];
            EmitVertex();

            gl_Position = u_mvp * vec4(pos.x + 1, pos.y + 1, pos.zw);
            geoColor = vertColor[0];
            EmitVertex();

            EndPrimitive();
        }
    )";
    
    inline std::string PointFragmentShader = R"(
        #version 330 core

        in vec4 geoColor;

        layout(location = 0) out vec4 fragColor;

        void main() {
            fragColor = geoColor;
        }
    )";
}
