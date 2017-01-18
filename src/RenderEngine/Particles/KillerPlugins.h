#ifndef PARTICLES_KILLER_PLUGINS_H
#define PARTICLES_KILLER_PLUGINS_H

#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/Plugin.h"

namespace Alamo
{

struct CommonKillerParameters
{
    float m_stompTime;
    float m_stompTimeVariation;
    bool  m_killOnRemoval;
    bool  m_spawnOnAge;
    bool  m_spawnOnConditional;
    bool  m_updateTime;
    bool  m_fullRangeVariation;
};

class AgeKillerPlugin : public KillerPlugin, public CommonKillerParameters
{
protected:
    void ReadParameters(ChunkReader& reader);
    void CheckParameter(int id);
    void InitializeParticle(Particle* p) const;
    bool KillParticle(const Particle& p, float time) const;
    bool SpawnOnDeath() const;

public:
    AgeKillerPlugin(ParticleSystem::Emitter& emitter);
    AgeKillerPlugin(ParticleSystem::Emitter& emitter, float stompTime, float stompTimeVariation);
};

class RadiusKillerPlugin : public KillerPlugin, public CommonKillerParameters
{
    float m_radius;

    void ReadParameters(ChunkReader& reader);
    void CheckParameter(int id);
    void InitializeParticle(Particle* p) const;
    bool KillParticle(const Particle& p, float time) const;
    bool SpawnOnDeath() const;

public:
    RadiusKillerPlugin(ParticleSystem::Emitter& emitter);
};

class TargetKillerPlugin : public KillerPlugin, public CommonKillerParameters
{
    int         m_target;
    std::string m_bone;
    float       m_radius;

    void ReadParameters(ChunkReader& reader);
    void CheckParameter(int id);
    void InitializeParticle(Particle* p) const;
    bool KillParticle(const Particle& p, float time) const;
    bool SpawnOnDeath() const;

public:
    TargetKillerPlugin(ParticleSystem::Emitter& emitter);
};

class HardwareStomperKillerPlugin : public AgeKillerPlugin
{
public:
    HardwareStomperKillerPlugin(ParticleSystem::Emitter& emitter);
};

class TerrainKillerPlugin : public AgeKillerPlugin
{
    bool KillParticle(const Particle& p, float time) const;
public:
    TerrainKillerPlugin(ParticleSystem::Emitter& emitter);
    TerrainKillerPlugin(ParticleSystem::Emitter& emitter, float stompTime, float stompTimeVariation);
};

}

#endif