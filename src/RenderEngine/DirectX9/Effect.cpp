#include <cassert>
#include "RenderEngine/DirectX9/Resources.h"
#include "General/Log.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

static const struct HandleMapping
{
    char*                       semantic;
    D3DXHANDLE EffectHandles::* handle;
} HandleMappings[] = {
    {"PROJECTION",          &EffectHandles::Projection},
    {"WORLD",               &EffectHandles::World},
    {"WORLDVIEW",           &EffectHandles::WorldView},
    {"WORLDVIEWPROJECTION", &EffectHandles::WorldViewProj},
    {"VIEWPROJECTION",      &EffectHandles::ViewProj},
    {"VIEW",                &EffectHandles::View},
    {"WORLDINVERSE",        &EffectHandles::WorldInv},
    {"VIEWINVERSE",         &EffectHandles::ViewInv},
    {"WORLDVIEWINVERSE",    &EffectHandles::WorldViewInv},
    {"WORLDTOSHADOW",       &EffectHandles::WorldToShadow},
    {"EYE_POSITION",        &EffectHandles::EyePos},
    {"EYE_OBJ_POSITION",    &EffectHandles::EyePosObj},
    {"RESOLUTION_CONSTANTS",&EffectHandles::ResolutionConstants},
    {"GLOBAL_AMBIENT",      &EffectHandles::LightAmbient},
    {"DIR_LIGHT_VEC_0",     &EffectHandles::Light0Vector},
    {"DIR_LIGHT_OBJ_VEC_0", &EffectHandles::Light0ObjVector},
    {"DIR_LIGHT_DIFFUSE_0", &EffectHandles::Light0Diffuse},
    {"DIR_LIGHT_SPECULAR_0",&EffectHandles::Light0Specular},
    {"SPH_LIGHT_ALL",       &EffectHandles::LightSphAll},
    {"SPH_LIGHT_FILL",      &EffectHandles::LightSphFill},
    {"LIGHT_SCALE",         &EffectHandles::LightScale},
    {"FOG_VALS",            &EffectHandles::FogVals},
    {"DISTANCE_FADE_VALS",  &EffectHandles::DistanceFadeVals},
    {"TIME",                &EffectHandles::Time},
    {"SKY_CUBE_TEXTURE",    &EffectHandles::SkyCubeTexture},
    {"FOW_TEXTURE",         &EffectHandles::FowTexture},
    {"CLOUD_TEXTURE",       &EffectHandles::CloudTexture},
    {"FOW_TEX_U",           &EffectHandles::FowTexU},
    {"FOW_TEX_V",           &EffectHandles::FowTexV},
    {"CLOUD_TEX_U",         &EffectHandles::CloudTexU},
    {"CLOUD_TEX_V",         &EffectHandles::CloudTexV},
    {"WIND_BEND_VECTOR",    &EffectHandles::WindBendFactor},
    {"WIND_GRASS_PARAMS",   &EffectHandles::WindGrassParams},
    {"SKINMATRIXARRAY",     &EffectHandles::SkinMatrixArray},
    {"SHADOWMAP",           &EffectHandles::ShadowTexture},
    {"SHADOWMAP_INV_RES",   &EffectHandles::ShadowTextureInvRes},
    {"ALPHA_TEST_SHADOW",   &EffectHandles::AlphaTestShadowTexture},
    {"WIND_BEND_VECTOR",    &EffectHandles::WindBendFactor},
    {"WORLDTOLIGHTFIELD",   &EffectHandles::WorldToLightField},
    {"LIGHTFIELDTEXTURE",   &EffectHandles::LightFieldTexture},
    {NULL}
};

HRESULT BaseEffect::OnLostDevice()
{
    return m_pEffect->OnLostDevice();
}

HRESULT BaseEffect::OnResetDevice()
{
    return m_pEffect->OnResetDevice();
}

BaseEffect::BaseEffect(ID3DXEffect* pEffect, BaseEffect** effects)
    : m_pEffect(pEffect)
{
    // Load the global parameters
    for (int i = 0; HandleMappings[i].semantic != NULL; i++)
    {
        m_handles.*HandleMappings[i].handle = m_pEffect->GetParameterBySemantic(NULL, HandleMappings[i].semantic);
    }

    if (effects != NULL)
    {
        // Add effect to list
        m_pNext = *effects;
        m_pPrev = effects;
        if (m_pNext != NULL)
        {
            m_pNext->m_pPrev = &m_pNext;
        }
        *effects = this;
    }
}

BaseEffect::~BaseEffect()
{
    // Unlink from list
    *m_pPrev = m_pNext;
    if (m_pNext != NULL)
    {
        m_pNext->m_pPrev = m_pPrev;
    }
}

/********************************************************************/
/********************************************************************/
/********************************************************************/

bool Effect::IsParameterUsed(D3DXHANDLE hParam) const
{
    return (m_pEffect->IsParameterUsed(hParam, m_selected->m_handle) != FALSE);
}

// Select the highest technique no higher than the specified detail level
void Effect::SetShaderDetail(ShaderDetail detail)
{
    m_shaderDetail = detail;
    while (detail > 0 && m_techniques[detail].m_handle == NULL)
    {
        detail--;
    }
    
    m_selected = &m_techniques[detail];
    m_pEffect->SetTechnique(m_selected->m_handle);
}

void Effect::Parse(bool isUaW, int vendorId, bool isShadowMap)
{
    static const struct {
        const char* name;
        RenderPhase phase;
    }
    Phases[] = {
        {"TERRAIN",     PHASE_TERRAIN},
        {"TERRAINMESH", PHASE_TERRAINMESH},
        {"OPAQUE",      PHASE_OPAQUE},
        {"SEMIOPAQUE",  PHASE_OPAQUE},      // Semi-opaque my ass
        {"SHADOW",      PHASE_SHADOW},
        {"WATER",       PHASE_WATER},
        {"TRANSPARENT", PHASE_TRANSPARENT},
        {"OCCLUDED",    PHASE_OCCLUDED},
        {"HEAT",        PHASE_HEAT},
        {"FISSURE",     PHASE_FISSURE},
        {"BLOOM",       PHASE_BLOOM},
        {NULL}
    };

    static const char* EaW_ShaderLODs[NUM_SHADER_DETAIL_LEVELS] = {
        "FIXEDFUNCTION",
        "DX8",
        "DX8ATI",
        "DX9",
    };

    static const char* UaW_ShaderLODs[NUM_SHADER_DETAIL_LEVELS] = {
        NULL,
        "LOW",
        "HIGH",
        "HIGHEST",
    };

    // Read the properties
    LPCSTR strPhase = "", strVertexType = "", strVertexProc = "";
    BOOL   bZSort = FALSE;
    m_numBonesPerVertex = 0;

    D3DXHANDLE hStringGroup = m_pEffect->GetParameterByName(NULL, "_ALAMO_STRING_GROUP");
    if (hStringGroup == NULL)
    {
        m_pEffect->GetString("_ALAMO_RENDER_PHASE", &strPhase);
        m_pEffect->GetString("_ALAMO_VERTEX_PROC",  &strVertexProc);
        m_pEffect->GetString("_ALAMO_VERTEX_TYPE",  &strVertexType);
    }
    else
    {
        m_pEffect->GetString(m_pEffect->GetAnnotationByName(hStringGroup, "_ALAMO_RENDER_PHASE"), &strPhase);
        m_pEffect->GetString(m_pEffect->GetAnnotationByName(hStringGroup, "_ALAMO_VERTEX_PROC"),  &strVertexProc);
        m_pEffect->GetString(m_pEffect->GetAnnotationByName(hStringGroup, "_ALAMO_VERTEX_TYPE"),  &strVertexType);
    }
    m_pEffect->GetBool("_ALAMO_Z_SORT",           &bZSort);
    m_pEffect->GetInt ("_ALAMO_BONES_PER_VERTEX", &m_numBonesPerVertex);
    m_numBonesPerVertex = min(max(m_numBonesPerVertex, 0), 4);

    // By-name parameters
    m_handles.Colorization = m_pEffect->GetParameterByName(NULL, "Colorization");
    m_handles.Color        = m_pEffect->GetParameterByName(NULL, "Color");

    // Parse the properties
    m_phase = PHASE_OPAQUE;
    for (int i = 0; Phases[i].name != NULL; i++)
    {
        if (_stricmp(Phases[i].name, strPhase) == 0)
        {
            m_phase = Phases[i].phase;
            break;
        }
    }

    m_vertexType   = strVertexProc;
    m_vertexFormat = VertexManager::GetVertexFormat(strVertexType);
    m_zSort        = (bZSort != FALSE);

    // Load the techniques and passes
    D3DXHANDLE hTech;
    for (int i = 0; (hTech = m_pEffect->GetTechnique(i)) != NULL; i++)
    {
        if (FAILED(m_pEffect->ValidateTechnique(hTech)))
        {
            continue;
        }

        // This is a valid technique, process it
        TechniqueDesc techdesc;
        techdesc.m_handle   = hTech;
        techdesc.m_skinType = SKIN_NONE;

        // Parse vendor type, if any
        D3DXHANDLE hVendor;
        if ((hVendor = m_pEffect->GetAnnotationByName(hTech, "VENDOR_SPECIFIC")) != NULL)
        {
            int matchVendorId;
            if (SUCCEEDED(m_pEffect->GetInt(hVendor, &matchVendorId)) && vendorId != -1 && matchVendorId != vendorId)
            {
                // Not a valid technique for this vendor, continue with next one
                continue;
            }
        }

        // Load the passes
        D3DXTECHNIQUE_DESC desc;
        m_pEffect->GetTechniqueDesc(hTech, &desc);
        techdesc.m_passes.resize(desc.Passes);
        for (UINT i = 0; i < desc.Passes; i++)
        {
            techdesc.m_passes[i].m_flags = 0;

            BOOL cleanup;
            D3DXHANDLE hPass = m_pEffect->GetPass(hTech, i);
            D3DXHANDLE hAnno = m_pEffect->GetAnnotationByName(hPass, "AlamoCleanup");
            if (SUCCEEDED(m_pEffect->GetBool(hAnno, &cleanup)) && cleanup)
            {
                // This is a cleanup pass
                techdesc.m_passes[i].m_flags |= PASS_CLEANUP;
            }
        }

        // Parse Skin type
        if (m_pEffect->IsParameterUsed(m_handles.SkinMatrixArray, hTech))
        {
            techdesc.m_skinType = SKIN_HARDWARE;
        }
        else
        {
            BOOL cpuSkin;
            D3DXHANDLE hAnno = m_pEffect->GetAnnotationByName(hTech, "CPUSKIN");
            if (SUCCEEDED(m_pEffect->GetBool(hAnno, &cpuSkin)) && cpuSkin)
            {
                techdesc.m_skinType = SKIN_SOFTWARE;
            }
        }

        // Parse shader LOD annotation
        D3DXHANDLE hAnno;
        if ((hAnno = m_pEffect->GetAnnotationByName(hTech, "LOD")) != NULL)
        {
            int lod = 0;

            LPCSTR strShaderLOD;
            if (SUCCEEDED(m_pEffect->GetString(hAnno, &strShaderLOD)))
            {
                const char** ShaderLODs = (isUaW ? UaW_ShaderLODs : EaW_ShaderLODs);
                for (lod = 0; lod < NUM_SHADER_DETAIL_LEVELS; lod++)
                {
                    if (ShaderLODs[lod] != NULL && _stricmp(ShaderLODs[lod], strShaderLOD) == 0)
                    {
                        break;
                    }
                }
                
                if (lod == NUM_SHADER_DETAIL_LEVELS)
                {
                    lod = 0;
                }
            }

            // Store the technique description
            if (m_techniques[lod].m_handle == NULL)
            {
                m_techniques[lod] = techdesc;
            }
        }
        else if (isUaW)
        {
            // Non-LOD-ed techniques are lowest quality
            m_techniques[0] = techdesc;
        }
    }

    // Fix the wind bending scale
    m_pEffect->SetFloat("BendScale", 1.0f);
}

Effect::Effect(BaseEffect** effects, ID3DXEffect* pEffect, bool isUaW, int vendorId, bool isShadowMap)
    : BaseEffect(pEffect, effects)
{
    // Clear effect
    m_selected = NULL;
    for (int i = 0; i < NUM_SHADER_DETAIL_LEVELS; i++)
    {
        m_techniques[i].m_handle   = NULL;
        m_techniques[i].m_skinType = SKIN_NONE;
    }

    Parse(isUaW, vendorId, isShadowMap);
    SetShaderDetail(SHADERDETAIL_FIXEDFUNCTION);
}

/********************************************************************/
/********************************************************************/
/********************************************************************/

void PhaseEffect::Begin()
{
    if (m_valid)
    {
        UINT nPasses;
        m_pEffect->Begin(&nPasses, D3DXFX_DONOTSAVESTATE);
        m_pEffect->BeginPass(0);
        m_pEffect->EndPass();
    }
}

void PhaseEffect::End()
{
    if (m_valid)
    {
        m_pEffect->BeginPass(1);
        m_pEffect->EndPass();
        m_pEffect->End();
    }
}

PhaseEffect::PhaseEffect(BaseEffect** effects, ID3DXEffect* pEffect)
    : BaseEffect(pEffect, effects)
{
    m_valid     = false;
    m_supported = false;

    // Validate effect
    D3DXEFFECT_DESC desc;
    m_pEffect->GetDesc(&desc);
    if (desc.Techniques == 1)
    {
        D3DXHANDLE hTech = m_pEffect->GetTechnique(0);
        D3DXTECHNIQUE_DESC desc;
        m_pEffect->GetTechniqueDesc(hTech, &desc);
        if (desc.Passes == 2)
        {
            m_valid = true;
            if (SUCCEEDED(m_pEffect->ValidateTechnique(hTech)))
            {
                m_supported = true;
                m_pEffect->SetTechnique(hTech);
            }
        }
    }
}

} }