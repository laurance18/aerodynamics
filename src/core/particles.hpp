// particles.hpp: CPU tracer particles advected through a velocity field.
//
// Midpoint RK2 with per-particle sub-stepping so fast particles near a singularity stay stable.
// Particles respawn uniformly inside the bounds when they leave them, reach a position the caller
// rejects (outside the flow domain, too close to a singular point), or exceed maxAge (stops
// everything collapsing onto a few paths).
#pragma once

#include "core/vec2.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace pfl {

struct ParticleSettings {
    Rect bounds{{-2.0, -1.25}, {2.0, 1.25}};
    double maxAge = 8.0;
    double maxStepLength = 0.01; // largest distance a particle may travel in one sub-step
    int maxSubsteps = 16;
};

struct Particle {
    Vec2 pos;
    double age = 0.0;
    double speed = 0.0;
};

class ParticleSystem {
public:
    explicit ParticleSystem(std::size_t count, ParticleSettings settings = {}, std::uint32_t seed = 12345);

    // Advances every particle by dt. `velocity` is any callable Vec2 -> Vec2 and `isValid` any
    // callable Vec2 -> bool (templates rather than std::function so the per-particle calls stay
    // cheap, even in Debug builds).
    template <typename VelocityFn, typename ValidFn>
    void step(double dt, const VelocityFn& velocity, const ValidFn& isValid);

    // Scatters every particle again, e.g. after the flow changed.
    template <typename ValidFn>
    void respawnAll(const ValidFn& isValid);

    ParticleSettings& settings() { return m_settings; }
    const ParticleSettings& settings() const { return m_settings; }
    std::span<const Particle> particles() const { return m_particles; }

private:
    template <typename ValidFn>
    void respawn(Particle& p, const ValidFn& isValid);

    ParticleSettings m_settings;
    std::vector<Particle> m_particles;
    std::mt19937 m_rng;
};

template <typename VelocityFn, typename ValidFn>
void ParticleSystem::step(double dt, const VelocityFn& velocity, const ValidFn& isValid) {
    for (Particle& p : m_particles) {
        const Vec2 v0 = velocity(p.pos);
        const double travel = length(v0) * dt;
        const int substeps =
            std::clamp(static_cast<int>(std::ceil(travel / m_settings.maxStepLength)), 1, m_settings.maxSubsteps);
        const double h = dt / substeps;

        // Midpoint RK2.
        Vec2 k2 = v0;
        for (int i = 0; i < substeps; ++i) {
            const Vec2 k1 = (i == 0) ? v0 : velocity(p.pos);
            k2 = velocity(p.pos + k1 * (0.5 * h));
            p.pos += k2 * h;
        }
        p.speed = length(k2);
        p.age += dt;

        const bool finite = std::isfinite(p.pos.x) && std::isfinite(p.pos.y);
        if (!finite || !m_settings.bounds.contains(p.pos) || p.age > m_settings.maxAge || !isValid(p.pos)) {
            respawn(p, isValid);
        }
    }
}

template <typename ValidFn>
void ParticleSystem::respawnAll(const ValidFn& isValid) {
    std::uniform_real_distribution<double> age(0.0, m_settings.maxAge);
    for (Particle& p : m_particles) {
        respawn(p, isValid);
        p.age = age(m_rng); // stagger respawns so they don't all happen on the same frame
    }
}

template <typename ValidFn>
void ParticleSystem::respawn(Particle& p, const ValidFn& isValid) {
    constexpr int maxTries = 16; // a few rejections are normal; give up rather than stall
    const Rect& b = m_settings.bounds;
    std::uniform_real_distribution<double> ux(b.min.x, b.max.x);
    std::uniform_real_distribution<double> uy(b.min.y, b.max.y);
    for (int i = 0; i < maxTries; ++i) {
        p.pos = {ux(m_rng), uy(m_rng)};
        if (isValid(p.pos)) break;
    }
    p.age = 0.0;
    p.speed = 0.0;
}

} // namespace pfl
