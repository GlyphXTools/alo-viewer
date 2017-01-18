#include "RenderEngine/Particles/ModifierPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Log.h"
using namespace std;

namespace Alamo
{

//
// ConstantUVModifierPlugin
//
void ConstantUVModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (100, m_writeOnce);
    PARAM_FLOAT4(200, m_texCoords);
    END_PARAM_LIST
}

void ConstantUVModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    if (!m_writeOnce)
    {
        p->texCoords = m_texCoords;
    }
}

void ConstantUVModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->texCoords = m_texCoords;
}

ConstantUVModifierPlugin::ConstantUVModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

ConstantUVModifierPlugin::ConstantUVModifierPlugin(ParticleSystem::Emitter& emitter, const Vector4& texCoords)
    : ModifierPlugin(emitter),
      m_texCoords(texCoords), m_writeOnce(true)
{
}

//
// KeyedUVModifierPlugin
//
struct KeyedUVModifierPlugin::PrivateData
{
    Track<Vector4>::Cursor pos;
};

size_t KeyedUVModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedUVModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_texcoords);
    PARAM_FLOAT (301, m_loopCount);
    PARAM_BOOL  (302, m_useGlobalTime);
    PARAM_FLOAT (303, m_globalAnimLength);
    PARAM_BOOL  (304, m_randomStartKey);
    END_PARAM_LIST
}

void KeyedUVModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_texcoords.UpdateCursor(data->pos, t);
    p->texCoords = m_texcoords.sample(data->pos, t);
}

void KeyedUVModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    m_texcoords.InitializeCursor(data->pos, m_randomStartKey);
    p->texCoords = m_texcoords[0].second;
}

void KeyedUVModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_texcoords.m_interpolation = (m_smooth) ? m_texcoords.IT_SMOOTH : m_texcoords.IT_LINEAR;
}

KeyedUVModifierPlugin::KeyedUVModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
    const string& name = emitter.GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently partially supported UV plugin", name.c_str());
}

KeyedUVModifierPlugin::KeyedUVModifierPlugin(ParticleSystem::Emitter& emitter, const Track<Vector4>& texcoords)
    : ModifierPlugin(emitter),
      m_texcoords(texcoords), m_randomStartKey(false)
{
    const string& name = emitter.GetSystem().GetName();
}

//
// RandomUVModifierPlugin
//
struct RandomUVModifierPlugin::PrivateData
{
    Vector4 offset;
};

size_t RandomUVModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void RandomUVModifierPlugin::CheckParameter(int id)
{
}

void RandomUVModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    const PrivateData* data = (PrivateData*)_data;
    p->texCoords += data->offset;
}

void RandomUVModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = (PrivateData*)_data;
    data->offset = Vector4(GetRandom(0.0f, 1.0f), GetRandom(0.0f, 1.0f), 0, 0);
    p->texCoords += data->offset;
}

RandomUVModifierPlugin::RandomUVModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// SlottedRandomUVModifierPlugin
//
struct SlottedRandomUVModifierPlugin::PrivateData
{
    Vector4 texCoords;
};

size_t SlottedRandomUVModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void SlottedRandomUVModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (100, m_writeOnce);
    PARAM_CUSTOM(200, m_slots);
    END_PARAM_LIST
}

void SlottedRandomUVModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    const PrivateData* data = (PrivateData*)_data;
    p->texCoords = data->texCoords;
}

void SlottedRandomUVModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    if (m_slots.size() > 0)
    {
        PrivateData* data = (PrivateData*)_data;
        data->texCoords = m_slots[GetRandom(0, (int)m_slots.size())];
        p->texCoords = data->texCoords;
    }
}

SlottedRandomUVModifierPlugin::SlottedRandomUVModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

}
