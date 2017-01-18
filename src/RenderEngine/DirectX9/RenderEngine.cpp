#include "RenderEngine/SphericalHarmonics.h"
#include "RenderEngine/DirectX9/Exceptions.h"
#include "RenderEngine/DirectX9/RenderObject.h"
#include "General/Log.h"
#include "General/Utils.h"
#include "resource.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

// Width and height of the ground plane
static const float GROUND_SIZE    = 2000.0f;

// Preferred (and max) Resolution of the shadow map
static const int   SHADOWMAP_SIZE = 4096;

// Preferred (and max) Resolution of the light field texture
static const int   LIGHTFIELD_SIZE = 1024;

static const struct {
    D3DFORMAT   format;
    bool        stencil;
    const char* name;
} DepthFormats[] = {
    {D3DFMT_D32,     false, "D32"},
    {D3DFMT_D15S1,   true,  "D15S1"},
    {D3DFMT_D24S8,   true,  "D24S8"},
    {D3DFMT_D24X8,   false, "D24X8"},
    {D3DFMT_D24X4S4, true,  "D24X4S4"},
    {D3DFMT_D16,     false, "D16"},
    {D3DFMT_UNKNOWN}
};

// Adapter Vendor IDs
static const DWORD VENDOR_ATI    = 0x1002;
static const DWORD VENDOR_NVIDIA = 0x10DE;

IDirect3DDevice9* RenderEngine::GetDevice() const
{
    return m_pDevice;
}

void RenderEngine::OnLostDevice()
{
    UnloadDynamicResources();
    for (BaseEffect* effect = m_effects; effect != NULL; effect = effect->Next())
    {
        effect->OnLostDevice();
    }
    m_vertexManager->OnLostDevice();
}

void RenderEngine::OnResetDevice()
{
    InitializeRenderStates();
    m_vertexManager->OnResetDevice();
    for (BaseEffect* effect = m_effects; effect != NULL; effect = effect->Next())
    {
        effect->OnResetDevice();
    }
    LoadDynamicResources();
}

void RenderEngine::InitializeRenderStates()
{
    // Initialize samplers
    for (UINT i = 0; i < m_deviceCaps.MaxSimultaneousTextures; i++)
    {
        m_pDevice->SetTexture(i, NULL);
        m_pDevice->SetSamplerState(i, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        m_pDevice->SetSamplerState(i, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        m_pDevice->SetSamplerState(i, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    }
    
    m_pDevice->SetTransform(D3DTS_VIEW,       &m_matrices.m_view);
    m_pDevice->SetTransform(D3DTS_PROJECTION, &m_matrices.m_proj);

    D3DMATERIAL9 material = {
        {1,1,1,1},
        {1,1,1,1},
        {1,1,1,1},
        {1,1,1,1},
        1
    };
    m_pDevice->SetMaterial(&material);
    m_pDevice->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE,  D3DMCS_COLOR1);
    m_pDevice->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR2);
    m_pDevice->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE,  D3DMCS_MATERIAL);
    m_pDevice->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, D3DMCS_MATERIAL);
    m_pDevice->SetRenderState(D3DRS_SHADEMODE,        D3DSHADE_GOURAUD);
    m_pDevice->SetRenderState(D3DRS_COLORVERTEX,      TRUE);
    m_pDevice->SetRenderState(D3DRS_LOCALVIEWER,      TRUE);
    m_pDevice->SetRenderState(D3DRS_FOGENABLE,        FALSE);
    m_pDevice->SetRenderState(D3DRS_SPECULARENABLE,   FALSE);
    m_pDevice->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);
    m_pDevice->SetRenderState(D3DRS_LIGHTING,         TRUE);

    SetLegacyLights();
}

void RenderEngine::SetDepthBias(float distance, Effect* pEffect)
{
    // Calculate projection matrix; change near and far Z based on view distance
    float viewdist = m_matrices.m_view.getTranslation().length();
    float znear = viewdist < 100.0f ? viewdist / 10 : 20.0f;
    float zfar  = 1000.0f * znear;

    m_cameraProj = CreatePerspectiveMatrix(ToRadians(45), (float)m_settings.m_screenWidth / m_settings.m_screenHeight, znear + distance, zfar - distance);
    SetProjectionMatrix(m_cameraProj, pEffect);
}

void RenderEngine::OnResolutionChanged()
{
    // Store new screen settings
    m_settings.m_screenWidth   = m_presentationParameters.BackBufferWidth;
    m_settings.m_screenHeight  = m_presentationParameters.BackBufferHeight;
    m_settings.m_screenRefresh = m_presentationParameters.FullScreen_RefreshRateInHz;

    m_resolutionConstants = Vector4(
        (float)m_presentationParameters.BackBufferWidth, (float)m_presentationParameters.BackBufferHeight,
        0.5f / m_presentationParameters.BackBufferWidth, 0.5f / m_presentationParameters.BackBufferHeight);
    
    // Adjust full-screen quad's texture coordinates
    m_sceneQuad[0].TexCoord = Vector2(0 + m_resolutionConstants.z, 0 + m_resolutionConstants.w);
    m_sceneQuad[1].TexCoord = Vector2(0 + m_resolutionConstants.z, 1 + m_resolutionConstants.w);
    m_sceneQuad[2].TexCoord = Vector2(1 + m_resolutionConstants.z, 0 + m_resolutionConstants.w);
    m_sceneQuad[3].TexCoord = Vector2(1 + m_resolutionConstants.z, 1 + m_resolutionConstants.w);

    // Apply the changed property
    for (BaseEffect* effect = m_effects; effect != NULL; effect = effect->Next())
    {
        const EffectHandles& handles = effect->GetHandles();
        ID3DXEffect* pD3DEffect = effect->GetEffect();
        pD3DEffect->SetVector(handles.ResolutionConstants, &m_resolutionConstants);
    }

    // Calculate projection matrix
    SetDepthBias(0.0f);
    ComputeShadowMatrix();
    ComputeLightFieldMatrix();
}

void RenderEngine::ComputeShadowMatrix()
{
    Vector3 lightdir(m_environment.m_lights[LT_SUN].m_direction);
    Vector3 center(0,0,0);
    Vector3 up(0,0,1);
    if (fabs(dot(lightdir, up)) > 0.999f)
    {
        up = Vector3(0,1,0);
    }

    m_matrices.m_shadowView = CreateViewMatrix(center - lightdir * 1000, center, up);
    D3DXMatrixOrthoOffCenterRH(&m_matrices.m_shadowProj, -512, 512, -512, 512, 0, 2000);
    
    // For the texture matrix, convert -1,1 domain to 0,1 domain
    m_matrices.m_shadow = m_matrices.m_shadowView * m_matrices.m_shadowProj *
        Matrix(0.5f,0,0,0, 0,-0.5f,0,0, 0,0,1,0, 0.5f,0.5f,0,1);

    // Apply the changed matrix
    for (BaseEffect* p = m_effects; p != NULL; p = p->Next())
    {
        ID3DXEffect*         effect  = p->GetEffect();
        const EffectHandles& handles = p->GetHandles();
            
        effect->SetMatrix (handles.WorldToShadow,       &m_matrices.m_shadow);
        effect->SetTexture(handles.ShadowTexture,        m_ShadowDepthMap);
        effect->SetVector (handles.ShadowTextureInvRes, &m_shadowTextureInvRes);
    }
}

void RenderEngine::ComputeLightFieldMatrix()
{
    m_matrices.m_lightFieldView = Matrix(1,0,0,0, 0,1,0,0, 0,0,0,0, 0,0,0,1) * 
        CreateViewMatrix(Vector3(0,0,0.5f), Vector3(0,0,0), Vector3(0,1,0));
    D3DXMatrixOrthoRH(&m_matrices.m_lightFieldProj, 512, 512, 0, 1.0f);

    // For the texture matrix, convert -1,1 domain to 0,1 domain
    m_matrices.m_lightField  = m_matrices.m_lightFieldView;
    m_matrices.m_lightField._13 = 0;
    m_matrices.m_lightField._23 = 0;
    m_matrices.m_lightField._33 = 1;
    m_matrices.m_lightField._43 = 0;
    m_matrices.m_lightField *= Matrix(0.5f * m_matrices.m_lightFieldProj._11,0,0,0,
               0,-0.5f * m_matrices.m_lightFieldProj._22,0,0,
               0,0,1,0, 0.5f,0.5f,0,1);
    
    // Apply the changed matrix
    for (BaseEffect* p = m_effects; p != NULL; p = p->Next())
    {
        ID3DXEffect*         effect  = p->GetEffect();
        const EffectHandles& handles = p->GetHandles();
            
        effect->SetMatrix (handles.WorldToLightField, &m_matrices.m_lightField);
        effect->SetTexture(handles.LightFieldTexture,  m_LightFieldTexture);
    }
}

void RenderEngine::SetProjectionMatrix(const Matrix& proj, Effect* pEffect)
{
    m_matrices.m_proj     = proj;
    m_matrices.m_viewProj = m_matrices.m_view  * m_matrices.m_proj;
    Matrix worldViewProj  = m_matrices.m_world * m_matrices.m_viewProj;
    
    // Apply the changed matrixes
    m_pDevice->SetTransform(D3DTS_PROJECTION, &proj);
    BaseEffect* p = m_effects;
    if (pEffect != NULL || p != NULL) do
    {
        BaseEffect* effect = (pEffect == NULL) ? p : pEffect;
        const EffectHandles& handles = effect->GetHandles();
        ID3DXEffect* pD3DEffect = effect->GetEffect();

        pD3DEffect->SetMatrix(handles.Projection,    &m_matrices.m_proj);
        pD3DEffect->SetMatrix(handles.ViewProj,      &m_matrices.m_viewProj);
        pD3DEffect->SetMatrix(handles.WorldViewProj, &worldViewProj);
    } while (pEffect == NULL && (p = p->Next()) != NULL);
}

void RenderEngine::SetWorldMatrix(const Matrix& world, Effect* pEffect)
{
    const Vector3& position = world.getTranslation();

    // Set world-based matrices
    m_matrices.m_world = world;
    Matrix  worldView       = world * m_matrices.m_view;
    Matrix  worldViewProj   = world * m_matrices.m_viewProj;
    Matrix  worldViewInv    = worldView.inverse();
    Vector4 light0ObjVector = Vector4(-m_environment.m_lights[LT_SUN].m_direction, 0.0f);
    Vector4 eyePosObj       = Vector4(m_matrices.m_viewInv.getTranslation(), 1.0f);

    // Create the various object-space vectors
    Matrix worldInv = world.inverse();
    light0ObjVector = normalize(light0ObjVector * worldInv);
    eyePosObj      *= worldInv;
    
    // Apply them
    m_pDevice->SetTransform(D3DTS_WORLD, &world);
    BaseEffect* p = m_effects;
    if (pEffect != NULL || p != NULL) do
    {
        BaseEffect* effect = (pEffect == NULL) ? p : pEffect;
        const EffectHandles& handles = effect->GetHandles();
        ID3DXEffect* pD3DEffect = effect->GetEffect();

        pD3DEffect->SetMatrix(handles.World,         &world);
        pD3DEffect->SetMatrix(handles.WorldView,     &worldView);
        pD3DEffect->SetMatrix(handles.WorldViewProj, &worldViewProj);
        pD3DEffect->SetMatrix(handles.WorldViewInv,  &worldViewInv);
        pD3DEffect->SetVector(handles.Light0ObjVector, &light0ObjVector);
        pD3DEffect->SetVector(handles.EyePosObj,       &eyePosObj);
    } while (pEffect == NULL && (p = p->Next()) != NULL);
}

void RenderEngine::SetViewMatrix(const Matrix& view, Effect* pEffect)
{
    // A matrix to correct billboarded meshes. They point to -Y but should point to +Z.
    static const Matrix BillboardCorrection(Quaternion(Vector3(1,0,0), ToRadians(-90)));

    // Recalculate view-based matrices
    m_matrices.m_view           = view;
    m_matrices.m_viewInv        = view.inverse();
    m_matrices.m_viewProj       = view * m_matrices.m_proj;
    m_matrices.m_billboardView  = BillboardCorrection * m_matrices.m_viewInv.getRotationScale();
    m_matrices.m_billboardZView = Matrix(Quaternion(Vector3(0,0,1), atan2(-m_matrices.m_viewInv.getTranslation().y, -m_matrices.m_viewInv.getTranslation().x) - D3DXToRadian(90)));

    Matrix  worldView     = m_matrices.m_world * view;
    Matrix  worldViewProj = m_matrices.m_world * m_matrices.m_viewProj;
    Matrix  worldViewInv  = worldView.inverse();
    Vector4 eyePos        = Vector4(m_matrices.m_viewInv.getTranslation(), 1);

    // Apply them
    m_pDevice->SetTransform(D3DTS_VIEW, &view);
    BaseEffect* p = m_effects;
    if (pEffect != NULL || p != NULL) do
    {
        BaseEffect* effect = (pEffect == NULL) ? p : pEffect;
        const EffectHandles& handles = effect->GetHandles();
        ID3DXEffect* pD3DEffect = effect->GetEffect();

        pD3DEffect->SetMatrix(handles.WorldView,     &worldView);
        pD3DEffect->SetMatrix(handles.WorldViewProj, &worldViewProj);
        pD3DEffect->SetMatrix(handles.WorldViewInv,  &worldViewInv);
        pD3DEffect->SetMatrix(handles.View,          &m_matrices.m_view);
        pD3DEffect->SetMatrix(handles.ViewInv,       &m_matrices.m_viewInv);
        pD3DEffect->SetMatrix(handles.ViewProj,      &m_matrices.m_viewProj);
        pD3DEffect->SetVector(handles.EyePos,        &eyePos);
    } while (pEffect == NULL && (p = p->Next()) != NULL);
}

void RenderEngine::OnResize()
{
    if (m_pDevice != NULL)
    {
        HRESULT hRes;

        OnLostDevice();
        m_presentationParameters.BackBufferWidth  = 0;
        m_presentationParameters.BackBufferHeight = 0;
        m_presentationParameters.BackBufferFormat = D3DFMT_UNKNOWN;
        if (FAILED(hRes = m_pDevice->Reset(&m_presentationParameters)))
        {
	        throw DirectXException(hRes);
        }

        OnResetDevice();
        OnResolutionChanged();
        SetCamera(m_camera);
    }
}

void RenderEngine::SetSettings(const RenderSettings& settings)
{
    if (m_settings.m_screenWidth != settings.m_screenWidth || m_settings.m_screenHeight != settings.m_screenHeight)
    {
        // Resolution has changed
        OnResize();
    }

    if (m_settings.m_antiAlias != settings.m_antiAlias && m_pDevice != NULL)
    {
        // Anti-aliasing has changed
        HRESULT hRes;

        OnLostDevice();
        if (settings.m_antiAlias)
        {
	        // Get the best multisample type
	        GetMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, m_presentationParameters.BackBufferFormat);
        }
        else
        {
            m_presentationParameters.MultiSampleQuality = 0;
	        m_presentationParameters.MultiSampleType    = D3DMULTISAMPLE_NONE;
        }
        if (FAILED(hRes = m_pDevice->Reset(&m_presentationParameters)))
        {
	        throw DirectXException(hRes);
        }
        OnResetDevice();
    }

    if (m_settings.m_shaderDetail != settings.m_shaderDetail)
    {
        // The shader detail level has changed, adjust all shaders
        for (BaseEffect* p = m_effects; p != NULL; p = p->Next())
        {
            p->SetShaderDetail(settings.m_shaderDetail);
        }
    }

    m_settings = settings;

    // Load and/or unload dynamic resources
    LoadDynamicResources();
}

void RenderEngine::SetEnvironment(const Environment &environment)
{
    m_environment = environment;

    SphericalHarmonics::Calculate_Matrices(m_LightSphAll,  m_environment.m_lights + 0, 3, m_environment.m_ambient);
    SphericalHarmonics::Calculate_Matrices(m_LightSphFill, m_environment.m_lights + 1, 2, m_environment.m_ambient);
    ComputeShadowMatrix();
    ComputeLightFieldMatrix();

    // Set shadow
    m_shadowQuad[0].Color = m_shadowQuad[1].Color = m_shadowQuad[2].Color = m_shadowQuad[3].Color =
    m_sceneQuad[0] .Color = m_sceneQuad[1] .Color = m_sceneQuad[2] .Color = m_sceneQuad[3] .Color =
        D3DCOLOR_COLORVALUE(m_environment.m_shadow.r, m_environment.m_shadow.g, m_environment.m_shadow.b, 0.0f);

    // Construct billboard matrices
    const DirectionalLight& sun = m_environment.m_lights[LT_SUN];
    m_matrices.m_billboardSun = Matrix( Quaternion(Vector3(0,-1,0), (-sun.m_direction).tilt()   )) *
                                Matrix( Quaternion(Vector3(0, 0,1), (-sun.m_direction).zAngle() ));
    m_matrices.m_billboardZLight = Matrix(Quaternion(Vector3(0,0,1), sun.m_direction.zAngle()     - D3DXToRadian(90)));
    m_matrices.m_billboardZWind  = Matrix(Quaternion(Vector3(0,0,1), m_environment.m_wind.heading - D3DXToRadian(90)));

    // Apply the changed settings
    for (BaseEffect* p = m_effects; p != NULL; p = p->Next())
    {
        const EffectHandles& handles = p->GetHandles();
        ID3DXEffect* pEffect = p->GetEffect();

        pEffect->SetMatrixArray(handles.LightSphAll,   m_LightSphAll,  3);
        pEffect->SetMatrixArray(handles.LightSphFill,  m_LightSphFill, 3);
        pEffect->SetVector(handles.Light0Diffuse,    &Vector4(Color(m_environment.m_lights[LT_SUN].m_color * m_environment.m_lights[LT_SUN].m_color.a, 1.0f)));
        pEffect->SetVector(handles.Light0Specular,   &Vector4(m_environment.m_specular));
        pEffect->SetVector(handles.Light0Vector,     &Vector4(-m_environment.m_lights[LT_SUN].m_direction, 0.0f) );
        pEffect->SetVector(handles.LightAmbient,     &Vector4(m_environment.m_ambient));
        pEffect->SetVector(handles.FogVals,          &Vector4(0,1,0,0));
        pEffect->SetVector(handles.DistanceFadeVals, &Vector4(0,1,0,0));
    }

    SetLegacyLights();
}

void RenderEngine::SetLegacyLights()
{
    // Set and enable lights
    D3DLIGHT9 LegacyLights[3] = {D3DLIGHT_DIRECTIONAL};
    LegacyLights[LT_SUN]  .Specular = m_environment.m_specular;
    LegacyLights[LT_FILL1].Specular = Color(0,0,0,0);
    LegacyLights[LT_FILL2].Specular = Color(0,0,0,0);
    for (int i = 0; i < 3; i++)
    {
        LegacyLights[i].Type      = D3DLIGHT_DIRECTIONAL;
        LegacyLights[i].Diffuse   = Color(m_environment.m_lights[i].m_color * m_environment.m_lights[i].m_color.a, 1.0f);
        LegacyLights[i].Ambient   = Color(0,0,0,0);
        LegacyLights[i].Direction = m_environment.m_lights[i].m_direction;
        m_pDevice->SetLight(i, &LegacyLights[i]);
        m_pDevice->LightEnable(i, TRUE);
    };
    m_pDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(m_environment.m_ambient.r, m_environment.m_ambient.g, m_environment.m_ambient.b, m_environment.m_ambient.a));
}

void RenderEngine::SetCamera(const Camera& camera)
{
    m_camera = camera;

    // Calculate and set view matrix
    m_cameraView = CreateViewMatrix(camera.m_position, camera.m_target, camera.m_up);
    SetViewMatrix( m_cameraView );
    SetDepthBias(0.0f, NULL);
    ComputeShadowMatrix();
    ComputeLightFieldMatrix();
}

void RenderEngine::UnloadDynamicResources()
{
    SAFE_RELEASE(m_DistortionTexture);
    SAFE_RELEASE(m_ShadowDepthDummy);
    SAFE_RELEASE(m_LightFieldTexture);
    SAFE_RELEASE(m_ShadowDepthMap);
    SAFE_RELEASE(m_HeatTexture);
    SAFE_RELEASE(m_BloomTexture);
    SAFE_RELEASE(m_ShadowSurface);
    SAFE_RELEASE(m_ShadowTexture);
    SAFE_RELEASE(m_uiFont);
}

void RenderEngine::LoadDynamicResources()
{
    HRESULT hRes;

    const bool doHeat        = (m_settings.m_heatDistortion && !m_settings.m_heatDebug && m_heatEffect != NULL && m_heatEffect->IsSupported());
    const bool doBloom       = (m_settings.m_bloom && m_bloomEffect != NULL && m_bloomEffect->IsSupported());
    const bool doSoftShadows = (!m_isUaW && m_hasStencilBuffer && m_settings.m_softShadows && m_stencilDarkenToAlpha != NULL && m_stencilDarkenToAlpha->IsSupported() && m_stencilDarkenFinalBlur != NULL && m_stencilDarkenFinalBlur->IsSupported());
    const bool doShadowMap   = (m_isUaW);
    const bool doLightField  = (m_isUaW);

    if (!doLightField)
    {
        SAFE_RELEASE(m_LightFieldTexture);
    }
    else if (m_LightFieldTexture == NULL)
    {
        int size = LIGHTFIELD_SIZE;
        hRes = E_FAIL;
        while (size >= 1)
        {
            IDirect3DTexture9* pLightFieldTexture;
            if (SUCCEEDED(hRes = m_pDevice->CreateTexture(size, size, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pLightFieldTexture, NULL)))
            {
                // Success
                m_LightFieldTexture = pLightFieldTexture;
                break;
            }
            // Try lower resolution
            size /= 2;
        }
        
        if (size < 1)
        {
            // Couldn't create any
            Log::WriteError("Couldn't create resources for light field rendering: %ls", DXGetErrorDescription( hRes ));
        }
    }

    if (!doShadowMap)
    {
        SAFE_RELEASE(m_ShadowDepthMap);
        SAFE_RELEASE(m_ShadowDepthDummy);
    }
    else if (m_ShadowDepthMap == NULL)
    {
        hRes = E_FAIL;
        // Try a series of resolutions
        for (int size = SHADOWMAP_SIZE; m_ShadowDepthMap == NULL && size >= 256; size /= 2)
        {
            IDirect3DTexture9* pShadowDepthMap = NULL;

            if (m_adapterInfo.VendorId != VENDOR_ATI)
            {
                // First check if we can use the D3DUSAGE_DEPTHSTENCIL approach to create a depth texture.
                // nVidia uses this method. ATI most certainly does not.
                for (int i = 0; DepthFormats[i].format != D3DFMT_UNKNOWN; i++)
                {
                    if (SUCCEEDED(hRes = m_pDevice->CreateTexture(size, size, 1, D3DUSAGE_DEPTHSTENCIL, DepthFormats[i].format, D3DPOOL_DEFAULT, &pShadowDepthMap, NULL)))
                    {
                        break;
                    }
                }
            }

            if (pShadowDepthMap == NULL)
            {
                static const D3DFORMAT D3DFMT_DF16 = (D3DFORMAT)MAKEFOURCC('D','F','1','6');
                static const D3DFORMAT D3DFMT_DF24 = (D3DFORMAT)MAKEFOURCC('D','F','2','4');

                // If the D3DUSAGE_DEPTHSTENCIL method failed, try the FOURCC method.
                // ATI cards, and maybe others, will use this.
                if (FAILED(hRes = m_pDevice->CreateTexture(size, size, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_DF16, D3DPOOL_DEFAULT, &pShadowDepthMap, NULL)))
                {
                    hRes = m_pDevice->CreateTexture(size, size, 1, D3DUSAGE_DEPTHSTENCIL, D3DFMT_DF24, D3DPOOL_DEFAULT, &pShadowDepthMap, NULL);
                }
            }

            if (pShadowDepthMap != NULL)
            {
                // We need a dummy render target with the same Multisample type as the depth buffer
                IDirect3DSurface9* pShadowDepthDummy;
                if (SUCCEEDED(hRes = m_pDevice->CreateRenderTarget(size, size, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, FALSE, &pShadowDepthDummy, NULL)))
                {
                    // Success
                    m_ShadowDepthMap   = pShadowDepthMap;
                    m_ShadowDepthDummy = pShadowDepthDummy;
                    m_shadowTextureInvRes = Vector4( 1.0f / size, 1.0f / size, 0, 0 );
                    break;
                }
                SAFE_RELEASE(pShadowDepthMap);
            }
        }
        
        if (m_ShadowDepthMap == NULL)
        {
            // Couldn't create any
            Log::WriteError("Couldn't create resources for shadow rendering: %ls", DXGetErrorDescription( hRes ));
        }
    }

    if (!doSoftShadows)
    {
        SAFE_RELEASE(m_ShadowTexture);
        SAFE_RELEASE(m_ShadowSurface);
    }
    else
    {
        if (m_ShadowTexture == NULL)
        {
            IDirect3DTexture9* pShadowTexture;
            if (FAILED(hRes = m_pDevice->CreateTexture(m_presentationParameters.BackBufferWidth, m_presentationParameters.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pShadowTexture, NULL))) {
                Log::WriteError("Couldn't create resources for shadow rendering: %ls", DXGetErrorDescription( hRes ));
            } else {
                m_ShadowTexture = pShadowTexture;
            }
        }

        if (m_ShadowTexture != NULL && m_ShadowSurface == NULL && m_presentationParameters.MultiSampleType != D3DMULTISAMPLE_NONE)
        {
            IDirect3DSurface9* pShadowSurface;
            if (FAILED(hRes = m_pDevice->CreateRenderTarget(m_presentationParameters.BackBufferWidth, m_presentationParameters.BackBufferHeight, D3DFMT_A8R8G8B8, m_presentationParameters.MultiSampleType, m_presentationParameters.MultiSampleQuality, FALSE, &pShadowSurface, NULL))) {
                SAFE_RELEASE(m_DistortionTexture);
                Log::WriteError("Couldn't create resources for shadow rendering: %ls", DXGetErrorDescription( hRes ));
            } else {
                m_ShadowSurface = pShadowSurface;
            }
        }
    }

    if (!doBloom)
    {
        SAFE_RELEASE(m_BloomTexture);
    }
    else if (m_BloomTexture == NULL)
    {
        IDirect3DTexture9* pBloomTexture;
        if (FAILED(hRes = m_pDevice->CreateTexture(m_presentationParameters.BackBufferWidth, m_presentationParameters.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pBloomTexture, NULL))) {
            Log::WriteError("Couldn't create resources for bloom rendering: %ls", DXGetErrorDescription( hRes ));
        } else {
            m_BloomTexture = pBloomTexture;
        }
    }

    if (!doHeat)
    {
        SAFE_RELEASE(m_HeatTexture);
        SAFE_RELEASE(m_DistortionTexture);
    }
    else
    {
        if (m_DistortionTexture == NULL)
        {
            IDirect3DTexture9* pDistortionTexture;
            if (FAILED(hRes = m_pDevice->CreateTexture(m_presentationParameters.BackBufferWidth, m_presentationParameters.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pDistortionTexture, NULL))) {
                Log::WriteError("Couldn't create resources for heat rendering: %ls", DXGetErrorDescription( hRes ));
            } else {
                m_DistortionTexture = pDistortionTexture;
            }
        }

        if (m_DistortionTexture != NULL && m_HeatTexture == NULL)
        {
            IDirect3DTexture9* pHeatTexture;
            if (FAILED(hRes = m_pDevice->CreateTexture(m_presentationParameters.BackBufferWidth, m_presentationParameters.BackBufferHeight, 1, D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &pHeatTexture, NULL))) {
                SAFE_RELEASE(m_DistortionTexture);
                Log::WriteError("Couldn't create resources for heat rendering: %ls", DXGetErrorDescription( hRes ));
            } else {
                m_HeatTexture = pHeatTexture;
            }
        }
    }

	// Create the UI font
	ID3DXFont* pFont;
	if (FAILED(hRes = D3DXCreateFont(m_pDevice, -11, 0, FW_REGULAR, 0, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_DONTCARE, NULL, &pFont))) {
		Log::WriteError("Couldn't create UI font for bones: %ls", DXGetErrorDescription( hRes ));
	}
	m_uiFont = pFont;
}

ptr<Effect> RenderEngine::LoadShadowDebugEffect(bool rskin)
{
    if (rskin)
    {
        if (m_shadowDebugEffect_RSkin == NULL)
        {
            ID3DXEffect* pEffect;
            HRESULT      hRes;
            if (FAILED(hRes = D3DXCreateEffectFromResource(m_pDevice, NULL, MAKEINTRESOURCE(IDS_SHADOW_DEBUG_RSKIN), NULL, NULL, D3DXSHADER_NO_PRESHADER | D3DXFX_NOT_CLONEABLE, NULL, &pEffect, NULL)))
            {
                throw DirectXException(hRes);
            }
            m_shadowDebugEffect_RSkin = new Effect(&m_effects, pEffect, m_isUaW);

            // Initialize once-only variables
            InitializeEffect(m_shadowDebugEffect_RSkin);
        }
        return m_shadowDebugEffect_RSkin;
    }

    if (m_shadowDebugEffect_Mesh == NULL)
    {
        ID3DXEffect* pEffect;
        HRESULT      hRes;
        if (FAILED(hRes = D3DXCreateEffectFromResource(m_pDevice, NULL, MAKEINTRESOURCE(IDS_SHADOW_DEBUG_MESH), NULL, NULL, D3DXSHADER_NO_PRESHADER | D3DXFX_NOT_CLONEABLE, NULL, &pEffect, NULL)))
        {
            throw DirectXException(hRes);
        }
        m_shadowDebugEffect_Mesh = new Effect(&m_effects, pEffect, m_isUaW);

        // Initialize once-only variables
        InitializeEffect(m_shadowDebugEffect_Mesh);
    }
    return m_shadowDebugEffect_Mesh;
}

ptr<Effect> RenderEngine::LoadShadowMapEffect(const std::string& vertexType)
{
    // Look in cache first
    string name = Uppercase(vertexType);
    EffectMap::const_iterator p = m_shadowEffectCache.find(name);
    if (p != m_shadowEffectCache.end())
    {
        // Found
        return p->second;
    }

    // Not found, load it
    string filename = "ShadowMap/" + name + "ShadowMap.fx";
    ptr<Effect> effect = LoadEffect(filename, FX_SHADOWMAP);
    if (effect != NULL)
    {
        // Add to cache
        m_shadowEffectCache.insert(make_pair(name, effect));
    }
    return effect;
}

void RenderEngine::LoadPhaseEffects()
{
    static const char* PhaseFiles[NUM_RENDER_PHASES] = {
        "Engine/PhaseTerrain.fx",
        "Engine/PhaseTerrainmesh.fx",
        "Engine/PhaseOpaque.fx",
        "Engine/PhaseShadow.fx",
        "Engine/PhaseWater.fx",
        "Engine/PhaseTransparent.fx",
        "Engine/PhaseOccluded.fx",
        "Engine/PhaseHeat.fx",
        "Engine/PhaseFissure.fx",
        "Engine/PhaseBloom.fx" };
    
    for (int i = 0; i < NUM_RENDER_PHASES; i++)
    {
        m_phases[i] = LoadPhaseEffect(PhaseFiles[i]);
        if (!m_phases[i]->IsValid())
        {
            Log::WriteError("Invalid effect \"%s\"\n", PhaseFiles[i]);
        }
    }
    
    if (IsUaW())
    {
        m_shadowMapEffect = LoadPhaseEffect("ShadowMap/ShadowMapGlobal.fx");
        if (!m_shadowMapEffect->IsValid())
        {
            Log::WriteError("Invalid effect \"%s\"\n", "ShadowMap/ShadowMapGlobal.fx");
        }
    }
}

void RenderEngine::LoadStandardResources()
{
    HRESULT hRes;

    // Create and fill the blend texture for the ground
    IDirect3DTexture9* pBlendTexture;
    if (FAILED(hRes = m_pDevice->CreateTexture(16, 16, 1, 0, D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, &pBlendTexture, NULL)))
    {
        throw DirectXException(hRes);
    }

    D3DLOCKED_RECT lock;
    if (FAILED(pBlendTexture->LockRect(0, &lock, NULL, 0)))
    {
        SAFE_RELEASE(pBlendTexture);
        throw DirectXException(hRes);
    }
    for (int y = 0; y < 16; y++)
    {
        D3DCOLOR* row = (D3DCOLOR*)((char*)lock.pBits + y * lock.Pitch);
        for (int x = 0; x < 16; x++)
        {
            row[x] = D3DCOLOR_RGBA(255,255,255,255);
        }
    }
    pBlendTexture->UnlockRect(0);
    m_BlendTexture = pBlendTexture;

    //
    // Load effects
    //
    LoadPhaseEffects();
    if (m_hasStencilBuffer)
    {
        m_stencilDarken          = LoadEffect("Engine/StencilDarken.fx",          FX_SCENE);
        m_stencilDarkenFinalBlur = LoadEffect("Engine/StencilDarkenFinalBlur.fx", FX_SCENE);
        m_stencilDarkenToAlpha   = LoadEffect("Engine/StencilDarkenToAlpha.fx",   FX_SCENE);
    }
    
    if ((m_heatEffect = LoadEffect("Engine/SceneHeat.fx", FX_SCENE)) != NULL)
    {
        m_heatEffect->GetEffect()->SetFloat("DistortionAmount", 0.05f);
    }
    
    if ((m_bloomEffect = LoadEffect("Engine/SceneBloom.fx", FX_SCENE)) != NULL)
    {
        m_bloomEffect->GetEffect()->SetFloat("BloomIteration", 1.0f);
    }

    m_groundEffect  = LoadEffect("Terrain/TerrainRenderBump.fx");

    if (IsUaW())
    {
        m_groundTexture = LoadTexture("WM_CEMENT.TGA",   false);
        m_groundBumpMap = LoadTexture("WM_CEMENT_B.TGA", false);
    }

    if (m_groundTexture == NULL || m_groundBumpMap == NULL)
    {
        m_groundTexture = LoadTexture("W_URBAN_ASPHALT.TGA");
        m_groundBumpMap = LoadTexture("W_URBAN_ASPHALT_BC.TGA");
    }

    D3DCOLOR shadow = D3DCOLOR_COLORVALUE(m_environment.m_shadow.r, m_environment.m_shadow.g, m_environment.m_shadow.b, m_environment.m_shadow.a);
    m_shadowQuad[0].Position = Vector3(-1, 1,0); m_shadowQuad[0].Color = shadow;
    m_shadowQuad[1].Position = Vector3(-1,-1,0); m_shadowQuad[1].Color = shadow;
    m_shadowQuad[2].Position = Vector3( 1, 1,0); m_shadowQuad[2].Color = shadow;
    m_shadowQuad[3].Position = Vector3( 1,-1,0); m_shadowQuad[3].Color = shadow;

    m_groundQuad[0].Position = Vector3(-GROUND_SIZE/2,  GROUND_SIZE/2, 0.0f); m_groundQuad[0].Normal = Vector3(0,0,1);
    m_groundQuad[1].Position = Vector3(-GROUND_SIZE/2, -GROUND_SIZE/2, 0.0f); m_groundQuad[1].Normal = Vector3(0,0,1);
    m_groundQuad[2].Position = Vector3( GROUND_SIZE/2,  GROUND_SIZE/2, 0.0f); m_groundQuad[2].Normal = Vector3(0,0,1);
    m_groundQuad[3].Position = Vector3( GROUND_SIZE/2, -GROUND_SIZE/2, 0.0f); m_groundQuad[3].Normal = Vector3(0,0,1);

    m_groundQuad[0].TexCoord = Vector4(m_groundQuad[0].Position,1);
    m_groundQuad[1].TexCoord = Vector4(m_groundQuad[1].Position,1);
    m_groundQuad[2].TexCoord = Vector4(m_groundQuad[2].Position,1);
    m_groundQuad[3].TexCoord = Vector4(m_groundQuad[3].Position,1);
    m_groundQuad[0].Color    = D3DCOLOR_RGBA(255,255,255,255);
    m_groundQuad[1].Color    = D3DCOLOR_RGBA(255,255,255,255);
    m_groundQuad[2].Color    = D3DCOLOR_RGBA(255,255,255,255);
    m_groundQuad[3].Color    = D3DCOLOR_RGBA(255,255,255,255);

    m_sceneQuad[0].Position = Vector3(-1, 1,0); m_sceneQuad[0].TexCoord = Vector2(0, 0);
    m_sceneQuad[1].Position = Vector3(-1,-1,0); m_sceneQuad[1].TexCoord = Vector2(0, 1);
    m_sceneQuad[2].Position = Vector3( 1, 1,0); m_sceneQuad[2].TexCoord = Vector2(1, 0);
    m_sceneQuad[3].Position = Vector3( 1,-1,0); m_sceneQuad[3].TexCoord = Vector2(1, 1);
}

D3DFORMAT RenderEngine::GetDepthStencilFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat)
{
	for (int i = 0; DepthFormats[i].format != D3DFMT_UNKNOWN; i++)
	{
		if (SUCCEEDED(m_pD3D->CheckDeviceFormat     (Adapter, DeviceType, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, DepthFormats[i].format)))
		if (SUCCEEDED(m_pD3D->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, AdapterFormat, DepthFormats[i].format)))
		{
            m_hasStencilBuffer = DepthFormats[i].stencil;
            Log::WriteInfo("Selected depth/stencil format for device: %s\n", DepthFormats[i].name);
			return DepthFormats[i].format;
		}
	}

	return D3DFMT_UNKNOWN;
}

void RenderEngine::GetMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT DisplayFormat)
{
	D3DMULTISAMPLE_TYPE MultiSampleTypes[16] = {
		D3DMULTISAMPLE_16_SAMPLES, D3DMULTISAMPLE_15_SAMPLES, D3DMULTISAMPLE_14_SAMPLES, D3DMULTISAMPLE_13_SAMPLES,
		D3DMULTISAMPLE_12_SAMPLES, D3DMULTISAMPLE_11_SAMPLES, D3DMULTISAMPLE_10_SAMPLES, D3DMULTISAMPLE_9_SAMPLES,
		D3DMULTISAMPLE_8_SAMPLES, D3DMULTISAMPLE_7_SAMPLES, D3DMULTISAMPLE_6_SAMPLES, D3DMULTISAMPLE_5_SAMPLES,
		D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_3_SAMPLES, D3DMULTISAMPLE_2_SAMPLES, D3DMULTISAMPLE_NONE
	};

	int i;
	for (i = 0; i < 16; i++)
	{
		if (SUCCEEDED(m_pD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, DisplayFormat,                                   m_presentationParameters.Windowed, MultiSampleTypes[i], &m_presentationParameters.MultiSampleQuality)))
        if (SUCCEEDED(m_pD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, m_presentationParameters.AutoDepthStencilFormat, m_presentationParameters.Windowed, MultiSampleTypes[i], &m_presentationParameters.MultiSampleQuality)))
		{
			m_presentationParameters.MultiSampleQuality--;
			m_presentationParameters.MultiSampleType = MultiSampleTypes[i];
			break;
		}
	}

	if (i == 16)
	{
		m_presentationParameters.MultiSampleQuality = 0;
		m_presentationParameters.MultiSampleType    = D3DMULTISAMPLE_NONE;
	}
}

class ShaderInclude : public ID3DXInclude
{
    struct Container
    {
        wchar_t* m_base;
        char     m_data[1];
    };

    wstring m_base;

    static const Container* ToContainer(const void* data)
    {
        return (const Container*)((const char*)data - offsetof(Container, m_data));
    }
    
    static wchar_t* StringDuplicate(const wchar_t* src)
    {
        size_t   size = (wcslen(src) + 1) * sizeof(wchar_t);
        wchar_t* dest = new wchar_t[size];
        memcpy(dest, src, size);
        return dest;
    }

    static wstring StripFilename(const wstring& path)
    {
        wstring::size_type ofs = path.find_last_of(L"\\/");
        if (ofs != wstring::npos) {
            return path.substr(0, ofs + 1);
        }
        return L"";
    }

public:
    STDMETHOD(Open)(D3DXINCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID * ppData, UINT * pBytes)
    {
        wstring filename = AnsiToWide(pFileName);
        if (filename.find_first_of(L":") == wstring::npos)
        {
            // Relative path
            if (pParentData == NULL) {
                filename = m_base + filename;
            } else {
                const Container* c = ToContainer(pParentData);
                filename = c->m_base + filename;
            }
        }

        HANDLE hFile = CreateFile(filename.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        {
            DWORD bytes = GetFileSize(hFile, NULL);
            Container* c = (Container*)malloc(offsetof(Container, m_data) + bytes);

            if (ReadFile(hFile, &c->m_data, bytes, &bytes, NULL))
            {
                CloseHandle(hFile);
                filename = StripFilename(filename);
                c->m_base = StringDuplicate(filename.c_str());

                *ppData   = &c->m_data;
                *pBytes   = bytes;
                return S_OK;
            }
            CloseHandle(hFile);
            free(c);
        }
        return E_FAIL;
    }
    
    STDMETHOD(Close)(LPCVOID pData)
    {
        const Container* c = ToContainer(pData);
        delete[] c->m_base;
        free((void*)c);
        return S_OK;
    }

    ShaderInclude(const wstring& basefile)
    {
        m_base = StripFilename(basefile);
    }
};

ID3DXEffect* RenderEngine::LoadShader(const std::string& name, bool usePlaceholder)
{
    ID3DXEffect* pEffect = NULL;

    // Read the file
    ptr<IFile> file;
    if (m_isUaW) {
        file = Assets::LoadShader("DX9/" + name);
    } else {
        file = Assets::LoadShader(name);
    }

    if (file != NULL)
    {
        static const D3DXMACRO Macros[] = {
            {"WIN32", "1"},
            {NULL, NULL}
        };

        HRESULT hRes;
        ID3DXBuffer* errors = NULL;

        PhysicalFile* physfile = dynamic_cast<PhysicalFile*>((IFile*)file);
        if (physfile != NULL)
        {
            ShaderInclude inc(physfile->name());
            hRes = D3DXCreateEffectFromFile(m_pDevice, physfile->name().c_str(), Macros, &inc, D3DXFX_NOT_CLONEABLE | D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY, NULL, &pEffect, &errors);
        }
        else
        {
            Buffer<char> data(file->size());
            file->read(data, file->size());
            hRes = D3DXCreateEffect(m_pDevice, data, (UINT)data.size(), Macros, NULL, D3DXFX_NOT_CLONEABLE | D3DXSHADER_ENABLE_BACKWARDS_COMPATIBILITY, NULL, &pEffect, &errors);
        }
        
        // Create the effect
        if (FAILED(hRes))
        {
            if (errors != NULL) {
                Log::WriteError("Unable to load effect %s: %ls:\n%.*s\n", name.c_str(), DXGetErrorDescription( hRes ), errors->GetBufferSize(), errors->GetBufferPointer());
            } else {
                Log::WriteError("Unable to load effect %s: %ls\n", name.c_str(), DXGetErrorDescription( hRes ));
            }
        }
        SAFE_RELEASE(errors);
    }
    // We ignore "alDefault.fx not found" errors
    else if (_stricmp(name.c_str(), "alDefault.fx") != 0)
    {
        Log::WriteError("Effect \"%s\" not found!\n", name.c_str());
    }

    if (pEffect == NULL && usePlaceholder)
    {
        // Load placeholder effect
        HRESULT hRes;
        UINT id = (m_isUaW) ? IDS_MISSING_UAW : IDS_MISSING_EAW;
        if (FAILED(hRes = D3DXCreateEffectFromResource(m_pDevice, NULL, MAKEINTRESOURCE(id), NULL, NULL, D3DXSHADER_NO_PRESHADER | D3DXFX_NOT_CLONEABLE, NULL, &pEffect, NULL)))
        {
            throw DirectXException(hRes);
        }
    }

    return pEffect;
}

ptr<Effect> RenderEngine::LoadEffect(const std::string& name, FxType type)
{
    string keyname = Uppercase(name);
    EffectMap::const_iterator p = m_effectCache.find(keyname);
    if (p != m_effectCache.end())
    {
        // Found
        return p->second;
    }

    ID3DXEffect* fx = LoadShader(name, type == FX_NORMAL);
    if (fx == NULL)
    {
        return NULL;
    }

    // Create the wrapper
    ptr<Effect> effect = new Effect(&m_effects, fx, m_isUaW, m_adapterInfo.VendorId, type == FX_SHADOWMAP);

    // Initialize once-only variables
    InitializeEffect(effect);

    m_effectCache.insert(make_pair(keyname, effect));
    return effect;
}

void RenderEngine::InitializeEffect(ptr<Effect> effect) const
{
    const EffectHandles& handles = effect->GetHandles();
    ID3DXEffect* pEffect = effect->GetEffect();

    // Camera
    pEffect->SetMatrix(handles.View,          &m_matrices.m_view);
    pEffect->SetMatrix(handles.ViewInv,       &m_matrices.m_viewInv);
    pEffect->SetMatrix(handles.ViewProj,      &m_matrices.m_viewProj);
    pEffect->SetMatrix(handles.Projection,    &m_matrices.m_proj);
    pEffect->SetVector(handles.EyePos,        &Vector4(m_matrices.m_viewInv.getTranslation(), 1));
    pEffect->SetVector(handles.ResolutionConstants, &m_resolutionConstants);
    
    // If it's a shadow shader, this'll increase the extrusion distance
    pEffect->SetFloat(pEffect->GetParameterBySemantic(NULL, "SHADOW_EXTRUSION_DISTANCE"), 4000.0f);

    // Environment
    pEffect->SetMatrixArray(handles.LightSphAll,  m_LightSphAll,  3);
    pEffect->SetMatrixArray(handles.LightSphFill, m_LightSphFill, 3);
    pEffect->SetMatrix (handles.WorldToShadow,       &m_matrices.m_shadow);
    pEffect->SetTexture(handles.ShadowTexture,        m_ShadowDepthMap);
    pEffect->SetTexture(handles.LightFieldTexture,    m_LightFieldTexture);
    pEffect->SetVector (handles.ShadowTextureInvRes, &m_shadowTextureInvRes);
    pEffect->SetVector (handles.Light0Diffuse,    &Vector4(Color(m_environment.m_lights[LT_SUN].m_color * m_environment.m_lights[LT_SUN].m_color.a, 1.0f)));
    pEffect->SetVector (handles.Light0Specular,   &Vector4(m_environment.m_specular));
    pEffect->SetVector (handles.Light0Vector,     &Vector4(-m_environment.m_lights[LT_SUN].m_direction, 0.0f) );
    pEffect->SetVector (handles.LightAmbient,     &Vector4(m_environment.m_ambient));
    pEffect->SetVector (handles.FogVals,          &Vector4(0,1,0,0));
    pEffect->SetVector (handles.DistanceFadeVals, &Vector4(0,1,0,0));

    effect->SetShaderDetail(m_settings.m_shaderDetail);
}

ptr<PhaseEffect> RenderEngine::LoadPhaseEffect(const std::string& name)
{
    // Return the wrapper
    return new PhaseEffect(&m_effects, LoadShader(name));
}

ptr<Texture> RenderEngine::LoadTexture(const std::string& name, bool usePlaceholder)
{
    string key = Uppercase(name);
    TextureMap::iterator p = m_textureCache.find(key);
    if (p != m_textureCache.end())
    {
        return p->second;
    }

    // Texture wasn't loaded before, load it
    IDirect3DTexture9* pD3DTexture = NULL;

    // Read the file
    ptr<IFile> file = Assets::LoadTexture(name);
    if (file != NULL)
    {
        Buffer<char> data(file->size());
        file->read(data, file->size());
        
        // Create the texture
        HRESULT hRes;
        if (FAILED(hRes = D3DXCreateTextureFromFileInMemory(m_pDevice, data, (UINT)data.size(), &pD3DTexture)))
        {
            Log::WriteError("Unable to load texture \"%s\": %ls.\n", name.c_str(), DXGetErrorDescription(hRes));
        }
    }
    else
    {
        Log::WriteError("Texture \"%s\" not found.\n", name.c_str());
    }
    
    if (pD3DTexture == NULL && usePlaceholder)
    {
        // Load placeholder texture
        HRESULT hRes;
        if (FAILED(hRes = D3DXCreateTextureFromResource(m_pDevice, GetModuleHandle(NULL), MAKEINTRESOURCE(IDT_MISSING), &pD3DTexture)))
        {
            throw DirectXException(hRes);
        }
    }

    if (pD3DTexture == NULL)
    {
        return NULL;
    }

    // Add to the cache
    ptr<Texture> texture = new Texture(pD3DTexture);
    m_textureCache.insert(make_pair(key, texture));
    return texture;
}

void RenderEngine::RegisterParticleSystemInstance(ParticleSystemInstance* instance)
{
    m_particleSystems.insert(instance);
}

void RenderEngine::UnregisterParticleSystemInstance(ParticleSystemInstance* instance)
{
    m_particleSystems.erase(instance);
}

void RenderEngine::RegisterLightFieldInstance(LightFieldInstance* instance)
{
    m_lightfields.insert(instance);
}

void RenderEngine::UnregisterLightFieldInstance(LightFieldInstance* instance)
{
    m_lightfields.erase(instance);
}

ptr<IObjectTemplate> RenderEngine::CreateObjectTemplate(ptr<Model> model)
{
    return new ObjectTemplate(model, *this, *m_vertexManager);
}

ptr<IRenderObject> RenderEngine::CreateRenderObject(ptr<IObjectTemplate> templ, int alt, int lod)
{
    // Verify that the passed object is of the correct type
    ptr<ObjectTemplate> t = dynamic_cast<ObjectTemplate*>((IObjectTemplate*)templ);
    if (t == NULL)
    {
        throw ArgumentException(L"Invalid template object");
    }
    
    // We created managed ptr from a cast plain pointer, so adjust references
    t->AddRef();
    
    return new RenderObject(m_objects, t, alt, lod);
}

RenderEngine::RenderEngine(HWND hWnd, const RenderSettings& settings, const Environment& env, bool isUaW)
    : m_hWnd(hWnd), m_settings(settings), m_isUaW(isUaW), m_effects(NULL)
{
	HINSTANCE  hInstance  = GetModuleHandle(NULL);
	UINT       adapter    = D3DADAPTER_DEFAULT;
	D3DDEVTYPE deviceType = D3DDEVTYPE_HAL;
	HRESULT    hRes;

	if ((m_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		throw DirectXException(E_FAIL);
	}

    // Get adapter info (primarily for vendor ID)
    if (FAILED(hRes = m_pD3D->GetAdapterIdentifier(adapter, 0, &m_adapterInfo)))
    {
        throw DirectXException(hRes);
    }

	//
	// Create the device
	//

	// Get current display mode
	D3DDISPLAYMODE displayMode;
	if (FAILED(hRes = m_pD3D->GetAdapterDisplayMode(adapter, &displayMode)))
	{
		throw DirectXException(hRes);
	}

    m_presentationParameters.BackBufferFormat	= displayMode.Format;
	m_presentationParameters.BackBufferWidth	= 0;
	m_presentationParameters.BackBufferHeight	= 0;
    m_presentationParameters.BackBufferCount	= 1;
    m_presentationParameters.SwapEffect			= D3DSWAPEFFECT_DISCARD;
    m_presentationParameters.hDeviceWindow		= m_hWnd;
    m_presentationParameters.Windowed			= TRUE;
    m_presentationParameters.Flags				= 0;
	m_presentationParameters.MultiSampleType    = D3DMULTISAMPLE_NONE;
	m_presentationParameters.MultiSampleQuality = 0;
    m_presentationParameters.EnableAutoDepthStencil	    = TRUE;
	m_presentationParameters.FullScreen_RefreshRateInHz = 0;
    m_presentationParameters.PresentationInterval		= D3DPRESENT_INTERVAL_DEFAULT;

	// Determine best depth/stencil buffer format
	if ((m_presentationParameters.AutoDepthStencilFormat = GetDepthStencilFormat(adapter, deviceType, displayMode.Format)) == D3DFMT_UNKNOWN)
	{
		throw DirectXException(L"Unable to determine the depth/stencil buffer format");
	}

    if (m_settings.m_antiAlias)
    {
	    // Get the best multisample type
	    GetMultiSampleType(adapter, deviceType, displayMode.Format);
    }
    else
    {
        m_presentationParameters.MultiSampleQuality = 0;
	    m_presentationParameters.MultiSampleType    = D3DMULTISAMPLE_NONE;
    }

    // Create device (first mixed, then SW)
    IDirect3DDevice9* pDevice;
	if (FAILED(hRes = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, deviceType, m_hWnd, D3DCREATE_MIXED_VERTEXPROCESSING,    &m_presentationParameters, &pDevice)))
	if (FAILED(hRes = m_pD3D->CreateDevice(D3DADAPTER_DEFAULT, deviceType, m_hWnd, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &m_presentationParameters, &pDevice)))
	{
		throw DirectXException(hRes);
	}
    m_pDevice = pDevice;

    // Get device caps
    m_pDevice->GetDeviceCaps(&m_deviceCaps);

    // Set current screen settings
    m_settings.m_screenWidth   = m_presentationParameters.BackBufferWidth;
    m_settings.m_screenHeight  = m_presentationParameters.BackBufferHeight;
    m_settings.m_screenRefresh = m_presentationParameters.FullScreen_RefreshRateInHz;

    m_vertexManager = new VertexManager(m_pDevice, isUaW);

    OnResolutionChanged();
    LoadStandardResources();
    LoadDynamicResources();
    SetEnvironment(env);
    InitializeRenderStates();

    Camera camera = {
        Vector3(1000, -1000, 1000),
        Vector3(0,0,0),
        Vector3(0,0,1)
    };
    SetCamera(camera);
}

RenderEngine::~RenderEngine()
{
    // Clear the caches
    m_textureCache.clear();
    m_effectCache.clear();
    m_shadowEffectCache.clear();
  
    SAFE_RELEASE(m_shadowDebugEffect_RSkin);
    SAFE_RELEASE(m_shadowDebugEffect_Mesh);
    SAFE_RELEASE(m_heatEffect);
    SAFE_RELEASE(m_bloomEffect);
    SAFE_RELEASE(m_groundBumpMap);
    SAFE_RELEASE(m_groundTexture);
    SAFE_RELEASE(m_groundEffect);
    SAFE_RELEASE(m_BlendTexture);
    SAFE_RELEASE(m_stencilDarken);
    SAFE_RELEASE(m_stencilDarkenFinalBlur);
    SAFE_RELEASE(m_stencilDarkenToAlpha);
    SAFE_RELEASE(m_shadowMapEffect);
    for (int i = 0; i < NUM_RENDER_PHASES; i++)
    {
        SAFE_RELEASE(m_phases[i]);
    }

    for (BaseEffect* next, *cur = m_effects; cur != NULL; cur = next)
    {
        next = cur->Next();
        SAFE_RELEASE(cur);
    }
    UnloadDynamicResources();
    SAFE_RELEASE(m_vertexManager);
	SAFE_RELEASE(m_pDevice);
	SAFE_RELEASE(m_pD3D);
}

} }