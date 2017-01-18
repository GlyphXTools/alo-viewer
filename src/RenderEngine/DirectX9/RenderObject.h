#ifndef RENDEROBJECT_DX9_H
#define RENDEROBJECT_DX9_H

#include "RenderEngine/DirectX9/RenderEngine.h"
#include "RenderEngine/DirectX9/ObjectTemplate.h"
#include "RenderEngine/DirectX9/ParticleSystemInstance.h"

namespace Alamo {
namespace DirectX9 {

class RenderObject : public IRenderObject, public LinkedListObject<RenderObject>
{
    friend class ParticleSystemInstance;
    friend class LightFieldInstance;

public:
    struct SubMesh
    {
        SubMesh*                       m_next;
        SubMesh**                      m_prev;
        const Model::Mesh*             m_mesh;
        const ObjectTemplate::SubMesh* m_resources;
        bool                           m_selected;
        int                            m_alt;
        int                            m_lod;
    };

    struct Proxy
    {
        bool               m_visible;
        ptr<ProxyInstance> m_instance;
        int                m_alt;
        int                m_lod;
    };

    struct Bone
    {
        bool m_visible;
    };

    struct Dazzle
    {
        bool m_visible;
        bool m_selected;
    };

private:
    // Rendering
    SubMesh*             m_meshlist[NUM_RENDER_PHASES];
    std::vector<SubMesh> m_subMeshes;
    std::vector<Proxy>   m_proxies;
    std::vector<Bone>    m_bones;
    std::vector<Dazzle>  m_dazzles;
    Color                m_colorization;
    int                  m_alt, m_lod;

    LinkedList<ProxyInstance> m_instances;
    ptr<ObjectTemplate>       m_templ;
    const Model&              m_model;
    ptr<Animation>            m_animation;
    float                     m_time;
    float                     m_prevTime;

    void SpawnProxy(size_t index, float time);
    void KillProxy(size_t index);
    void CheckAltLod(bool altdesc);

public:
    void SetColorization(const Color& color);

    void ShowBone(size_t index);
    void HideBone(size_t index);
    bool IsBoneVisible(size_t index) const;

    void ShowMesh(size_t index);
    void HideMesh(size_t index);
    bool IsMeshVisible(size_t index) const;
    void SelectMesh(size_t index, bool selected);

    void ShowProxy(size_t index);
    void HideProxy(size_t index);
    bool IsProxyVisible(size_t index) const;

    void ShowDazzle(size_t index);
    void HideDazzle(size_t index);
    bool IsDazzleVisible(size_t index) const;
    void SelectDazzle(size_t index, bool selected);

    void SetAnimation(const ptr<Animation> anim);
    void ResetAnimation();
    void SetAnimationTime(float t);

    int GetALT() const { return m_alt; }
    void SetALT(int alt);

    int GetLOD() const { return m_lod; }
    void SetLOD(int lod);

    bool Render(const SubMesh* m, bool special) const;
    void RenderBoundingBox(const SubMesh* subMesh) const;
    void RenderDazzleBoundingBoxes() const;
    void RenderBones() const;
    void RenderDazzles() const;
    void Update();

    const IObjectTemplate*  GetTemplate()                  const { return m_templ; }
    const Model&            GetModel()                     const { return m_model; }
    const SubMesh*          GetMesh(RenderPhase phase)     const { return m_meshlist[phase]; }
    const Color&            GetColorization()              const { return m_colorization; }
    Matrix                  GetBoneTransform(size_t bone)  const;
    bool                    GetBoneVisibility(size_t bone) const;

    RenderObject(LinkedList<RenderObject> &objects, ptr<ObjectTemplate> templ, int alt, int lod);
    void Destroy();
    ~RenderObject();
};

} }

#endif