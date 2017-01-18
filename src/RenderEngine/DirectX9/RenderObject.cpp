#include "RenderEngine/DirectX9/RenderObject.h"
#include "General/GameTime.h"
using namespace std;

namespace Alamo {
namespace DirectX9 {

// Extracts the alt and lod level from a name like "NAME_ALT1_LOD0"
// and returns the name without the suffixes. If any of the two suffixes
// aren't present, they're set to -1.
static void ParseName(string name, int* alt, int *lod)
{
    int* const values[2] = {alt, lod};
    static const char* names[2] = {"_ALT", "_LOD"};

    for (int i = 0; i < 2; i++)
    {
        *values[i] = -1;
        string::size_type pos = name.find(names[i]);
        if (pos != string::npos)
        {
            // Extract value
            string tail = name.substr(pos + 4);
            char* endptr;
            int val = strtoul(tail.c_str(), &endptr, 10);
            string::size_type n = endptr - tail.c_str();
            if (n > 0)
            {
                // We have at least one number
                *values[i] = max(0, val);
            }
            name = name.substr(0, pos) + tail.substr(n);
        }
    }
}

void RenderObject::SetColorization(const Color& color)
{
    // Set colorization, always force alpha channel to 100%
    m_colorization = color;
    m_colorization.a = 1.0f;
}

void RenderObject::SetAnimation(const ptr<Animation> anim)
{
    m_animation = anim;
    m_time      = 0.0f;
}

void RenderObject::ResetAnimation()
{
    m_prevTime = 0.0f;
    m_time     = 0.0f;

    // The animation has looped around. Kill any proxies which are not supposed to be visible
    // at the beginning of the animation.
    for (size_t i = 0; i < m_proxies.size(); i++)
    {
        const Model::Proxy& proxy = m_model.GetProxy(i);
        if (!proxy.isVisible || !proxy.bone->visible)
        {
            KillProxy(i);
        }
    }
}

void RenderObject::SetAnimationTime(float t)
{
    m_prevTime = (t < m_time) ? 0.0f : m_time;
    m_time     = t;
}

Matrix RenderObject::GetBoneTransform(size_t bone) const
{
    if (m_animation != NULL)
    {
        return m_animation->GetFrame(bone, m_time);
    }
    return m_model.GetBone(bone).absTransform;
}

bool RenderObject::GetBoneVisibility(size_t bone) const
{
    if (m_animation != NULL)
    {
        return m_animation->IsVisible(bone, m_time);
    }
    return m_model.GetBone(bone).visible;
}

void RenderObject::RenderBoundingBox(const SubMesh* subMesh) const
{
    if (subMesh->m_selected)
    {
        subMesh->m_resources->RenderBoundingBox(*this);
    }
}

void RenderObject::RenderDazzleBoundingBoxes() const
{
    for (size_t i = 0; i < m_dazzles.size(); i++)
    {
        if (m_dazzles[i].m_selected && m_dazzles[i].m_visible && GetBoneVisibility(m_model.GetDazzle(i).bone->index))
        {
            m_templ->RenderDazzleBoundingBox(i, *this);
        }
    }
}

bool RenderObject::Render(const SubMesh* subMesh, bool special) const
{
    if (GetBoneVisibility(subMesh->m_mesh->bone->index))
    {
        return subMesh->m_resources->Render(*this, special);
    }
    return false;
}

void RenderObject::RenderBones() const
{
    m_templ->RenderBones(*this);
}

void RenderObject::RenderDazzles() const
{
    m_templ->RenderDazzles(*this);
}

void RenderObject::SpawnProxy(size_t index, float time)
{
    const ObjectTemplate::Proxy* templ = m_templ->GetProxy(index);
    if (templ != NULL)
    {
        const Model::Proxy& proxy = m_model.GetProxy(index);
        ptr<RenderObject> pThis = this;
        pThis->AddRef();
        
        if (m_proxies[index].m_instance != NULL)
        {
            // We're replacing an existing proxy
            m_proxies[index].m_instance->Detach();
        }
        
        m_proxies[index].m_instance = templ->CreateInstance(*this, time);
    }
}

void RenderObject::KillProxy(size_t index)
{
    if (m_proxies[index].m_instance != NULL)
    {
        m_proxies[index].m_instance->Detach();
        SAFE_RELEASE(m_proxies[index].m_instance);
    }
}

void RenderObject::Update()
{
    // Update all proxies
    for (ProxyInstance *next, *cur = m_instances; cur != NULL; cur = next)
    {
        next = cur->GetNext();
        cur->Update();
    }

    if (m_animation != NULL)
    {
        // Create new or stop existing proxies
        for (size_t i = 0; i < m_proxies.size(); i++)
        {
            const Model::Proxy&          proxy = m_model.GetProxy(i);
            const ObjectTemplate::Proxy* templ = m_templ->GetProxy(i);
            if (m_proxies[i].m_instance == NULL)
            {
                // A proxy instance does not exists, check if we should create one
                if (m_proxies[i].m_visible && templ != NULL)
                {
                    float time = m_animation->GetVisibleEvent(proxy.bone->index, min(m_prevTime, m_time), max(m_prevTime, m_time));
                    if (time != -1)
                    {
                        // Convert animation time to absolute game time and spawn proxy
                        SpawnProxy(i, GetGameTime() - (m_time - time));
                    }
                }
            }
            else
            {
                // A proxy instance already exists, check if it should be stopped
                if (m_animation->GetInvisibleEvent(proxy.bone->index, min(m_prevTime, m_time), max(m_prevTime, m_time)) != -1)
                {
                    KillProxy(i);
                }
            }
        }
    }
}

void RenderObject::ShowBone(size_t index)
{
    m_bones[index].m_visible = true;
}

void RenderObject::HideBone(size_t index)
{
    m_bones[index].m_visible = false;
}

bool RenderObject::IsBoneVisible(size_t index) const
{
    return m_bones[index].m_visible;
}

void RenderObject::ShowMesh(size_t index)
{
    const Model::Mesh& mesh = m_model.GetMesh(index);
    for (size_t j = 0; j < mesh.subMeshes.size(); j++)
    {
        // Show this submesh
        size_t s = mesh.firstSubMesh + j;
        if (m_subMeshes[s].m_prev == NULL)
        {
            // It was hidden
            RenderPhase phase = m_subMeshes[s].m_resources->m_effect->GetRenderPhase();
            m_subMeshes[s].m_next =  m_meshlist[phase];
            m_subMeshes[s].m_prev = &m_meshlist[phase];
            m_meshlist[phase] = &m_subMeshes[s];
            if (m_subMeshes[s].m_next != NULL)
            {
                m_subMeshes[s].m_next->m_prev = &m_subMeshes[s].m_next;
            }
        }
    }
}

void RenderObject::HideMesh(size_t index)
{
    const Model::Mesh& mesh = m_model.GetMesh(index);
    for (size_t j = 0; j < mesh.subMeshes.size(); j++)
    {
        // Hide this submesh
        size_t s = mesh.firstSubMesh + j;
        if (m_subMeshes[s].m_prev != NULL)
        {
            // It was visible
            *m_subMeshes[s].m_prev = m_subMeshes[s].m_next;
            if (m_subMeshes[s].m_next != NULL)
            {
                m_subMeshes[s].m_next->m_prev = m_subMeshes[s].m_prev;
            }
            m_subMeshes[s].m_prev = NULL;
        }
    }
}

void RenderObject::SelectMesh(size_t index, bool selected)
{
    m_subMeshes[index].m_selected = selected;
}

bool RenderObject::IsMeshVisible(size_t index) const
{
    return m_subMeshes[m_model.GetMesh(index).firstSubMesh].m_prev != NULL;
}

void RenderObject::ShowProxy(size_t index)
{
    m_proxies[index].m_visible = true;
    if (GetBoneVisibility(m_model.GetProxy(index).bone->index))
    {
        SpawnProxy(index, GetGameTime());
    }
}

void RenderObject::HideProxy(size_t index)
{
    m_proxies[index].m_visible = false;
    KillProxy(index);
}

bool RenderObject::IsProxyVisible(size_t index) const
{
    return m_proxies[index].m_visible;
}

void RenderObject::ShowDazzle(size_t index)
{
    m_dazzles[index].m_visible = true;
}

void RenderObject::HideDazzle(size_t index)
{
    m_dazzles[index].m_visible = false;
}

bool RenderObject::IsDazzleVisible(size_t index) const
{
    return m_dazzles[index].m_visible;
}

void RenderObject::SelectDazzle(size_t index, bool selected)
{
    m_dazzles[index].m_selected = selected;
}

void RenderObject::CheckAltLod(bool altdesc)
{
    for (size_t i = 0; i < m_model.GetNumMeshes(); i++)
    {
        const Model::Mesh& mesh = m_model.GetMesh(i);
        const SubMesh&  submesh = m_subMeshes[mesh.firstSubMesh];
        if (submesh.m_alt != -1 || submesh.m_lod != -1)
        {
            // We only change meshes that have an ALT or LOD tag
            if (mesh.isVisible &&
                (submesh.m_lod == -1 || submesh.m_lod == m_lod) &&
                (submesh.m_alt == -1 || submesh.m_alt == m_alt)) {
                ShowMesh(i);
            } else {
                HideMesh(i);
            }
        }
    }

    for (size_t i = 0; i < m_proxies.size(); i++)
    {
        if (m_proxies[i].m_alt != -1 || m_proxies[i].m_lod != -1)
        {
            // We only change proxies that have an ALT or LOD tag
            if ( (m_proxies[i].m_lod == -1 || m_proxies[i].m_lod == m_lod) &&
                ((m_proxies[i].m_alt == -1 || m_proxies[i].m_alt == m_alt) &&
                 (!m_model.GetProxy(i).altDecreaseStayHidden || !altdesc))) {
                if (!IsProxyVisible(i)) {
                    ShowProxy(i);
                }
            } else {
                HideProxy(i);
            }
        }
    }
}

void RenderObject::SetLOD(int lod)
{
    if (m_lod != lod)
    {
        m_lod = lod;
        CheckAltLod(false);
    }
}

void RenderObject::SetALT(int alt)
{
    if (m_alt != alt)
    {
        bool altdesc = alt < m_alt;
        m_alt = alt;
        CheckAltLod(altdesc);
    }
}

RenderObject::RenderObject(LinkedList<RenderObject> &objects, ptr<ObjectTemplate> templ, int alt, int lod)
    : m_templ(templ), m_model(*templ->GetModel())
{
    Link(objects);
    
    for (int i = 0; i < NUM_RENDER_PHASES; i++) {
        m_meshlist[i] = NULL;
    }

    // Create the bones
    m_bones.resize(m_model.GetNumBones());
    for (size_t i = 0; i < m_bones.size(); i++)
    {
        m_bones[i].m_visible = true;
    }

    int maxAlt = -1, maxLod = -1;

    // Create the meshes
    m_subMeshes.resize(m_model.GetNumSubMeshes());
    for (size_t j = 0, i = 0; i < m_model.GetNumMeshes(); i++)
    {
        const Model::Mesh& mesh = m_model.GetMesh(i);
        for (size_t k = 0; k < mesh.subMeshes.size(); k++, j++)
        {
            m_subMeshes[j].m_next      = NULL;
            m_subMeshes[j].m_prev      = NULL;
            m_subMeshes[j].m_mesh      = &mesh;
            m_subMeshes[j].m_resources = &templ->GetSubMesh(j);
            m_subMeshes[j].m_selected  = false;
        }

        ParseName(mesh.name, &m_subMeshes[mesh.firstSubMesh].m_alt, &m_subMeshes[mesh.firstSubMesh].m_lod);
        if (m_subMeshes[mesh.firstSubMesh].m_alt != -1) maxAlt = max(maxAlt, m_subMeshes[mesh.firstSubMesh].m_alt);
        if (m_subMeshes[mesh.firstSubMesh].m_lod != -1) maxLod = max(maxLod, m_subMeshes[mesh.firstSubMesh].m_lod);
    }
        
    // Create the proxies
    m_proxies.resize(m_model.GetNumProxies());
    for (size_t i = 0; i < m_model.GetNumProxies(); i++)
    {
        const Model::Proxy& proxy = m_model.GetProxy(i);

        ParseName(proxy.name, &m_proxies[i].m_alt, &m_proxies[i].m_lod);
        if (m_proxies[i].m_alt != -1) maxAlt = max(maxAlt, m_proxies[i].m_alt);
        if (m_proxies[i].m_lod != -1) maxLod = max(maxLod, m_proxies[i].m_lod);
    }

    m_alt = max(0, min(maxAlt, alt));
    m_lod = max(0, min(maxLod, lod));

    // Set mesh visibilities
    for (size_t j = 0, i = 0; i < m_model.GetNumMeshes(); i++)
    {
        const Model::Mesh& mesh = m_model.GetMesh(i);
        if (mesh.isVisible &&
            (m_subMeshes[mesh.firstSubMesh].m_alt == -1 || m_subMeshes[mesh.firstSubMesh].m_alt == m_alt) &&
            (m_subMeshes[mesh.firstSubMesh].m_lod == -1 || m_subMeshes[mesh.firstSubMesh].m_lod == m_lod))
        {
            ShowMesh(i);
        }
    }

    // Set proxy visibilities
    for (size_t i = 0; i < m_proxies.size(); i++)
    {
        const Model::Proxy& proxy = m_model.GetProxy(i);
        m_proxies[i].m_visible = proxy.isVisible &&
            (m_proxies[i].m_alt == -1 || m_proxies[i].m_alt == m_alt) &&
            (m_proxies[i].m_lod == -1 || m_proxies[i].m_lod == m_lod);
        
        // Create it if its visible
        if (m_proxies[i].m_visible && proxy.bone->visible)
        {
            ShowProxy(i);
        }
    }

    // Create the dazzles
    m_dazzles.resize(m_model.GetNumDazzles());
    for (size_t i = 0; i < m_dazzles.size(); i++)
    {
        m_dazzles[i].m_visible = true;
    }

    SetColorization(Color(1,1,1,1));
}

void RenderObject::Destroy()
{
    // Clear all proxies
    m_proxies.clear();
    m_animation = NULL;

    while (m_instances != NULL)
    {
        delete m_instances;
    }
}

RenderObject::~RenderObject()
{
    Destroy();
}

} }