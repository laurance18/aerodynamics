// vec2.hpp: minimal 2D vector type used throughout the core.
#pragma once

#include <cmath>

namespace pfl {

struct Vec2 {
    double x = 0.0;
    double y = 0.0;

    constexpr Vec2& operator+=(Vec2 o) {
        x += o.x;
        y += o.y;
        return *this;
    }
    constexpr Vec2& operator-=(Vec2 o) {
        x -= o.x;
        y -= o.y;
        return *this;
    }
    constexpr Vec2& operator*=(double s) {
        x *= s;
        y *= s;
        return *this;
    }

    friend constexpr bool operator==(Vec2, Vec2) = default;
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator-(Vec2 a) { return {-a.x, -a.y}; }
constexpr Vec2 operator*(Vec2 a, double s) { return {a.x * s, a.y * s}; }
constexpr Vec2 operator*(double s, Vec2 a) { return {a.x * s, a.y * s}; }
constexpr Vec2 operator/(Vec2 a, double s) { return {a.x / s, a.y / s}; }

constexpr double dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr double cross(Vec2 a, Vec2 b) { return a.x * b.y - a.y * b.x; }
constexpr double lengthSquared(Vec2 a) { return dot(a, a); }
inline double length(Vec2 a) { return std::sqrt(lengthSquared(a)); }

// Counter-clockwise perpendicular: (x, y) -> (-y, x).
constexpr Vec2 perp(Vec2 a) { return {-a.y, a.x}; }

} // namespace pfl
