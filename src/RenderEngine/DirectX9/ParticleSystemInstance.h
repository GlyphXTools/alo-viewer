#ifndef PARTICLESYSTEMINSTANCE_H
#define PARTICLESYSTEMINSTANCE_H

#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/DirectX9/RenderEngine.h"
#include "General/3DTypes.h"

namespace Alamo {
namespace DirectX9 {

class ParticleEmitterInstance;

class ParticleSystemInstance : public ProxyInstance
{
    LinkedList<ParticleEmitterInstance> m_emitters;

    RenderEngine&       m_engine;
    RenderObject&       m_object;
    size_t              m_bone;
    const Model::Mesh*  m_mesh;
    Matrix              m_transform;
    Matrix              m_prevTransform;
    ptr<ParticleSystem> m_system;

public:
    void SpawnEmitter(const ParticleSystem::Emitter& emitter, Particle* parent, float time);

    void Update();
    bool Render(RenderPhase phase) const;
    void Detach();

    RenderEngine& GetRenderEngine()  const { return m_engine;        }
    RenderObject& GetRenderObject()  const { return m_object;        }
    const Matrix& GetPrevTransform() const { return m_prevTransform; }
    const Matrix& GetTransform()     const { return m_transform;     }

    ParticleSystemInstance(ptr<ParticleSystem> system, RenderObject& object, size_t index, float time);
    ~ParticleSystemInstance();
};

}
}
#endif