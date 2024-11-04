#include "RenderEngine/Particles/CreatorPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "RenderEngine/RenderEngine.h"
#include "General/Exceptions.h"
#include "General/Math.h"
#include "General/Log.h"
using namespace std;

namespace Alamo
{

static const int ALIGNMENT_NONE    = 0;
static const int ALIGNMENT_FORWARD = 1;
static const int ALIGNMENT_REVERSE = 2;

static Matrix MakeAlignmentMatrix(const Vector3& dir)
{
    return Matrix(
        Quaternion(Vector3(0, 0,1), PI/2) *
        Quaternion(Vector3(0, 1,0), PI/2 - dir.tilt()) *
        Quaternion(Vector3(0, 0,1), dir.zAngle())
        );
}

void PropertyGroup::Read(ChunkReader& reader)
{
    if (reader.group())
    {
        Verify(reader.next() == 1);
        Read(reader);
        Verify(reader.next() == -1);
        return;
    }
    m_type             = (Type)reader.readInteger();
    m_point            = reader.readVector3();
    m_position         = reader.readVector3();
    m_magnitude.min    = reader.readFloat();
    m_magnitude.max    = reader.readFloat();
    m_radius.min       = reader.readFloat();
    m_radius.max       = reader.readFloat();
    m_minPosition      = reader.readVector3();
    m_maxPosition      = reader.readVector3();
    m_sphereAngle.min  = PI/2 * (1.0f - reader.readFloat());
    m_sphereAngle.max  = PI/2 * (1.0f - reader.readFloat());
    m_sphereRadius.min = reader.readFloat();
    m_sphereRadius.max = reader.readFloat();
    m_cylRadius        = reader.readFloat();
    m_cylHeight.min    = reader.readFloat();
    m_cylHeight.max    = reader.readFloat();
    m_torusRadius      = reader.readFloat();
    m_tubeRadius       = reader.readFloat();
}

Vector3 PropertyGroup::SampleTorus(float torusRadius, float tubeRadius, bool hollow)
{
    float radius = (hollow) ? tubeRadius : GetRandom(0.0f, tubeRadius);
    float zAngle = GetRandom(0.0f, 2*PI);
    Vector2 circle(GetRandom(0.0f, 2*PI));
    circle.x = circle.x * radius + torusRadius;
    return Vector3(
        cosf(zAngle) * circle.x,
        sinf(zAngle) * circle.x,
        circle.y * radius);
}

Vector3 PropertyGroup::Sample(bool hollow) const
{
    switch (m_type)
    {
        case DIRECTION: {
            float magnitude = (hollow) ? ((GetRandom(0.0f, 1.0f) < 0.5f) ? m_magnitude.min : m_magnitude.max) : GetRandom(m_magnitude.min, m_magnitude.max);
            return normalize(m_position) * magnitude;
        }

        case SPHERE: {
            float radius = (hollow) ?  m_radius.max : GetRandom(m_radius.min, m_radius.max);
            return Vector3(GetRandom(0.0f, 2*PI), GetRandom(-PI, PI)) * radius;
        }

        case RANGE: return Vector3(
                        GetRandom(m_minPosition.x, m_maxPosition.x),
                        GetRandom(m_minPosition.y, m_maxPosition.y),
                        GetRandom(m_minPosition.z, m_maxPosition.z) );

        case SPHERICAL_RANGE: {
            float tilt, radius;
            if (!hollow)
            {
                tilt   = GetRandom(m_sphereAngle .min, m_sphereAngle .max);
                radius = GetRandom(m_sphereRadius.min, m_sphereRadius.max);
            }
            else switch (GetRandom(0,100) % (m_sphereAngle.min == 0 ? 3 : 4)) {
                case 0: radius = m_sphereRadius.max; tilt = GetRandom(m_sphereAngle.min, m_sphereAngle.max); break;
                case 1: radius = m_sphereRadius.min; tilt = GetRandom(m_sphereAngle.min, m_sphereAngle.max); break;
                case 2: radius = GetRandom(m_sphereRadius.min, m_sphereRadius.max); tilt = m_sphereAngle.max; break;
                case 3: radius = GetRandom(m_sphereRadius.min, m_sphereRadius.max); tilt = m_sphereAngle.min; break;
            }
            float zAngle = GetRandom(0.0f, 2*PI);
            return Vector3(zAngle, tilt) * radius;
        }

        case CYLINDER: {
            float radius = (hollow) ? m_cylRadius : GetRandom(0.0f, m_cylRadius);
            return Vector3(
                Vector2(GetRandom(0.0f, 2*PI)) * radius,
                GetRandom(m_cylHeight.min, m_cylHeight.max)
                );
        }

        case TORUS: 
            return SampleTorus(m_torusRadius, m_tubeRadius, hollow);
    }
    return m_point;
}

//
// PointCreatorPlugin
//
void PointCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT( 0, m_pps);
    PARAM_FLOAT( 1, m_speed);
    PARAM_FLOAT( 2, m_speedVariation);
    PARAM_FLOAT( 3, m_burstThreshold);
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    END_PARAM_LIST
}

float PointCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float PointCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long PointCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void PointCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->position     = Vector3(0,0,0);
    p->velocity     = Vector3(GetRandom(0.0f, 2*PI), GetRandom(-PI, PI)) * speed;
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;

    p->position += transform.getTranslation();
}

void PointCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* PointCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

void PointCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

PointCreatorPlugin::PointCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// TrailCreatorPlugin
//
void TrailCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(  0, m_pps);
    PARAM_FLOAT(  1, m_speed);
    PARAM_FLOAT(  2, m_speedVariation);
    PARAM_FLOAT(  3, m_burstThreshold);
    PARAM_INT  (  4, m_globalLOD);
    PARAM_FLOAT(  6, m_preSimulateSeconds);
    PARAM_BOOL (  7, m_autoUpdatePositions);
    PARAM_FLOAT(  8, m_minDLOD);
    PARAM_FLOAT(  9, m_DLODNearDistance);
    PARAM_FLOAT( 10, m_DLODFarDistance);
    PARAM_INT  (100, m_parent);
    END_PARAM_LIST
}

float TrailCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float TrailCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long TrailCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void TrailCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    assert(parent != NULL);

    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->position     = Vector3(0,0,0);
    p->velocity     = Vector3(GetRandom(0.0f, 2*PI), GetRandom(-PI, PI)) * speed;
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;

    p->position += parent->position;
}

void TrailCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* TrailCreatorPlugin::Initialize()
{
    ParticleSystem& system = m_emitter.GetSystem();
    if (m_parent < system.GetNumEmitters())
    {
        return system.GetEmitter(m_parent).RegisterSpawn(&m_emitter, ParticleSystem::Emitter::SPAWN_BIRTH);
    }
    return NULL;
}

void TrailCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

TrailCreatorPlugin::TrailCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// SphereCreatorPlugin
//
void SphereCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(  0, m_pps);
    PARAM_FLOAT(  1, m_speed);
    PARAM_FLOAT(  2, m_speedVariation);
    PARAM_FLOAT(  3, m_burstThreshold);
    PARAM_INT  (  4, m_globalLOD);
    PARAM_FLOAT(  6, m_preSimulateSeconds);
    PARAM_BOOL (  7, m_autoUpdatePositions);
    PARAM_FLOAT(  8, m_minDLOD);
    PARAM_FLOAT(  9, m_DLODNearDistance);
    PARAM_FLOAT( 10, m_DLODFarDistance);
    PARAM_FLOAT(100, m_radius);
    END_PARAM_LIST
}

float SphereCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float SphereCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long SphereCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void SphereCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->position     = Vector3(GetRandom(0.0f, 2*PI), GetRandom(-PI, PI)) * m_radius;
    p->velocity     = normalize(p->position) * speed;
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;

    p->position     += transform.getTranslation();
}

void SphereCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* SphereCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

void SphereCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

SphereCreatorPlugin::SphereCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// BoxCreatorPlugin
//
void BoxCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(  0, m_pps);
    PARAM_FLOAT(  1, m_speed);
    PARAM_FLOAT(  2, m_speedVariation);
    PARAM_FLOAT(  3, m_burstThreshold);
    PARAM_INT  (  4, m_globalLOD);
    PARAM_FLOAT(  6, m_preSimulateSeconds);
    PARAM_BOOL (  7, m_autoUpdatePositions);
    PARAM_FLOAT(  8, m_minDLOD);
    PARAM_FLOAT(  9, m_DLODNearDistance);
    PARAM_FLOAT( 10, m_DLODFarDistance);
    PARAM_FLOAT(100, m_length);
    PARAM_FLOAT(101, m_width);
    PARAM_FLOAT(102, m_height);
    END_PARAM_LIST
}

float BoxCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float BoxCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long BoxCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void BoxCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->position     = Vector3(
        GetRandom(-0.5f, 0.5f) * m_length,
        GetRandom(-0.5f, 0.5f) * m_width,
        GetRandom(-0.5f, 0.5f) * m_height);
    p->velocity     = normalize(p->position) * speed;
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;

    p->position     += transform.getTranslation();
}

void BoxCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* BoxCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

void BoxCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

BoxCreatorPlugin::BoxCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// TorusCreatorPlugin
//
void TorusCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(  0, m_pps);
    PARAM_FLOAT(  1, m_speed);
    PARAM_FLOAT(  2, m_speedVariation);
    PARAM_FLOAT(  3, m_burstThreshold);
    PARAM_INT  (  4, m_globalLOD);
    PARAM_FLOAT(  6, m_preSimulateSeconds);
    PARAM_BOOL (  7, m_autoUpdatePositions);
    PARAM_FLOAT(  8, m_minDLOD);
    PARAM_FLOAT(  9, m_DLODNearDistance);
    PARAM_FLOAT( 10, m_DLODFarDistance);
    PARAM_FLOAT(100, m_torusRadius);
    PARAM_FLOAT(101, m_tubeRadius);
    END_PARAM_LIST
}

float TorusCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float TorusCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long TorusCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void TorusCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->position     = PropertyGroup::SampleTorus(m_torusRadius, m_tubeRadius, false);
    p->velocity     = normalize(p->position) * speed;
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;

    p->position     += transform.getTranslation();
}

void TorusCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* TorusCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

void TorusCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

TorusCreatorPlugin::TorusCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// MeshCreatorPlugin
//
struct MeshCreatorBase::MeshCreatorData
{
    const IRenderObject* m_object;
    const Model::Mesh*   m_mesh;
    size_t               m_subMesh;
    size_t               m_vertex;
};

void MeshCreatorBase::InitializeParticle(Particle* p, MeshCreatorData* data) const
{
    p->position = Vector3(0,0,0);
    if (data->m_mesh != NULL)
    {
        const Model::Mesh*  mesh = data->m_mesh;
        MASTER_VERTEX tmp;
        const MASTER_VERTEX* v = &tmp;
        switch (m_spawnLocation)
        {
            case MESH_ALL_VERTICES:
                v = &mesh->subMeshes[data->m_subMesh].vertices[data->m_vertex];
                // Go to next vertex
                if (++data->m_vertex == mesh->subMeshes[data->m_subMesh].vertices.size())
                {
                    data->m_vertex  = 0;
                    data->m_subMesh = (data->m_subMesh + 1) % mesh->subMeshes.size();
                }
                break;

            case MESH_RANDOM_VERTEX: {
                // Pick random submesh and vertex
                int subMesh = GetRandom(0, (int)mesh->subMeshes.size());
                int vertex  = GetRandom(0, (int)mesh->subMeshes[subMesh].vertices.size());
                v = &mesh->subMeshes[subMesh].vertices[vertex];
                break;
            }

            case MESH_RANDOM_SURFACE: {
                // Pick random submesh
                int subMesh = GetRandom(0, (int)mesh->subMeshes.size());
                const Model::SubMesh& submesh = mesh->subMeshes[subMesh];

                // Pick random face on submesh
                int face    = GetRandom(0, (int)submesh.indices.size() / 3) * 3;
                const MASTER_VERTEX& v1 = submesh.vertices[submesh.indices[face + 0]];
                const MASTER_VERTEX& v2 = submesh.vertices[submesh.indices[face + 1]];
                const MASTER_VERTEX& v3 = submesh.vertices[submesh.indices[face + 2]];

                // Pick random position on face (barycentric coordinates)
                float w1 = GetRandom(0.0f, 1.0f);
                float w2 = GetRandom(0.0f, 1.0f - w1);
                float w3 = 1.0f - w2 - w1;

                tmp.Position = v1.Position * w1 + v2.Position * w2 + v3.Position * w3;
                tmp.Normal   = v1.Normal   * w1 + v2.Normal   * w2 + v3.Normal   * w3;
                break;
            }
        }

        // Initialize particle based on vertex v
        const Matrix transform = data->m_object->GetBoneTransform(mesh->bone->index);
        Vector3 normal   = Vector4(v->Normal,   0) * transform;
        Vector3 position = Vector4(v->Position, 1) * transform;
        
        p->position = position + normal * m_surfaceOffset;
        if (m_alignVelocityToNormal)
        {
            // Rotate +Z to normal
            p->velocity = Vector4(p->velocity, 0) * MakeAlignmentMatrix(normal);
        }
    }
}

MeshCreatorBase::MeshCreatorBase()
{
    m_spawnLocation         = MESH_RANDOM_SURFACE;
    m_alignVelocityToNormal = true;
    m_surfaceOffset         = 0.0f;
}

void MeshCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(  0, m_pps);
    PARAM_FLOAT(  1, m_speed);
    PARAM_FLOAT(  2, m_speedVariation);
    PARAM_FLOAT(  3, m_burstThreshold);
    PARAM_INT  (  4, m_globalLOD);
    PARAM_FLOAT(  6, m_preSimulateSeconds);
    PARAM_BOOL (  7, m_autoUpdatePositions);
    PARAM_FLOAT(  8, m_minDLOD);
    PARAM_FLOAT(  9, m_DLODNearDistance);
    PARAM_FLOAT( 10, m_DLODFarDistance);
    END_PARAM_LIST
}

float MeshCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return max(m_burstThreshold, 1.0f) / m_pps;
}

float MeshCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

unsigned long MeshCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return (m_burstThreshold >= m_pps) ? (unsigned long)(m_pps * m_burstThreshold) : 1;
}

void MeshCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    float speed = m_speed;
    if (m_speedVariation != 0.0f)
    {
        speed += GetRandom(-m_speedVariation, m_speedVariation) * m_speed;
    }

    p->spawnTime    = time;
    p->velocity     = Vector3(0,0,speed);
    MeshCreatorBase::InitializeParticle(p, (MeshCreatorData*)data);

    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;
}

size_t MeshCreatorPlugin::GetPrivateDataSize() const
{
    return sizeof(MeshCreatorData);
}

void MeshCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
    MeshCreatorData* mcd = (MeshCreatorData*)data;
    mcd->m_object  = object;
    mcd->m_mesh    = mesh;
    mcd->m_vertex  = 0;
    mcd->m_subMesh = 0;
}

ParticleSystem::Emitter* MeshCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

void MeshCreatorPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_speedVariation /= 100.0f;
}

MeshCreatorPlugin::MeshCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

//
// ShapeCreatorPlugin
//
void ShapeCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_FLOAT (101, m_spawnInterval);
    PARAM_BOOL  (102, m_bursting);
    PARAM_FLOAT (103, m_startDelay);
    PARAM_FLOAT (104, m_stopTime);
    PARAM_CUSTOM(105, m_position);
    PARAM_BOOL  (106, m_hollowPosition);
    PARAM_CUSTOM(107, m_velocity);
    PARAM_BOOL  (108, m_hollowVelocity);
    PARAM_BOOL  (109, m_localVelocity);
    PARAM_BOOL  (110, m_inheritVelocity);
    PARAM_FLOAT (111, m_inheritedVelocityScale);
    PARAM_FLOAT (112, m_maxInheritedVelocity);
    PARAM_BOOL  (113, m_hiresEmission);
    PARAM_BOOL  (114, m_burstOnShown);
    END_PARAM_LIST
}

float ShapeCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return (m_stopTime == 0 || currentTime < m_stopTime)
        ? (!m_bursting ? m_spawnInterval / m_particlePerInterval : m_spawnInterval)
        : -1;
}

float ShapeCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return m_startDelay;
}

unsigned long ShapeCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return m_bursting ? (unsigned long)m_particlePerInterval : 1;
}

void ShapeCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    p->spawnTime    = time;
    p->position     = Vector4(m_position.Sample(m_hollowPosition), 1) * transform;
    p->velocity     = m_velocity.Sample(m_hollowVelocity);
    if (m_localVelocity)
    {
        p->velocity = Vector4(p->velocity, 0) * transform;
    }
    if (parent != NULL)
    {
        if (m_inheritVelocity)
        {
            float speed = parent->velocity.length();
            p->velocity += normalize(parent->velocity) * min(speed * m_inheritedVelocityScale, m_maxInheritedVelocity);
        }
        
        // Add parent position (relative to emitter, otherwise we add emitter twice)
        p->position += parent->position - transform.getTranslation();
    }
    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;
}

void ShapeCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* ShapeCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

ShapeCreatorPlugin::ShapeCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
}

ShapeCreatorPlugin::ShapeCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params)
    : CreatorPlugin(emitter), ShapeCreator(params)
{
}

//
// OutwardVelocityShapeCreatorPlugin
//
void OutwardVelocityShapeCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_FLOAT (101, m_spawnInterval);
    PARAM_BOOL  (102, m_bursting);
    PARAM_FLOAT (103, m_startDelay);
    PARAM_FLOAT (104, m_stopTime);
    PARAM_CUSTOM(105, m_position);
    PARAM_BOOL  (106, m_hollowPosition);
    PARAM_CUSTOM(107, m_velocity);
    PARAM_BOOL  (108, m_hollowVelocity);
    PARAM_BOOL  (109, m_localVelocity);
    PARAM_BOOL  (110, m_inheritVelocity);
    PARAM_FLOAT (111, m_inheritedVelocityScale);
    PARAM_FLOAT (112, m_maxInheritedVelocity);
    PARAM_BOOL  (113, m_hiresEmission);
    PARAM_BOOL  (114, m_burstOnShown);

    PARAM_BOOL  (300, m_outwardSpeed);
    END_PARAM_LIST
}

void OutwardVelocityShapeCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    ShapeCreatorPlugin::InitializeParticle(p, data, parent, time);
}

OutwardVelocityShapeCreatorPlugin::OutwardVelocityShapeCreatorPlugin(ParticleSystem::Emitter& emitter)
    : ShapeCreatorPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported creator plugin", name.c_str());
}

//
// DeathCreatorPlugin
//
void DeathCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_CUSTOM(101, m_position);
    PARAM_BOOL  (102, m_hollowPosition);
    PARAM_CUSTOM(103, m_velocity);
    PARAM_BOOL  (104, m_hollowVelocity);
    PARAM_BOOL  (105, m_localVelocity);
    PARAM_BOOL  (106, m_inheritVelocity);
    PARAM_FLOAT (107, m_inheritedVelocityScale);
    PARAM_FLOAT (108, m_maxInheritedVelocity);
    PARAM_STRING(109, m_parentName);
    PARAM_INT   (110, m_positionAlignment);
    PARAM_INT   (111, m_velocityAlignment);
    END_PARAM_LIST
}

float DeathCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return -1;
}

float DeathCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return 0;
}

void DeathCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    assert(parent != NULL);
    ShapeCreatorPlugin::InitializeParticle(p, data, parent, time);

    if (m_positionAlignment != ALIGNMENT_NONE || m_velocityAlignment != ALIGNMENT_NONE)
    {
        // Align Z with particle velocity vector
        if (m_velocityAlignment != ALIGNMENT_NONE) {
            p->velocity *= MakeAlignmentMatrix( (m_velocityAlignment == ALIGNMENT_REVERSE) ? -parent->velocity : parent->velocity);
        }

        if (m_positionAlignment != ALIGNMENT_NONE) {
            p->position *= MakeAlignmentMatrix( (m_positionAlignment == ALIGNMENT_REVERSE) ? -parent->position : parent->position);
        }
    }
}

ParticleSystem::Emitter* DeathCreatorPlugin::Initialize()
{
    ParticleSystem& system = m_emitter.GetSystem();
    if (m_parentEmitter != NULL)
    {
        return m_parentEmitter->RegisterSpawn(&m_emitter, ParticleSystem::Emitter::SPAWN_DEATH);
    }
    
    for (size_t i = 0; i < system.GetNumEmitters(); i++)
    {
        ParticleSystem::Emitter& e = system.GetEmitter(i);
        if (e.GetName() == m_parentName)
        {
            return e.RegisterSpawn(&m_emitter, ParticleSystem::Emitter::SPAWN_DEATH);
        }
    }
    return NULL;
}

DeathCreatorPlugin::DeathCreatorPlugin(ParticleSystem::Emitter& emitter)
    : ShapeCreatorPlugin(emitter), m_parentEmitter(NULL)
{
    m_spawnInterval = 1.0f;
    m_bursting      = true;
    m_startDelay    = 0.0f;
    m_stopTime      = 0.5f;
    m_hiresEmission = false;
    m_burstOnShown  = true;
}

DeathCreatorPlugin::DeathCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, ParticleSystem::Emitter* parentEmitter)
    : ShapeCreatorPlugin(emitter, params), m_parentEmitter(parentEmitter),
      m_positionAlignment(ALIGNMENT_NONE), m_velocityAlignment(ALIGNMENT_NONE)
{
    m_spawnInterval   = 1.0f;
    m_bursting        = true;
    m_startDelay      = 0.0f;
    m_stopTime        = 0.5f;
    m_hiresEmission   = false;
    m_burstOnShown    = true;
    m_inheritVelocity = false;
}


//
// EnhancedTrailCreatorPlugin
//
void EnhancedTrailCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_FLOAT (101, m_spawnInterval);
    PARAM_BOOL  (102, m_bursting);
    PARAM_FLOAT (103, m_startDelay);
    PARAM_FLOAT (104, m_stopTime);
    PARAM_CUSTOM(105, m_position);
    PARAM_BOOL  (106, m_hollowPosition);
    PARAM_CUSTOM(107, m_velocity);
    PARAM_BOOL  (108, m_hollowVelocity);
    PARAM_BOOL  (109, m_localVelocity);
    PARAM_BOOL  (110, m_inheritVelocity);
    PARAM_FLOAT (111, m_inheritedVelocityScale);
    PARAM_FLOAT (112, m_maxInheritedVelocity);
    PARAM_BOOL  (113, m_hiresEmission);
    PARAM_BOOL  (114, m_burstOnShown);
    PARAM_STRING(300, m_parentName);
    PARAM_INT   (301, m_positionAlignment);
    PARAM_INT   (302, m_velocityAlignment);
    END_PARAM_LIST
}

void EnhancedTrailCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    assert(parent != NULL);
    ShapeCreatorPlugin::InitializeParticle(p, data, parent, time);
    if (m_positionAlignment != ALIGNMENT_NONE || m_velocityAlignment != ALIGNMENT_NONE)
    {
        // Align Z with particle velocity vector
        if (m_velocityAlignment != ALIGNMENT_NONE) {
            p->velocity *= MakeAlignmentMatrix( (m_velocityAlignment == ALIGNMENT_REVERSE) ? -parent->velocity : parent->velocity);
        }

        if (m_positionAlignment != ALIGNMENT_NONE) {
            p->position *= MakeAlignmentMatrix( (m_positionAlignment == ALIGNMENT_REVERSE) ? -parent->position : parent->position);
        }
    }
}

ParticleSystem::Emitter* EnhancedTrailCreatorPlugin::Initialize()
{
    ParticleSystem& system = m_emitter.GetSystem();
    if (m_parentEmitter != NULL)
    {
        return m_parentEmitter->RegisterSpawn(&m_emitter, ParticleSystem::Emitter::SPAWN_BIRTH);
    }
    
    for (size_t i = 0; i < system.GetNumEmitters(); i++)
    {
        ParticleSystem::Emitter& e = system.GetEmitter(i);
        if (e.GetName() == m_parentName)
        {
            return e.RegisterSpawn(&m_emitter, ParticleSystem::Emitter::SPAWN_BIRTH);
        }
    }
    return NULL;
}

EnhancedTrailCreatorPlugin::EnhancedTrailCreatorPlugin(ParticleSystem::Emitter& emitter)
    : ShapeCreatorPlugin(emitter), m_parentEmitter(NULL)
{
}

EnhancedTrailCreatorPlugin::EnhancedTrailCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, ParticleSystem::Emitter* parentEmitter)
    : ShapeCreatorPlugin(emitter, params), m_parentEmitter(parentEmitter),
      m_positionAlignment(ALIGNMENT_NONE), m_velocityAlignment(ALIGNMENT_NONE)
{
}

//
// EnhancedMeshCreatorPlugin
//
void EnhancedMeshCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);
    
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_FLOAT (101, m_spawnInterval);
    PARAM_BOOL  (102, m_bursting);
    PARAM_FLOAT (103, m_startDelay);
    PARAM_FLOAT (104, m_stopTime);
    PARAM_INT   (105, m_spawnLocation);
    PARAM_BOOL  (106, m_alignVelocityToNormal);
    PARAM_CUSTOM(107, m_velocity);
    PARAM_BOOL  (108, m_hollowVelocity);
    PARAM_BOOL  (109, m_localVelocity);
    PARAM_BOOL  (110, m_inheritVelocity);
    PARAM_FLOAT (111, m_inheritedVelocityScale);
    PARAM_FLOAT (112, m_maxInheritedVelocity);
    PARAM_BOOL  (113, m_hiresEmission);
    PARAM_BOOL  (114, m_burstOnShown);
    PARAM_FLOAT (200, m_surfaceOffset);
    END_PARAM_LIST
}

size_t EnhancedMeshCreatorPlugin::GetPrivateDataSize() const
{
    return sizeof(MeshCreatorData);
}

unsigned long EnhancedMeshCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    MeshCreatorData* mcd = (MeshCreatorData*)data;
    unsigned long base = (mcd->m_mesh != NULL && m_spawnLocation == MESH_ALL_VERTICES ? (unsigned long)mcd->m_mesh->nVertices : 1);
    return base * ShapeCreatorPlugin::GetNumParticlesPerSpawn(data);
}

void EnhancedMeshCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();

    p->spawnTime    = time;
    p->velocity     = m_velocity.Sample(m_hollowVelocity);
    if (m_localVelocity) {
        p->velocity = Vector4(p->velocity, 0) * transform;
    }
    MeshCreatorBase::InitializeParticle(p, (MeshCreatorData*)data);

    p->acceleration = Vector3(0,0,0);
    p->texCoords    = Vector4(0,0,1,1);
    p->size         = 1.0f;
    p->color        = Color(0.1f, 1.0f, 0.5f, 1.0f);
    p->rotation     = 0.0f;
}

void EnhancedMeshCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
    MeshCreatorData* mcd = (MeshCreatorData*)data;
    mcd->m_object  = object;
    mcd->m_mesh    = mesh;
    mcd->m_vertex  = 0;
    mcd->m_subMesh = 0;
}

EnhancedMeshCreatorPlugin::EnhancedMeshCreatorPlugin(ParticleSystem::Emitter& emitter)
    : ShapeCreatorPlugin(emitter)
{
    m_position.m_type  = PropertyGroup::POINT;
    m_position.m_point = Vector3(0,0,0);
    m_hollowPosition   = false;
}

EnhancedMeshCreatorPlugin::EnhancedMeshCreatorPlugin(ParticleSystem::Emitter& emitter, const ShapeCreator& params, 
        unsigned long spawnLocation,
        float         surfaceOffset)
    : ShapeCreatorPlugin(emitter, params)
{
    m_spawnLocation         = spawnLocation;
    m_surfaceOffset         = surfaceOffset;
    m_alignVelocityToNormal = true;
}

//
// HardwareSpawnerCreatorPlugin
//
void HardwareSpawnerCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);   
    PARAM_FLOAT (6000, m_particleCount);
    PARAM_FLOAT (6001, m_particleLifetime);
    PARAM_BOOL  (6002, m_enableBursting);
    PARAM_CUSTOM(6003, m_position);
    PARAM_BOOL  (6004, m_hollowPosition);
    PARAM_CUSTOM(6005, m_velocity);
    PARAM_BOOL  (6006, m_hollowVelocity);
    PARAM_FLOAT3(6007, m_acceleration);
    PARAM_FLOAT (6008, m_attractAcceleration);
    PARAM_FLOAT3(6009, m_vortexAxis);
    PARAM_FLOAT (6010, m_vortexRotation);
    PARAM_FLOAT (6011, m_vortexAttraction);
    END_PARAM_LIST
}

float HardwareSpawnerCreatorPlugin::GetSpawnDelay(void* data, float currentTime) const
{
    return -1;
}

float HardwareSpawnerCreatorPlugin::GetInitialSpawnDelay(void* data) const
{
    return -1;
}

unsigned long HardwareSpawnerCreatorPlugin::GetNumParticlesPerSpawn(void* data) const
{
    return 0;
}

void HardwareSpawnerCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
}

void HardwareSpawnerCreatorPlugin::InitializeInstance(void* data, IRenderObject* object, const Model::Mesh* mesh) const
{
}

ParticleSystem::Emitter* HardwareSpawnerCreatorPlugin::Initialize()
{
    return m_emitter.GetSystem().RegisterSpawn(&m_emitter);
}

HardwareSpawnerCreatorPlugin::HardwareSpawnerCreatorPlugin(ParticleSystem::Emitter& emitter)
    : CreatorPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported creator plugin", name.c_str());
}

//
// AlignedShapeCreatorPlugin
//
void AlignedShapeCreatorPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT  ( 4, m_globalLOD);
    PARAM_FLOAT( 6, m_preSimulateSeconds);
    PARAM_BOOL ( 7, m_autoUpdatePositions);
    PARAM_FLOAT( 8, m_minDLOD);
    PARAM_FLOAT( 9, m_DLODNearDistance);
    PARAM_FLOAT(10, m_DLODFarDistance);   
    PARAM_FLOAT (100, m_particlePerInterval);
    PARAM_FLOAT (101, m_spawnInterval);
    PARAM_BOOL  (102, m_bursting);
    PARAM_FLOAT (103, m_startDelay);
    PARAM_FLOAT (104, m_stopTime);
    PARAM_CUSTOM(105, m_position);
    PARAM_BOOL  (106, m_hollowPosition);
    PARAM_CUSTOM(107, m_velocity);
    PARAM_BOOL  (108, m_hollowVelocity);
    PARAM_BOOL  (109, m_localVelocity);
    PARAM_BOOL  (110, m_inheritVelocity);
    PARAM_FLOAT (111, m_inheritedVelocityScale);
    PARAM_FLOAT (112, m_maxInheritedVelocity);
    PARAM_BOOL  (113, m_hiresEmission);
    PARAM_BOOL  (114, m_burstOnShown);
    PARAM_INT   (200, m_positionAlignmentDirection);
    PARAM_INT   (201, m_positionAlignment);
    PARAM_INT   (202, m_velocityAlignmentDirection);
    PARAM_INT   (203, m_velocityAlignment);
    PARAM_STRING(204, m_bone);
    END_PARAM_LIST
}

void AlignedShapeCreatorPlugin::InitializeParticle(Particle* p, void* data, const Particle* parent, float time) const
{
    ShapeCreatorPlugin::InitializeParticle(p, data, parent, time);
}

AlignedShapeCreatorPlugin::AlignedShapeCreatorPlugin(ParticleSystem::Emitter& emitter)
    : ShapeCreatorPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported creator plugin", name.c_str());
}

}
