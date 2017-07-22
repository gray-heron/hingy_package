#pragma once

#include <cmath>

#define PI 3.1415f
#define HALF_PI (3.1415f / 2.0f)

float sgn(float& a1);

struct Direction {
    float h;

    float operator-(const Direction& rhs) const;
    Direction operator+(float rhs) const;
    Direction operator+(Direction rhs);
    Direction Inv();
};

class Vector2D {
public:
    Vector2D(float x, float y) : x(x), y(y) {}
    Vector2D() : x(0.0f), y(0.0f) {}

    float x, y;

    Vector2D operator-(Vector2D rhs) {
        return Vector2D{ x - rhs.x, y - rhs.y };
    }

    Vector2D operator+(Vector2D rhs) {
        return Vector2D{ x + rhs.x, y + rhs.y };
    }

    Vector2D operator*(float rhs) {
        return Vector2D{ x * rhs, y * rhs };
    }

    Vector2D operator*(Direction rhs);

    void operator+=(Vector2D rhs) {
        x += rhs.x;
        y += rhs.y;
    }

    Direction ToDirection();
    float Length();
};