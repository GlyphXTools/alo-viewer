#ifndef PARTICLES_CREATOR_PLUGINS_H
#define PARTICLES_CREATOR_PLUGINS_H

#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/Plugin.h"
#include "General/GameTypes.h"

namespace Alamo
{

struct PropertyGroup : public PluginProperty
{
    enum Type
    {
        POINT, DIRECTION, SPHERE, RANGE, SPHERICAL_RANGE, CYLINDER, TORUS
    };

    void    Read(ChunkReader& reader);
    Vector3 Sample(bool hollow) const;

    static Vector3 SampleTorus(float torusRadius, float tubeRadius, bool hollow);

    Type    m_type;
    Vector3 m_point;        // Point
    Vector3 m_position;     // Direction
    Range   m_magnitude;    // Direction
    Range   m_radius;       // Sphere
    Vector3 m_minPosition;  // Range
    Vector3 m_maxPosition;  // Range
    Range   m_sphereAngle;  // Spherical Range
    Range   m_sphereRadius; // Spherical Range
    float   m_cylRadius;    // Cylinder
    Range   m_cylHeight;    // Cylinder
    float   m_torusRadius;  // Torus
    float   m_tubeRadius;   // Torus
};

// These parameters are used in all creators
struct CommonCreatorParameters
{
    unsigned int m_globalLOD;
    float        m_preSimulateSeconds;
    bool         m_autoUpdatePositions;
    float        m_minDLOD;
    float        m_DLODNearDistance;
    float        m_DLODFarDistance;
};

// These parameters are used in obsolete creators
struct CommonObsoleteCreatorParameters : public CommonCreatorParameters
{
    float m_pps;
    float m_speed;
    float m_speedVariation;
    float m_burstThreshold;
};

// These parameters are used in all non-obsolete creators
struct ShapeCreator : public CommonCreatorParameters
{
    float         m_particlePerInterval;
    float         m_spawnInterval;
    bool          m_bursting;
    float         m_startDelay;
    float         m_stopTime;
    PropertyGroup m_position;
    bool          m_hollowPosition;
    PropertyGroup m_velocity;
    bool          m_hollowVelocity;
    bool          m_localVelocity;
    bool          m_inheritVelocity;
    float         m_inheritedVelocityScale;
    float         m_maxInheritedVelocity;
    bool          m_hiresEmission;
    bool          m_burstOnShown;
};

class PointCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters
{
    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    PointCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class TrailCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters
{
    unsigned long m_parent;

    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    TrailCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class SphereCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters
{
    float         m_radius;

    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    SphereCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class BoxCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters
{
    float m_length, m_width, m_height;

    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    BoxCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class TorusCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters
{
    float m_torusRadius, m_tubeRadius;

    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    TorusCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class MeshCreatorBase
{
public:
    enum {
        MESH_RANDOM_VERTEX = 0,
        MESH_RANDOM_SURFACE,
        MESH_ALL_VERTICES
    };

protected:
    struct MeshCreatorData;

    unsigned long m_spawnLocation;
    bool          m_alignVelocityToNormal;
    float         m_surfaceOffset;

    void InitializeParticle(Particle* p, MeshCreatorData* data) const;
    MeshCreatorBase();
};

class MeshCreatorPlugin : public CreatorPlugin, public CommonObsoleteCreatorParameters, public MeshCreatorBase
{
    void          ReadParameters(ChunkReader& reader);
    void          CheckParameter(int id);
    size_t        GetPrivateDataSize() const;
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    MeshCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class ShapeCreatorPlugin : public CreatorPlugin, public ShapeCreator
{
    void          CheckParameter(int id);
protected:
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    ShapeCreatorPlugin(ParticleSystem::Emitter& emitter);
    ShapeCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params);
};

class OutwardVelocityShapeCreatorPlugin : public ShapeCreatorPlugin
{
    float m_outwardSpeed;

    void CheckParameter(int id);
    void InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
public:
    OutwardVelocityShapeCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class DeathCreatorPlugin : public ShapeCreatorPlugin
{
    std::string              m_parentName;
    ParticleSystem::Emitter* m_parentEmitter;
    int                      m_positionAlignment;
    int                      m_velocityAlignment;

    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    ParticleSystem::Emitter* Initialize();

public:
    DeathCreatorPlugin(ParticleSystem::Emitter& emitter);
    DeathCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, ParticleSystem::Emitter* parentEmitter);
};

class EnhancedTrailCreatorPlugin : public ShapeCreatorPlugin
{
    std::string              m_parentName;
    ParticleSystem::Emitter* m_parentEmitter;
    int                      m_positionAlignment;
    int                      m_velocityAlignment;

    void          CheckParameter(int id);
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    ParticleSystem::Emitter* Initialize();

public:
    EnhancedTrailCreatorPlugin(ParticleSystem::Emitter& emitter);
    EnhancedTrailCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, ParticleSystem::Emitter* parentEmitter);
};

class EnhancedMeshCreatorPlugin : public ShapeCreatorPlugin, public MeshCreatorBase
{
    size_t        GetPrivateDataSize() const;
    void          CheckParameter(int id);
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;

public:
    EnhancedMeshCreatorPlugin(ParticleSystem::Emitter& emitter);
    EnhancedMeshCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, 
        unsigned long spawnLocation,
        float         surfaceOffset);
};

class HardwareSpawnerCreatorPlugin : public CreatorPlugin, public CommonCreatorParameters
{
    float         m_particleCount;
    float         m_particleLifetime;
    bool          m_enableBursting;
    PropertyGroup m_position;
    bool          m_hollowPosition;
    PropertyGroup m_velocity;
    bool          m_hollowVelocity;
    Vector3       m_acceleration;
    float         m_attractAcceleration;
    Vector3       m_vortexAxis;
    float         m_vortexRotation;
    float         m_vortexAttraction;

    void          CheckParameter(int id);
    float         GetSpawnDelay(void* data, float currentTime) const;
    float         GetInitialSpawnDelay(void* data) const;
    unsigned long GetNumParticlesPerSpawn(void* data) const;
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;
    void          InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const;
    ParticleSystem::Emitter* Initialize();

public:
    HardwareSpawnerCreatorPlugin(ParticleSystem::Emitter& emitter);
};

class AlignedShapeCreatorPlugin : public ShapeCreatorPlugin
{
    unsigned int  m_positionAlignmentDirection;
    unsigned int  m_positionAlignment;
    unsigned int  m_velocityAlignmentDirection;
    unsigned int  m_velocityAlignment;
    std::string   m_bone;

    void          CheckParameter(int id);
    void          InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const;

public:
    AlignedShapeCreatorPlugin(ParticleSystem::Emitter& emitter);
};

}
#endif