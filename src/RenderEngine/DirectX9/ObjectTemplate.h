#ifndef MODELRESOURCES_DX9_H
#define MODELRESOURCES_DX9_H

#include "RenderEngine/DirectX9/RenderEngine.h"

namespace Alamo {
namespace DirectX9 {

class VertexManager;
class RenderEngine;
class ProxyInstance;

class ObjectTemplate : public IObjectTemplate
{
    class ParticleSystemProxy;
    class LightFieldProxy;

public:
    class Proxy
    {
    public:
        virtual ProxyInstance* CreateInstance(RenderObject& object, float time) const = 0;
        virtual ~Proxy() {}
    };

    struct Parameter
    {
        const ShaderParameter* m_parameter;
        D3DXHANDLE             m_handle;
        ptr<Texture>           m_texture;
    };

    struct SubMesh
    {
        bool                    m_isShieldMesh; // Special FX parameter
        const Model::SubMesh*   m_subMesh;
        ptr<Effect>             m_effect;
        ptr<Effect>             m_shadowEffect; // ShadowMap effect, UaW only
        ptr<Effect>             m_debugEffect;  // ShadowDebug effect, EaW/FoC only
        std::vector<Parameter>  m_parameters;
        ptr<VertexBuffer>       m_vertices;
        ptr<IndexBuffer>        m_indices;
        ObjectTemplate*         m_template;

        bool Render(const RenderObject& object, bool special) const;
        void RenderBoundingBox(const RenderObject& object) const;
    };

private:
    struct DazzleVertex
    {
        Vector3 Position;
        Vector2 TexCoord;
    };

    struct Dazzle
    {
        DazzleVertex vertices[4];
        ptr<Texture> texture;
    };

    ptr<Model>           m_model;
    std::vector<SubMesh> m_subMeshes;
    std::vector<Proxy*>  m_proxies;
    RenderEngine&        m_engine;
    std::vector<Dazzle>  m_dazzles;

    void DoBillboard(Matrix& world, const Model::Bone* bone) const;
public:
    void RenderDazzleBoundingBox(size_t index, const RenderObject& object) const;
    void RenderBones(const RenderObject& object) const;
    void RenderDazzles(const RenderObject& object) const;

    RenderEngine&  GetEngine()          const { return m_engine; }
    const Model*   GetModel()           const { return m_model; }
    size_t         GetNumSubMeshes()    const { return m_subMeshes.size(); }
    const SubMesh& GetSubMesh(size_t i) const { return m_subMeshes[i]; }
    size_t         GetNumProxies()      const { return m_proxies.size(); }
    const Proxy*   GetProxy(size_t i)   const { return m_proxies[i]; }

    ObjectTemplate(ptr<Model> model, RenderEngine& engine, VertexManager& manager);
    ~ObjectTemplate();
};

} }

#endif