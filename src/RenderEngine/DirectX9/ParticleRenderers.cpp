#include "RenderEngine/DirectX9/ParticleRenderers.h"
#include "General/Log.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

static const char* PARTICLES_SHADER_DIR = "Particle/";

#pragma pack(1)
struct ParticleRenderer::ParticleVertex
{
    Vector3  position;
    D3DCOLOR color;
    Vector2  texCoord;
};

struct ParticleRenderer::ParticlePrimitiveVertex { ParticleVertex v[4]; };
struct ParticleRenderer::ParticlePrimitiveIndex  { uint16_t       i[6]; };
#pragma pack()

string ParticleRenderer::GetEffectName(const string& shader) const
{
    return (m_engine.IsUaW()) ? PARTICLES_SHADER_DIR + shader : shader;
}

RenderPhase ParticleRenderer::GetRenderPhase() const
{
    return PHASE_TRANSPARENT;
}

ParticleRenderer::ParticleRenderer(RenderEngine& engine)
    : m_engine(engine)
{
}

//
// QuadParticleRenderer
//
void QuadParticleRenderer::AllocatePrimitives(size_t count)
{
    m_vertices .resize(m_vertices.size() + count);
    m_indices  .resize(m_indices .size() + count);
    for (size_t i = m_indices.size() - count; i < m_indices.size(); i++)
    {
        // Initialize vertex
        for (size_t j = 0; j < 4; j++) {
            m_vertices[i].v[j].position = Vector3(0,0,0);
            m_vertices[i].v[j].color    = D3DCOLOR_ARGB(0,0,0,0);
        }

        // Initialize index
        m_indices[i].i[0] = (uint16_t)(i * 4 + 0);
        m_indices[i].i[1] = (uint16_t)(i * 4 + 1);
        m_indices[i].i[2] = (uint16_t)(i * 4 + 2);
        m_indices[i].i[3] = (uint16_t)(i * 4 + 2);
        m_indices[i].i[4] = (uint16_t)(i * 4 + 1);
        m_indices[i].i[5] = (uint16_t)(i * 4 + 3);
    }
}

void QuadParticleRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const
{
    ParticlePrimitiveVertex& q = m_vertices[index];
    
    // Rotate quad
    Matrix transform = Matrix(Quaternion(Vector3(0,0,1), p.rotation * 2*PI));
    transform.setTranslation(p.position);

    q.v[0].position = Vector4(-p.size, p.size,0,1) * transform;
    q.v[1].position = Vector4(-p.size,-p.size,0,1) * transform;
    q.v[2].position = Vector4( p.size, p.size,0,1) * transform;
    q.v[3].position = Vector4( p.size,-p.size,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void QuadParticleRenderer::AllocatePrimitive(size_t index)
{
}

void QuadParticleRenderer::FreePrimitive(size_t index)
{
    // Since we still render killed particles (to avoid complex sparse
    // administration), set color to transparent black and collapse
    // quads to a point
    for (size_t j = 0; j < 4; j++) {
        m_vertices[index].v[j].position = Vector3(0,0,0);
        m_vertices[index].v[j].color    = D3DCOLOR_ARGB(0,0,0,0);
    }
}

void QuadParticleRenderer::RenderParticles(Effect& effect) const
{
    IDirect3DDevice9* pDevice = GetEngine().GetDevice();
    pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
    UINT nPasses = effect.Begin();
    for (UINT i = 0; i < nPasses; i++)
    {
        if (effect.BeginPass(i))
        {
            OverrideStates(pDevice, i);
            pDevice->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, (UINT)m_vertices.size() * 4, (UINT)m_indices.size() * 2, &m_indices[0].i[0], D3DFMT_INDEX16, &m_vertices[0].v[0], sizeof(ParticleVertex));
        }
        effect.EndPass();
    }
    effect.End();
}

QuadParticleRenderer::QuadParticleRenderer(RenderEngine& engine)
    : ParticleRenderer(engine)
{
}

//
// BillboardRenderer
//
void BillboardRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const
{
    ParticlePrimitiveVertex& q = m_vertices[index];
    
    Matrix transform =
        Matrix(Quaternion(Vector3(0,0,1), p.rotation * 2*PI)) * // Rotate
        GetEngine().GetMatrices().m_viewInv;                    // Billboard
    transform.setTranslation(p.position);

    q.v[0].position = Vector4(-p.size, p.size,0,1) * transform;
    q.v[1].position = Vector4(-p.size,-p.size,0,1) * transform;
    q.v[2].position = Vector4( p.size, p.size,0,1) * transform;
    q.v[3].position = Vector4( p.size,-p.size,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void BillboardRenderer::RenderParticles() const
{
    RenderEngine&     engine  = GetEngine();
    IDirect3DDevice9* pDevice = engine.GetDevice();

    engine.SetWorldMatrix(Matrix::Identity, m_effect);
    if (engine.IsUaW()) {
        m_effect->GetEffect()->SetTexture(m_hBaseTexture, m_texture->GetTexture());
    } else {
        pDevice->SetTexture(0, m_texture->GetTexture());
    }
    pDevice->SetRenderState(D3DRS_ZENABLE, !m_plugin.m_disableDepthTest);
    QuadParticleRenderer::RenderParticles(*m_effect);
}

BillboardRenderer::BillboardRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    m_texture = engine.LoadTexture(plugin.m_textureName);
    m_effect  = engine.LoadEffect(GetEffectName(plugin.m_shaderName));
    m_hBaseTexture = m_effect->GetEffect()->GetParameterByName(NULL, "BaseTexture");
}

//
// VelocityAlignedRenderer
//
void VelocityAlignedRenderer::UpdatePrimitive(size_t index, const Particle& p, void* _data) const
{
    VelocityAlignedRendererPlugin::PrivateData* data = static_cast<VelocityAlignedRendererPlugin::PrivateData*>(_data);

    ParticlePrimitiveVertex& q = m_vertices[index];

    // Determine anchor offset
    Vector3 trans(0,0,0);
    switch (m_plugin.m_anchor) {
        case 0: break; // Center
        case 1: trans.y =  p.size; break; // Bottom
        case 2: trans.y = -p.size; break; // Top
    }

    // Scale, rotate and translate particle
    Matrix transform = Matrix(
        Vector3(data->aspect,1,1),
        Quaternion(Vector3(0,0,-1), m_plugin.m_rotation * PI/2),
        trans );

    // Calculate postion of camera in before-rotation-space (i.e., transform by inverse rotation)
    const Vector3 cam = (GetEngine().GetCamera().m_position - p.position) * Matrix(
            Quaternion(Vector3(0,0,-1), p.velocity.zAngle() - PI/2) *
            Quaternion(Vector3(-1,0,0), p.velocity.tilt())
        );
    
    // Now rotate around +Y to face camera and then rotate to align +Y with the velocity
    transform *=
        Matrix( Quaternion(Vector3(0,-1,0), atan2(cam.z, cam.x) - PI/2) ) *
        Matrix(
            Quaternion(Vector3(1,0,0), p.velocity.tilt()) *
            Quaternion(Vector3(0,0,1), p.velocity.zAngle() - PI/2)
        );

    // And move the particle into position
    transform.addTranslation(p.position);

    q.v[0].position = Vector4(-p.size, p.size,0,1) * transform;
    q.v[1].position = Vector4(-p.size,-p.size,0,1) * transform;
    q.v[2].position = Vector4( p.size, p.size,0,1) * transform;
    q.v[3].position = Vector4( p.size,-p.size,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void VelocityAlignedRenderer::RenderParticles() const
{
    RenderEngine&     engine  = GetEngine();
    IDirect3DDevice9* pDevice = engine.GetDevice();

    engine.SetWorldMatrix(Matrix::Identity, m_effect);
    if (engine.IsUaW()) {
        m_effect->GetEffect()->SetTexture(m_hBaseTexture, m_texture->GetTexture());
    } else {
        pDevice->SetTexture(0, m_texture->GetTexture());
    }
    pDevice->SetRenderState(D3DRS_ZENABLE, !m_plugin.m_disableDepthTest);
    QuadParticleRenderer::RenderParticles(*m_effect);
}

VelocityAlignedRenderer::VelocityAlignedRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    m_texture = engine.LoadTexture(plugin.m_textureName);
    m_effect  = engine.LoadEffect(GetEffectName(plugin.m_shaderName));
    m_hBaseTexture = m_effect->GetEffect()->GetParameterByName(NULL, "BaseTexture");
}

//
// XYAlignedRenderer
//
void XYAlignedRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const
{
    ParticlePrimitiveVertex& q = m_vertices[index];
    
    Matrix transform = Matrix(Quaternion(Vector3(0,0,1), p.rotation * 2*PI));
    transform.setTranslation(p.position);

    q.v[0].position = Vector4(-p.size, p.size,0,1) * transform;
    q.v[1].position = Vector4(-p.size,-p.size,0,1) * transform;
    q.v[2].position = Vector4( p.size, p.size,0,1) * transform;
    q.v[3].position = Vector4( p.size,-p.size,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void XYAlignedRenderer::RenderParticles() const
{
    RenderEngine&     engine  = GetEngine();
    IDirect3DDevice9* pDevice = engine.GetDevice();

    engine.SetWorldMatrix(Matrix::Identity, m_effect);
    if (engine.IsUaW()) {
        m_effect->GetEffect()->SetTexture(m_hBaseTexture, m_texture->GetTexture());
    } else {
        pDevice->SetTexture(0, m_texture->GetTexture());
    }
    pDevice->SetRenderState(D3DRS_ZENABLE, !m_plugin.m_disableDepthTest);
    QuadParticleRenderer::RenderParticles(*m_effect);
}

XYAlignedRenderer::XYAlignedRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    m_texture = engine.LoadTexture(plugin.m_textureName);
    m_effect  = engine.LoadEffect(GetEffectName(plugin.m_shaderName));
    m_hBaseTexture = m_effect->GetEffect()->GetParameterByName(NULL, "BaseTexture");
}

//
// HeatSaturationRenderer
//
RenderPhase HeatSaturationRenderer::GetRenderPhase() const
{
    return PHASE_HEAT;
}

void HeatSaturationRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const
{
    ParticlePrimitiveVertex& q = m_vertices[index];
    
    Matrix transform(Quaternion(Vector3(0,0,1), p.rotation * 2*PI)); // Rotate
    if (!m_plugin.m_xyAligned) {
        // Billboard
        transform *= GetEngine().GetMatrices().m_viewInv;
    }
    transform.setTranslation(p.position);

    q.v[0].position = Vector4(-p.size, p.size,0,1) * transform;
    q.v[1].position = Vector4(-p.size,-p.size,0,1) * transform;
    q.v[2].position = Vector4( p.size, p.size,0,1) * transform;
    q.v[3].position = Vector4( p.size,-p.size,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void HeatSaturationRenderer::OverrideStates(IDirect3DDevice9* pDevice, int pass) const
{
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void HeatSaturationRenderer::RenderParticles() const
{
    RenderEngine&     engine  = GetEngine();
    IDirect3DDevice9* pDevice = engine.GetDevice();
    ID3DXEffect*      effect  = m_effect->GetEffect();

    engine.SetWorldMatrix(Matrix::Identity, m_effect);
    if (engine.IsUaW()) {
        effect->SetTexture(m_hBaseTexture,  m_texture->GetTexture());
        effect->SetFloat(m_hDistanceCutoff, m_plugin.m_distanceCutoff);
        effect->SetFloat(m_hDistortion,     m_plugin.m_distortion);
        effect->SetFloat(m_hSaturation,     m_plugin.m_saturation);
    } else {
        pDevice->SetTexture(0, m_texture->GetTexture());
    }

    pDevice->SetRenderState(D3DRS_ZENABLE, !m_plugin.m_disableDepthTest);
    QuadParticleRenderer::RenderParticles(*m_effect);
}

HeatSaturationRenderer::HeatSaturationRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    m_texture         = engine.LoadTexture(plugin.m_textureName);
    m_effect          = engine.LoadEffect(GetEffectName(plugin.m_shaderName));

    ID3DXEffect* effect = m_effect->GetEffect();
    m_hBaseTexture    = effect->GetParameterByName(NULL, "BaseTexture");
    m_hDistanceCutoff = effect->GetParameterByName(NULL, "DistanceCutoff");
    m_hDistortion     = effect->GetParameterByName(NULL, "DistortionScale");
    m_hSaturation     = effect->GetParameterByName(NULL, "SaturationScale");
}

//
// KitesRenderer
//
void KitesRenderer::UpdatePrimitive(size_t index, const Particle& p, void* _data) const
{
    VelocityAlignedRendererPlugin::PrivateData* data = static_cast<VelocityAlignedRendererPlugin::PrivateData*>(_data);

    ParticlePrimitiveVertex& q = m_vertices[index];

    // Rotate particle
    Matrix transform(Quaternion(Vector3(0,0,-1), m_plugin.m_rotation * PI/2 - 3*PI/4));

    // Calculate postion of camera in before-rotation-space (i.e., transform by inverse rotation)
    const Vector3 cam = (GetEngine().GetCamera().m_position - p.position) * Matrix(
            Quaternion(Vector3(0,0,-1), p.velocity.zAngle() - PI/2) *
            Quaternion(Vector3(-1,0,0), p.velocity.tilt())
        );
    
    // Now rotate around +Y to face camera and then rotate to align +Y with the velocity
    transform *=
        Matrix( Quaternion(Vector3(0,-1,0), atan2(cam.z, cam.x) - PI/2) ) *
        Matrix(
            Quaternion(Vector3(1,0,0), p.velocity.tilt()) *
            Quaternion(Vector3(0,0,1), p.velocity.zAngle() - PI/2)
        );

    // And move the particle into position
    transform.addTranslation(p.position);

    float tail = m_plugin.m_tailSize;

    q.v[0].position = Vector4(-(p.size + tail)/ 2, (p.size + tail) / 2,0,1) * transform;
    q.v[1].position = Vector4(-p.size / 2,-p.size / 2,0,1) * transform;
    q.v[2].position = Vector4( p.size / 2, p.size / 2,0,1) * transform;
    q.v[3].position = Vector4( p.size / 2,-p.size / 2,0,1) * transform;
    q.v[0].texCoord = Vector2(p.texCoords.x, p.texCoords.y);
    q.v[1].texCoord = Vector2(p.texCoords.x, p.texCoords.y + p.texCoords.w);
    q.v[2].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y);
    q.v[3].texCoord = Vector2(p.texCoords.x + p.texCoords.z, p.texCoords.y + p.texCoords.w);
    q.v[0].color = q.v[1].color = q.v[2].color = 
    q.v[3].color = D3DCOLOR_COLORVALUE(p.color.r, p.color.g, p.color.b, p.color.a);
}

void KitesRenderer::RenderParticles() const
{
    RenderEngine&     engine  = GetEngine();
    IDirect3DDevice9* pDevice = engine.GetDevice();

    engine.SetWorldMatrix(Matrix::Identity, m_effect);
    if (engine.IsUaW()) {
        m_effect->GetEffect()->SetTexture(m_hBaseTexture, m_texture->GetTexture());
    } else {
        pDevice->SetTexture(0, m_texture->GetTexture());
    }
    pDevice->SetRenderState(D3DRS_ZENABLE, !m_plugin.m_disableDepthTest);
    QuadParticleRenderer::RenderParticles(*m_effect);
}

KitesRenderer::KitesRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    m_texture = engine.LoadTexture(plugin.m_textureName);
    m_effect  = engine.LoadEffect(GetEffectName(plugin.m_shaderName));
    m_hBaseTexture = m_effect->GetEffect()->GetParameterByName(NULL, "BaseTexture");
}

//
// VolumetricRenderer
//
void VolumetricRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void VolumetricRenderer::RenderParticles() const {}
VolumetricRenderer::VolumetricRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// ChainRenderer
//
void ChainRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void ChainRenderer::RenderParticles() const {}
ChainRenderer::ChainRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// XYAlignedChainRenderer
//
void XYAlignedChainRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void XYAlignedChainRenderer::RenderParticles() const {}
XYAlignedChainRenderer::XYAlignedChainRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// StretchedTextureChainRenderer
//
void StretchedTextureChainRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void StretchedTextureChainRenderer::RenderParticles() const {}
StretchedTextureChainRenderer::StretchedTextureChainRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// HardwareBillboardsRenderer
//
void HardwareBillboardsRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void HardwareBillboardsRenderer::RenderParticles() const {}
HardwareBillboardsRenderer::HardwareBillboardsRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// BumpMapRenderer
//
void BumpMapRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void BumpMapRenderer::RenderParticles() const {}
BumpMapRenderer::BumpMapRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}

//
// LineRenderer
//
void LineRenderer::UpdatePrimitive(size_t index, const Particle& p, void* data) const {}
void LineRenderer::RenderParticles() const {}
LineRenderer::LineRenderer(RenderEngine& engine, const PluginType& plugin)
    : QuadParticleRenderer(engine), m_plugin(plugin)
{
    const string& name = plugin.GetEmitter().GetSystem().GetName();
    Log::WriteInfo("\"%s\" uses currently unsupported renderer plugin", name.c_str());
}
}
}