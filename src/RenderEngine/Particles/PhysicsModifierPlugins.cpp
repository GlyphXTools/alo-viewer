#include "RenderEngine/Particles/ModifierPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "RenderEngine/RenderEngine.h"
#include "General/Math.h"
#include "General/Log.h"
using namespace std;

namespace Alamo
{

//
// AttractorModifierPlugin
//
void AttractorModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (100, m_target);
    PARAM_STRING(101, m_bone);
    PARAM_FLOAT (200, m_constant);
    PARAM_FLOAT (201, m_linear);
    PARAM_FLOAT (202, m_invLinear);
    PARAM_FLOAT (203, m_valid.min);
    PARAM_FLOAT (204, m_valid.max);
    PARAM_BOOL  (205, m_swim);
    END_PARAM_LIST
}

void AttractorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    // TODO: All targets and position
    Vector3 target(p->emitter->GetTransform().getTranslation());
    Vector3 dir = p->position - target;
    float   distance = dir.length();
    p->acceleration = (distance > m_valid.min && distance < m_valid.max)
        ? m_constant  * normalize(dir) +
          m_linear    * dir +
          m_invLinear * dir / distance
        : Vector3(0,0,0);
}

void AttractorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    ModifyParticle(p, _data, time);
}

AttractorModifierPlugin::AttractorModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported physics plugin", name.c_str());
}

//
// InwardSpeedModifierPlugin
//
void InwardSpeedModifierPlugin::CheckParameter(int id)
{
}

void InwardSpeedModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void InwardSpeedModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    Vector3 dir = p->position - p->emitter->GetTransform().getTranslation();
    p->velocity = m_speed * normalize(dir);
}

InwardSpeedModifierPlugin::InwardSpeedModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

InwardSpeedModifierPlugin::InwardSpeedModifierPlugin(ParticleSystem::Emitter& emitter, float speed)
    : ModifierPlugin(emitter), m_speed(speed)
{
}

//
// AccelerationModifierPlugin
//
void AccelerationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT3(200, m_acceleration);
    PARAM_BOOL  (201, m_localSpace);
    END_PARAM_LIST
}

void AccelerationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    p->acceleration = m_acceleration;
    if (m_localSpace)
    {
        p->acceleration = Vector4(p->acceleration, 0.0f) * p->emitter->GetTransform();
    }
}

void AccelerationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

AccelerationModifierPlugin::AccelerationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

AccelerationModifierPlugin::AccelerationModifierPlugin(ParticleSystem::Emitter& emitter, const Vector3& acceleration, bool localSpace)
    : ModifierPlugin(emitter),
      m_acceleration(acceleration), m_localSpace(localSpace)
{
}

//
// AttractAccelerationModifierPlugin
//
void AttractAccelerationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(200, m_attract);
    END_PARAM_LIST
}

void AttractAccelerationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    const Vector3 acceleration = normalize(p->position - p->emitter->GetTransform().getTranslation()) * m_attract;
    if (m_replace) {
        p->acceleration = acceleration;
    } else {
        p->acceleration += acceleration; 
    }
}

void AttractAccelerationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

AttractAccelerationModifierPlugin::AttractAccelerationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter), m_replace(true)
{
}

AttractAccelerationModifierPlugin::AttractAccelerationModifierPlugin(ParticleSystem::Emitter& emitter, float attract)
    : ModifierPlugin(emitter),
      m_attract(attract), m_replace(false)
{
}

//
// KeyedAccelerationModifierPlugin
//
struct KeyedAccelerationModifierPlugin::PrivateData
{
    Track<float>::Cursor pos;
};

size_t KeyedAccelerationModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedAccelerationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_FLOAT3(300, m_direction);
    PARAM_CUSTOM(301, m_magnitude);
    PARAM_BOOL  (302, m_localSpace);
    END_PARAM_LIST
}

void KeyedAccelerationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_magnitude.UpdateCursor(data->pos, t);
    p->acceleration = m_direction * m_magnitude.sample(data->pos, t);
    if (m_localSpace)
    {
        p->acceleration = Vector4(p->acceleration, 0) * p->emitter->GetTransform();
    }
}

void KeyedAccelerationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    m_magnitude.InitializeCursor(data->pos);
}

void KeyedAccelerationModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_magnitude.m_interpolation = (m_smooth) ? m_magnitude.IT_SMOOTH : m_magnitude.IT_LINEAR;
}

KeyedAccelerationModifierPlugin::KeyedAccelerationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// VortexModifierPlugin
//
void VortexModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT3(200, m_axis);
    PARAM_FLOAT (201, m_speed);
    PARAM_BOOL  (202, m_localSpace);
    END_PARAM_LIST
}

void VortexModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();
    Vector3 axis = (m_localSpace) ? Vector4(m_axis, 0) * transform : m_axis;
    p->acceleration = m_speed * cross(axis, p->position - transform.getTranslation());
}

void VortexModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    const Matrix& transform = p->emitter->GetTransform();
    Vector3 axis = (m_localSpace) ? Vector4(m_axis, 0) * transform : m_axis;
    p->acceleration = m_speed * cross(axis, p->position - transform.getTranslation());
}

VortexModifierPlugin::VortexModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// SpeedLimitModifierPlugin
//
void SpeedLimitModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_minSpeed);
    PARAM_FLOAT(101, m_maxSpeed);
    END_PARAM_LIST
}

void SpeedLimitModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    const float speed = p->velocity.length();
    p->velocity *= min(m_maxSpeed, max(m_minSpeed, speed)) / speed;
}

void SpeedLimitModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

SpeedLimitModifierPlugin::SpeedLimitModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// LinearSpeedModifierPlugin
//
void LinearSpeedModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_startTime);
    PARAM_FLOAT(101, m_endTime);
    PARAM_BOOL (102, m_smooth);
    PARAM_FLOAT(200, m_startSpeed);
    PARAM_FLOAT(201, m_endSpeed);
    END_PARAM_LIST
}

void LinearSpeedModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    t = saturate((t - m_startTime) / (m_endTime - m_startTime));     // Time relative to slope

    // Scale velocity vector
    p->velocity = normalize(p->velocity) * (m_smooth
        ? cubic(m_startSpeed, m_endSpeed, t)
        : lerp(m_startSpeed, m_endSpeed, t));
}

void LinearSpeedModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->velocity = normalize(p->velocity) * m_startSpeed;
}

LinearSpeedModifierPlugin::LinearSpeedModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// WindAccelerationModifierPlugin
//
struct WindAccelerationModifierPlugin::PrivateData
{
    const Wind* wind;
};

size_t WindAccelerationModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void WindAccelerationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(200, m_windResponse);
    END_PARAM_LIST
}

void WindAccelerationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    float heading = data->wind->heading - D3DXToRadian(90);
    p->acceleration += Vector3(cos(heading), sin(heading), 0.0f) * (data->wind->speed * m_windResponse);
}

void WindAccelerationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    data->wind = &p->emitter->GetRenderEngine().GetEnvironment().m_wind;
    ModifyParticle(p, _data, time);
}

WindAccelerationModifierPlugin::WindAccelerationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

WindAccelerationModifierPlugin::WindAccelerationModifierPlugin(ParticleSystem::Emitter& emitter, float windResponse)
    : ModifierPlugin(emitter), m_windResponse(windResponse)
{
}

//
// TorqueModifierPlugin
//
void TorqueModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT   (100, m_targetPosition);
    PARAM_STRING(101, m_boneName);
    PARAM_FLOAT3(200, m_axis);
    PARAM_FLOAT (201, m_constantAcceleration);
    PARAM_FLOAT (202, m_linearAcceleration);
    PARAM_FLOAT (203, m_invLinearAcceleration);
    PARAM_FLOAT (204, m_innerCutoffDistance);
    PARAM_FLOAT (205, m_outerCutoffDistance);
    END_PARAM_LIST
}

void TorqueModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void TorqueModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

TorqueModifierPlugin::TorqueModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

//
// TurbulenceModifierPlugin
//
void TurbulenceModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT3(200, m_noiseOffset);
    PARAM_FLOAT3(201, m_noiseSpeed);
    PARAM_FLOAT (202, m_noiseScale);
    END_PARAM_LIST
}

void TurbulenceModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void TurbulenceModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

TurbulenceModifierPlugin::TurbulenceModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

//
// KeyedFrictionModifierPlugin
//
void KeyedFrictionModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_friction);
    END_PARAM_LIST
}

void KeyedFrictionModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void KeyedFrictionModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

KeyedFrictionModifierPlugin::KeyedFrictionModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

//
// DistanceKeyTimeModifierPlugin
//
void DistanceKeyTimeModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT   (100, m_target);
    PARAM_STRING(101, m_bone);
    PARAM_FLOAT (200, m_radius.min);
    PARAM_FLOAT (201, m_radius.max);
    PARAM_BOOL  (202, m_invert);
    PARAM_BOOL  (203, m_smoothStep);
    END_PARAM_LIST
}

void DistanceKeyTimeModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void DistanceKeyTimeModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

DistanceKeyTimeModifierPlugin::DistanceKeyTimeModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

//
// AxisAttractorModifierPlugin
//
void AxisAttractorModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_INT   (100, m_target);
    PARAM_STRING(101, m_bone);
    PARAM_FLOAT3(200, m_axis);
    PARAM_FLOAT (201, m_constantAcceleration);
    PARAM_FLOAT (202, m_linearAcceleration);
    PARAM_FLOAT (203, m_invLinearAcceleration);
    PARAM_FLOAT (204, m_cutoffDistance.min);
    PARAM_FLOAT (205, m_cutoffDistance.max);
    END_PARAM_LIST
}

void AxisAttractorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void AxisAttractorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

AxisAttractorModifierPlugin::AxisAttractorModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

//
// TerrainBounceModifierPlugin
//
void TerrainBounceModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(200, m_elasticity);
    END_PARAM_LIST
}

void TerrainBounceModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
}

void TerrainBounceModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
}

TerrainBounceModifierPlugin::TerrainBounceModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

TerrainBounceModifierPlugin::TerrainBounceModifierPlugin(ParticleSystem::Emitter& emitter, float elasticity)
    : ModifierPlugin(emitter), m_elasticity(elasticity)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported physics plugin", name.c_str());
}

}