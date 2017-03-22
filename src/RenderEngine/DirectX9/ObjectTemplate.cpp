#include "RenderEngine/DirectX9/ObjectTemplate.h"
#include "RenderEngine/DirectX9/VertexManager.h"
#include "RenderEngine/DirectX9/RenderObject.h"
#include "RenderEngine/DirectX9/LightFieldInstance.h"
#include "RenderEngine/LightSources.h"
#include "General/Exceptions.h"
#include "General/GameTime.h"
#include "General/Log.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

class ObjectTemplate::ParticleSystemProxy : public ObjectTemplate::Proxy
{
    size_t              m_index;
    ptr<ParticleSystem> m_system;

public:
    ProxyInstance* CreateInstance(RenderObject& object, float time) const
    {
        return new ParticleSystemInstance(m_system, object, m_index, time);
    }

    ParticleSystemProxy(size_t index, ptr<ParticleSystem> system)
        : m_index(index), m_system(system)
    {
    }
};

class ObjectTemplate::LightFieldProxy : public ObjectTemplate::Proxy
{
    size_t                  m_index;
    const LightFieldSource& m_lightfield;

public:
    ProxyInstance* CreateInstance(RenderObject& object, float time) const
    {
        return new LightFieldInstance(m_lightfield, object, m_index, time);
    }

    LightFieldProxy(size_t index, const LightFieldSource& lightfield)
        : m_index(index), m_lightfield(lightfield)
    {
    }
};

void ObjectTemplate::DoBillboard(Matrix& world, const Model::Bone* bone) const
{
    const RenderEngine::Matrices& matrices = m_engine.GetMatrices();
    switch (bone->billboard)
    {
        case BBT_DISABLE:     break; // No billboarding
        case BBT_PARALLEL:
        case BBT_FACE:        world = matrices.m_billboardView   * world; break; // Orient towards camera 
        case BBT_ZAXIS_VIEW:  world = matrices.m_billboardZView  * world; break; // Orient towards camera (Z Axis)
        case BBT_ZAXIS_LIGHT: world = matrices.m_billboardZLight * world; break; // Orient towards light  (Z Axis)
        case BBT_ZAXIS_WIND:  world = matrices.m_billboardZWind  * world; break; // Orient towards wind   (Z Axis)

        case BBT_SUNLIGHT_GLOW:
        case BBT_SUN:           // Orient towards camera, rotate to light
            if (bone != NULL)
            {
                Matrix tmp = Matrix::Identity;
                tmp.setTranslation(bone->absTransform.getTranslation() * matrices.m_billboardSun);
                world = matrices.m_billboardView * tmp * bone->absTransform.inverse() * world;
            }
            break;
    }
}

bool ObjectTemplate::SubMesh::Render(const RenderObject& object, bool special) const
{
    bool isUaW = m_template->m_engine.IsUaW();
    IDirect3DDevice9* pDevice = m_template->m_engine.GetDevice();
    
    // Use the ShadowMap effect if we're rendering for a shadow map
    Effect* effect = (special) ? (isUaW ? m_shadowEffect : m_debugEffect) : m_effect;
    if (effect == NULL || !effect->IsSupported())
    {
        return false;
    }

    // Determine vertex processing mode
    static const DWORD VertexBlends[5] = {
        D3DVBF_DISABLE, D3DVBF_0WEIGHTS, D3DVBF_1WEIGHTS, D3DVBF_2WEIGHTS, D3DVBF_3WEIGHTS
    };

    bool software = (effect->GetSkinType() == SKIN_SOFTWARE);
    pDevice->SetSoftwareVertexProcessing(software);
    pDevice->SetRenderState(D3DRS_VERTEXBLEND, software ? VertexBlends[effect->GetNumBonesPerVertex()] : D3DVBF_DISABLE);
    pDevice->SetRenderState(D3DRS_INDEXEDVERTEXBLENDENABLE, software);

    const EffectHandles& handles = effect->GetHandles();
    ID3DXEffect*         pEffect = effect->GetEffect();

    if (!special)
    {
        // Set parameters
        for (size_t i = 0; i < m_parameters.size(); i++)
        {
            const Parameter& p = m_parameters[i];
            switch (p.m_parameter->m_type)
            {
                case SPT_TEXTURE: pEffect->SetTexture   (p.m_handle, p.m_texture->GetTexture()); break;
                case SPT_INT:     pEffect->SetInt       (p.m_handle, p.m_parameter->m_int); break;
                case SPT_FLOAT:   pEffect->SetFloat     (p.m_handle, p.m_parameter->m_float); break;
                case SPT_FLOAT3:  pEffect->SetFloatArray(p.m_handle, &p.m_parameter->m_float3.x, 3); break;
                case SPT_FLOAT4: {
                    Vector4 value = p.m_parameter->m_float4;
                    if (p.m_parameter->m_colorize)
                    {
                        // Replace color with colorization
                        value = object.GetColorization();
                    }
                    pEffect->SetVector(p.m_handle, &value); break;
                }
            }
        }

        // Set colorization
        pEffect->SetVector(handles.Colorization, &Vector4(object.GetColorization()));

        if (m_isShieldMesh)
        {
            // Override shield parameter
            pEffect->SetFloat("EdgeBrightness", 2.0f);	
        }
    }
    else if (isUaW)
    {
        // When rendering the shadow map, if the original effect has an designated
        // ALPHA_TEST_SHADOW texture, put that in sampler 0 for the shadow map shader
        // so it can use it to generate alpha-tested shadows.
        D3DXHANDLE hAlphaTestShadowTexture = m_effect->GetHandles().AlphaTestShadowTexture;
        if (hAlphaTestShadowTexture != NULL)
        {
            IDirect3DBaseTexture9* pTexture;
            m_effect->GetEffect()->GetTexture(hAlphaTestShadowTexture, &pTexture);
            pDevice->SetTexture(0, pTexture);
            SAFE_RELEASE(pTexture);
        }
    }
    else
    {
        // Shadow debugging
        DWORD fillmode = D3DFILL_SOLID;
        pDevice->GetRenderState(D3DRS_FILLMODE, &fillmode);
        pEffect->SetFloat("Intensity", (fillmode == D3DFILL_SOLID) ? 0.5f : 1.0f);
    }

    if (effect->GetSkinType() != SKIN_NONE)
    {
        const Model& model = *m_template->GetModel();

        // Get bone poses
        Matrix skinArray[MAX_NUM_SKIN_BONES];
        for (unsigned int i = 0; i < m_subMesh->nSkinBones; i++)
        {
            unsigned long bone = m_subMesh->skin[i];
            skinArray[i] = model.GetBone(bone).invAbsTransform * object.GetBoneTransform(bone);
        }

        if (software)
        {
            for (unsigned int i = 0; i < m_subMesh->nSkinBones; i++)
            {
                pDevice->SetTransform(D3DTS_WORLDMATRIX(i), &skinArray[i]);
            }
        }
        else
        {
            pEffect->SetMatrixArray(handles.SkinMatrixArray, skinArray, m_subMesh->nSkinBones);
        }
    }
    else
    {
        // Set world transform
        Matrix world = object.GetBoneTransform(m_subMesh->mesh->bone->index);
        m_template->DoBillboard(world, m_subMesh->mesh->bone);
        m_template->m_engine.SetWorldMatrix(world, effect);
    }

    // Render mesh
    m_vertices->m_buffer->SetAsSource(software);
    m_indices->m_buffer->SetAsSource(software);
    UINT nPasses = effect->Begin();
    for (UINT i = 0; i < nPasses; i++)
    {
        if (effect->BeginPass(i))
        {
            pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, m_vertices->m_start, 0, m_vertices->m_count, m_indices->m_start, m_indices->m_count / 3);
        }
        effect->EndPass();
    }
    effect->End();
    return true;
}

void ObjectTemplate::RenderDazzles(const RenderObject& object) const
{
    IDirect3DDevice9* pDevice   = m_engine.GetDevice();
    const Matrix&     billboard = m_engine.GetMatrices().m_billboardView;

    pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
    pDevice->SetVertexShader(NULL);
    pDevice->SetPixelShader(NULL);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    pDevice->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    pDevice->SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE,     FALSE);
    pDevice->SetRenderState(D3DRS_ZFUNC,            D3DCMP_LESSEQUAL);
    pDevice->SetRenderState(D3DRS_SRCBLEND,         D3DBLEND_ONE);
    pDevice->SetRenderState(D3DRS_DESTBLEND,        D3DBLEND_ONE);

    Color colorization = object.GetColorization().ToHSV();
    for (size_t i = 0; i < m_dazzles.size(); i++)
    {
        const Model::Dazzle& dazzle = m_model->GetDazzle(i);
        if (object.IsDazzleVisible(i) && object.GetBoneVisibility(dazzle.bone->index))
        {
            float t = (dazzle.frequency == 0 || dazzle.bias == 1)
                ? 0.5f
                : fmodf(GetGameTime() * dazzle.frequency + dazzle.phase, 1) / dazzle.bias;

            if (t <= 1)
            {
                Matrix transform = billboard;
                transform.setTranslation( Vector4(dazzle.position, 1) * object.GetBoneTransform(dazzle.bone->index) );

                Color c = dazzle.color * sin(PI * t);
                if (dazzle.colorize)// && colorization.r != -1)
                {
                    // Replace the dazzle's hue with the colorization's hue
                    c = c.ToHSV();
                    c.r = colorization.r;
                    c = c.ToRGB();
                }

                pDevice->SetTransform(D3DTS_WORLD, &transform);
                pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_COLORVALUE(c.r, c.g, c.b, c.a));
                pDevice->SetTexture(0, m_dazzles[i].texture->GetTexture() );
                pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_dazzles[i].vertices, sizeof(DazzleVertex));
            }
        }
    }
}

void ObjectTemplate::RenderDazzleBoundingBox(size_t index, const RenderObject& object) const
{   
    const Model::Dazzle& dazzle = m_model->GetDazzle(index);
    float ofs = dazzle.radius;

    // 3 vertices/4 indices per corner
    const Vector3 vertices[3 * 4] = {
        Vector3(-ofs, 0, -ofs),
        Vector3(-ofs, 0, -ofs) + Vector3(ofs / 4, 0, 0),
        Vector3(-ofs, 0, -ofs) + Vector3(0, 0, ofs / 4),

        Vector3(-ofs, 0,  ofs),
        Vector3(-ofs, 0,  ofs) + Vector3(ofs / 4, 0, 0),
        Vector3(-ofs, 0,  ofs) - Vector3(0, 0, ofs / 4),

        Vector3( ofs, 0, -ofs),
        Vector3( ofs, 0, -ofs) - Vector3(ofs / 4, 0, 0),
        Vector3( ofs, 0, -ofs) + Vector3(0, 0, ofs / 4),

        Vector3( ofs, 0,  ofs),
        Vector3( ofs, 0,  ofs) - Vector3(ofs / 4, 0, 0),
        Vector3( ofs, 0,  ofs) - Vector3(0, 0, ofs / 4),
    };
    static const uint16_t indices [4 * 4] = {
         0,  1,  0,  2,
         3,  4,  3,  5,
         6,  7,  6,  8,
         9, 10,  9, 11,
    };

    IDirect3DDevice9* pDevice   = m_engine.GetDevice();
    const Matrix&     billboard = m_engine.GetMatrices().m_billboardView;

    // Set world transform
    Matrix transform = billboard;
    transform.setTranslation( Vector4(dazzle.position, 1) * object.GetBoneTransform(dazzle.bone->index) );

    pDevice->SetTransform(D3DTS_WORLD, &transform);
    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(255,0,255,255));
    pDevice->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, 12, 8, &indices[0], D3DFMT_INDEX16, &vertices[0], sizeof(Vector3));
}

void ObjectTemplate::SubMesh::RenderBoundingBox(const RenderObject& object) const
{
    Vector3 min = m_subMesh->mesh->bounds.min;
    Vector3 max = m_subMesh->mesh->bounds.max;
    if (max.x - min.x < 1.0f) { min.x -= 1.0f; max.x += 1.0f; }
    if (max.y - min.y < 1.0f) { min.y -= 1.0f; max.y += 1.0f; }
    if (max.z - min.z < 1.0f) { min.z -= 1.0f; max.z += 1.0f; }

    // 4 vertices/6 indices per corner
    const Vector3  vertices[4 * 8] = {
        Vector3(min.x, min.y, min.z),
        Vector3(min.x, min.y, min.z) + Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(min.x, min.y, min.z) + Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(min.x, min.y, min.z) + Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(max.x, min.y, min.z),
        Vector3(max.x, min.y, min.z) - Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(max.x, min.y, min.z) + Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(max.x, min.y, min.z) + Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(min.x, max.y, min.z),
        Vector3(min.x, max.y, min.z) + Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(min.x, max.y, min.z) - Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(min.x, max.y, min.z) + Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(max.x, max.y, min.z),
        Vector3(max.x, max.y, min.z) - Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(max.x, max.y, min.z) - Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(max.x, max.y, min.z) + Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(min.x, min.y, max.z),
        Vector3(min.x, min.y, max.z) + Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(min.x, min.y, max.z) + Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(min.x, min.y, max.z) - Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(max.x, min.y, max.z),
        Vector3(max.x, min.y, max.z) - Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(max.x, min.y, max.z) + Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(max.x, min.y, max.z) - Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(min.x, max.y, max.z),
        Vector3(min.x, max.y, max.z) + Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(min.x, max.y, max.z) - Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(min.x, max.y, max.z) - Vector3(0, 0, (max.z - min.z) / 8),

        Vector3(max.x, max.y, max.z),
        Vector3(max.x, max.y, max.z) - Vector3((max.x - min.x) / 8, 0, 0),
        Vector3(max.x, max.y, max.z) - Vector3(0, (max.y - min.y) / 8, 0),
        Vector3(max.x, max.y, max.z) - Vector3(0, 0, (max.z - min.z) / 8)
    };
    static const uint16_t indices [6 * 8] = {
         0,  1,  0,  2,  0,  3,
         4,  5,  4,  6,  4,  7,
         8,  9,  8, 10,  8, 11,
        12, 13, 12, 14, 12, 15,
        16, 17, 16, 18, 16, 19,
        20, 21, 20, 22, 20, 23,
        24, 25, 24, 26, 24, 27,
        28, 29, 28, 30, 28, 31
    };

    IDirect3DDevice9* pDevice = m_template->GetEngine().GetDevice();

    // Set world transform
    Matrix world = object.GetBoneTransform(m_subMesh->mesh->bone->index);
    m_template->DoBillboard(world, m_subMesh->mesh->bone);
    pDevice->SetTransform(D3DTS_WORLD, &world);

    pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFFFF00);
    pDevice->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, 32, 24, &indices[0], D3DFMT_INDEX16, &vertices[0], sizeof(Vector3));
}

void ObjectTemplate::RenderBones(const RenderObject& object) const
{
    vector<Vector3> points (m_model->GetNumBones());
    vector<Matrix>  trans  (points.size());
    vector<Vector3> bonesX (points.size() * 2);
    vector<Vector3> bonesY (points.size() * 2);
    vector<Vector3> bonesZ (points.size() * 2);
    vector<Vector3> bones  (points.size() * 2);
    vector<Vector3> dazzles(m_model->GetNumDazzles());
    
    size_t nBones   = 0;
    size_t nLines   = 0;
    for (size_t i = 0; i < m_model->GetNumBones(); i++)
    {
        const Model::Bone& bone = m_model->GetBone(i);
        if (object.IsBoneVisible(i))
        {
            trans[i]    = object.GetBoneTransform(i);
            Vector3 pos = Vector4(0,0,0,1) * trans[i];

            points[nBones]     = pos;
            bonesX[nBones*2+0] = pos; bonesX[nBones*2+1] = pos + Vector4(2,0,0,0) * trans[i];
            bonesY[nBones*2+0] = pos; bonesY[nBones*2+1] = pos + Vector4(0,2,0,0) * trans[i];
            bonesZ[nBones*2+0] = pos; bonesZ[nBones*2+1] = pos + Vector4(0,0,2,0) * trans[i];
            if (bone.parent != NULL && object.IsBoneVisible(bone.parent->index))
            {
                bones[nLines*2+0] = pos;
                bones[nLines*2+1] = Vector4(0,0,0,1) * trans[bone.parent->index];
                nLines++;
            }
            nBones++;
        }
    }
    
    size_t nDazzles = 0;
    for (size_t i = 0; i < m_model->GetNumDazzles(); i++)
    {
        const Model::Dazzle& dazzle = m_model->GetDazzle(i);
        if (object.IsDazzleVisible(i))
        {
            dazzles[nDazzles++] = Vector4(dazzle.position,1) * object.GetBoneTransform(dazzle.bone->index);
        }
    }

    if (nBones > 0 || nDazzles > 0)
    {
        // Draw the bones and/or dazzles
        float psize = 5.0f;
        IDirect3DDevice9* pDevice = m_engine.GetDevice();
        pDevice->SetRenderState(D3DRS_POINTSIZE , *(DWORD*)&psize);
        pDevice->SetTransform(D3DTS_WORLD, &Matrix::Identity);
        
        if (nBones > 0)
        {
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(255,255,0,255));
            pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, (UINT)nBones, &points[0], sizeof(Vector3));
            if (nLines > 0)
            {
                // Draw the lines
                pDevice->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)nLines, &bones[0], sizeof(Vector3));
            }
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFF0000);
            pDevice->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)nBones, &bonesX[0], sizeof(Vector3));
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF00FF00);
            pDevice->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)nBones, &bonesY[0], sizeof(Vector3));
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF0000FF);
            pDevice->DrawPrimitiveUP(D3DPT_LINELIST, (UINT)nBones, &bonesZ[0], sizeof(Vector3));
        }

        if (nDazzles > 0)
        {
            // Draw the dazzles
            pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA(255,0,255,255));
            pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, (UINT)nDazzles, &dazzles[0], sizeof(Vector3));
        }
    }
}

ObjectTemplate::ObjectTemplate(ptr<Model> model, RenderEngine& engine, VertexManager& manager)
    : m_model(model), m_engine(engine)
{
    // Create submeshes
    m_subMeshes.reserve(model->GetNumSubMeshes());
    for (size_t i = 0; i < model->GetNumMeshes(); i++)
    {
        const Model::Mesh& mesh = model->GetMesh(i);
        bool isShieldMesh = (_stricmp(mesh.name.c_str(), "SHIELD") == 0);
        for (size_t j = 0; j < mesh.subMeshes.size(); j++)
        {
            m_subMeshes.push_back(SubMesh());
            SubMesh& submesh = m_subMeshes.back();
            const Model::SubMesh& srcmesh = mesh.subMeshes[j];

            submesh.m_isShieldMesh = isShieldMesh;
            submesh.m_subMesh      = &srcmesh;
            submesh.m_template     = this;
            submesh.m_effect       = engine.LoadEffect(srcmesh.shader);
            if (engine.IsUaW())
            {
                submesh.m_shadowEffect = engine.LoadShadowMapEffect(submesh.m_effect->GetVertexType());
            }
            else
            {
                submesh.m_debugEffect = engine.LoadShadowDebugEffect(submesh.m_effect->GetSkinType() != SKIN_NONE);
            }

            VertexFormat format = submesh.m_effect->GetVertexFormat();
            submesh.m_vertices = manager.CreateVertexBuffer(format, srcmesh.vertices, (DWORD)srcmesh.vertices.size());
            submesh.m_indices  = manager.CreateIndexBuffer(srcmesh.indices, (DWORD)srcmesh.indices.size());

            // Load shader parameters
            submesh.m_parameters.resize(srcmesh.parameters.size());
            for (size_t k = 0; k < submesh.m_parameters.size(); k++)
            {
                ID3DXEffect* effect = submesh.m_effect->GetEffect();
                submesh.m_parameters[k].m_handle    = effect->GetParameterByName(NULL, srcmesh.parameters[k].m_name.c_str());
                submesh.m_parameters[k].m_parameter = &srcmesh.parameters[k];
                if (srcmesh.parameters[k].m_type == SPT_TEXTURE)
                {
                    // Load texture
                    submesh.m_parameters[k].m_texture = m_engine.LoadTexture(srcmesh.parameters[k].m_texture);
                }
            }
        }
    }

    // Load proxies
    m_proxies.resize(model->GetNumProxies(), NULL);
    for (size_t i = 0; i < model->GetNumProxies(); i++)
    {
        string name = model->GetProxy(i).name;
        
        // Strip ALT and LOD from name
        string::size_type ofs;
        while ((ofs = name.find("_ALT")) != string::npos) {
            name = name.substr(0, ofs) + name.substr(ofs + 5);
        }
        while ((ofs = name.find("_LOD")) != string::npos) {
            name = name.substr(0, ofs) + name.substr(ofs + 5);
        }

        try
        {
            ptr<ParticleSystem> system = Assets::LoadParticleSystem( name );
            if (system != NULL)
            {
                m_proxies[i] = new ParticleSystemProxy(i, system);
            }
            else
            {
                const LightFieldSource* lightfield = GetLightFieldSource( name );
                if (lightfield != NULL)
                {
                    m_proxies[i] = new LightFieldProxy(i, *lightfield);
                }
            }
        }
        catch (IOException&) {}

        if (m_proxies[i] == NULL)
        {
            Log::WriteError("Error loading proxy \"%s\"\n", name.c_str());
        }
    }

    // Initialize dazzles
    m_dazzles.resize(model->GetNumDazzles());
    for (size_t i = 0; i < model->GetNumDazzles(); i++)
    {
        const Model::Dazzle& dazzle = model->GetDazzle(i);

        m_dazzles[i].texture = m_engine.LoadTexture(dazzle.texture);
        m_dazzles[i].vertices[0].TexCoord = Vector2( (float)(dazzle.texX + 0) / dazzle.texSize, (float)(dazzle.texY + 0) / dazzle.texSize);
        m_dazzles[i].vertices[1].TexCoord = Vector2( (float)(dazzle.texX + 1) / dazzle.texSize, (float)(dazzle.texY + 0) / dazzle.texSize);
        m_dazzles[i].vertices[2].TexCoord = Vector2( (float)(dazzle.texX + 1) / dazzle.texSize, (float)(dazzle.texY + 1) / dazzle.texSize);
        m_dazzles[i].vertices[3].TexCoord = Vector2( (float)(dazzle.texX + 0) / dazzle.texSize, (float)(dazzle.texY + 1) / dazzle.texSize);
        m_dazzles[i].vertices[0].Position = dazzle.radius * Vector3(-1.0f, 0, -1.0f);
        m_dazzles[i].vertices[1].Position = dazzle.radius * Vector3( 1.0f, 0, -1.0f);
        m_dazzles[i].vertices[2].Position = dazzle.radius * Vector3( 1.0f, 0,  1.0f);
        m_dazzles[i].vertices[3].Position = dazzle.radius * Vector3(-1.0f, 0,  1.0f);
    }
}

ObjectTemplate::~ObjectTemplate()
{
    for (size_t i = 0; i < m_proxies.size(); i++)
    {
        delete m_proxies[i];
    }
}

} }