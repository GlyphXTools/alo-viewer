#include "RenderEngine/Particles/ModifierPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Math.h"

namespace Alamo
{

//
// ConstantRotationModifierPlugin
//
struct ConstantRotationModifierPlugin::PrivateData
{
    float rotation;
};

size_t ConstantRotationModifierPlugin::GetPrivateDataSize() const
{
    return m_writeOnce ? 0 : sizeof(PrivateData);
}

void ConstantRotationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL (100, m_writeOnce);
    PARAM_FLOAT(200, m_rotation);
    PARAM_FLOAT(201, m_rotationVariation);
    PARAM_BOOL (202, m_reverse);
    END_PARAM_LIST
}

void ConstantRotationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    if (!m_writeOnce)
    {
        const PrivateData* data = static_cast<const PrivateData*>(_data);
        p->rotation = data->rotation;
    }
}

void ConstantRotationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->rotation = m_rotation;
    if (m_rotationVariation > 0.0f)
    {
        p->rotation += GetRandom(-m_rotationVariation, m_rotationVariation) * p->rotation;
    }
    if (m_reverse && GetRandom(0.0f, 1.0f) < 0.5f)
    {
        p->rotation = -p->rotation;
    }
    
    if (!m_writeOnce)
    {
        PrivateData* data = static_cast<PrivateData*>(_data);
        data->rotation = p->rotation;
    }
}

ConstantRotationModifierPlugin::ConstantRotationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

ConstantRotationModifierPlugin::ConstantRotationModifierPlugin(ParticleSystem::Emitter& emitter, float rotation, float rotationVariation, bool reverse)
    : ModifierPlugin(emitter),
      m_rotation(rotation), m_rotationVariation(rotationVariation), m_reverse(reverse), m_writeOnce(true)
{
}

//
// LinearRotationModifierPlugin
//
struct LinearRotationModifierPlugin::PrivateData
{
    float rotation;
    float direction;
};

size_t LinearRotationModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void LinearRotationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_startTime);
    PARAM_FLOAT(101, m_endTime);
    PARAM_BOOL (102, m_smooth);
    PARAM_FLOAT(200, m_startRotation);
    PARAM_FLOAT(201, m_endRotation);
    PARAM_BOOL (202, m_reverse);
    PARAM_BOOL (203, m_randomize);
    PARAM_FLOAT(204, m_rotationVariation);
    END_PARAM_LIST
}

void LinearRotationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    t = saturate((t - m_startTime) / (m_endTime - m_startTime));     // Time relative to slope

    p->rotation = data->rotation + data->direction * (m_smooth
        ? cubic(m_startRotation, m_endRotation, t)
        : lerp(m_startRotation, m_endRotation, t)
        );
}

void LinearRotationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->rotation = 0;
    if (m_randomize && m_rotationVariation > 0.0f)
    {
        p->rotation = GetRandom(-m_rotationVariation, m_rotationVariation);
    }

    PrivateData* data = static_cast<PrivateData*>(_data);
    data->rotation  = p->rotation;
    data->direction = (m_reverse && GetRandom(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
}

LinearRotationModifierPlugin::LinearRotationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// LinearRotationRateModifierPlugin
//
struct LinearRotationRateModifierPlugin::PrivateData
{
    float direction;
    float prevTime;
};

size_t LinearRotationRateModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void LinearRotationRateModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_startTime);
    PARAM_FLOAT(101, m_endTime);
    PARAM_BOOL (102, m_smooth);
    PARAM_FLOAT(200, m_startRPS);
    PARAM_FLOAT(201, m_endRPS);
    PARAM_BOOL (202, m_reverse);
    END_PARAM_LIST
}

void LinearRotationRateModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    t = saturate((t - m_startTime) / (m_endTime - m_startTime));     // Time relative to slope

    p->rotation += (time - data->prevTime) * data->direction * (m_smooth
        ? cubic(m_startRPS, m_endRPS, t)
        : lerp(m_startRPS, m_endRPS, t)
        );
    data->prevTime = time;
}

void LinearRotationRateModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    data->direction = (m_reverse && GetRandom(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
    data->prevTime  = 0;
}

LinearRotationRateModifierPlugin::LinearRotationRateModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// KeyedRotationModifierPlugin
//
struct KeyedRotationModifierPlugin::PrivateData
{
    float  rotation;
    float  direction;
    Track<float>::Cursor pos;
};

size_t KeyedRotationModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedRotationModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_rotations);
    PARAM_BOOL  (301, m_reverse);
    PARAM_BOOL  (302, m_randomize);
    PARAM_FLOAT (303, m_rotationVariation);
    END_PARAM_LIST
}

void KeyedRotationModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_rotations.UpdateCursor(data->pos, t);
    p->rotation = data->rotation + data->direction * m_rotations.sample(data->pos, t);
}

void KeyedRotationModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    
    data->rotation = 0;
    if (m_randomize && m_rotationVariation > 0.0f)
    {
        data->rotation = GetRandom(-m_rotationVariation, m_rotationVariation);
    }

    data->direction = (m_reverse && GetRandom(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
    m_rotations.InitializeCursor(data->pos);

    p->rotation += data->rotation + data->direction * m_rotations[0].second;
}

void KeyedRotationModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_rotations.m_interpolation = (m_smooth) ? m_rotations.IT_SMOOTH : m_rotations.IT_LINEAR;
}

KeyedRotationModifierPlugin::KeyedRotationModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// KeyedRotationRateModifierPlugin
//
struct KeyedRotationRateModifierPlugin::PrivateData
{
    float  direction;
    float  prevTime;
    Track<float>::Cursor pos;
};

size_t KeyedRotationRateModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedRotationRateModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_rps);
    PARAM_BOOL  (301, m_reverse);
    END_PARAM_LIST
}

void KeyedRotationRateModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_rps.UpdateCursor(data->pos, t);
    p->rotation += (time - data->prevTime) * data->direction * m_rps.sample(data->pos, t);
    data->prevTime = time;
}

void KeyedRotationRateModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    data->direction = (m_reverse && GetRandom(0.0f, 1.0f) < 0.5f) ? -1.0f : 1.0f;
    data->prevTime  = time;
    m_rps.InitializeCursor(data->pos);
}

void KeyedRotationRateModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_rps.m_interpolation = (m_smooth) ? m_rps.IT_SMOOTH : m_rps.IT_LINEAR;
}

KeyedRotationRateModifierPlugin::KeyedRotationRateModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

KeyedRotationRateModifierPlugin::KeyedRotationRateModifierPlugin(ParticleSystem::Emitter& emitter, const Track<float>& rps, bool reverse)
    : ModifierPlugin(emitter),
      m_rps(rps), m_reverse(reverse)
{
}

}
