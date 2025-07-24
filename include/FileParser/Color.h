#pragma once
#include <fstream>

class Color {
public:
    float r = 0, g = 0, b = 0, a = 0;
    Color() = default;
    Color(float r, float g, float b);
    void print() const;
    void normalizeColor();
    static Color readFromFile(std::ifstream& file);
};
