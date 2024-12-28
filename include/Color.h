#pragma once
#include <fstream>

class Color {
public:
    float r, g, b, a;
    void print() const;
    void normalizeColor();
    static Color readFromFile(std::ifstream& file);
};
