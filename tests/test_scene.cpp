#include "core/image_system.hpp"
#include "core/scene.hpp"
#include "core/streamline.hpp"
#include "io/scene_io.hpp"

#include <doctest/doctest.h>

#include <cmath>
#include <filesystem>
#include <numbers>

using namespace pfl;

namespace {

constexpr double pi = std::numbers::pi;
constexpr double deg = pi / 180.0;

// Largest |v . n| relative to the largest |v| along the wall within a 10 x 10 box.
double wallResidual(const FlowModel& model, const SymmetryWall& wall) {
    const WallCheck check = checkWall(model.field, wall, {{-5.0, -5.0}, {5.0, 5.0}}, 200);
    REQUIRE(check.samples > 100);
    REQUIRE(check.maxSpeed > 0.0);
    return check.maxNormalSpeed / check.maxSpeed;
}

} // namespace

TEST_CASE("images across a wall: sources keep, vortices flip, doublets reflect their axis") {
    const SymmetryWall wall{.point = {0.0, -1.0}, .angle = 0.0}; // the line y = -1
    CHECK(reflect({2.0, 1.0}, wall).x == doctest::Approx(2.0));
    CHECK(reflect({2.0, 1.0}, wall).y == doctest::Approx(-3.0));

    const auto source = std::get<SourceSink>(*mirror(SourceSink{.strength = 3.0, .position = {1.0, 0.0}}, wall));
    CHECK(source.strength == 3.0);
    CHECK(source.position.y == doctest::Approx(-2.0));
    const auto vortex = std::get<Vortex>(*mirror(Vortex{.circulation = 2.0, .position = {1.0, 0.0}}, wall));
    CHECK(vortex.circulation == -2.0);
    const auto doublet = std::get<Doublet>(*mirror(Doublet{.strength = 1.0, .angle = 30.0 * deg}, wall));
    CHECK(doublet.angle == doctest::Approx(-30.0 * deg));
    CHECK_FALSE(mirror(UniformFlow{}, wall).has_value());
}

TEST_CASE("a symmetry wall has zero normal velocity") {
    const SymmetryWall wall{.point = {0.3, -0.8}, .angle = 25.0 * deg};
    const Scene scene{.elements = {UniformFlow{.speed = 1.0, .angle = 10.0 * deg}, SourceSink{.strength = 4.0, .position = {1.0, 1.0}},
                                   Vortex{.circulation = -3.0, .position = {-1.5, 0.5}},
                                   Doublet{.strength = 0.7, .position = {0.5, 2.0}, .angle = 70.0 * deg}, wall}};
    const FlowModel model = buildFlowModel(scene);
    CHECK(model.imagesExact);
    CHECK(model.images.size() == 3);
    CHECK(wallResidual(model, wall) < 1e-12);
    // The uniform flow keeps only its component along the wall.
    CHECK(model.freeStreamSpeed == doctest::Approx(std::cos(15.0 * deg)));
}

TEST_CASE("two perpendicular walls close the image system exactly") {
    const SymmetryWall floor{.point = {0.0, 0.0}, .angle = 0.0};
    const SymmetryWall side{.point = {0.0, 0.0}, .angle = 90.0 * deg};
    const Scene scene{.elements = {Vortex{.circulation = 1.0, .position = {1.0, 2.0}}, floor, side}};
    const FlowModel model = buildFlowModel(scene);
    CHECK(model.imagesExact);
    CHECK(model.images.size() == 3);
    CHECK(wallResidual(model, floor) < 1e-12);
    CHECK(wallResidual(model, side) < 1e-12);
}

TEST_CASE("parallel walls truncate the image series and say so") {
    const Scene scene{.elements = {SourceSink{.strength = 1.0, .position = {0.0, 0.2}},
                                   SymmetryWall{.point = {0.0, -1.0}}, SymmetryWall{.point = {0.0, 1.0}}}};
    const FlowModel model = buildFlowModel(scene);
    CHECK_FALSE(model.imagesExact);
    CHECK_FALSE(sceneWarnings(scene, model).empty());
}

TEST_CASE("warnings for coincident singularities and elements on walls") {
    const Scene clean{.elements = {UniformFlow{}, Doublet{.strength = 1.0}}};
    CHECK(sceneWarnings(clean, buildFlowModel(clean)).empty());

    const Scene coincident{.elements = {SourceSink{.position = {1.0, 1.0}}, Vortex{.position = {1.0, 1.0}}}};
    CHECK(sceneWarnings(coincident, buildFlowModel(coincident)).size() == 1);

    const Scene onWall{.elements = {Vortex{.position = {2.0, 0.0}}, SymmetryWall{}}};
    CHECK(sceneWarnings(onWall, buildFlowModel(onWall)).size() == 1);
}

TEST_CASE("streamlines of a uniform flow are straight, evenly spaced and point downstream") {
    const FlowField field{.elements = {UniformFlow{.speed = 2.0, .angle = 45.0 * deg}}};
    StreamlineSettings settings;
    settings.bounds = {{-2.0, -2.0}, {2.0, 2.0}};
    settings.separation = 0.25;
    const StreamlineSet set = computeStreamlines(field, settings);

    REQUIRE(set.lines.size() > 15);
    CHECK_FALSE(set.arrows.empty());
    for (const auto& line : set.lines) {
        REQUIRE(line.size() >= 3);
        const Vec2 d = line.back() - line.front();
        CHECK(cross(d, Vec2{1.0, 1.0}) == doctest::Approx(0.0).epsilon(1e-6).scale(length(d)));
        CHECK(dot(d, Vec2{1.0, 1.0}) > 0.0);
    }
    for (const Arrow& a : set.arrows) {
        CHECK(a.direction.x == doctest::Approx(std::sqrt(0.5)));
        CHECK(a.direction.y == doctest::Approx(std::sqrt(0.5)));
    }
}

TEST_CASE("streamlines of a vortex are closed circles") {
    const FlowField field{.elements = {Vortex{.circulation = 2.0 * pi}}};
    StreamlineSettings settings;
    settings.bounds = {{-2.0, -2.0}, {2.0, 2.0}};
    settings.separation = 0.3;
    const StreamlineSet set = computeStreamlines(field, settings);

    int closedCircles = 0;
    for (const auto& line : set.lines) {
        const double r = length(line.front());
        double drift = 0.0;
        for (const Vec2 p : line) drift = std::max(drift, std::abs(length(p) - r) / r);
        INFO("r = " << r << ", points = " << line.size());
        CHECK(drift < 1e-5); // RK4 truncation over one loop
        if (line.front() == line.back() && line.size() > 10) ++closedCircles;
    }
    CHECK(closedCircles >= 3);
}

TEST_CASE("streamlines stay inside a sector's wedge") {
    const FlowField field{.elements = {SectorFlow{.strength = 1.0, .wall1 = 30.0 * deg, .wall2 = 120.0 * deg}}};
    StreamlineSettings settings;
    settings.bounds = {{-2.0, -2.0}, {2.0, 2.0}};
    settings.separation = 0.2;
    const StreamlineSet set = computeStreamlines(field, settings);
    REQUIRE_FALSE(set.lines.empty());
    for (const auto& line : set.lines) {
        for (const Vec2 p : line) CHECK(field.inDomain(p));
    }
}

TEST_CASE("scenes survive a save/load round trip") {
    Scene scene;
    scene.elements = {UniformFlow{.speed = 1.5, .angle = 30.0 * deg},
                      SourceSink{.strength = -2.0, .position = {1.0, -0.5}},
                      Vortex{.circulation = 3.0, .position = {0.0, 2.0}},
                      Doublet{.strength = 0.5, .position = {-1.0, 0.0}, .angle = 45.0 * deg},
                      SectorFlow{.strength = 2.0, .vertex = {0.5, 0.5}, .wall1 = 10.0 * deg, .wall2 = 100.0 * deg},
                      SymmetryWall{.point = {0.0, -2.0}, .angle = 5.0 * deg}};
    scene.plot.view = {{-3.0, -1.0}, {5.0, 4.0}};
    scene.plot.streamlineDensity = 31;
    scene.plot.showArrows = false;
    scene.plot.showParticles = false;

    const SceneLoadResult loaded = sceneFromJson(sceneToJson(scene));
    REQUIRE_MESSAGE(loaded.scene.has_value(), loaded.error);
    const Scene& copy = *loaded.scene;
    REQUIRE(copy.elements.size() == scene.elements.size());
    for (std::size_t i = 0; i < scene.elements.size(); ++i) {
        INFO("element " << i);
        CHECK(copy.elements[i].index() == scene.elements[i].index());
        CHECK(describe(copy.elements[i]) == describe(scene.elements[i]));
    }
    // Same velocity field.
    const FlowModel a = buildFlowModel(scene);
    const FlowModel b = buildFlowModel(copy);
    for (const Vec2 p : {Vec2{0.7, 1.1}, Vec2{-0.3, 0.9}}) {
        CHECK(a.field.velocity(p).x == doctest::Approx(b.field.velocity(p).x));
        CHECK(a.field.velocity(p).y == doctest::Approx(b.field.velocity(p).y));
    }
    CHECK(copy.plot.view == scene.plot.view);
    CHECK(copy.plot.streamlineDensity == 31);
    CHECK_FALSE(copy.plot.showArrows);
    CHECK_FALSE(copy.plot.showParticles);
}

TEST_CASE("invalid scene files are rejected with a message") {
    CHECK_FALSE(sceneFromJson("not json").scene);
    CHECK_FALSE(sceneFromJson(R"({"format": "something-else", "version": 1})").scene);
    const SceneLoadResult badSector = sceneFromJson(
        R"({"format": "pfl-scene", "version": 1, "elements": [
            {"type": "sector", "strength": 1, "x": 0, "y": 0, "theta1_deg": 90, "theta2_deg": 10}]})");
    CHECK_FALSE(badSector.scene);
    CHECK(badSector.error.find("element #1") != std::string::npos);
    const SceneLoadResult missing =
        sceneFromJson(R"({"format": "pfl-scene", "version": 1, "elements": [{"type": "vortex", "x": 0, "y": 0}]})");
    CHECK_FALSE(missing.scene);
    CHECK(missing.error.find("circulation") != std::string::npos);
}

TEST_CASE("every example scene loads") {
    int count = 0;
    for (const auto& entry : std::filesystem::directory_iterator(PFL_EXAMPLES_DIR)) {
        if (entry.path().extension() != ".json") continue;
        INFO(entry.path().filename().string());
        const SceneLoadResult result = loadSceneFile(entry.path());
        CHECK_MESSAGE(result.scene.has_value(), result.error);
        ++count;
    }
    CHECK(count > 0);
}
