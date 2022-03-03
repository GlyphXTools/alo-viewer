#include "RenderEngine/DirectX9/ParticleEmitterInstance.h"
#include "RenderEngine/DirectX9/RenderObject.h"
#include "General/GameTime.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

struct ParticleEmitterInstance::ResourceBlock
{
    size_t           m_base;
    ResourceBlock*   m_next;
    Buffer<Particle> m_particles;
    Buffer<char>     m_modifierData;
    Buffer<char>     m_rendererData;

    // Creates a block of resources for @count particles
    ResourceBlock(size_t count, ResourceBlock* next, size_t totalModifierDataSize, size_t rendererDataSize)
        : m_next(next),
          m_particles(count),
          m_modifierData(count * totalModifierDataSize),
          m_rendererData(count * rendererDataSize)
    {
        m_base = (m_next != NULL) ? m_next->m_base + m_next->m_particles.size() : 0;

        // Initialize new particles
        for (size_t i = 0; i < m_particles.size(); i++)
        {
            m_particles[i].resources = this;
        }
    }
};

void ParticleEmitterInstance::SpawnParticle(float time, const Alamo::Particle* parent)
{
    Particle& p = *AllocateParticle();
    p.emitter  = this;
    p.attached = NULL;

    size_t index = &p - &p.resources->m_particles[0];
    m_debug.insert(p.resources->m_base + index);

    m_emitter.GetCreator().InitializeParticle(&p, m_creatorData, parent, time);
    m_emitter.GetKiller().InitializeParticle(&p);
    m_emitter.GetRenderer().InitializeParticle(&p, p.resources->m_rendererData.empty() ? NULL : &p.resources->m_rendererData[m_renderer.m_dataSize * index]);
    for (size_t i = 0; i < m_modifiers.size(); i++)
    {
        ModifierInfo& modifier = m_modifiers[i];
        modifier.m_plugin->InitializeParticle(&p, &p.resources->m_modifierData[m_totalModifierDataSize * index + modifier.m_dataOffset], time);
    }
    m_renderer.m_plugin->UpdatePrimitive(p.resources->m_base + index, p, p.resources->m_rendererData.empty() ? NULL : &p.resources->m_rendererData[m_renderer.m_dataSize * index]);
    m_numParticles++;

    // Spawn emitters registered for particle birth
    for (const ParticleSystem::Emitter* emitter = m_emitter.GetSpawnList(ParticleSystem::Emitter::SPAWN_BIRTH); emitter != NULL; emitter = emitter->GetNext())
    {
        m_instance.SpawnEmitter(*emitter, &p, time);
    }
}

void ParticleEmitterInstance::UpdateParticle(Particle& p)
{
    float time = GetGameTime();
    if (m_emitter.GetKiller().KillParticle(p, time))
    {
        // Detach attached emitters
        for (ParticleEmitterInstance* cur = p.attached; cur != NULL; )
        {
            cur = cur->Detach();
        }

        // Spawn dependent particles
        if (m_emitter.GetKiller().SpawnOnDeath())
        {
            for (const ParticleSystem::Emitter* e = m_emitter.GetSpawnList(ParticleSystem::Emitter::SPAWN_DEATH); e != NULL; e = e->GetNext())
            {
                m_instance.SpawnEmitter(*e, &p, time);
            }
        }

        // Kill the particle
        m_debug.erase(p.resources->m_base + (&p - &p.resources->m_particles[0]));
        FreeParticle(&p);
        m_numParticles--;
    }
    else
    {
        // Update the particle
        size_t index = &p - &p.resources->m_particles[0];
        assert(m_debug.find(p.resources->m_base + index) != m_debug.end());
        for (size_t i = 0; i < m_emitter.GetNumModifiers(); i++)
        {
            ModifierInfo& modifier = m_modifiers[i];
            modifier.m_plugin->ModifyParticle(&p, &p.resources->m_modifierData[m_totalModifierDataSize * index + modifier.m_dataOffset], time);
        }

        float diff = GetGameTime() - GetPreviousGameTime();
        p.position += p.velocity     * diff;
        p.velocity += p.acceleration * diff;

        m_emitter.GetTranslater().TranslateParticle(&p);
        m_renderer.m_plugin->UpdatePrimitive(p.resources->m_base + index, p, p.resources->m_rendererData.empty() ? NULL : &p.resources->m_rendererData[m_renderer.m_dataSize * index]);
    }
}

void ParticleEmitterInstance::Update()
{
    // Update existing particles
    for (ResourceBlock* res = m_resources; res != NULL; res = res->m_next)
    {
        for (size_t i = 0; i < res->m_particles.size(); i++)
        {
            if (res->m_particles[i].used)
            {
                UpdateParticle(res->m_particles[i]);
            }
        }
    }

    // Spawn new particles, if any
    float time = GetGameTime();
    while (m_nextSpawnTime != -1 && time >= m_nextSpawnTime)
    {
        // Spawn another batch of particles
        for (size_t i = 0; i < m_emitter.GetCreator().GetNumParticlesPerSpawn(m_creatorData); i++)
        {
            SpawnParticle(m_nextSpawnTime, m_parent);
        }
        float t = m_emitter.GetCreator().GetSpawnDelay(m_creatorData, m_nextSpawnTime - m_spawnTime);
        float lastSpawnTime = m_nextSpawnTime;
        m_nextSpawnTime = (t != -1) ? m_nextSpawnTime + t : -1;
        if (m_nextSpawnTime == lastSpawnTime)
        {
            break;
        }
    }
    CheckDestruction();
}

bool ParticleEmitterInstance::Render(RenderPhase phase) const
{
    if (m_resources != NULL && phase == m_renderer.m_plugin->GetRenderPhase())
    {
        m_renderer.m_plugin->RenderParticles();
        return true;
    }
    return false;
}

// Check if we can be destroyed. Call when either of the conditions change
void ParticleEmitterInstance::CheckDestruction()
{
    if (m_detached && m_nextSpawnTime == -1 && m_numParticles == 0)
    {
        // We're done spawning and all particles are dead as well, suicide!
        delete this;
    }
}

ParticleEmitterInstance* ParticleEmitterInstance::Detach()
{
    ParticleEmitterInstance* next = m_nextAttached;
    m_nextAttached = NULL;

    // Detach and stop spawning
    m_detached      = true;
    m_nextSpawnTime = -1;

    CheckDestruction();
    return next;
}

ParticleEmitterInstance::Particle* ParticleEmitterInstance::AllocateParticle()
{
    if (m_freeParticles == NULL)
    {
        // There are no more free particles, allocate more
        size_t count = (m_resources != NULL) ? m_resources->m_particles.size() * 2 : 16;
        
        m_resources = new ResourceBlock(count, m_resources, m_totalModifierDataSize, m_renderer.m_dataSize);

        m_renderer.m_plugin->AllocatePrimitives(count);

        // Put new particles on the free list
        for (size_t i = 0; i < m_resources->m_particles.size(); i++)
        {
            // Reset particle
            m_resources->m_particles[i].next = m_freeParticles;
            m_resources->m_particles[i].used = false;
            m_freeParticles = &m_resources->m_particles[i];
        }
    }
    // Pop particle off of free queue
    Particle* p = m_freeParticles;
    m_freeParticles = m_freeParticles->next;
    assert(p != NULL);
    p->used = true;

    // Link it in the list
    p->prev                  = NULL;
    p->Alamo::Particle::next = m_lastParticle;
    if (m_lastParticle != NULL)
    {
        m_lastParticle->prev = p;
    }
    m_lastParticle = p;

    // Notify the renderer that we've allocated a particle
    m_renderer.m_plugin->AllocatePrimitive(p->resources->m_base + p - &p->resources->m_particles[0]);

    return p;
}

void ParticleEmitterInstance::FreeParticle(Particle* p)
{
    // Unlink it
    if (p->prev != NULL) p->prev->next = p->next;
    if (p->next != NULL) p->next->prev = p->prev;

    // Push particle onto free queue
    p->next = m_freeParticles;
    p->used = false;
    m_renderer.m_plugin->FreePrimitive(p->resources->m_base + p - &p->resources->m_particles[0]);
    m_freeParticles = p;
}

template <typename T>
static ParticleRenderer* CastRendererPlugin(RenderEngine& engine, const RendererPlugin& plugin)
{
    const T::PluginType* p = dynamic_cast<const T::PluginType*>(&plugin);
    return (p != NULL) ? new T(engine, *p) : NULL;
}

ParticleEmitterInstance::ParticleEmitterInstance(LinkedList<ParticleEmitterInstance> &list, const ParticleSystem::Emitter& emitter, RenderEngine& engine, ParticleSystemInstance& instance, Alamo::Particle* parent, const Model::Mesh* mesh, float time)
    : m_emitter(emitter), m_instance(instance), m_freeParticles(NULL), m_engine(engine), m_parent(parent), 
      m_resources(NULL), m_nextAttached(NULL), m_detached(false), m_numParticles(0), m_creatorData(NULL),
      m_lastParticle(NULL), m_spawnTime(time)
{
    Link(list);

    if (parent != NULL)
    {
        ParticleEmitterInstance::Particle* p = static_cast<ParticleEmitterInstance::Particle*>(parent);
        m_nextAttached = p->attached;
        p->attached = this;
    }

    try
    {
        // Create the renderer
        const RendererPlugin& plugin = m_emitter.GetRenderer();
        if ((m_renderer.m_plugin = CastRendererPlugin<BillboardRenderer>            (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<XYAlignedRenderer>            (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<VelocityAlignedRenderer>      (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<HeatSaturationRenderer>       (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<KitesRenderer>                (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<VolumetricRenderer>           (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<ChainRenderer>                (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<XYAlignedChainRenderer>       (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<StretchedTextureChainRenderer>(m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<HardwareBillboardsRenderer>   (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<BumpMapRenderer>              (m_engine, plugin)) == NULL)
        if ((m_renderer.m_plugin = CastRendererPlugin<LineRenderer>                 (m_engine, plugin)) == NULL)
        {
            throw runtime_error("Unable to create emitter instance");
        }
        m_renderer.m_dataSize = plugin.GetPrivateDataSize();

        // Get the data size requirements of the modifier plugins
        m_totalModifierDataSize = 0;
        m_modifiers.resize(m_emitter.GetNumModifiers());
        for (size_t i = 0; i < m_modifiers.size(); i++)
        {
            m_modifiers[i].m_plugin     = &m_emitter.GetModifier(i);
            m_modifiers[i].m_dataSize   = m_modifiers[i].m_plugin->GetPrivateDataSize();
            m_modifiers[i].m_dataOffset = m_totalModifierDataSize;
            m_totalModifierDataSize    += m_modifiers[i].m_dataSize;
        }

        // Set the creator data
        size_t size = m_emitter.GetCreator().GetPrivateDataSize();
        if (size > 0)
        {
            m_creatorData = new char[size];
            m_emitter.GetCreator().InitializeInstance(m_creatorData, &m_instance.GetRenderObject(), mesh);
        }

        // Do the initial spawn, if necessary
        float t = m_emitter.GetCreator().GetInitialSpawnDelay(m_creatorData);
        m_nextSpawnTime = (t != -1) ? m_spawnTime + t : -1;

        while (m_nextSpawnTime != -1 && time >= m_nextSpawnTime)
        {
            // Spawn another batch of particles
            for (size_t i = 0; i < m_emitter.GetCreator().GetNumParticlesPerSpawn(m_creatorData); i++)
            {
                SpawnParticle(m_nextSpawnTime, m_parent);
            }
            float t = m_emitter.GetCreator().GetSpawnDelay(m_creatorData, m_nextSpawnTime - m_spawnTime);
            float lastSpawnTime = m_nextSpawnTime;
            m_nextSpawnTime = (t != -1) ? m_nextSpawnTime + t : -1;
            if (m_nextSpawnTime == lastSpawnTime)
            {
                break;
            }
        }
    }
    catch (...)
    {
        Cleanup();
        throw;
    }
}

void ParticleEmitterInstance::Cleanup()
{
    for (ResourceBlock *next, *res = m_resources; res != NULL; res = next)
    {
        next = res->m_next;
        delete res;
    }
    delete m_renderer.m_plugin;
    delete[] m_creatorData;
}

ParticleEmitterInstance::~ParticleEmitterInstance()
{
    Cleanup();
}

}
}