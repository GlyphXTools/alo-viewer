#include "RenderEngine/DirectX9/LightFieldInstance.h"
#include "RenderEngine/DirectX9/RenderObject.h"
#include "General/GameTime.h"
#include "General/Math.h"

namespace Alamo {
namespace DirectX9 {

// These are the global light field assets. The LightFieldInstance constructor
// and destructor reference count it to ensure it's gone when there are no more
// instances left.
static Texture*   g_LightFieldTexture = NULL;
static Effect*    g_LightFieldEffect  = NULL;

void LightFieldInstance::Update()
{
    if (m_attached)
    {
        float time   = GetGameTime() - m_spawnTime;
        float scale  = 0.5f;
        bool  update = false;
        
        if (m_source.m_fadeType == LFFT_TIME && m_source.m_autoDestructTime != 0.0f)
        {
            // Check destruction or fade
            if (time >= m_source.m_autoDestructTime)
            {
                Detach();
            }
            else if (m_source.m_autoDestructFadeTime != 0.0f)
            {
                float t = (m_source.m_autoDestructTime - time) / m_source.m_autoDestructFadeTime;
                scale *= saturate(t);
            }
            update = true;
        }

        if (m_source.m_intensityNoiseScale != 0.0f && m_source.m_intensityNoiseTimeScale > 0.0f)
        {
            if (time - m_lastChange > 1.0f / m_source.m_intensityNoiseTimeScale)
            {
                // Change intensity
                scale *= 1.0f + GetRandom(-m_source.m_intensityNoiseScale, m_source.m_intensityNoiseScale);
                m_lastChange = time;
                update       = true;
            }
        }
        
        if (update)
        {
            m_quad[0].Color = m_quad[1].Color = m_quad[2].Color = m_quad[3].Color =
                m_source.m_diffuse * m_source.m_intensity * scale;
        }
    }
}

void LightFieldInstance::Render() const
{
    // Build rotation matrix
    Matrix transform = Matrix(Quaternion(Vector3(0,0,1), m_rotation + m_source.m_angularVelocity * GetGameTime()));

    // Translate to position (XY only)
    Vector3 trans = m_object.GetBoneTransform(m_bone).getTranslation();
    trans.z = 0.0f;
    transform.setTranslation(trans);

    IDirect3DDevice9* pDevice = m_engine.GetDevice();
    pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));

    m_engine.SetWorldMatrix(transform, g_LightFieldEffect);
    pDevice->SetTexture(0, g_LightFieldTexture->GetTexture());
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

    UINT nPasses = g_LightFieldEffect->Begin();
    for (UINT i = 0; i < nPasses; i++)
    {
        if (g_LightFieldEffect->BeginPass(i))
        {
            pDevice->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, TRUE);
            pDevice->SetRenderState(D3DRS_BLENDOPALPHA,   D3DBLENDOP_MAX);
            pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 2, m_quad, sizeof(VERTEX_MESH_NU2C));
        }
        g_LightFieldEffect->EndPass();
    }
}

void LightFieldInstance::Detach()
{
    m_engine.UnregisterLightFieldInstance(this);
    m_attached = false;
}

LightFieldInstance::LightFieldInstance(const LightFieldSource& source, RenderObject& object, size_t index, float time)
    : m_engine(dynamic_cast<const ObjectTemplate*>(m_object.GetTemplate())->GetEngine()),
    m_source(source), m_object(object), m_bone(m_object.GetModel().GetProxy(index).bone->index),
    m_lastChange(0.0f), m_spawnTime(time), m_attached(true)
{
    Link(object.m_instances);

    if (g_LightFieldTexture == NULL) {
        // Get the light field texture
        ptr<Texture> t = m_engine.LoadTexture("LightFieldSources.tga");
        if (t != NULL) {
            t->AddRef(); // Add a reference because we're losing the ptr<> reference
            g_LightFieldTexture = t;
        }
    } else {
        g_LightFieldTexture->AddRef();
    }

    if (g_LightFieldEffect == NULL) {
        // Get the light field effect
        ptr<Effect> e = m_engine.LoadEffect("Engine/LightFieldPointSource.fx");
        if (e != NULL) {
            e->AddRef(); // Add a reference because we're losing the ptr<> reference
            g_LightFieldEffect = e;
        }
    } else {
        g_LightFieldEffect->AddRef();
    }

    if (g_LightFieldTexture != NULL && g_LightFieldEffect != NULL)
    {
        static const Vector4 Templates[5] = {
            Vector4(0.0f, 0.00f, 0.5f, 0.5f),
            Vector4(0.5f, 0.00f, 0.5f, 0.5f),
            Vector4(0.0f, 0.50f, 0.5f, 0.5f),
            Vector4(0.5f, 0.50f, 0.5f, 0.5f),
            Vector4(0.0f, 0.25f, 0.5f, 0.0f)
        };
        const Vector4& t = Templates[source.m_type];

        // Initialize the lightfield quad
        m_quad[0].Position = Vector3(-source.m_length, -source.m_width, source.m_height);
        m_quad[1].Position = Vector3( source.m_length, -source.m_width, source.m_height);
        m_quad[2].Position = Vector3( source.m_length,  source.m_width, source.m_height);
        m_quad[3].Position = Vector3(-source.m_length,  source.m_width, source.m_height);
        m_quad[0].TexCoord = Vector2(t.x      , t.y);
        m_quad[1].TexCoord = Vector2(t.x      , t.y + t.w);
        m_quad[2].TexCoord = Vector2(t.x + t.z, t.y + t.w);
        m_quad[3].TexCoord = Vector2(t.x + t.z, t.y);
        m_quad[0].Color    = m_quad[1].Color = m_quad[2].Color = m_quad[3].Color = source.m_diffuse * source.m_intensity * 0.5f;

        // Register with the engine
        m_engine.RegisterLightFieldInstance(this);
    }

    // Determine XY rotation of attached bone as initial rotation
    const Vector3 dir = Vector4(1,0,0,0) * m_object.GetModel().GetBone(m_bone).absTransform;
    m_rotation = atan2(dir.y, dir.x);
}

LightFieldInstance::~LightFieldInstance()
{
    m_engine.UnregisterLightFieldInstance(this);

    if (g_LightFieldTexture != NULL && g_LightFieldTexture->Release() == 0) {
        g_LightFieldTexture = NULL;
    }

    if (g_LightFieldEffect != NULL && g_LightFieldEffect->Release() == 0) {
        g_LightFieldEffect = NULL;
    }
}

}
}
