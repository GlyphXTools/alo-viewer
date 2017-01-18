#include "RenderEngine/Particles/KillerPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Math.h"
#include "General/Log.h"
using namespace std;

namespace Alamo
{

//
// AgeKillerPlugin
//
void AgeKillerPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(0, m_stompTime);
    PARAM_FLOAT(1, m_stompTimeVariation);
    PARAM_BOOL (2, m_killOnRemoval);
    PARAM_BOOL (3, m_spawnOnAge);
    PARAM_BOOL (4, m_spawnOnConditional);
    PARAM_BOOL (5, m_updateTime);
    PARAM_BOOL (6, m_fullRangeVariation);
    END_PARAM_LIST
}

void AgeKillerPlugin::InitializeParticle(Particle* p) const
{
    p->stompTime = p->spawnTime + m_stompTime;
    if (m_stompTimeVariation > 0.0f)
    {
        p->stompTime += GetRandom(0.0f, m_stompTimeVariation) * m_stompTime;
    }
}

bool AgeKillerPlugin::KillParticle(const Particle& p, float time) const
{
    return time >= p.stompTime;
}

bool AgeKillerPlugin::SpawnOnDeath() const
{
    return m_spawnOnAge;
}

void AgeKillerPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_stompTimeVariation /= 100;
}

AgeKillerPlugin::AgeKillerPlugin(ParticleSystem::Emitter& emitter)
    : KillerPlugin(emitter)
{
}

AgeKillerPlugin::AgeKillerPlugin(ParticleSystem::Emitter& emitter, float stompTime, float stompTimeVariation)
    : KillerPlugin(emitter)
{
    m_spawnOnAge         = true;
    m_stompTime          = stompTime;
    m_stompTimeVariation = stompTimeVariation;
}

//
// RadiusKillerPlugin
//
void RadiusKillerPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(0, m_stompTime);
    PARAM_FLOAT(1, m_stompTimeVariation);
    PARAM_BOOL (2, m_killOnRemoval);
    PARAM_BOOL (3, m_spawnOnAge);
    PARAM_BOOL (4, m_spawnOnConditional);
    PARAM_BOOL (5, m_updateTime);
    PARAM_BOOL (6, m_fullRangeVariation);
    PARAM_FLOAT(100, m_radius);
    END_PARAM_LIST
}

void RadiusKillerPlugin::InitializeParticle(Particle* p) const
{
    p->stompTime = p->spawnTime + m_stompTime;
    if (m_stompTimeVariation > 0.0f)
    {
        p->stompTime += GetRandom(0.0f, m_stompTimeVariation) * m_stompTime;
    }
}

bool RadiusKillerPlugin::KillParticle(const Particle& p, float time) const
{
    const Vector3 distance = p.position - p.emitter->GetTransform().getTranslation();
    return (time >= p.stompTime) || (distance.length() >= m_radius);
}

bool RadiusKillerPlugin::SpawnOnDeath() const
{
    return m_spawnOnAge;
}

void RadiusKillerPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_stompTimeVariation /= 100;
}

RadiusKillerPlugin::RadiusKillerPlugin(ParticleSystem::Emitter& emitter)
    : KillerPlugin(emitter)
{
}

//
// TargetKillerPlugin
//
void TargetKillerPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(0, m_stompTime);
    PARAM_FLOAT(1, m_stompTimeVariation);
    PARAM_BOOL (2, m_killOnRemoval);
    PARAM_BOOL (3, m_spawnOnAge);
    PARAM_BOOL (4, m_spawnOnConditional);
    PARAM_BOOL (5, m_updateTime);
    PARAM_BOOL (6, m_fullRangeVariation);
    PARAM_BOOL  (100, m_target);
    PARAM_STRING(101, m_bone);
    PARAM_FLOAT (102, m_radius);
    END_PARAM_LIST
}

void TargetKillerPlugin::InitializeParticle(Particle* p) const
{
    p->stompTime = p->spawnTime + m_stompTime;
    if (m_stompTimeVariation > 0.0f)
    {
        p->stompTime += GetRandom(0.0f, m_stompTimeVariation) * m_stompTime;
    }
}

bool TargetKillerPlugin::KillParticle(const Particle& p, float time) const
{
    // TODO: different targets and different emitter position
    return (time >= p.stompTime) || (p.position.length() < m_radius);
}

bool TargetKillerPlugin::SpawnOnDeath() const
{
    return m_spawnOnAge;
}

void TargetKillerPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_stompTimeVariation /= 100;
    const string& name = GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported killer plugin", name.c_str());
}

TargetKillerPlugin::TargetKillerPlugin(ParticleSystem::Emitter& emitter)
    : KillerPlugin(emitter)
{
    const string& name = GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported killer plugin", name.c_str());
}

//
// HardwareStomperKillerPlugin
//
HardwareStomperKillerPlugin::HardwareStomperKillerPlugin(ParticleSystem::Emitter& emitter)
    : AgeKillerPlugin(emitter)
{
    // It's just the Age Killer in terms of functionality
}

//
// TerrainKillerPlugin
//
bool TerrainKillerPlugin::KillParticle(const Particle& p, float time) const
{
    return p.position.z < 0 || AgeKillerPlugin::KillParticle(p, time);
}

TerrainKillerPlugin::TerrainKillerPlugin(ParticleSystem::Emitter& emitter)
    : AgeKillerPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
}

TerrainKillerPlugin::TerrainKillerPlugin(ParticleSystem::Emitter& emitter, float stompTime, float stompTimeVariation)
    : AgeKillerPlugin(emitter, stompTime, stompTimeVariation)
{
    const string& name = emitter.GetSystem().GetName();
}

}