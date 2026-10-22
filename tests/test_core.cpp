#include "core/particles.hpp"
#include "core/vec2.hpp"

#include <doctest/doctest.h>

using namespace pfl;

TEST_CASE("Vec2 arithmetic") {
    constexpr Vec2 a{1.0, 2.0};
    constexpr Vec2 b{3.0, -4.0};
    static_assert(a + b == Vec2{4.0, -2.0});
    CHECK(a - b == Vec2{-2.0, 6.0});
    CHECK(2.0 * a == Vec2{2.0, 4.0});
    CHECK(dot(a, b) == doctest::Approx(-5.0));
    CHECK(cross(a, b) == doctest::Approx(-10.0));
    CHECK(length(b) == doctest::Approx(5.0));
    CHECK(perp(Vec2{1.0, 0.0}) == Vec2{0.0, 1.0});
}

TEST_CASE("clipLine clips a line to a rectangle") {
    const Rect rect{{-1.0, -1.0}, {1.0, 1.0}};
    double t0 = -1e300;
    double t1 = 1e300;
    REQUIRE(clipLine({0.0, 0.5}, {1.0, 0.0}, rect, t0, t1));
    CHECK(t0 == doctest::Approx(-1.0));
    CHECK(t1 == doctest::Approx(1.0));

    t0 = 0.0; // a ray
    t1 = 1e300;
    REQUIRE(clipLine({0.0, 0.0}, {1.0, 1.0}, rect, t0, t1));
    CHECK(t0 == doctest::Approx(0.0));
    CHECK(t1 == doctest::Approx(1.0));

    t0 = -1e300;
    t1 = 1e300;
    CHECK_FALSE(clipLine({0.0, 2.0}, {1.0, 0.0}, rect, t0, t1));
}

TEST_CASE("particles stay inside the bounds and in valid positions") {
    ParticleSettings settings;
    settings.bounds = {{-1.0, -1.0}, {1.0, 1.0}};
    ParticleSystem system(500, settings, 42);
    const auto velocity = [](Vec2 p) { return perp(p) / (lengthSquared(p) + 0.05) + Vec2{0.3, 0.0}; };
    const auto valid = [](Vec2 p) { return length(p) >= 0.1; };
    for (int i = 0; i < 200; ++i) system.step(1.0 / 60.0, velocity, valid);

    REQUIRE(system.particles().size() == 500);
    for (const Particle& p : system.particles()) {
        CHECK(settings.bounds.contains(p.pos));
        CHECK(valid(p.pos));
    }
}
