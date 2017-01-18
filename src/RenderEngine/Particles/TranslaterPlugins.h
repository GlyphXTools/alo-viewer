#ifndef PARTICLES_TRANSLATER_PLUGINS_H
#define PARTICLES_TRANSLATER_PLUGINS_H

#include "RenderEngine/Particles/Plugin.h"

namespace Alamo
{

class WorldTranslaterPlugin : public TranslaterPlugin
{
    void CheckParameter(int id);
    void TranslateParticle(Particle* p) const;
    void InitializeParticle(Particle* p) const;
public:
    WorldTranslaterPlugin(ParticleSystem::Emitter& emitter);
};

class EmitterTranslaterPlugin : public TranslaterPlugin
{
    void CheckParameter(int id);
    void TranslateParticle(Particle* p) const;
    void InitializeParticle(Particle* p) const;
public:
    EmitterTranslaterPlugin(ParticleSystem::Emitter& emitter);
};

}

#endif