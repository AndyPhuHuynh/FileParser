#pragma once
#include <fstream>

class Color {
public:
    float r, g, b, a;
    Color() = default;
    Color(float r, float g, float b);
    void print() const;
    void normalizeColor();
    static Color readFromFile(std::ifstream& file);
};
