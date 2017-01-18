#include "RenderEngine/DirectX9/Exceptions.h"
#include "RenderEngine/DirectX9/RenderObject.h"
#include "RenderEngine/DirectX9/LightFieldInstance.h"
#include "General/GameTime.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

// Vendor ID for nVidia
static const int VENDOR_NVIDIA = 4318;

// Special is either
// UaW: for a shadow map
// EaW: for shadow debugging
bool RenderEngine::RenderRenderPhase(RenderPhase phase, bool special) const
{
    bool rendered = false;
    if (phase != PHASE_TRANSPARENT)
    {
        // Render meshes and particles unsorted
        for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
        {
            // Render the meshes from the object
            for (const RenderObject::SubMesh* submesh = object->GetMesh(phase); submesh != NULL; submesh = submesh->m_next)
            {
                rendered |= object->Render(submesh, special);
            }
        }
        
        for (set<ParticleSystemInstance*>::const_iterator p = m_particleSystems.begin(); p != m_particleSystems.end(); p++)
        {
            rendered |= (*p)->Render(phase);
        }
    }
    else
    {
        // Render meshes and particles sorted by distance
        multimap<float, pair<const RenderObject*, const RenderObject::SubMesh*> > submeshes;
        multimap<float, const ParticleSystemInstance*> particleSystems;

        for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
        {
            // Render the meshes from the object
            for (const RenderObject::SubMesh* submesh = object->GetMesh(phase); submesh != NULL; submesh = submesh->m_next)
            {
                float distance = (object->GetBoneTransform(submesh->m_mesh->bone->index).getTranslation() * m_matrices.m_view).z;
                submeshes.insert(make_pair(distance, make_pair(object, submesh)));                
            }
        }
        
        for (set<ParticleSystemInstance*>::const_iterator p = m_particleSystems.begin(); p != m_particleSystems.end(); p++)
        {
            float distance = ((*p)->GetTransform().getTranslation() * m_matrices.m_view).z;
            particleSystems.insert(make_pair(distance, *p));
        }

        // Render meshes
        for (multimap<float, pair<const RenderObject*, const RenderObject::SubMesh*> >::const_iterator p = submeshes.begin(); p != submeshes.end(); p++)
        {
            rendered |= (p->second.first)->Render(p->second.second, false);
        }

        if (m_isUaW)
        {
            // Render dazzles
            for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
            {
                object->RenderDazzles();
            }
        }

        // Render particle systems, sorted
        for (multimap<float, const ParticleSystemInstance*>::const_iterator p = particleSystems.begin(); p != particleSystems.end(); p++)
        {
            rendered |= p->second->Render(phase);
        }
    }

    return rendered;
}

void RenderEngine::RenderBoneNames() const
{
    D3DVIEWPORT9 viewport;
    m_pDevice->GetViewport(&viewport);
    for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
    {
        const Model& model = object->GetModel();
        for (size_t i = 0; i < model.GetNumBones(); i++)
        {
            if (object->IsBoneVisible(i))
            {
                Matrix trans       = object->GetBoneTransform(i);
                const string& name = model.GetBone(i).name;

                // Transform bone pos into screen space
                Vector3 pos;
                D3DXVec3Project(&pos, &Vector3(0,0,0), &viewport, &m_matrices.m_proj, &m_matrices.m_view, &trans);
                if (pos.z > 0 && pos.z < 1)
                {
                    // Calculate text dimensions
				    RECT r = {0,0,0,0};
                    m_uiFont->DrawTextA(NULL, name.c_str(), (int)name.length(), &r, DT_CALCRECT, 0);
                    
                    // Render text
                    r.left   = (int)pos.x - r.right  / 2;
                    r.top    = (int)pos.y - r.bottom - 2;
                    r.right  = r.left + r.right;
                    r.bottom = r.top  + r.bottom;
				    m_uiFont->DrawTextA(NULL, name.c_str(), (int)name.length(), &r, 0, D3DCOLOR_XRGB(255,255,0));
                }
            }
        }

        for (size_t i = 0; i < model.GetNumDazzles(); i++)
        {
            const Model::Dazzle& dazzle = model.GetDazzle(i);
            Matrix trans       = object->GetBoneTransform(dazzle.bone->index);

            // Transform bone pos into screen space
            Vector3 pos;
            D3DXVec3Project(&pos, &dazzle.position, &viewport, &m_matrices.m_proj, &m_matrices.m_view, &trans);
            if (pos.z > 0 && pos.z < 1)
            {
                // Calculate text dimensions
			    RECT r = {0,0,0,0};
                m_uiFont->DrawTextA(NULL, dazzle.name.c_str(), (int)dazzle.name.length(), &r, DT_CALCRECT, 0);
                
                // Render text
                r.left   = (int)pos.x - r.right  / 2;
                r.top    = (int)pos.y - r.bottom - 2;
                r.right  = r.left + r.right;
                r.bottom = r.top  + r.bottom;
			    m_uiFont->DrawTextA(NULL, dazzle.name.c_str(), (int)dazzle.name.length(), &r, 0, D3DCOLOR_XRGB(255,0,255));
            }
        }
    }
}

// Renders GUI overlays
void RenderEngine::RenderOverlays(bool showBones) const
{
    m_pDevice->SetVertexShader(NULL);
    m_pDevice->SetPixelShader(NULL);
    m_pDevice->SetFVF(D3DFVF_XYZ);
    m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
    m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
    m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TFACTOR);
    m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);

    if (showBones)
    {
        // Render bones
        for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
        {
            object->RenderBones();
        }
    }

    // Render mesh bounding boxes
    for (const RenderObject* object = m_objects; object != NULL; object = object->GetNext())
    {
        // Render the meshes from the object
        for (int phase = 0; phase < NUM_RENDER_PHASES; phase++)
        {
            const RenderObject::SubMesh* submesh = object->GetMesh((RenderPhase)phase);
            while (submesh != NULL)
            {
                object->RenderBoundingBox(submesh);
                submesh = submesh->m_next;
            }
        }
        object->RenderDazzleBoundingBoxes();
    }

    // Restore the old states
    m_pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
}

void RenderEngine::RenderGround(float groundLevel, D3DCOLOR color)
{
    // Render the ground plane
    Matrix world = Matrix::Identity;
    world.setTranslation(Vector3(0,0,groundLevel));
    m_pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE4(0));
    m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    m_pDevice->SetSoftwareVertexProcessing(m_groundEffect->IsFixedFunction());

    if (m_groundEffect != NULL && m_groundEffect->IsSupported())
    {
        SetWorldMatrix(world, m_groundEffect);
        
        ID3DXEffect* effect = m_groundEffect->GetEffect();
        if (m_groundTexture != NULL && m_groundBumpMap != NULL)
        {
            effect->SetTexture(effect->GetParameterBySemantic(NULL, "CLOUD_TEXTURE"), m_BlendTexture);
            effect->SetTexture("blendTexture", m_BlendTexture);
            effect->SetTexture("diffuseTexture", m_groundTexture->GetTexture());
            effect->SetTexture("normalTexture",  m_groundBumpMap->GetTexture());            
            effect->SetVector("diffuseTexU", &Vector4( 1.0f / 50, 0, 0, 0));
            effect->SetVector("diffuseTexV", &Vector4( 0, 1.0f / 50, 0, 0));
        }
        effect->SetTexture(m_groundEffect->GetHandles().ShadowTexture, m_ShadowDepthMap);
        effect->SetVector("materialAdditive", &Vector4(0,0,0,0));

        UINT nPasses = m_groundEffect->Begin();
        for (UINT i = 0; i < nPasses; i++)
        {
            if (m_groundEffect->BeginPass(i))
            {
                m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_groundQuad[0], sizeof m_groundQuad[0]);
            }
            m_groundEffect->EndPass();
        }
        m_groundEffect->End();
    }
    else
    {
        Matrix shadow = world * m_matrices.m_shadow;

        static const D3DMATERIAL9 material = {{1,1,1,1}, {1,1,1,1}, {0,0,0,0}, {0,0,0,0}, 1.0f};
        m_pDevice->SetMaterial(&material);
        m_pDevice->SetTransform(D3DTS_WORLD, &world);
        m_pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);
        
        if (m_adapterInfo.VendorId == VENDOR_NVIDIA)
        {
            // We can show the shadow map in fixed function.
            // This is because texture lookups for depth textures in nVidia cards
            // result in black or white based on the depth comparison.
            m_pDevice->SetTexture(0, m_ShadowDepthMap);
            m_pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4);
            m_pDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
            m_pDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
            m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            m_pDevice->SetTransform(D3DTS_TEXTURE0, &shadow);

            m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
            m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_CURRENT);
            m_pDevice->SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pDevice->SetTextureStageState(2, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        }
        else
        {
            // No shadow map, just regular texture
            m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
            m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
            m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
        }
        m_pDevice->SetVertexShader(NULL);
        m_pDevice->SetPixelShader(NULL);
        m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_groundQuad[0], sizeof m_groundQuad[0]);
        m_pDevice->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }
    m_vertexManager->ResetActiveStreams();
}

// Renders a complete, single frame
void RenderEngine::Render(const RenderOptions& options)
{
    HRESULT hRes;
	if (FAILED(hRes = m_pDevice->TestCooperativeLevel()))
	{
		// Check if we lost or should restore the device
		switch (hRes)
		{
			case D3DERR_DEVICELOST:
				OnLostDevice();
				return;

			case D3DERR_DEVICENOTRESET:
                // Sometimes we can get a D3DERR_DEVICENOTRESET without a D3DERR_DEVICELOST
                // before it. So call OnLostDevice() just to be sure.
                OnLostDevice();

				if (FAILED(hRes = m_pDevice->Reset(&m_presentationParameters)))
				{
					throw DirectXException(hRes);
				}
				OnResetDevice();
				break;

			default:
				throw DirectXException(hRes);
		}
	}

    const float time = GetGameTime();

    // Update all effects with time-based values
    float windHeading = m_environment.m_wind.heading - ToRadians(90);
    const Vector4 windBendVector(cos(windHeading) * sin(time), sin(windHeading) * sin(time), 0, 0.002f);
    for (BaseEffect* p = m_effects; p != NULL; p = p->Next())
    {
        const EffectHandles& handles = p->GetHandles();
        ID3DXEffect* pEffect = p->GetEffect();
        pEffect->SetFloat(handles.Time, time);
        pEffect->SetVector(handles.WindBendFactor, &windBendVector);
    }

    const bool doHeat        = (m_settings.m_heatDistortion && !m_settings.m_heatDebug && m_DistortionTexture != NULL && m_heatEffect != NULL && m_heatEffect->IsSupported());
    const bool doBloom       = (m_settings.m_bloom && m_BloomTexture != NULL && m_bloomEffect != NULL && m_bloomEffect->IsSupported());
    const bool doSoftShadows = (!m_isUaW && m_hasStencilBuffer && m_ShadowTexture != NULL && m_settings.m_softShadows && m_stencilDarkenToAlpha != NULL && m_stencilDarkenToAlpha->IsSupported() && m_stencilDarkenFinalBlur != NULL && m_stencilDarkenFinalBlur->IsSupported());
    const bool doShadows     = (!m_isUaW && m_hasStencilBuffer && (doSoftShadows || m_stencilDarken != NULL));
    const bool multiSampling = (m_presentationParameters.MultiSampleType != D3DMULTISAMPLE_NONE);
    const bool doLightFields = (m_isUaW && m_LightFieldTexture != NULL);
    const bool doShadowMaps  = (m_isUaW && m_ShadowDepthMap    != NULL);

    // Render the scene
	m_pDevice->BeginScene();

    IDirect3DSurface9* pBackBuffer   = NULL;
    IDirect3DSurface9* pBloomSurface = NULL;
    IDirect3DSurface9* pHeatSurface  = NULL;
    if (doBloom || doHeat || doSoftShadows)
    {
        // Backup render target
        m_pDevice->GetRenderTarget(0, &pBackBuffer);

        if (doBloom || doHeat)
        {
            if (doBloom) m_BloomTexture->GetSurfaceLevel(0, &pBloomSurface);
            if (doHeat)  m_HeatTexture ->GetSurfaceLevel(0, &pHeatSurface);

            if (!multiSampling)
            {
                // No multisampling; render directly into heat or bloom scene texture
                m_pDevice->SetRenderTarget(0, doHeat ? pHeatSurface : pBloomSurface);
            }
        }
    }

    // Clear all textures to be safe
    for (UINT i = 0; i < m_deviceCaps.MaxSimultaneousTextures; i++)
    {
    	m_pDevice->SetTexture(i, NULL);
        m_pDevice->SetSamplerState(i, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        m_pDevice->SetSamplerState(i, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    }

    D3DCOLOR ClearColor = D3DCOLOR_COLORVALUE(m_environment.m_clearColor.r, m_environment.m_clearColor.g, m_environment.m_clearColor.b, m_environment.m_clearColor.a);
    DWORD    ClearFlags = D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER;
    DWORD    FillMode   = (options.showWireframe) ? D3DFILL_WIREFRAME : D3DFILL_SOLID;
    if (m_hasStencilBuffer)
    {
        ClearFlags |= D3DCLEAR_STENCIL;
    }

    //
    // Render the phases
    //
    m_pDevice->Clear(0, NULL, ClearFlags, ClearColor, 1.0f, 0);
    m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);

    m_vertexManager->ResetActiveStreams();

    if (doShadowMaps || doLightFields)
    {
        IDirect3DSurface9* pOldRenderTarget, *pOldDepthSurface;
        m_pDevice->GetRenderTarget(0, &pOldRenderTarget);
        m_pDevice->GetDepthStencilSurface(&pOldDepthSurface);

        if (doShadowMaps)
        {
            IDirect3DSurface9* pShadowDepthMap;
            m_ShadowDepthMap->GetSurfaceLevel(0, &pShadowDepthMap);
            m_pDevice->SetRenderTarget(0, m_ShadowDepthDummy);
            m_pDevice->SetDepthStencilSurface(pShadowDepthMap);
            m_pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
            SAFE_RELEASE(pShadowDepthMap);

            SetViewMatrix(m_matrices.m_shadowView);
            SetProjectionMatrix(m_matrices.m_shadowProj);

            // Render opaque meshes in shadow phase
            m_phases[PHASE_SHADOW]->Begin();
            m_shadowMapEffect->Begin();
            RenderRenderPhase(PHASE_OPAQUE, true);
            m_shadowMapEffect->End();
            m_phases[PHASE_SHADOW]->End();
        }

        if (doLightFields)
        {
            IDirect3DSurface9* pLightFieldTexture;
            m_LightFieldTexture->GetSurfaceLevel(0, &pLightFieldTexture);
            m_pDevice->SetDepthStencilSurface(NULL);
            m_pDevice->SetRenderTarget(0, pLightFieldTexture);
            m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0, 1.0f, 0);
            SAFE_RELEASE(pLightFieldTexture);

            SetViewMatrix(m_matrices.m_lightFieldView);
            SetProjectionMatrix(m_matrices.m_lightFieldProj);

            for (set<LightFieldInstance*>::const_iterator p = m_lightfields.begin(); p != m_lightfields.end(); ++p)
            {
                (*p)->Render();
            }
        }

        m_pDevice->SetRenderTarget(0, pOldRenderTarget);
        m_pDevice->SetDepthStencilSurface(pOldDepthSurface);
        SAFE_RELEASE(pOldDepthSurface);
        SAFE_RELEASE(pOldRenderTarget);

        SetViewMatrix(m_cameraView);
        SetProjectionMatrix(m_cameraProj);
    }

    m_vertexManager->ResetActiveStreams();

    // Render Opaque phase
    m_phases[PHASE_OPAQUE]->Begin();
    if (options.showGround)
    {
        RenderGround(options.groundLevel, ClearColor);

        // Reset address mapping
        m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
        m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
    }

    m_pDevice->SetRenderState(D3DRS_FILLMODE, FillMode );
    RenderRenderPhase(PHASE_OPAQUE);
    m_phases[PHASE_OPAQUE]->End();

    if (!m_isUaW)
    {
        if (m_settings.m_shadowDebug)
        {
            // Render shadow meshes as opaque meshes with highlighted edges
            m_phases[PHASE_OPAQUE]->Begin();

            m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
            RenderRenderPhase(PHASE_SHADOW, true);

            SetDepthBias(0.001f);
            DWORD cullmode;
            m_pDevice->GetRenderState(D3DRS_CULLMODE, &cullmode);
            m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
            m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME);
            RenderRenderPhase(PHASE_SHADOW, true);
            m_pDevice->SetRenderState(D3DRS_CULLMODE, cullmode);
            SetDepthBias(0.0f);

            m_phases[PHASE_OPAQUE]->End();
        }
        else if (doShadows)
        {
            // Use shadow volume
            // Render the shadows to the stencil buffer
            m_phases[PHASE_SHADOW]->Begin();

            SetDepthBias(-0.01f);
            bool rendered = RenderRenderPhase(PHASE_SHADOW);
            SetDepthBias(0.0f);

            m_pDevice->SetSoftwareVertexProcessing(m_stencilDarken->IsFixedFunction());
            m_pDevice->SetRenderState(D3DRS_FILLMODE,            D3DFILL_SOLID);
            m_pDevice->SetRenderState(D3DRS_VERTEXBLEND,         FALSE);
            m_pDevice->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE);

            if (rendered)
            {
                // We've rendered shadows meshes; apply rest of effect
                m_pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE);
                m_pDevice->SetTexture(0, NULL);
                m_pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1);
                m_pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_CURRENT);
                m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
                m_pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
                m_pDevice->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
                m_pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);

                if (!doSoftShadows)
                {
                    // Do regular, hard shadows
                    SetWorldMatrix     (Matrix::Identity, m_stencilDarken);
                    SetViewMatrix      (Matrix::Identity, m_stencilDarken);
                    SetProjectionMatrix(Matrix::Identity, m_stencilDarken);

                    UINT nPasses = m_stencilDarken->Begin();
                    for (UINT i = 0; i < nPasses; i++)
                    {
                        if (m_stencilDarken->BeginPass(i))
                        {
                            m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_shadowQuad[0], sizeof(VERTEX_MESH_NC));
                        }
                        m_stencilDarken->EndPass();
                    }
                    m_stencilDarken->End();
                }
                else
                {
                    // Do soft shadows
                    // Render stencil to texture's alpha
                    IDirect3DSurface9* pShadowSurface;
                    m_ShadowTexture->GetSurfaceLevel(0, &pShadowSurface);
                    if (m_ShadowSurface != NULL) {
                        // Render to multisampled backbuffer first, then copy to texture
                        m_pDevice->SetRenderTarget(0, m_ShadowSurface);
                    } else {
                        // No multisampling; render to texture immediately
                        m_pDevice->SetRenderTarget(0, pShadowSurface);
                    }
                    m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255,255,255,255), 1.0f, 0);

                    // Render the stencil buffer into the alpha buffer
                    SetWorldMatrix     (Matrix::Identity, m_stencilDarkenToAlpha);
                    SetViewMatrix      (Matrix::Identity, m_stencilDarkenToAlpha);
                    SetProjectionMatrix(Matrix::Identity, m_stencilDarkenToAlpha);

                    UINT nPasses = m_stencilDarkenToAlpha->Begin();
                    for (UINT i = 0; i < nPasses; i++)
                    {
                        if (m_stencilDarkenToAlpha->BeginPass(i))
                        {
                            m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_shadowQuad[0], sizeof(VERTEX_MESH_NC));
                        }
                        m_stencilDarkenToAlpha->EndPass();
                    }
                    m_stencilDarkenToAlpha->End();

                    if (m_ShadowSurface != NULL)
                    {
                        // Copy (and downsample) to texture
                        m_pDevice->StretchRect(m_ShadowSurface, NULL, pShadowSurface, NULL, D3DTEXF_POINT);
                    }
                    SAFE_RELEASE(pShadowSurface);

                    // Restore render target
                    m_pDevice->SetRenderTarget(0, (multiSampling || !(doHeat || doBloom)) ? pBackBuffer : (doHeat ? pHeatSurface : pBloomSurface));

                    SetWorldMatrix     (Matrix::Identity, m_stencilDarkenFinalBlur);
                    SetViewMatrix      (Matrix::Identity, m_stencilDarkenFinalBlur);
                    SetProjectionMatrix(Matrix::Identity, m_stencilDarkenFinalBlur);

                    // Use the alpha texture to render a soft shadow
                    m_pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
		            m_pDevice->SetTexture(0, m_ShadowTexture);
		            m_pDevice->SetTexture(1, m_ShadowTexture);
		            m_pDevice->SetTexture(2, m_ShadowTexture);
		            m_pDevice->SetTexture(3, m_ShadowTexture);
                    nPasses = m_stencilDarkenFinalBlur->Begin();
                    for (UINT i = 0; i < nPasses; i++)
                    {
                        if (m_stencilDarkenFinalBlur->BeginPass(i))
                        {
                            m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_sceneQuad[0], sizeof(VERTEX_MESH_NU2C));
                        }
                        m_stencilDarkenFinalBlur->EndPass();
                    }
                    m_stencilDarkenFinalBlur->End();
                }
                
                // Restore view and projection matrices
                SetViewMatrix      (m_cameraView, m_stencilDarken);
                SetProjectionMatrix(m_cameraProj, m_stencilDarken);
            }
            
            m_pDevice->SetRenderState(D3DRS_FILLMODE, FillMode );
            m_phases[PHASE_SHADOW]->End();
        }
    }
    m_vertexManager->ResetActiveStreams();
    m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID );

    m_phases[PHASE_TRANSPARENT]->Begin();
    RenderRenderPhase(PHASE_TRANSPARENT);
    if (m_settings.m_heatDistortion && m_settings.m_heatDebug)
    {
        // If we're debugging heat, render them in the transparent phase
        RenderRenderPhase(PHASE_HEAT);
    }
    m_phases[PHASE_TRANSPARENT]->End();

    m_pDevice->SetRenderState(D3DRS_VERTEXBLEND, FALSE);
    
    //
    // When we get here, the shadowed scene is rendered in:
    // * pBackBuffer if multisampling is enabled or heat and bloom are disabled
    // * pHeatSurface otherwise and if doHeat
    // * pBloomSurface otherwise

    if (doHeat)
    {
        if (multiSampling)
        {
            // Copy render target into scene texture
            m_pDevice->StretchRect(pBackBuffer, NULL, pHeatSurface, NULL, D3DTEXF_POINT);
        }

        // Render heat phase
        m_phases[PHASE_HEAT]->Begin();

        // Render heat distortions to the heat texture
	    IDirect3DSurface9* pDistortionSurface;
	    m_DistortionTexture->GetSurfaceLevel(0, &pDistortionSurface);
        m_pDevice->SetRenderTarget(0, pDistortionSurface);
    	m_pDevice->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(129,128,255), 1.0f, 0);
	    SAFE_RELEASE(pDistortionSurface);

        RenderRenderPhase(PHASE_HEAT);

        // Restore render target
        m_pDevice->SetRenderTarget(0, (doBloom && !multiSampling) ? pBloomSurface : pBackBuffer );

        // We rendered heat primitives, apply rest of effect
        ID3DXEffect* pEffect = m_heatEffect->GetEffect();
        pEffect->SetTexture("SceneTexture",      m_HeatTexture);
        pEffect->SetTexture("DistortionTexture", m_DistortionTexture);

        m_pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
        SetWorldMatrix     (Matrix::Identity, m_heatEffect);
        SetViewMatrix      (Matrix::Identity, m_heatEffect);
        SetProjectionMatrix(Matrix::Identity, m_heatEffect);
        UINT nPasses = m_heatEffect->Begin();
        for (UINT i = 0; i < nPasses; i++)
        {
	        m_heatEffect->BeginPass(i);
            m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            m_pDevice->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
            m_pDevice->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
	        m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_sceneQuad[0], sizeof(VERTEX_MESH_NU2C));
	        m_heatEffect->EndPass();
        }
        m_heatEffect->End();

        pEffect->SetTexture("SceneTexture",      NULL);
        pEffect->SetTexture("DistortionTexture", NULL);
        SetViewMatrix      (m_cameraView, m_heatEffect);
        SetProjectionMatrix(m_cameraProj, m_heatEffect);

        m_phases[PHASE_HEAT]->End();
    }
    m_vertexManager->ResetActiveStreams();

    //
    // Post-processing
    //

    if (doBloom)
    {
        // Bloom
        if (multiSampling)
        {
            // Copy render target into scene texture
            m_pDevice->StretchRect(pBackBuffer, NULL, pBloomSurface, NULL, D3DTEXF_POINT);
        }

        // Restore render target
        m_pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID );
        m_pDevice->SetRenderTarget(0, pBackBuffer);

        m_pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
        SetWorldMatrix     (Matrix::Identity, m_bloomEffect);
        SetViewMatrix      (Matrix::Identity, m_bloomEffect);
        SetProjectionMatrix(Matrix::Identity, m_bloomEffect);
        m_bloomEffect->GetEffect()->SetTexture("SceneTexture", m_BloomTexture);
        UINT nPasses = m_bloomEffect->Begin();
        for (UINT i = 0; i < nPasses; i++)
        {
            if (m_bloomEffect->BeginPass(i))
            {
                m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, &m_sceneQuad[0], sizeof(VERTEX_MESH_NU2C));
            }
            m_bloomEffect->EndPass();
        }
        m_bloomEffect->End();
        m_bloomEffect->GetEffect()->SetTexture("SceneTexture", NULL);
        SetViewMatrix      (m_cameraView, m_bloomEffect);
        SetProjectionMatrix(m_cameraProj, m_bloomEffect);
    }
    SAFE_RELEASE(pBackBuffer);
    SAFE_RELEASE(pHeatSurface);
    SAFE_RELEASE(pBloomSurface);

    //
    // Render UI overlays
    //
    RenderOverlays(options.showBones);

    if (options.showBones && options.showBoneNames && m_uiFont != NULL)
    {
        // Render bone names
        RenderBoneNames();
    }

    m_pDevice->EndScene();
	m_pDevice->Present(NULL, NULL, NULL, NULL);
}

}
}