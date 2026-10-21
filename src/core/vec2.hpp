// vec2.hpp: minimal 2D vector type used throughout the core.
#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

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

// Axis-aligned rectangle [min, max].
struct Rect {
    Vec2 min{-1.0, -1.0};
    Vec2 max{1.0, 1.0};

    constexpr bool contains(Vec2 p) const { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }
    constexpr Vec2 size() const { return max - min; }
    constexpr Vec2 center() const { return (min + max) * 0.5; }
    // Grown by `fraction` of its size on every side.
    constexpr Rect expanded(double fraction) const {
        const Vec2 margin = size() * fraction;
        return {min - margin, max + margin};
    }

    friend constexpr bool operator==(const Rect&, const Rect&) = default;
};

// Parameter range [t0, t1] for which origin + t * direction lies inside `rect`, restricted to
// [tMin, tMax] (Liang-Barsky). Returns false if the line misses the rectangle.
inline bool clipLine(Vec2 origin, Vec2 direction, const Rect& rect, double& tMin, double& tMax) {
    const double d[2] = {direction.x, direction.y};
    const double o[2] = {origin.x, origin.y};
    const double lo[2] = {rect.min.x, rect.min.y};
    const double hi[2] = {rect.max.x, rect.max.y};
    for (int axis = 0; axis < 2; ++axis) {
        if (d[axis] == 0.0) {
            if (o[axis] < lo[axis] || o[axis] > hi[axis]) return false;
            continue;
        }
        double t0 = (lo[axis] - o[axis]) / d[axis];
        double t1 = (hi[axis] - o[axis]) / d[axis];
        if (t0 > t1) std::swap(t0, t1);
        tMin = std::max(tMin, t0);
        tMax = std::min(tMax, t1);
    }
    return tMin <= tMax;
}

} // namespace pfl
