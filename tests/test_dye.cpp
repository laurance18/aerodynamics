#include "core/dye.hpp"
#include "core/scene.hpp"
#include "io/scene_io.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <numbers>
#include <vector>

using namespace pfl;

namespace {

constexpr double pi = std::numbers::pi;

const auto alwaysValid = [](Vec2) { return true; };

DyeSettings testSettings() {
    DyeSettings settings;
    settings.bounds = {{-10.0, -10.0}, {10.0, 10.0}};
    settings.maxAge = 4.0;
    settings.releaseInterval = 0.1;
    settings.maxStepLength = 0.01;
    return settings;
}

} // namespace

TEST_CASE("rake emitters are evenly spaced from end to end") {
    const std::vector<Vec2> three = emitterPositions({.start = {0.0, -1.0}, .end = {0.0, 1.0}, .emitters = 3});
    REQUIRE(three.size() == 3);
    CHECK(three[0] == Vec2{0.0, -1.0});
    CHECK(three[1].y == doctest::Approx(0.0));
    CHECK(three[2] == Vec2{0.0, 1.0});

    const std::vector<Vec2> one = emitterPositions({.start = {1.0, 1.0}, .end = {3.0, 5.0}, .emitters = 1});
    REQUIRE(one.size() == 1);
    CHECK(one[0] == Vec2{2.0, 3.0});

    CHECK_FALSE(dyeRakeError({}));
    CHECK(dyeRakeError({.emitters = 0}));
    CHECK(dyeRakeError({.emitters = k_maxRakeEmitters + 1}));
    CHECK(dyeRakeError({.start = {NAN, 0.0}}));
}

TEST_CASE("dye released into a uniform stream travels with it at a steady spacing") {
    DyeSystem dye;
    dye.settings() = testSettings();
    const std::vector<DyeEmitter> emitters = {{.position = {0.0, 0.0}}, {.position = {0.0, 1.0}, .colour = 3}};
    const auto velocity = [](Vec2) { return Vec2{2.0, 0.0}; };
    // Uneven steps: releases fall between steps and are advected for the time since their release.
    for (int i = 0; i < 40; ++i) dye.step(i % 2 == 0 ? 0.03 : 0.07, emitters, velocity, alwaysValid);

    REQUIRE(dye.streaks().size() == 2);
    CHECK(dye.streaks()[1].colour == 3);
    for (const DyeStreak& streak : dye.streaks()) {
        REQUIRE(streak.points.size() >= 19);
        for (std::size_t k = 0; k < streak.points.size(); ++k) {
            const DyePoint& p = streak.points[k];
            CHECK(p.alive);
            CHECK(p.pos.x == doctest::Approx(2.0 * p.age)); // x = V t since release
            if (k > 0) {
                CHECK(p.serial == streak.points[k - 1].serial + 1);
                CHECK(streak.points[k - 1].pos.x - p.pos.x == doctest::Approx(2.0 * 0.1)); // V * interval
            }
        }
    }
}

TEST_CASE("dye dies outside the bounds, at invalid positions and of old age") {
    DyeSystem dye;
    DyeSettings settings = testSettings();
    settings.bounds = {{-1.0, -1.0}, {1.0, 1.0}};
    dye.settings() = settings;
    const std::vector<DyeEmitter> emitters = {{.position = {0.0, 0.0}}, {.position = {0.0, 0.5}}};
    const auto velocity = [](Vec2) { return Vec2{1.0, 0.0}; };
    const auto valid = [](Vec2 p) { return p.y < 0.25 || p.x < 0.5; }; // the second streak hits a wall at x = 0.5
    for (int i = 0; i < 300; ++i) dye.step(1.0 / 60.0, emitters, velocity, valid);

    REQUIRE(dye.streaks().size() == 2);
    for (const DyeStreak& streak : dye.streaks()) {
        for (const DyePoint& p : streak.points) {
            CHECK(p.alive);
            CHECK(settings.bounds.contains(p.pos));
            CHECK(valid(p.pos));
        }
    }
    CHECK(dye.streaks()[0].points.front().pos.x <= 1.0);
    CHECK(dye.streaks()[1].points.front().pos.x < 0.5);

    // Old age, with no bounds to leave: a vortex keeps the dye going round.
    dye.clear();
    settings.bounds = {{-10.0, -10.0}, {10.0, 10.0}};
    dye.settings() = settings;
    const std::vector<DyeEmitter> one = {{.position = {1.0, 0.0}}};
    for (int i = 0; i < 600; ++i) dye.step(1.0 / 60.0, one, [](Vec2 p) { return perp(p); }, alwaysValid);
    REQUIRE(dye.streaks().size() == 1);
    for (const DyePoint& p : dye.streaks()[0].points) {
        CHECK(p.age <= settings.maxAge);
        CHECK(length(p.pos) == doctest::Approx(1.0).epsilon(1e-3)); // on the streamline through the emitter
    }
}

TEST_CASE("detached dye drifts away and a moved emitter keeps its streak") {
    DyeSystem dye;
    dye.settings() = testSettings();
    const auto velocity = [](Vec2) { return Vec2{1.0, 0.0}; };
    std::vector<DyeEmitter> emitters = {{.position = {0.0, 0.0}}};
    for (int i = 0; i < 10; ++i) dye.step(0.05, emitters, velocity, alwaysValid);
    REQUIRE(dye.streaks().size() == 1);

    emitters[0].position = {0.0, 0.2}; // moved: same streak
    for (int i = 0; i < 10; ++i) dye.step(0.05, emitters, velocity, alwaysValid);
    REQUIRE(dye.streaks().size() == 1);
    CHECK(dye.streaks()[0].points.back().pos.y == doctest::Approx(0.2));
    CHECK(dye.streaks()[0].points.front().pos.y == doctest::Approx(0.0));

    dye.detach(); // e.g. a rake was added: new streaks start, the old one is no longer fed
    emitters.push_back({.position = {0.0, -1.0}, .colour = 1});
    dye.step(0.05, emitters, velocity, alwaysValid);
    REQUIRE(dye.streaks().size() == 3);
    CHECK(dye.streaks()[0].attached);
    CHECK(dye.streaks()[1].attached);
    CHECK_FALSE(dye.streaks()[2].attached);
    const std::size_t oldCount = dye.streaks()[2].points.size();

    for (int i = 0; i < 200; ++i) dye.step(0.05, emitters, velocity, alwaysValid);
    CHECK(dye.streaks().size() == 2); // the detached streak aged out and was dropped
    CHECK(oldCount > 0);
}

TEST_CASE("elements move by their anchor point; a uniform flow has none") {
    CHECK_FALSE(anchorPoint(UniformFlow{}));
    const SceneElement source = SourceSink{.strength = 2.0, .position = {1.0, 2.0}};
    CHECK(*anchorPoint(source) == Vec2{1.0, 2.0});
    const SceneElement moved = movedTo(source, {-1.0, 0.5});
    CHECK(std::get<SourceSink>(moved).position == Vec2{-1.0, 0.5});
    CHECK(std::get<SourceSink>(moved).strength == 2.0);

    const SceneElement sector = SectorFlow{.vertex = {0.0, 0.0}, .wall1 = 0.0, .wall2 = pi / 3.0};
    CHECK(std::get<SectorFlow>(movedTo(sector, {1.0, 1.0})).vertex == Vec2{1.0, 1.0});
    CHECK(std::get<SectorFlow>(movedTo(sector, {1.0, 1.0})).wall2 == doctest::Approx(pi / 3.0));
    const SceneElement wall = SymmetryWall{.point = {0.0, -1.0}, .angle = 0.3};
    CHECK(std::get<SymmetryWall>(movedTo(wall, {0.0, -2.0})).point == Vec2{0.0, -2.0});
    CHECK(std::get<SymmetryWall>(movedTo(wall, {0.0, -2.0})).angle == 0.3);
    for (const SceneElement& e : {SceneElement{Vortex{.position = {3.0, 4.0}}}, SceneElement{Doublet{.position = {3.0, 4.0}}}}) {
        CHECK(*anchorPoint(movedTo(e, {5.0, 6.0})) == Vec2{5.0, 6.0});
    }
    CHECK(std::get<UniformFlow>(movedTo(UniformFlow{.speed = 2.0}, {1.0, 1.0})).speed == 2.0);
}

TEST_CASE("dye rakes and the stagnation-point setting survive a save/load round trip") {
    Scene scene;
    scene.elements = {UniformFlow{}};
    scene.rakes = {{.start = {-3.0, -1.0}, .end = {-3.0, 1.0}, .emitters = 12}, {.start = {0.5, 2.0}, .end = {1.5, 2.5}, .emitters = 1}};
    scene.plot.showStagnationPoints = false;
    const SceneLoadResult loaded = sceneFromJson(sceneToJson(scene));
    REQUIRE_MESSAGE(loaded.scene.has_value(), loaded.error);
    REQUIRE(loaded.scene->rakes.size() == 2);
    for (std::size_t i = 0; i < 2; ++i) {
        CHECK(loaded.scene->rakes[i].start == scene.rakes[i].start);
        CHECK(loaded.scene->rakes[i].end == scene.rakes[i].end);
        CHECK(loaded.scene->rakes[i].emitters == scene.rakes[i].emitters);
    }
    CHECK_FALSE(loaded.scene->plot.showStagnationPoints);

    // Files without rakes (all older ones) have none; a bad rake is rejected by number.
    const SceneLoadResult none = sceneFromJson(R"({"format": "pfl-scene", "version": 1, "elements": []})");
    REQUIRE(none.scene);
    CHECK(none.scene->rakes.empty());
    CHECK(none.scene->plot.showStagnationPoints);
    const SceneLoadResult bad = sceneFromJson(R"({"format": "pfl-scene", "version": 1, "elements": [],
        "dye_rakes": [{"x1": 0, "y1": 0, "x2": 1, "y2": 0}, {"x1": 0, "y1": 0, "x2": 1, "y2": 0, "emitters": 0}]})");
    CHECK_FALSE(bad.scene);
    CHECK(bad.error.find("dye rake #2") != std::string::npos);
}
