#include "core/dummy_flow.hpp"
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

TEST_CASE("pure vortex velocity is tangential and counter-clockwise") {
    const DemoFlowParams params{.strength = 1.5, .drift = 0.0, .coreEps = 0.05};
    for (const Vec2 p : {Vec2{1.0, 0.0}, Vec2{-0.3, 0.7}, Vec2{2.0, -1.0}}) {
        const Vec2 v = demoVelocity(p, params);
        CHECK(dot(v, p) == doctest::Approx(0.0));
        CHECK(cross(p, v) > 0.0);
        CHECK(length(v) == doctest::Approx(params.strength * length(p) / (lengthSquared(p) + params.coreEps)));
    }
}

TEST_CASE("zero strength leaves only the uniform drift") {
    const DemoFlowParams params{.strength = 0.0, .drift = 0.7, .coreEps = 0.05};
    CHECK(demoVelocity({0.4, -1.2}, params) == Vec2{0.7, 0.0});
    CHECK(demoMaxSpeed(params) == doctest::Approx(0.7));
}

TEST_CASE("particles stay inside the domain and outside the respawn radius") {
    ParticleSettings settings;
    settings.domain = {{-1.0, -1.0}, {1.0, 1.0}};
    ParticleSystem system(500, settings, 42);
    const DemoFlowParams params{};
    for (int i = 0; i < 200; ++i) {
        system.step(1.0 / 60.0, [&](Vec2 p) { return demoVelocity(p, params); });
    }
    REQUIRE(system.particles().size() == 500);
    for (const Particle& p : system.particles()) {
        CHECK(settings.domain.contains(p.pos));
        CHECK(length(p.pos) >= settings.respawnRadius);
    }
}
