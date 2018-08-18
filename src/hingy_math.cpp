

#include "hingy_math.h"

float sgn(float& a1) {
    if (a1 > 0) {
        return 1.0;
    }

    return -1.0f;
}

Vector2D Vector2D::operator*(Direction rhs)
{
    float nx = std::cos(ToDirection().h + rhs.h) * Length();
    float ny = std::sin(ToDirection().h + rhs.h) * Length();

    return Vector2D(nx, ny);
}

Direction Vector2D::ToDirection()
{
    return{ std::atan2(y, x) };
}

float Vector2D::Length()
{
    return std::sqrt(x * x + y * y);
}

float Direction::operator-(const Direction& rhs) const {
    float a1 = rhs.h;
    float a2 = h;

    float opt1 = a2 - a1;
    a1 = PI * sgn(a1) - a1;
    a2 = PI * sgn(a2) - a2;
    float opt2 = a1 - a2;

    if (std::abs(opt1) < std::abs(opt2))
        return opt1;
    else
        return opt2;
}

Direction Direction::operator+(float rhs) const
{
    return Direction{ std::fmod((h + rhs + PI), 2.0f * PI) - PI };
}

Direction Direction::operator+(Direction rhs)
{
    return (*this) + rhs.h;
}

Direction Direction::Inv()
{
    return Direction{ std::fmod((h + 2.0f * PI), 2.0f * PI) - PI };
}

