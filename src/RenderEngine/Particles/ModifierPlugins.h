#ifndef PARTICLES_MODIFIER_PLUGINS_H
#define PARTICLES_MODIFIER_PLUGINS_H

#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/Plugin.h"
#include "General/Math.h"
#include "General/Utils.h"
#include <vector>

namespace Alamo
{

class ListBase : public PluginProperty
{
    virtual void ReadItem(ChunkReader& reader) = 0;
    virtual void Reserve(size_t count) = 0;
    void Read(ChunkReader& reader);
};

template <typename T>
struct List : public ListBase
{
    typedef std::vector<T> ListData;
    ListData m_data;
    
    size_t size() const { return m_data.size(); }
    const T& operator[](size_t index) const { return m_data[index]; }
          T& operator[](size_t index)       { return m_data[index]; }

    void Reserve(size_t count) {
        m_data.reserve(count);
    }

    void ReadItem(ChunkReader& reader) {
        m_data.push_back(TemplateReader<T>::read(reader));
    }

    void push_back(const T& val) { m_data.push_back(val); }
    void clear() { m_data.clear(); }

    typename ListData::iterator               begin()        { return m_data.begin(); }
    typename ListData::const_iterator         begin()  const { return m_data.begin(); }
    typename ListData::reverse_iterator       rbegin()       { return m_data.rbegin(); }
    typename ListData::const_reverse_iterator rbegin() const { return m_data.rbegin(); }
    typename ListData::iterator               end()        { return m_data.end(); }
    typename ListData::const_iterator         end()  const { return m_data.end(); }
    typename ListData::reverse_iterator       rend()       { return m_data.rend(); }
    typename ListData::const_reverse_iterator rend() const { return m_data.rend(); }
};

struct TrackBase
{
    enum InterpolationType {
        IT_STEP, IT_LINEAR, IT_SMOOTH
    };
    InterpolationType m_interpolation;
};

template <typename T>
struct Track : public TrackBase, public List< std::pair<float, T> >
{
    using Key = std::pair<float, T>;
    using ListData = typename List< std::pair<float, T> >::ListData;

    struct Cursor
    {
        typename ListData::size_type prev, next;
    };

    void UpdateCursor(Cursor& c, float time) const
    {
        while (m_data[c.next].first < time && c.next < m_data.size() - 1)
        {
            c.prev++;
            c.next++;
        }
    }

    void InitializeCursor(Cursor& c, bool random = false) const
    {
        c.prev = (random) ? GetRandom(0, (int)size() - 1) : 0;
        c.next = c.prev + 1;
    }

    T sample(const Cursor& c, float t) const
    {
        // Normalize t to slope domain
        t = saturate((t - m_data[c.prev].first) / (m_data[c.next].first - m_data[c.prev].first));
        switch (m_interpolation)
        {
            default:        
            case IT_STEP:   return m_data[c.prev].second;
            case IT_SMOOTH: return cubic(m_data[c.prev].second, m_data[c.next].second, t);
            case IT_LINEAR: return lerp (m_data[c.prev].second, m_data[c.next].second, t);
        }
    }
};

class ConstantUVModifierPlugin : public ModifierPlugin
{
    bool     m_writeOnce;
    Vector4  m_texCoords;

    void CheckParameter(int id);
    void ModifyParticle(Particle* p, void* data, float time) const;
    void InitializeParticle(Particle* p, void* data, float time) const;

public:
    ConstantUVModifierPlugin(ParticleSystem::Emitter& emitter);
    ConstantUVModifierPlugin(ParticleSystem::Emitter& emitter, const Vector4& texCoords);
};

class KeyedUVModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool           m_smooth;
    Track<Vector4> m_texcoords;
    float          m_loopCount;
    bool           m_useGlobalTime;
    float          m_globalAnimLength;
    bool           m_randomStartKey;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedUVModifierPlugin(ParticleSystem::Emitter& emitter);
    KeyedUVModifierPlugin(ParticleSystem::Emitter& emitter, const Track<Vector4>& texcoords);
};

class RandomUVModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    RandomUVModifierPlugin(ParticleSystem::Emitter& emitter);
};

class SlottedRandomUVModifierPlugin : public ModifierPlugin
{
    struct PrivateData;
    
    bool          m_writeOnce;
    List<Vector4> m_slots;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    SlottedRandomUVModifierPlugin(ParticleSystem::Emitter& emitter);
};

class ConstantSizeModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool     m_writeOnce;
    float    m_size;
    float    m_sizeVariation;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    ConstantSizeModifierPlugin(ParticleSystem::Emitter& emitter);
    ConstantSizeModifierPlugin(ParticleSystem::Emitter& emitter, float size, float sizeVariation);
};

class LinearSizeModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    float    m_startTime, m_endTime;
    float    m_startSize, m_endSize;
    bool     m_smooth;
    float    m_sizeVariation;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    LinearSizeModifierPlugin(ParticleSystem::Emitter& emitter);
    LinearSizeModifierPlugin(ParticleSystem::Emitter& emitter, float startSize, float endSize, float sizeVariation, bool smooth);
};

class KeyedSizeModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool         m_smooth;
    Track<float> m_sizes;
    float        m_sizeVariation;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedSizeModifierPlugin(ParticleSystem::Emitter& emitter);
    KeyedSizeModifierPlugin(ParticleSystem::Emitter& emitter, Track<float> sizes, float sizeVariation);
};

class ConstantColorModifierPlugin : public ModifierPlugin
{
    bool     m_writeOnce;
    Color    m_color;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    ConstantColorModifierPlugin(ParticleSystem::Emitter& emitter);
};

class LinearColorModifierPlugin : public ModifierPlugin
{
    float    m_startTime,  m_endTime;
    Color    m_startColor, m_endColor;
    bool     m_smooth;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    LinearColorModifierPlugin(ParticleSystem::Emitter& emitter);
};

class ColorVarianceModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    Color m_min;
    Color m_max;
    bool  m_writeOnce;
    bool  m_grayscale;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    ColorVarianceModifierPlugin(ParticleSystem::Emitter& emitter);
    ColorVarianceModifierPlugin(ParticleSystem::Emitter& emitter, const Color& max, bool grayscale);
};

class KeyedColorModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool         m_smooth;
    Track<Color> m_colors;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedColorModifierPlugin(ParticleSystem::Emitter& emitter);
};

class KeyedChannelColorModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    struct ChannelInfo;
    static const ChannelInfo Channels[4];

    Track<float> m_red, m_green, m_blue, m_alpha;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedChannelColorModifierPlugin(ParticleSystem::Emitter& emitter,
        const Track<float>& red,
        const Track<float>& green,
        const Track<float>& blue,
        const Track<float>& alpha);
};

class ConstantRotationModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool     m_writeOnce;
    float    m_rotation;
    float    m_rotationVariation;
    bool     m_reverse;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    ConstantRotationModifierPlugin(ParticleSystem::Emitter& emitter);
    ConstantRotationModifierPlugin(ParticleSystem::Emitter& emitter, float rotation, float rotationVariation, bool reverse);
};

class LinearRotationRateModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    float m_startTime, m_endTime;
    bool  m_smooth;
    float m_startRPS, m_endRPS;
    bool  m_reverse;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    LinearRotationRateModifierPlugin(ParticleSystem::Emitter& emitter);
};

class LinearRotationModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    float    m_startTime,  m_endTime;
    float    m_startRotation, m_endRotation;
    bool     m_smooth, m_reverse, m_randomize;
    float    m_rotationVariation;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    LinearRotationModifierPlugin(ParticleSystem::Emitter& emitter);
};

class KeyedRotationModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    Track<float> m_rotations;
    bool         m_smooth, m_reverse, m_randomize;
    float        m_rotationVariation;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedRotationModifierPlugin(ParticleSystem::Emitter& emitter);
};

class KeyedRotationRateModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    Track<float> m_rps;
    bool         m_smooth, m_reverse;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedRotationRateModifierPlugin(ParticleSystem::Emitter& emitter);
    KeyedRotationRateModifierPlugin(ParticleSystem::Emitter& emitter, const Track<float>& rps, bool reverse);
};

class AccelerationModifierPlugin : public ModifierPlugin
{
    Vector3      m_acceleration;
    bool         m_localSpace;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    AccelerationModifierPlugin(ParticleSystem::Emitter& emitter);
    AccelerationModifierPlugin(ParticleSystem::Emitter& emitter, const Vector3& acceleration, bool localSpace);
};

class AttractAccelerationModifierPlugin : public ModifierPlugin
{
    float m_attract;
    bool  m_replace;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    AttractAccelerationModifierPlugin(ParticleSystem::Emitter& emitter);
    AttractAccelerationModifierPlugin(ParticleSystem::Emitter& emitter, float attract);
};

class KeyedAccelerationModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    bool         m_smooth;
    Track<float> m_magnitude;
    Vector3      m_direction;
    bool         m_localSpace;

    void   ReadParameters(ChunkReader& reader);
    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedAccelerationModifierPlugin(ParticleSystem::Emitter& emitter);
};

class AttractorModifierPlugin : public ModifierPlugin
{
    int          m_target;
    std::string  m_bone;
    float        m_constant, m_linear, m_invLinear;
    range<float> m_valid;
    bool         m_swim;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    AttractorModifierPlugin(ParticleSystem::Emitter& emitter);
};

class InwardSpeedModifierPlugin : public ModifierPlugin
{
    float m_speed;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    InwardSpeedModifierPlugin(ParticleSystem::Emitter& emitter);
    InwardSpeedModifierPlugin(ParticleSystem::Emitter& emitter, float speed);
};

class VortexModifierPlugin : public ModifierPlugin
{
    Vector3      m_axis;
    float        m_speed;
    bool         m_localSpace;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    VortexModifierPlugin(ParticleSystem::Emitter& emitter);
};

class SpeedLimitModifierPlugin : public ModifierPlugin
{
    float m_minSpeed;
    float m_maxSpeed;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    SpeedLimitModifierPlugin(ParticleSystem::Emitter& emitter);
};

class LinearSpeedModifierPlugin : public ModifierPlugin
{
    float m_startTime,  m_endTime;
    float m_startSpeed, m_endSpeed;
    bool  m_smooth;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    LinearSpeedModifierPlugin(ParticleSystem::Emitter& emitter);
};

class WindAccelerationModifierPlugin : public ModifierPlugin
{
    struct PrivateData;

    float m_windResponse;

    size_t GetPrivateDataSize() const;
    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    WindAccelerationModifierPlugin(ParticleSystem::Emitter& emitter);
    WindAccelerationModifierPlugin(ParticleSystem::Emitter& emitter, float windResponse);
};

class TorqueModifierPlugin : public ModifierPlugin
{
    int         m_targetPosition;
    std::string m_boneName;
    Vector3     m_axis;
    float       m_constantAcceleration;
    float       m_linearAcceleration;
    float       m_invLinearAcceleration;
    float       m_innerCutoffDistance;
    float       m_outerCutoffDistance;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    TorqueModifierPlugin(ParticleSystem::Emitter& emitter);
};

class TurbulenceModifierPlugin : public ModifierPlugin
{
    Vector3 m_noiseOffset;
    Vector3 m_noiseSpeed;
    float   m_noiseScale;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    TurbulenceModifierPlugin(ParticleSystem::Emitter& emitter);
};

class KeyedFrictionModifierPlugin : public ModifierPlugin
{
    Track<float> m_friction;
    bool         m_smooth;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    KeyedFrictionModifierPlugin(ParticleSystem::Emitter& emitter);
};

class DistanceKeyTimeModifierPlugin : public ModifierPlugin
{
    int          m_target;
    std::string  m_bone;
    range<float> m_radius;
    bool         m_invert;
    bool         m_smoothStep;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    DistanceKeyTimeModifierPlugin(ParticleSystem::Emitter& emitter);
};

class AxisAttractorModifierPlugin : public ModifierPlugin
{
    int          m_target;
    std::string  m_bone;
    Vector3      m_axis;
    float        m_constantAcceleration;
    float        m_linearAcceleration;
    float        m_invLinearAcceleration;
    range<float> m_cutoffDistance;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    AxisAttractorModifierPlugin(ParticleSystem::Emitter& emitter);
};

class TerrainBounceModifierPlugin : public ModifierPlugin
{
    float m_elasticity;

    void   CheckParameter(int id);
    void   ModifyParticle(Particle* p, void* data, float time) const;
    void   InitializeParticle(Particle* p, void* data, float time) const;

public:
    TerrainBounceModifierPlugin(ParticleSystem::Emitter& emitter);
    TerrainBounceModifierPlugin(ParticleSystem::Emitter& emitter, float elasticity);
};

}

#endif
