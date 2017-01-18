#include "RenderEngine/Particles/ModifierPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Math.h"

namespace Alamo
{

//
// ConstantColorModifierPlugin
//
void ConstantColorModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL (100, m_writeOnce);
    PARAM_COLOR(200, m_color);
    END_PARAM_LIST
}

void ConstantColorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    if (!m_writeOnce)
    {
        p->color = m_color;
    }
}

void ConstantColorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->color = m_color;
}

ConstantColorModifierPlugin::ConstantColorModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// LinearColorModifierPlugin
//
void LinearColorModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(100, m_startTime);
    PARAM_FLOAT(101, m_endTime);
    PARAM_BOOL (102, m_smooth);
    PARAM_COLOR(200, m_startColor);
    PARAM_COLOR(201, m_endColor);
    END_PARAM_LIST
}

void LinearColorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    t = saturate((t - m_startTime) / (m_endTime - m_startTime));     // Time relative to slope

    p->color = (m_smooth) 
        ? cubic(m_startColor, m_endColor, t)
        : lerp(m_startColor, m_endColor, t);
}

void LinearColorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    p->color = m_startColor;
}

LinearColorModifierPlugin::LinearColorModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// KeyedColorModifierPlugin
//
struct KeyedColorModifierPlugin::PrivateData
{
    Track<Color>::Cursor pos;
};

size_t KeyedColorModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedColorModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL  (200, m_smooth);
    PARAM_CUSTOM(300, m_colors);
    END_PARAM_LIST
}

void KeyedColorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    m_colors.UpdateCursor(data->pos, t);
    p->color = m_colors.sample(data->pos, t);
}

void KeyedColorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    m_colors.InitializeCursor(data->pos);
    p->color = m_colors[0].second;
}

void KeyedColorModifierPlugin::ReadParameters(ChunkReader& reader)
{
    Plugin::ReadParameters(reader);
    m_colors.m_interpolation = (m_smooth) ? m_colors.IT_SMOOTH : m_colors.IT_LINEAR;
}

KeyedColorModifierPlugin::KeyedColorModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

//
// KeyedChannelColorModifierPlugin
//
struct KeyedChannelColorModifierPlugin::PrivateData
{
    Track<float>::Cursor pos[4];
};

size_t KeyedChannelColorModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KeyedChannelColorModifierPlugin::CheckParameter(int id)
{
}

struct KeyedChannelColorModifierPlugin::ChannelInfo
{
    float Color::*component;
    const Track<float> KeyedChannelColorModifierPlugin::*track;
};

const KeyedChannelColorModifierPlugin::ChannelInfo KeyedChannelColorModifierPlugin::Channels[4] =
{
    {&Color::r, &KeyedChannelColorModifierPlugin::m_red},
    {&Color::g, &KeyedChannelColorModifierPlugin::m_green},
    {&Color::b, &KeyedChannelColorModifierPlugin::m_blue},
    {&Color::a, &KeyedChannelColorModifierPlugin::m_alpha}
};

void KeyedChannelColorModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    float t = (time - p->spawnTime) / (p->stompTime - p->spawnTime); // Time relative to lifetime
    for (int i = 0; i < 4; i++)
    {
        const Track<float>& track = this->*(Channels[i].track);
        track.UpdateCursor(data->pos[i], t);
        p->color.*(Channels[i].component) = track.sample(data->pos[i], t);
    }
}

void KeyedChannelColorModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);

    for (int i = 0; i < 4; i++)
    {
        const Track<float>& track = this->*(Channels[i].track);
        track.InitializeCursor(data->pos[i]);
        p->color.*(Channels[i].component) = track[0].second;
    }
}

KeyedChannelColorModifierPlugin::KeyedChannelColorModifierPlugin(ParticleSystem::Emitter& emitter,
        const Track<float>& red,
        const Track<float>& green,
        const Track<float>& blue,
        const Track<float>& alpha)
    : ModifierPlugin(emitter),
      m_red(red), m_green(green), m_blue(blue), m_alpha(alpha)
{
}

//
// ColorVarianceModifierPlugin
//
struct ColorVarianceModifierPlugin::PrivateData
{
    Color addition;
};

size_t ColorVarianceModifierPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void ColorVarianceModifierPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_FLOAT(200, m_min.r);
    PARAM_FLOAT(201, m_max.r);
    PARAM_FLOAT(202, m_min.g);
    PARAM_FLOAT(203, m_max.g);
    PARAM_FLOAT(204, m_min.b);
    PARAM_FLOAT(205, m_max.b);
    PARAM_BOOL (206, m_grayscale);
    PARAM_FLOAT(207, m_min.a);
    PARAM_FLOAT(208, m_max.a);
    PARAM_BOOL (209, m_writeOnce);
    END_PARAM_LIST
}

void ColorVarianceModifierPlugin::ModifyParticle(Particle* p, void* _data, float time) const
{
    if (!m_writeOnce)
    {
        PrivateData* data = static_cast<PrivateData*>(_data);
        p->color.r = saturate(p->color.r + data->addition.r);
        p->color.g = saturate(p->color.g + data->addition.g);
        p->color.b = saturate(p->color.b + data->addition.b);
        p->color.a = saturate(p->color.a + data->addition.a);
    }
}

void ColorVarianceModifierPlugin::InitializeParticle(Particle* p, void* _data, float time) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    data->addition.r = GetRandom(m_min.r, m_max.r);
    data->addition.g = m_grayscale ? data->addition.r : GetRandom(m_min.g, m_max.g);
    data->addition.b = m_grayscale ? data->addition.r : GetRandom(m_min.b, m_max.b);
    data->addition.a = m_grayscale ? data->addition.r : GetRandom(m_min.a, m_max.a);
    p->color.r = saturate(p->color.r + data->addition.r);
    p->color.g = saturate(p->color.g + data->addition.g);
    p->color.b = saturate(p->color.b + data->addition.b);
    p->color.a = saturate(p->color.a + data->addition.a);
}

ColorVarianceModifierPlugin::ColorVarianceModifierPlugin(ParticleSystem::Emitter& emitter)
    : ModifierPlugin(emitter)
{
}

ColorVarianceModifierPlugin::ColorVarianceModifierPlugin(ParticleSystem::Emitter& emitter, const Color& max, bool grayscale)
    : ModifierPlugin(emitter),
      m_max(max), m_min(0,0,0,0), m_grayscale(grayscale), m_writeOnce(false)
{
}

}
