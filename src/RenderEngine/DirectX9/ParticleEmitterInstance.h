#ifndef PARTICLEEMITTERINSTANCE_H
#define PARTICLEEMITTERINSTANCE_H

#include "RenderEngine/DirectX9/ParticleSystemInstance.h"
#include "RenderEngine/DirectX9/ParticleRenderers.h"

namespace Alamo {
namespace DirectX9 {

class ParticleEmitterInstance : public IObject, public Emitter, public LinkedListObject<ParticleEmitterInstance>
{
    struct ResourceBlock;

    struct Particle : public Alamo::Particle
    {
        bool                     used;      // Is this particle used or free?
        Particle*                next;      // Next free particle
        ParticleEmitterInstance* attached;  // Attached particle emitters
        ResourceBlock*           resources; // Resource block of this particle
    };

    struct ModifierInfo
    {
        size_t                m_dataSize;
        size_t                m_dataOffset;
        const ModifierPlugin* m_plugin;
    };

    struct RendererInfo
    {
        size_t            m_dataSize;
        ParticleRenderer* m_plugin;
    };

    const ParticleSystem::Emitter& m_emitter;

    RenderEngine&             m_engine;
    ParticleSystemInstance&   m_instance;
    ParticleEmitterInstance*  m_nextAttached;
    const Alamo::Particle*    m_parent;
    std::vector<ModifierInfo> m_modifiers;
    size_t                    m_totalModifierDataSize;
    char*                     m_creatorData;
    RendererInfo              m_renderer;
    ResourceBlock*            m_resources;
    Particle*                 m_freeParticles;
    bool                      m_detached;
    size_t                    m_numParticles;
    float                     m_nextSpawnTime;
    float                     m_spawnTime;
    Alamo::Particle*          m_lastParticle;
    std::set<size_t> m_debug;

    Particle* AllocateParticle();
    void      FreeParticle(Particle* p);
    void      SpawnParticle(float time, const Alamo::Particle* parent);
    void      UpdateParticle(Particle& p);
    void      CheckDestruction();
    void      Cleanup();

public:
    ParticleEmitterInstance* Detach();
    void Update();
    bool Render(RenderPhase phase) const;

    const Matrix&          GetPrevTransform() const { return m_instance.GetPrevTransform(); }
    const Matrix&          GetTransform()     const { return m_instance.GetTransform();     }
    const IRenderEngine&   GetRenderEngine()  const { return m_instance.GetRenderEngine();  }
    const Alamo::Particle* GetParent()        const { return m_parent; }

    ParticleEmitterInstance(LinkedList<ParticleEmitterInstance> &list, const ParticleSystem::Emitter& emitter, RenderEngine& engine, ParticleSystemInstance& instance, Alamo::Particle* parent, const Model::Mesh* mesh, float time);
    ~ParticleEmitterInstance();
};

}
}
#endif