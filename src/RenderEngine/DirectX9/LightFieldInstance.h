#ifndef LIGHTFIELDINSTANCE_H
#define LIGHTFIELDINSTANCE_H

#include "RenderEngine/DirectX9/RenderEngine.h"

namespace Alamo {
namespace DirectX9 {

class RenderObject;

class LightFieldInstance : public ProxyInstance
{
    const LightFieldSource& m_source;
    RenderObject&           m_object;
    RenderEngine&           m_engine;
    size_t                  m_bone;
    float                   m_attached;
    float                   m_rotation;
    float                   m_lastChange;
    float                   m_spawnTime;

    // The lightfield quad
    VERTEX_MESH_NU2C m_quad[4];

    void Update();
    void Detach();
public:
    void Render() const;
    LightFieldInstance(const LightFieldSource& source, RenderObject& object, size_t index, float time);
    ~LightFieldInstance();
};

}
}

#endif