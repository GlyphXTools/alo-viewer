#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include "Assets/ChunkFile.h"
#include "General/3DTypes.h"
#include "General/GameTypes.h"
#include "General/ExactTypes.h"
//#include "RenderEngine/Particles/Plugin.h"

namespace Alamo
{

class CreatorPlugin;
class RendererPlugin;
class TranslaterPlugin;
class KillerPlugin;
class ModifierPlugin;
class Plugin;
class IRenderEngine;

class Emitter
{
public:
    virtual const Matrix&        GetPrevTransform() const = 0;
    virtual const Matrix&        GetTransform()     const = 0;
    virtual const IRenderEngine& GetRenderEngine()  const = 0;
};

struct Particle
{
    const Emitter*  emitter;
    
    Particle* prev;
    Particle* next;

    Vector3  position;
    Vector3  velocity;
    Vector3  acceleration;
    Vector4  texCoords;
    Color    color;
    float    size;
    float    rotation;
    float    spawnTime;
    float    stompTime;
};

class ParticleSystem : public IObject
{
public:
    class Emitter;

    typedef std::vector<std::pair<Emitter*, bool> > DependencyList;

    class Emitter
    {
    public:
        enum SpawnEvent {
            SPAWN_BIRTH,
            SPAWN_DEATH,
            NUM_SPAWN_EVENTS
        };

    private:
        struct OldEmitter;

        ParticleSystem& m_system;
        std::string     m_name;
        Emitter*        m_next;        // Next in spawn list
        Emitter*        m_spawns[NUM_SPAWN_EVENTS];
        
        // The plugins
        CreatorPlugin*               m_creator;
        RendererPlugin*              m_renderer;
        TranslaterPlugin*            m_translater;
        KillerPlugin*                m_killer;
        std::vector<ModifierPlugin*> m_modifiers;

        Plugin* ReadPlugin(ChunkReader& reader, int type);
        void    ConvertOldEmitter(const OldEmitter& e, Emitter* dependency, bool onDeath);
        void    Cleanup();

    public:
        Emitter*                RegisterSpawn(Emitter* emitter, SpawnEvent e);

        Emitter*                GetNext()       const { return m_next; }
        ParticleSystem&         GetSystem()     const { return m_system; }
        const std::string&      GetName()       const { return m_name; }
        const CreatorPlugin&    GetCreator()    const { return *m_creator; }
        const RendererPlugin&   GetRenderer()   const { return *m_renderer; }
        const TranslaterPlugin& GetTranslater() const { return *m_translater; }
        const KillerPlugin&     GetKiller()     const { return *m_killer; }
        const size_t            GetNumModifiers()          const { return m_modifiers.size(); }
        const ModifierPlugin&   GetModifier(size_t i)      const { return *m_modifiers[i]; }
        const Emitter*          GetSpawnList(SpawnEvent e) const { return m_spawns[e]; }
        
        void Initialize();

        Emitter(ChunkReader& reader, ParticleSystem& system, size_t index, DependencyList* dependencies = NULL);
        ~Emitter();
    };

    ParticleSystem(ptr<IFile> file, const std::string& name);
    ~ParticleSystem();

    Emitter* RegisterSpawn(Emitter* emitter);

    const std::string&      GetName()                const { return m_name; }
    const Emitter*          GetSpawnList()           const { return m_spawn; }
          Emitter&          GetEmitter(size_t index) const { return *m_emitters[index]; }
	size_t                  GetNumEmitters()         const { return m_emitters.size(); }
	bool                    GetLeaveParticles()      const { return m_leaveParticles;  }
    const LightFieldSource* GetLightFieldSource()    const { return m_hasLightFieldSource ? &m_lightFieldSource : NULL; }
	
private:
    void Cleanup();
    void ReadFormat_V1(ChunkReader& reader);
    void ReadFormat_V2(ChunkReader& reader);
    void ReadLightFieldSource(ChunkReader& reader);

	bool			      m_leaveParticles;
    bool                  m_hasLightFieldSource;
    LightFieldSource      m_lightFieldSource;
	std::vector<Emitter*> m_emitters;
    Emitter*              m_spawn;
    std::string           m_name;
};

}
#endif