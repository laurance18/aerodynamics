#include "core/particles.hpp"

namespace pfl {

ParticleSystem::ParticleSystem(std::size_t count, ParticleSettings settings, std::uint32_t seed)
    : m_settings(settings), m_particles(count), m_rng(seed) {
    respawnAll([](Vec2) { return true; });
}

} // namespace pfl
