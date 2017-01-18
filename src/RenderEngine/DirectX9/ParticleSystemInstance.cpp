#include "RenderEngine/DirectX9/ParticleEmitterInstance.h"
#include "RenderEngine/DirectX9/RenderObject.h"

namespace Alamo {
namespace DirectX9 {

void ParticleSystemInstance::Update()
{
    if (m_emitters != NULL)
    {
        // There are emitters to update
        m_prevTransform = m_transform;
        m_transform     = m_object.GetBoneTransform(m_bone);
        for (ParticleEmitterInstance *next, *cur = m_emitters; cur != NULL; cur = next)
        {
            next = cur->GetNext();
            cur->Update();
        }
        
        if (m_emitters == NULL)
        {
            // There are no more emitters left, release reference
            Release();
        }
    }
}

bool ParticleSystemInstance::Render(RenderPhase phase) const
{
    bool rendered = false;
    for (ParticleEmitterInstance *cur = m_emitters; cur != NULL; cur = cur->GetNext())
    {
        rendered |= cur->Render(phase);
    }
    return rendered;
}

void ParticleSystemInstance::Detach()
{
    if (m_system->GetLeaveParticles())
    {
        // Detach emitters
        for (ParticleEmitterInstance *next, *cur = m_emitters; cur != NULL; cur = next)
        {
            next = cur->GetNext();
            if (cur->GetParent() == NULL)
            {
                cur->Detach();
            }
        }
    }
    else if (m_emitters != NULL)
    {
        // The release after this Detach will delete the instance
        Release();
    }
}

void ParticleSystemInstance::SpawnEmitter(const ParticleSystem::Emitter& emitter, Particle* parent, float time)
{
    new ParticleEmitterInstance(m_emitters, emitter, m_engine, *this, parent, m_mesh, time);
}

ParticleSystemInstance::ParticleSystemInstance(ptr<ParticleSystem> system, RenderObject& object, size_t index, float time)
    : m_engine(dynamic_cast<const ObjectTemplate*>(object.GetTemplate())->GetEngine()),
      m_system(system), m_object(object)
{
    Link(object.m_instances);

    const Model::Proxy& proxy = object.GetModel().GetProxy(index);
    m_mesh          = proxy.mesh;
    m_bone          = proxy.bone->index;
    m_transform     = object.GetBoneTransform(m_bone);
    m_prevTransform = m_transform;

    printf("New instance of \"%s\"\n", m_system->GetName().c_str());
    for (const ParticleSystem::Emitter* emitter = system->GetSpawnList(); emitter != NULL; emitter = emitter->GetNext())
    {
        SpawnEmitter(*emitter, NULL, time);
    }
    
    if (m_emitters != NULL)
    {
        // There are emitters to process, so register us with the engine
        m_engine.RegisterParticleSystemInstance(this);

        // Also keep a reference alive, so even if we're detached, we're
        // not deleted until all emitters are gone.
        AddRef();
    }
}

ParticleSystemInstance::~ParticleSystemInstance()
{
    m_engine.UnregisterParticleSystemInstance(this);
    while (m_emitters != NULL)
    {
        delete m_emitters;
    }
    printf("Deleted instance\n");
}

}
}