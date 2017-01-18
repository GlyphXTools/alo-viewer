#ifndef PARTICLES_PLUGINS_H
#define PARTICLES_PLUGINS_H

#include "Assets/ChunkFile.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "Assets/Models.h"

namespace Alamo
{

struct Particle;
class IRenderObject;

class PluginProperty
{
public:
    virtual void Read(ChunkReader& reader) = 0;
};

class Plugin
{
    ChunkReader* m_reader;

protected:
    virtual void CheckParameter(int id) = 0;

    int         ReadInteger();
    float       ReadFloat();
    Vector3     ReadVector3();
    Vector4     ReadVector4();
    Color       ReadColor();
    std::string ReadString();
    bool        ReadBool();
    void        ReadProperty(PluginProperty& prop);

    ParticleSystem::Emitter& m_emitter;
public:
    virtual void ReadParameters(ChunkReader& reader);

    const ParticleSystem::Emitter& GetEmitter() const { return m_emitter; }

    Plugin(ParticleSystem::Emitter& emitter);
    virtual ~Plugin() {}
};

class CreatorPlugin : public Plugin
{
public:
    virtual size_t        GetPrivateDataSize() const { return 0; }
    virtual float         GetSpawnDelay(void* data, float currentTime) const = 0;
    virtual float         GetInitialSpawnDelay(void* data) const = 0;
    virtual unsigned long GetNumParticlesPerSpawn(void* data) const = 0;
    virtual void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const = 0;
    virtual void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const = 0;
    virtual ParticleSystem::Emitter* Initialize() = 0;
    
    CreatorPlugin(ParticleSystem::Emitter& emitter);
};

class RendererPlugin : public Plugin
{
public:
    virtual size_t GetPrivateDataSize() const { return 0; }
    virtual void   InitializeParticle(Particle* p, void* data) const {}
    
    RendererPlugin(ParticleSystem::Emitter& emitter);
};

class TranslaterPlugin : public Plugin
{
public:
    virtual void TranslateParticle(Particle* p) const = 0;
    virtual void InitializeParticle(Particle* p) const = 0;
    
    TranslaterPlugin(ParticleSystem::Emitter& emitter);
};

class KillerPlugin : public Plugin
{
public:
    virtual bool KillParticle(const Particle& p, float time) const = 0;
    virtual void InitializeParticle(Particle* p) const = 0;
    virtual bool SpawnOnDeath() const = 0;

    KillerPlugin(ParticleSystem::Emitter& emitter);
};

class ModifierPlugin : public Plugin
{
public:
    virtual size_t GetPrivateDataSize() const { return 0; }
    virtual void   ModifyParticle(Particle* p, void* data, float time) const = 0;
    virtual void   InitializeParticle(Particle* p, void* data, float time) const = 0;

    ModifierPlugin(ParticleSystem::Emitter& emitter);
};

}

#endif