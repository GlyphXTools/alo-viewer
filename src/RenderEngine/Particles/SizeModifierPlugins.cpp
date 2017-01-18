#include "RenderEngine/Particles/ModifierPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Math.h"

namespace Alamo
{

void ListBase::Read(ChunkReader& reader)
{
    unsigned long count = reader.readInteger();
    Reserve(count);
    for (unsigned long i = 0; i < count; i++)
    {
        ReadItem(reader);
    }
}

//
// ConstantSizeModifierPlugin
//
struct ConstantSizeModifierPlugin::PrivateData
{
    float size;
};

size_t ConstantSizeModifierPlugin::GetPrivateDataSize() const
{
    return m_writeOnce ? 0 : sizeof(PrivateData);
}

void ConstantSizeModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL (100, m_writeOnce);
    PARAM_FLOAT(200, m_size);
    PARAM_FLOAT(201, m_sizeVariation);
    END_PARAM_LIST
}

void ConstantSizeModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    if (!m_writeOnce)
    {
        const PrivateData* data = static_cast<const PrivateData*>(_data);
        p->size = data->size;
    }
}

void ConstantSizeModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->size = m_size;
    if (m_sizeVariation > 0.0f)
    {
        p->size += GetRandom(-m_sizeVariation, m_sizeVariation) * p->size;
    }
    
    if (!m_writeOnce)
    {
        PrivateData* data = static_cast<PrivateData*>(_data);
        data->size = p->size;
    }
}

ConstantSizeModifierPlugin::ConstantSizeModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

ConstantSizeModifierPlugin::ConstantSizeModifierPlugin(ParticleSystem::Emitter& emitter, float size, float sizeVariation)
    : ModifierPlugin(emitter),
      m_size(size), m_sizeVariation(sizeVariation), m_writeOnce(true)
{
}

//
// LinearSizeModifierPlugin
//
struct LinearSizeModifierPlugin::PrivateData
{
    float size;
};

size_t LinearSizeModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void LinearSizeModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_startTime);
    PARAM_FLOAT(101, m_endTime);
    PARAM_BOOL (102, m_smooth);
    PARAM_FLOAT(200, m_startSize);
    PARAM_FLOAT(201, m_endSize);
    PARAM_FLOAT(202, m_sizeVariation);
    END_PARAM_LIST
}

void LinearSizeModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    t = saturate((t - m_startTime) / (m_endTime - m_startTime));     // Time relative to slope

    p->size = data->size * (m_smooth
        ? cubic(m_startSize, m_endSize, t)
        : lerp(m_startSize, m_endSize, t)
        );
}

void LinearSizeModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    data->size = 1.0f;
    if (m_sizeVariation > 0.0f)
    {
        data->size += GetRandom(-m_sizeVariation, m_sizeVariation) * data->size;
    }

    p->size = m_startSize * p->size;
}

LinearSizeModifierPlugin::LinearSizeModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

LinearSizeModifierPlugin::LinearSizeModifierPlugin(ParticleSystem::Emitter& emitter, float startSize, float endSize, float sizeVariation, bool smooth)
    : ModifierPlugin(emitter), m_startTime(0.0f), m_endTime(1.0f),
      m_startSize(startSize), m_endSize(endSize), m_sizeVariation(sizeVariation), m_smooth(smooth)
{
}

//
// KeyedSizeModifierPlugin
//
struct KeyedSizeModifierPlugin::PrivateData
{
    float size;
    Track<float>::Cursor pos;
};

size_t KeyedSizeModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedSizeModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_sizes);
    PARAM_FLOAT (301, m_sizeVariation);
    END_PARAM_LIST
}

void KeyedSizeModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_sizes.UpdateCursor(data->pos, t);
    p->size = data->size * m_sizes.sample(data->pos, t);
}

void KeyedSizeModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->size = 1.0f;
    if (m_sizeVariation > 0.0f)
    {
        p->size += GetRandom(-m_sizeVariation, m_sizeVariation) * p->size;
    }

    PrivateData* data = static_cast<PrivateData*>(_data);
    m_sizes.InitializeCursor(data->pos);
    data->size = p->size;

    p->size *= m_sizes[0].second;
}

void KeyedSizeModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_sizes.m_interpolation = (m_smooth) ? m_sizes.IT_SMOOTH : m_sizes.IT_LINEAR;
}

KeyedSizeModifierPlugin::KeyedSizeModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

KeyedSizeModifierPlugin::KeyedSizeModifierPlugin(ParticleSystem::Emitter& emitter, Track<float> sizes, float sizeVariation)
    : ModifierPlugin(emitter),
      m_sizes(sizes), m_sizeVariation(sizeVariation)
{
}

}
