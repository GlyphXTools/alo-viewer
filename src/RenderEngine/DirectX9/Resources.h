#ifndef RESOURCES_DX9_H
#define RESOURCES_DX9_H

#include "Assets/Files.h"
#include "General/GameTypes.h"
#include "General/ExactTypes.h"
#include "RenderEngine/DirectX9/VertexManager.h"
#include <vector>

namespace Alamo {
namespace DirectX9 {

// A wrapper for textures
class Texture : public IObject
{
	IDirect3DTexture9* m_pTexture;

	~Texture()
    {
        m_pTexture->Release();
    }
public:
    IDirect3DTexture9* operator->() const { assert(m_pTexture != NULL); return m_pTexture; }
    operator IDirect3DTexture9*() const   { return m_pTexture; }
    IDirect3DTexture9* GetTexture() const  { return m_pTexture; }
	
    Texture(IDirect3DTexture9* pTexture)
    {
        m_pTexture = pTexture;
    }
};

enum RenderPhase
{
    PHASE_TERRAIN,
    PHASE_TERRAINMESH,
    PHASE_OPAQUE,
    PHASE_SHADOW,
    PHASE_WATER,
    PHASE_TRANSPARENT,
    PHASE_OCCLUDED,
    PHASE_HEAT,
    PHASE_FISSURE,
    PHASE_BLOOM,
    NUM_RENDER_PHASES
};

enum SkinType
{
    SKIN_NONE,
    SKIN_HARDWARE,
    SKIN_SOFTWARE,
};

struct EffectHandles
{
    D3DXHANDLE Projection;
    D3DXHANDLE World;
    D3DXHANDLE WorldView;
    D3DXHANDLE WorldViewProj;
    D3DXHANDLE ViewProj;
    D3DXHANDLE View;
    D3DXHANDLE WorldInv;
    D3DXHANDLE ViewInv;
    D3DXHANDLE WorldViewInv;
    D3DXHANDLE WorldToShadow;
    D3DXHANDLE EyePos;
    D3DXHANDLE EyePosObj;
    D3DXHANDLE ResolutionConstants;
    D3DXHANDLE LightAmbient;
    D3DXHANDLE Light0Vector;
    D3DXHANDLE Light0ObjVector;
    D3DXHANDLE Light0Diffuse;
    D3DXHANDLE Light0Specular;
    D3DXHANDLE LightSphAll;
    D3DXHANDLE LightSphFill;
    D3DXHANDLE LightScale;
    D3DXHANDLE FogVals;
    D3DXHANDLE DistanceFadeVals;
    D3DXHANDLE Time;
    D3DXHANDLE SkyCubeTexture;
    D3DXHANDLE FowTexture;
    D3DXHANDLE CloudTexture;
    D3DXHANDLE FowTexU;
    D3DXHANDLE FowTexV;
    D3DXHANDLE CloudTexU;
    D3DXHANDLE CloudTexV;
    D3DXHANDLE WindBendFactor;
    D3DXHANDLE WindGrassParams;
    D3DXHANDLE SkinMatrixArray;
    D3DXHANDLE Colorization;
    D3DXHANDLE Color;
    D3DXHANDLE ShadowTexture;
    D3DXHANDLE ShadowTextureInvRes;
    D3DXHANDLE AlphaTestShadowTexture;
    D3DXHANDLE WindBendVector;
    D3DXHANDLE WorldToLightField;
    D3DXHANDLE LightFieldTexture;
};

class BaseEffect : public IObject
{
    BaseEffect*      m_pNext;
    BaseEffect**     m_pPrev;
protected:
    ptr<ID3DXEffect> m_pEffect;
    EffectHandles    m_handles;

    ~BaseEffect();
public:
	HRESULT OnLostDevice();
	HRESULT OnResetDevice();

    virtual void SetShaderDetail(ShaderDetail detail) {}

    BaseEffect*          Next()       const { return m_pNext; }
    const EffectHandles& GetHandles() const { return m_handles; }
    ID3DXEffect*         GetEffect()  const { return m_pEffect; }

    BaseEffect(ID3DXEffect* pEffect, BaseEffect** effects = NULL);
};

class Effect : public BaseEffect
{
public:
    // Pass flags
    static const int PASS_CLEANUP = 1;
    static const int PASS_CPUSKIN = 2;

private:
    struct PassDesc
    {
        int m_flags;
    };

    struct TechniqueDesc
    {
        D3DXHANDLE            m_handle;
        std::vector<PassDesc> m_passes;
        SkinType              m_skinType;
    };

    RenderPhase    m_phase;
    VertexFormat   m_vertexFormat;  // alD3dVertNU2U3U3, etc
    std::string    m_vertexType;    // Mesh, Skin, RSkin, etc
    bool           m_zSort;
    int            m_numBonesPerVertex;
    TechniqueDesc  m_techniques[NUM_SHADER_DETAIL_LEVELS];
    TechniqueDesc* m_selected;
    ShaderDetail   m_shaderDetail;

    void Parse(bool isUaW, int vendorId, bool isShadowMap);
public:
    const std::string& GetVertexType()  const { return m_vertexType; }
    RenderPhase  GetRenderPhase()       const { return m_phase; }
    VertexFormat GetVertexFormat()      const { return m_vertexFormat; }
    SkinType     GetSkinType()          const { return m_selected->m_skinType; }
    int          GetNumBonesPerVertex() const { return m_numBonesPerVertex; }
    bool         NeedsZSort()           const { return m_zSort; }
    bool         IsFixedFunction()      const { return m_selected == &m_techniques[SHADERDETAIL_FIXEDFUNCTION]; }
    bool         IsSupported()          const { return m_selected->m_handle != NULL; }
    bool         IsParameterUsed(D3DXHANDLE hParam) const;

    UINT Begin() {
        UINT nPasses = 0;
        if (m_selected->m_handle != NULL) {
            m_pEffect->Begin(&nPasses, D3DXFX_DONOTSAVESTATE);
        }
        return nPasses;
    };
    bool BeginPass(UINT pass) { return (m_pEffect->BeginPass(pass) == D3D_OK) && ~m_selected->m_passes[pass].m_flags & PASS_CLEANUP; }
    void EndPass()            { m_pEffect->EndPass(); }
    void End()                { m_pEffect->End(); }

    void SetShaderDetail(ShaderDetail detail);

    Effect(BaseEffect** effects, ID3DXEffect* pEffect, bool isUaW, int vendorId = -1, bool isShadowMap = false);
    Effect(DWORD ResourceID);
};

// Phase effects are stubs that set render states before and after each
// render phase. They are a strict type of effect.
class PhaseEffect : public BaseEffect
{
    bool m_valid;
    bool m_supported;

public:
    bool IsValid()     const { return m_valid; }
    bool IsSupported() const { return m_supported; }
    void Begin();
    void End();

    PhaseEffect(BaseEffect** effects, ID3DXEffect* pEffect);
};

} }
#endif