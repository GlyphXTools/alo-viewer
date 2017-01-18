#include "RenderEngine/Particles/TranslaterPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"

namespace Alamo
{

//
// WorldTranslaterPlugin
//
void WorldTranslaterPlugin::CheckParameter(int id)
{
}

void WorldTranslaterPlugin::TranslateParticle(Particle* p) const
{
    // No action required
}

void WorldTranslaterPlugin::InitializeParticle(Particle* p) const
{
}

WorldTranslaterPlugin::WorldTranslaterPlugin(ParticleSystem::Emitter& emitter)
    : TranslaterPlugin(emitter)
{
}

//
// EmitterTranslaterPlugin
//
void EmitterTranslaterPlugin::CheckParameter(int id)
{
}

void EmitterTranslaterPlugin::TranslateParticle(Particle* p) const
{
    p->position += p->emitter->GetTransform().getTranslation() - p->emitter->GetPrevTransform().getTranslation();
}

void EmitterTranslaterPlugin::InitializeParticle(Particle* p) const
{
}

EmitterTranslaterPlugin::EmitterTranslaterPlugin(ParticleSystem::Emitter& emitter)
    : TranslaterPlugin(emitter)
{
}

}