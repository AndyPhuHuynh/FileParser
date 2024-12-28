#version 330 core

in vec4 colorToSend;

layout(location = 0) out vec4 color;

void main() {
    color = colorToSend;
}