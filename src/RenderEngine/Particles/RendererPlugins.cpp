#include "RenderEngine/Particles/RendererPlugins.h"
#include "RenderEngine/Particles/ParticleSystem.h"
#include "RenderEngine/Particles/PluginDefs.h"
#include "General/Math.h"
using namespace std;

namespace Alamo
{

//
// BillboardRendererPlugin
//
void BillboardRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    END_PARAM_LIST
}

BillboardRendererPlugin::BillboardRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

BillboardRendererPlugin::BillboardRendererPlugin(ParticleSystem::Emitter& emitter, const string& textureName, const string& shaderName, bool disableDepthTest)
    : RendererPlugin(emitter), m_textureName(textureName), m_shaderName(shaderName), m_sort(false)
{
    m_disableDepthTest = disableDepthTest;
}

//
// XYAlignedRendererPlugin
//
void XYAlignedRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    PARAM_BOOL  (200, m_localSpace);
    END_PARAM_LIST
}

XYAlignedRendererPlugin::XYAlignedRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

XYAlignedRendererPlugin::XYAlignedRendererPlugin(ParticleSystem::Emitter& emitter, const string& textureName, const string& shaderName, bool disableDepthTest)
    : RendererPlugin(emitter), m_textureName(textureName), m_shaderName(shaderName),
      m_sort(false), m_localSpace(false)
{
    m_disableDepthTest = disableDepthTest;
}

//
// VelocityAlignedRendererPlugin
//
void VelocityAlignedRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    PARAM_INT   (200, m_anchor);
    PARAM_FLOAT (201, m_aspect.min);
    PARAM_FLOAT (202, m_aspect.max);
    PARAM_INT   (203, m_rotation);
    END_PARAM_LIST
}

size_t VelocityAlignedRendererPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void VelocityAlignedRendererPlugin::InitializeParticle(Particle* p, void* _data) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    data->aspect = GetRandom(m_aspect.min, m_aspect.max);
}

VelocityAlignedRendererPlugin::VelocityAlignedRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// KitesRendererPlugin
//
void KitesRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    PARAM_INT   (200, m_rotation);
    PARAM_FLOAT (201, m_tailSize);
    PARAM_FLOAT (202, m_tailSpeed.min);
    PARAM_FLOAT (203, m_tailSpeed.max);
    END_PARAM_LIST
}

size_t KitesRendererPlugin::GetPrivateDataSize() const
{
    return sizeof(PrivateData);
}

void KitesRendererPlugin::InitializeParticle(Particle* p, void* _data) const
{
    PrivateData* data = static_cast<PrivateData*>(_data);
    //data->aspect = GetRandom(m_aspect.min, m_aspect.max);
}

KitesRendererPlugin::KitesRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

KitesRendererPlugin::KitesRendererPlugin(ParticleSystem::Emitter& emitter, const std::string& textureName, const std::string& shaderName, bool disableDepthTest, float tailSize)
    : RendererPlugin(emitter),
      m_textureName(textureName), m_shaderName(shaderName), m_tailSize(tailSize),
      m_sort(false), m_rotation(0)
{
    m_disableDepthTest = disableDepthTest;
    m_tailSpeed.min    = 0;
    m_tailSpeed.max    = 0;
}

//
// HeatSaturationRendererPlugin
//
void HeatSaturationRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    PARAM_FLOAT (200, m_distanceCutoff);
    PARAM_FLOAT (201, m_distortion);
    PARAM_FLOAT (202, m_saturation);
    END_PARAM_LIST
}

HeatSaturationRendererPlugin::HeatSaturationRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

HeatSaturationRendererPlugin::HeatSaturationRendererPlugin(ParticleSystem::Emitter& emitter, const string& textureName, const string& shaderName, bool disableDepthTest, bool xyAligned)
    : RendererPlugin(emitter), m_textureName(textureName), m_shaderName(shaderName), m_xyAligned(xyAligned),
      m_sort(false), m_distanceCutoff(10000.0f), m_distortion(1.0f), m_saturation(1.0f)
{
    m_disableDepthTest = disableDepthTest;
}

//
// VolumetricRendererPlugin
//
void VolumetricRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_maskTextureName);
    PARAM_STRING(101, m_baseTextureName);
    PARAM_STRING(102, m_bumpTextureName);
    PARAM_STRING(103, m_shaderName);
    PARAM_FLOAT (104, m_textureSize);
    PARAM_FLOAT (105, m_scroll.x);
    PARAM_FLOAT (106, m_scroll.y);
    PARAM_BOOL  (107, m_sort);
    PARAM_COLOR (108, m_specular);
    PARAM_COLOR (109, m_emissive);
    END_PARAM_LIST
}

VolumetricRendererPlugin::VolumetricRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// ChainRendererPlugin
//
void ChainRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(102, m_shaderName);
    END_PARAM_LIST
}

ChainRendererPlugin::ChainRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// StretchedTextureChainRendererPlugin
//
void StretchedTextureChainRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (104, m_renderEmitter);
    PARAM_FLOAT (200, m_emitterSize);
    END_PARAM_LIST
}

StretchedTextureChainRendererPlugin::StretchedTextureChainRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// XYAlignedChainRendererPlugin
//
void XYAlignedChainRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_STRING(101, m_shaderName);
    PARAM_BOOL  (102, m_sort);
    PARAM_BOOL  (200, m_localSpace);
    END_PARAM_LIST
}

XYAlignedChainRendererPlugin::XYAlignedChainRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// LineRendererPlugin
//
void LineRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_textureName);
    PARAM_FLOAT (101, m_maxTime);
    PARAM_STRING(102, m_shaderName);
    END_PARAM_LIST
}

LineRendererPlugin::LineRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}

//
// BumpMapRendererPlugin
//
void BumpMapRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(100, m_baseTextureName);
    PARAM_STRING(101, m_bumpTextureName);
    PARAM_STRING(102, m_shaderName);
    PARAM_BOOL  (103, m_sort);
    PARAM_COLOR (104, m_specular);
    PARAM_COLOR (105, m_emissive);
    END_PARAM_LIST
}

BumpMapRendererPlugin::BumpMapRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
}
//
// HardwareBillboardsRendererPlugin
//
void HardwareBillboardsRendererPlugin::CheckParameter(int id)
{
    BEGIN_PARAM_LIST(id)
    PARAM_BOOL(0, m_normalRenderMode);
    PARAM_BOOL(1, m_reflectionRenderMode);
    PARAM_BOOL(2, m_refractionRenderMode);
    PARAM_BOOL(3, m_shadowMapRender);
    PARAM_BOOL(4, m_disableDepthTest);
    PARAM_STRING(6400, m_textureName);
    PARAM_STRING(6401, m_shaderName);
    PARAM_INT   (6415, m_renderMode);
    PARAM_INT   (6416, m_geometryMode);
    PARAM_INT   (6402, m_anchorPoint);
    PARAM_FLOAT (6404, m_aspect);
    PARAM_INT   (6405, m_rotation);
    PARAM_FLOAT4(6406, m_textureCoords);
    PARAM_COLOR (6407, m_colors[0].second);
    PARAM_COLOR (6408, m_colors[1].second);
    PARAM_COLOR (6409, m_colors[2].second);
    PARAM_FLOAT (6410, m_colors[1].first);
    PARAM_FLOAT (6411, m_sizes[0].second);
    PARAM_FLOAT (6412, m_sizes[1].second);
    PARAM_FLOAT (6413, m_sizes[2].second);
    PARAM_FLOAT (6414, m_sizes[1].first);
    END_PARAM_LIST
}

HardwareBillboardsRendererPlugin::HardwareBillboardsRendererPlugin(ParticleSystem::Emitter& emitter)
    : RendererPlugin(emitter)
{
    m_sizes[0].first = m_colors[0].first = 0.0f;
    m_sizes[2].first = m_colors[2].first = 1.0f;
}

}
