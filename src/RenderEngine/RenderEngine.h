#ifndef RENDERENGINE_H
#define RENDERENGINE_H

#include "General/Objects.h"
#include "General/GameTypes.h"
#include "Assets/Assets.h"

namespace Alamo
{

class IObjectTemplate : public IObject
{
public:
    virtual const Model* GetModel() const = 0;
};

class IRenderObject : public IObject
{
public:
    virtual void   ShowBone(size_t index) = 0;
    virtual void   HideBone(size_t index) = 0;
    virtual bool   IsBoneVisible(size_t index) const = 0;
    virtual Matrix GetBoneTransform(size_t bone) const = 0;

    virtual void ShowMesh(size_t index) = 0;
    virtual void HideMesh(size_t index) = 0;
    virtual bool IsMeshVisible(size_t index) const = 0;
    virtual void SelectMesh(size_t index, bool selected) = 0;

    virtual void ShowProxy(size_t index) = 0;
    virtual void HideProxy(size_t index) = 0;
    virtual bool IsProxyVisible(size_t index) const = 0;

    virtual void ShowDazzle(size_t index) = 0;
    virtual void HideDazzle(size_t index) = 0;
    virtual bool IsDazzleVisible(size_t index) const = 0;
    virtual void SelectDazzle(size_t index, bool selected) = 0;

    virtual void SetColorization(const Color& color) = 0;
    virtual void SetAnimation(const ptr<Animation> anim) = 0;
    virtual void ResetAnimation() = 0;
    virtual void SetAnimationTime(float t) = 0;
    virtual void Update() = 0;
    virtual void Destroy() = 0;

    virtual int GetALT() const = 0;
    virtual void SetALT(int alt) = 0;

    virtual int GetLOD() const = 0;
    virtual void SetLOD(int lod) = 0;

    virtual const IObjectTemplate* GetTemplate() const = 0;
};

struct RenderOptions
{
    bool  showBones;
    bool  showBoneNames;
    bool  showGround;
    float groundLevel;
    bool  showWireframe;
};

class IRenderEngine : public IObject
{
public:
    // Setters
    virtual void SetCamera(const Camera& camera) = 0;
    virtual void SetSettings(const RenderSettings& settings) = 0;
    virtual void SetEnvironment(const Environment& env) = 0;
    
    // Getters
    virtual const Camera&         GetCamera() const = 0;
    virtual const RenderSettings& GetSettings() const = 0;
    virtual const Environment&    GetEnvironment() const = 0;

	virtual void Render(const RenderOptions& options) = 0;

    // Factory methods
    virtual ptr<IObjectTemplate> CreateObjectTemplate(ptr<Model> model) = 0;
    virtual ptr<IRenderObject>   CreateRenderObject(ptr<IObjectTemplate> templ, int alt, int lod) = 0;
};

}
#endif