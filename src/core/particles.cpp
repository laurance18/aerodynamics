#include "core/particles.hpp"

#include <algorithm>
#include <cmath>

namespace pfl {

ParticleSystem::ParticleSystem(std::size_t count, ParticleSettings settings, std::uint32_t seed)
    : m_settings(settings), m_particles(count), m_rng(seed) {
    std::uniform_real_distribution<double> age(0.0, m_settings.maxAge);
    for (Particle& p : m_particles) {
        respawn(p);
        p.age = age(m_rng); // stagger respawns so they don't all happen on the same frame
    }
}

bool ParticleSystem::needsRespawn(const Particle& p) const {
    const double r = m_settings.respawnRadius;
    return !m_settings.domain.contains(p.pos) || lengthSquared(p.pos) < r * r || p.age > m_settings.maxAge ||
           !std::isfinite(p.pos.x) || !std::isfinite(p.pos.y);
}

void ParticleSystem::respawn(Particle& p) {
    const Domain& d = m_settings.domain;
    std::uniform_real_distribution<double> ux(d.min.x, d.max.x);
    std::uniform_real_distribution<double> uy(d.min.y, d.max.y);
    const double r2 = m_settings.respawnRadius * m_settings.respawnRadius;
    do {
        p.pos = {ux(m_rng), uy(m_rng)};
    } while (lengthSquared(p.pos) < r2);
    p.age = 0.0;
    p.speed = 0.0;
}

} // namespace pfl
