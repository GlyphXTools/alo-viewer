#ifndef RENDERENGINE_DX9_H
#define RENDERENGINE_DX9_H

#include "RenderEngine/RenderEngine.h"
#include "RenderEngine/DirectX9/Resources.h"
#include <set>

namespace Alamo {
namespace DirectX9 {

class VertexManager;
class Terrain;
class Texture;
class BaseEffect;
class PhaseEffect;
class Effect;
class RenderObject;
class ParticleSystemInstance;
class ParticleEmitterInstance;
class LightFieldInstance;

class ProxyInstance : public IObject, public LinkedListObject<ProxyInstance>
{
public:
    virtual void Update() = 0;
    virtual void Detach() = 0;
};

class RenderEngine : public IRenderEngine
{
    friend LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
public:
    struct Matrices
    {
        Matrix m_world;
        Matrix m_view;
        Matrix m_proj;
        Matrix m_viewInv;
        Matrix m_viewProj;
        Matrix m_billboardView;
        Matrix m_billboardSun;
        Matrix m_billboardZLight;
        Matrix m_billboardZView;
        Matrix m_billboardZWind;
        Matrix m_shadowView;
        Matrix m_shadowProj;
        Matrix m_shadow;
        Matrix m_lightFieldView;
        Matrix m_lightFieldProj;
        Matrix m_lightField;
    };

    enum FxType
    {
        FX_NORMAL,
        FX_SCENE,
        FX_SHADOWMAP,
    };

private:
	ptr<IDirect3DDevice9>  m_pDevice;
	ptr<IDirect3D9>		   m_pD3D;
    ptr<VertexManager>     m_vertexManager;
	ptr<ID3DXFont>         m_uiFont;
    D3DADAPTER_IDENTIFIER9 m_adapterInfo;
	D3DPRESENT_PARAMETERS  m_presentationParameters;
    D3DCAPS9               m_deviceCaps;
	HWND				   m_hWnd;
    Camera                 m_camera;
    Matrix                 m_cameraView;
    Matrix                 m_cameraProj;
    RenderSettings         m_settings;
    Matrices               m_matrices;
    Vector4                m_resolutionConstants;
    Vector4                m_shadowTextureInvRes;
    bool                   m_isUaW;

    // Cache of loaded assets
    typedef std::map<std::string, ptr<Texture> > TextureMap;
    typedef std::map<std::string, ptr<Effect> > EffectMap;
    TextureMap m_textureCache;
    EffectMap  m_effectCache;
    EffectMap  m_shadowEffectCache;

    struct VERTEX_GROUND
    {
    	Vector3  Position;
    	Vector3  Normal;
        D3DCOLOR Color;
        Vector4  TexCoord;
    };

    // Shadows and bloom
    bool             m_hasStencilBuffer;
    ptr<Effect>      m_stencilDarken;
    ptr<Effect>      m_stencilDarkenFinalBlur;
    ptr<Effect>      m_stencilDarkenToAlpha;
    ptr<Effect>      m_heatEffect;
    VERTEX_MESH_NC   m_shadowQuad[4];
    VERTEX_GROUND    m_groundQuad[4];
    ptr<Effect>      m_bloomEffect;
    ptr<Texture>     m_groundTexture;
    ptr<Texture>     m_groundBumpMap;
    ptr<Effect>      m_groundEffect;
    ptr<Effect>      m_shadowDebugEffect_RSkin;
    ptr<Effect>      m_shadowDebugEffect_Mesh;
    VERTEX_MESH_NU2C m_sceneQuad[4];

    // Linked list of loaded effects
    BaseEffect*      m_effects;
    
    // Renderable instance
    LinkedList<RenderObject>          m_objects;
    std::set<ParticleSystemInstance*> m_particleSystems;
    std::set<LightFieldInstance*>     m_lightfields;

    //
    // Resources
    //
    Environment            m_environment;
    ptr<IDirect3DTexture9> m_BloomTexture;
    ptr<IDirect3DTexture9> m_HeatTexture;
    ptr<IDirect3DTexture9> m_DistortionTexture;
    ptr<IDirect3DSurface9> m_ShadowSurface;
    ptr<IDirect3DSurface9> m_ShadowDepthDummy;
    ptr<IDirect3DTexture9> m_ShadowDepthMap;
    ptr<IDirect3DTexture9> m_ShadowTexture;
    ptr<IDirect3DTexture9> m_LightFieldTexture;
    ptr<IDirect3DTexture9> m_BlendTexture;
    ptr<PhaseEffect>       m_phases[NUM_RENDER_PHASES];
    ptr<PhaseEffect>       m_shadowMapEffect;

    Matrix       m_LightSphAll[3];
    Matrix       m_LightSphFill[3];

    D3DFORMAT GetDepthStencilFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat);
	void      GetMultiSampleType   (UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT DisplayFormat);

    void OnResize();
	void OnLostDevice();
	void OnResetDevice();
    void OnResolutionChanged();
    void LoadStandardResources();
    void LoadPhaseEffects();
    void LoadDynamicResources();
    void UnloadDynamicResources();

    void SetViewMatrix(const Matrix& view, Effect* pEffect = NULL);
    void SetProjectionMatrix(const Matrix& proj, Effect* pEffect = NULL);
    void ComputeShadowMatrix();
    void ComputeLightFieldMatrix();
    
    bool RenderRenderPhase(RenderPhase phase, bool shadowMap = false) const;
    void RenderOverlays(bool showBones) const;
    void RenderBoneNames() const;
    void RenderGround(float groundLevel, D3DCOLOR color);

    void SetLegacyLights();
    void InitializeRenderStates();

    ID3DXEffect*     LoadShader(const std::string& name, bool usePlaceholder = true);
    ptr<PhaseEffect> LoadPhaseEffect(const std::string& name);
    void             InitializeEffect(ptr<Effect> effect) const;

	~RenderEngine();
public:

    void SetCamera(const Camera& camera);
    void SetSettings(const RenderSettings& settings);
    void SetEnvironment(const Environment& environment);

    void SetDepthBias(float distance, Effect* pEffect = NULL);
    void SetWorldMatrix(const Matrix& world, Effect* pEffect = NULL);

    const Camera&         GetCamera()      const { return m_camera; }
    const RenderSettings& GetSettings()    const { return m_settings; }
    const Matrices&       GetMatrices()    const { return m_matrices; }
    const Environment&    GetEnvironment() const { return m_environment; }
    bool                  IsUaW()          const { return m_isUaW; }

    IDirect3DDevice9* GetDevice() const;
    
    void RegisterParticleSystemInstance(ParticleSystemInstance* instance);
    void UnregisterParticleSystemInstance(ParticleSystemInstance* instance);

    void RegisterLightFieldInstance(LightFieldInstance* instance);
    void UnregisterLightFieldInstance(LightFieldInstance* instance);

	void Render(const RenderOptions& options);

    void ClearTextureCache();

    ptr<Effect>          LoadEffect(const std::string& name, FxType type = FX_NORMAL);
    ptr<Effect>          LoadShadowMapEffect(const std::string& vertexType);
    ptr<Effect>          LoadShadowDebugEffect(bool rskin);
    ptr<Texture>         LoadTexture(const std::string& name, bool usePlaceholder = true);
    ptr<IObjectTemplate> CreateObjectTemplate(ptr<Model> model);
    ptr<IRenderObject>   CreateRenderObject(ptr<IObjectTemplate> templ, int alt, int lod);

	RenderEngine(HWND hWnd, const RenderSettings& settings, const Environment& env, bool isUaW);
};

} }
#endif