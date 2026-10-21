// particles.hpp: CPU tracer particles advected through a velocity field.
//
// Demo-grade: midpoint RK2 with per-particle sub-stepping so fast particles near a vortex core
// stay stable. Particles respawn uniformly inside the domain when they leave it, come within
// respawnRadius of the origin, or exceed maxAge (stops everything collapsing onto a few paths).
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

struct Domain {
    Vec2 min{-1.0, -1.0};
    Vec2 max{1.0, 1.0};

    bool contains(Vec2 p) const { return p.x >= min.x && p.x <= max.x && p.y >= min.y && p.y <= max.y; }
};

struct ParticleSettings {
    Domain domain{{-2.0, -1.25}, {2.0, 1.25}};
    double respawnRadius = 0.06;
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

    // Advances every particle by dt. `velocity` is any callable Vec2 -> Vec2 (a template rather
    // than std::function so the per-particle call stays cheap, even in Debug builds).
    template <typename VelocityFn>
    void step(double dt, const VelocityFn& velocity);

    void setDomain(const Domain& domain) { m_settings.domain = domain; }
    const ParticleSettings& settings() const { return m_settings; }
    std::span<const Particle> particles() const { return m_particles; }

private:
    bool needsRespawn(const Particle& p) const;
    void respawn(Particle& p);

    ParticleSettings m_settings;
    std::vector<Particle> m_particles;
    std::mt19937 m_rng;
};

template <typename VelocityFn>
void ParticleSystem::step(double dt, const VelocityFn& velocity) {
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

        if (needsRespawn(p)) respawn(p);
    }
}

} // namespace pfl
