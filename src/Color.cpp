#include "Color.h"
#include <iostream>

Color::Color(const float r, const float g, const float b) {
    this->r = r;
    this->g = g;
    this->b = b;
    this->a = 255;
}

void Color::print() const {
    std::cout
        <<   "r: " << r
        << ", g: " << g
        << ", b: " << b;
}

void Color::normalizeColor() {
    r = r / 255.0f;
    g = g / 255.0f;
    b = b / 255.0f;
    a = a / 255.0f;
}


Color Color::readFromFile(std::ifstream& file) {
    Color color;
    unsigned char byte;
    file.read(reinterpret_cast<char*>(&byte), 1);
    color.b = static_cast<float>(byte);
    file.read((reinterpret_cast<char*>(&byte)), 1);
    color.g = static_cast<float>(byte);
    file.read((reinterpret_cast<char*>(&byte)), 1);
    color.r = static_cast<float>(byte);
    file.read((reinterpret_cast<char*>(&byte)), 1);
    color.a = 255.0f;
    color.normalizeColor();
    return color;
}
