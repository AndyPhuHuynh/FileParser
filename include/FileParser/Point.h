#pragma once

#include "Color.h"

struct Point {
    float x;
    float y;
    Color color;

    Point() : x(0), y(0), color(Color()) {}
    Point(const float x, const float y, const Color& color) : x(x), y(y), color(color) {}
};
