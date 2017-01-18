#ifndef PARTICLES_RENDERER_PLUGINS_H
#define PARTICLES_RENDERER_PLUGINS_H

#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/Plugin.h"
#include "General/Utils.h"

namespace Alamo {

struct CommonRendererParameters
{
    bool m_normalRenderMode;
    bool m_reflectionRenderMode;
    bool m_refractionRenderMode;
    bool m_shadowMapRender;
    bool m_disableDepthTest;
};

class BillboardRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    bool        m_sort;

    BillboardRendererPlugin(ParticleSystem::Emitter& emitter);
    BillboardRendererPlugin(ParticleSystem::Emitter& emitter, const std::string& textureName, const std::string& shaderName, bool disableDepthTest);
};

class XYAlignedRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    bool        m_sort;
    bool        m_localSpace;

    XYAlignedRendererPlugin(ParticleSystem::Emitter& emitter);
    XYAlignedRendererPlugin(ParticleSystem::Emitter& emitter, const std::string& textureName, const std::string& shaderName, bool disableDepthTest);
};

class VelocityAlignedRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);

    size_t GetPrivateDataSize() const;
    void   InitializeParticle(Particle* p, void* data) const;
public:
    struct PrivateData
    {
        float aspect;
    };

    std::string  m_textureName;
    std::string  m_shaderName;
    bool         m_sort;
    int          m_anchor, m_rotation;
    range<float> m_aspect;

    VelocityAlignedRendererPlugin(ParticleSystem::Emitter& emitter);
};

class HeatSaturationRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    bool        m_sort;
    bool        m_xyAligned;
    float       m_distanceCutoff;
    float       m_distortion;
    float       m_saturation;

    HeatSaturationRendererPlugin(ParticleSystem::Emitter& emitter);
    HeatSaturationRendererPlugin(ParticleSystem::Emitter& emitter, const std::string& textureName, const std::string& shaderName, bool disableDepthTest, bool xyAligned);
};

class KitesRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void   CheckParameter(int id);
    size_t GetPrivateDataSize() const;
    void   InitializeParticle(Particle* p, void* data) const;
public:
    struct PrivateData
    {
        float m_tailSpeed;
    };

    std::string  m_textureName;
    std::string  m_shaderName;
    bool         m_sort;
    int          m_rotation;
    float        m_tailSize;
    range<float> m_tailSpeed;

    KitesRendererPlugin(ParticleSystem::Emitter& emitter);
    KitesRendererPlugin(ParticleSystem::Emitter& emitter, const std::string& textureName, const std::string& shaderName, bool disableDepthTest, float tailSize);
};

class ChainRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;

    ChainRendererPlugin(ParticleSystem::Emitter& emitter);
};

class XYAlignedChainRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    bool        m_sort;
    bool        m_localSpace;

    XYAlignedChainRendererPlugin(ParticleSystem::Emitter& emitter);
};

class StretchedTextureChainRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    bool        m_renderEmitter;
    float       m_emitterSize;

    StretchedTextureChainRendererPlugin(ParticleSystem::Emitter& emitter);
};

class LineRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    float       m_maxTime;

    LineRendererPlugin(ParticleSystem::Emitter& emitter);
};

class BumpMapRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_baseTextureName;
    std::string m_bumpTextureName;
    std::string m_shaderName;
    bool        m_sort;
    Color       m_specular;
    Color       m_emissive;

    BumpMapRendererPlugin(ParticleSystem::Emitter& emitter);
};

class VolumetricRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_maskTextureName;
    std::string m_baseTextureName;
    std::string m_bumpTextureName;
    std::string m_shaderName;
    float       m_textureSize;
    Vector2     m_scroll;
    bool        m_sort;
    Color       m_specular;
    Color       m_emissive;

    VolumetricRendererPlugin(ParticleSystem::Emitter& emitter);
};

class HardwareBillboardsRendererPlugin : public RendererPlugin, public CommonRendererParameters
{
    void CheckParameter(int id);
public:
    std::string m_textureName;
    std::string m_shaderName;
    int         m_renderMode;
    int         m_geometryMode;
    int         m_anchorPoint;
    float       m_aspect;
    int         m_rotation;
    Vector4     m_textureCoords;

    std::pair<float, Color> m_colors[3];
    std::pair<float, float> m_sizes[3];

    HardwareBillboardsRendererPlugin(ParticleSystem::Emitter& emitter);
};

}
#endif