#include "core/flow_element.hpp"

#include <cmath>

namespace pfl {
namespace {

using std::numbers::pi;
constexpr double k_twoPi = 2.0 * pi;
constexpr Complex k_i{0.0, 1.0};

// Points within this angle of a sector wall count as on the wall, so rounding in atan2 does not
// push wall points out of the wedge.
constexpr double k_angleTolerance = 1e-9;

template <typename... Ts>
struct Overloaded : Ts... {
    using Ts::operator()...;
};

bool isFinite(Vec2 p) { return std::isfinite(p.x) && std::isfinite(p.y); }

} // namespace

double sectorLocalAngle(const SectorFlow& sector, Vec2 p) {
    const Vec2 d = p - sector.vertex;
    double angle = std::fmod(std::atan2(d.y, d.x) - sector.wall1, k_twoPi);
    if (angle < 0.0) angle += k_twoPi;
    // Just below the first wall wraps to ~2 pi; snap it back unless the wedge really is 2 pi wide.
    if (angle > k_twoPi - k_angleTolerance && sector.opening() < k_twoPi - k_angleTolerance) angle = 0.0;
    return angle;
}

Complex complexVelocity(const FlowElement& element, Vec2 p) {
    return std::visit(
        Overloaded{
            [](const UniformFlow& e) -> Complex { return e.speed * std::polar(1.0, -e.angle); },
            [p](const SourceSink& e) -> Complex { return e.strength / (k_twoPi * toComplex(p - e.position)); },
            [p](const Vortex& e) -> Complex { return k_i * e.circulation / (k_twoPi * toComplex(p - e.position)); },
            [p](const Doublet& e) -> Complex {
                const Complex zeta = toComplex(p - e.position);
                return -e.strength * std::polar(1.0, e.angle) / (zeta * zeta);
            },
            [p](const SectorFlow& e) -> Complex {
                // V n e^{-i theta1} |zeta|^{n-1} e^{i (n-1) theta_l}
                const double n = e.exponent();
                const double r = length(p - e.vertex);
                return e.strength * n * std::polar(1.0, -e.wall1) * std::pow(r, n - 1.0) *
                       std::polar(1.0, (n - 1.0) * sectorLocalAngle(e, p));
            },
        },
        element);
}

Complex complexPotential(const FlowElement& element, Vec2 p) {
    return std::visit(
        Overloaded{
            [p](const UniformFlow& e) -> Complex { return e.speed * std::polar(1.0, -e.angle) * toComplex(p); },
            [p](const SourceSink& e) -> Complex { return e.strength / k_twoPi * std::log(toComplex(p - e.position)); },
            [p](const Vortex& e) -> Complex {
                return k_i * e.circulation / k_twoPi * std::log(toComplex(p - e.position));
            },
            [p](const Doublet& e) -> Complex {
                return e.strength * std::polar(1.0, e.angle) / toComplex(p - e.position);
            },
            [p](const SectorFlow& e) -> Complex {
                // V |zeta|^n e^{i n theta_l}
                const double n = e.exponent();
                return e.strength * std::pow(length(p - e.vertex), n) * std::polar(1.0, n * sectorLocalAngle(e, p));
            },
        },
        element);
}

std::optional<Vec2> singularPoint(const FlowElement& element) {
    return std::visit(Overloaded{
                          [](const UniformFlow&) -> std::optional<Vec2> { return std::nullopt; },
                          [](const SourceSink& e) -> std::optional<Vec2> { return e.position; },
                          [](const Vortex& e) -> std::optional<Vec2> { return e.position; },
                          [](const Doublet& e) -> std::optional<Vec2> { return e.position; },
                          [](const SectorFlow& e) -> std::optional<Vec2> {
                              if (e.exponent() < 1.0) return e.vertex;
                              return std::nullopt;
                          },
                      },
                      element);
}

bool inElementDomain(const FlowElement& element, Vec2 p) {
    const auto* sector = std::get_if<SectorFlow>(&element);
    return sector == nullptr || sectorLocalAngle(*sector, p) <= sector->opening() + k_angleTolerance;
}

std::optional<std::string> validationError(const FlowElement& element) {
    using Result = std::optional<std::string>;
    const Result notFinite = "All parameters must be finite numbers.";
    return std::visit(
        Overloaded{
            [&](const UniformFlow& e) -> Result {
                if (!std::isfinite(e.speed) || !std::isfinite(e.angle)) return notFinite;
                if (e.speed < 0.0) return "Speed must not be negative; reverse the flow with the angle instead.";
                return std::nullopt;
            },
            [&](const SourceSink& e) -> Result {
                if (!std::isfinite(e.strength) || !isFinite(e.position)) return notFinite;
                return std::nullopt;
            },
            [&](const Vortex& e) -> Result {
                if (!std::isfinite(e.circulation) || !isFinite(e.position)) return notFinite;
                return std::nullopt;
            },
            [&](const Doublet& e) -> Result {
                if (!std::isfinite(e.strength) || !isFinite(e.position) || !std::isfinite(e.angle)) return notFinite;
                return std::nullopt;
            },
            [&](const SectorFlow& e) -> Result {
                if (!std::isfinite(e.strength) || !isFinite(e.vertex) || !std::isfinite(e.wall1) ||
                    !std::isfinite(e.wall2)) {
                    return notFinite;
                }
                if (e.opening() <= 0.0 || e.opening() > k_twoPi + k_angleTolerance) {
                    return "Wall angles must satisfy 0 < theta2 - theta1 <= 360 degrees.";
                }
                return std::nullopt;
            },
        },
        element);
}

} // namespace pfl
