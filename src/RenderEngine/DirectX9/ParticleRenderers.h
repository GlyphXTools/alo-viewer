#ifndef PARTICLE_RENDERERS_H
#define PARTICLE_RENDERERS_H

#include "RenderEngine/Particles/RendererPlugins.h"
#include "RenderEngine/DirectX9/RenderEngine.h"
#include "General/ExactTypes.h"

namespace Alamo {
namespace DirectX9 {

// Base for all particle renderers
class ParticleRenderer
{
protected:
    struct ParticleVertex;
    struct ParticlePrimitiveVertex;
    struct ParticlePrimitiveIndex;

    RenderEngine& GetEngine() const { return m_engine; }
    std::string GetEffectName(const std::string& shader) const;

    ParticleRenderer(RenderEngine& engine);

public:
    virtual void AllocatePrimitives(size_t count) = 0;
    virtual void UpdatePrimitive(size_t index, const Particle& particle, void* data) const = 0;
    virtual void AllocatePrimitive(size_t index) = 0;
    virtual void FreePrimitive(size_t index) = 0;
    virtual void RenderParticles() const = 0;
    virtual RenderPhase GetRenderPhase() const;

    virtual ~ParticleRenderer() {}
private:
    RenderEngine& m_engine;
};

// Base for all particle renderers based on independent quads.
// Contains the common vertex and index management and rendering.
class QuadParticleRenderer : public ParticleRenderer
{
protected:
    Buffer<ParticlePrimitiveVertex>  m_vertices;
    Buffer<ParticlePrimitiveIndex>   m_indices;

    void RenderParticles(Effect& effect) const;
    virtual void OverrideStates(IDirect3DDevice9* pDevice, int pass) const {}

public:
    void AllocatePrimitives(size_t count);
    void UpdatePrimitive(size_t index, const Particle& particle, void* data) const;
    void AllocatePrimitive(size_t index);
    void FreePrimitive(size_t index);

    QuadParticleRenderer(RenderEngine& engine);
};

class BillboardRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::BillboardRendererPlugin PluginType;

    BillboardRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;
    ptr<Texture>      m_texture;
    ptr<Effect>       m_effect;
    D3DXHANDLE        m_hBaseTexture;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class XYAlignedRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::XYAlignedRendererPlugin PluginType;

    XYAlignedRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;
    ptr<Texture>      m_texture;
    ptr<Effect>       m_effect;
    D3DXHANDLE        m_hBaseTexture;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class VelocityAlignedRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::VelocityAlignedRendererPlugin PluginType;

    VelocityAlignedRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;
    ptr<Texture>      m_texture;
    ptr<Effect>       m_effect;
    D3DXHANDLE        m_hBaseTexture;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class HeatSaturationRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::HeatSaturationRendererPlugin PluginType;

    HeatSaturationRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;
    ptr<Texture>      m_texture;
    ptr<Effect>       m_effect;
    D3DXHANDLE        m_hBaseTexture;
    D3DXHANDLE        m_hDistanceCutoff;
    D3DXHANDLE        m_hDistortion;
    D3DXHANDLE        m_hSaturation;

    RenderPhase GetRenderPhase() const;
    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
    void OverrideStates(IDirect3DDevice9* pDevice, int pass) const;
};

class KitesRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::KitesRendererPlugin PluginType;

    KitesRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;
    ptr<Texture>      m_texture;
    ptr<Effect>       m_effect;
    D3DXHANDLE        m_hBaseTexture;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class VolumetricRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::VolumetricRendererPlugin PluginType;

    VolumetricRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class ChainRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::ChainRendererPlugin PluginType;

    ChainRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class XYAlignedChainRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::XYAlignedChainRendererPlugin PluginType;

    XYAlignedChainRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class StretchedTextureChainRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::StretchedTextureChainRendererPlugin PluginType;

    StretchedTextureChainRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class HardwareBillboardsRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::HardwareBillboardsRendererPlugin PluginType;

    HardwareBillboardsRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class BumpMapRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::BumpMapRendererPlugin PluginType;

    BumpMapRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

class LineRenderer : public QuadParticleRenderer
{
public:
    typedef Alamo::LineRendererPlugin PluginType;

    LineRenderer(RenderEngine& engine, const PluginType& plugin);

private:
    const PluginType& m_plugin;

    void UpdatePrimitive(size_t index, const Particle& p, void* data) const;
    void RenderParticles() const;
};

}
}

#endif