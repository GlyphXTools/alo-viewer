#ifndef ANIMATIONS_H
#define ANIMATIONS_H

#include "Assets/Models.h"
#include <vector>

namespace Alamo
{

class Animation : public IObject
{
    struct Frame
    {
        Quaternion rotation;
        Vector3    translation;
        Vector3    scale;
        bool       visible;
    };

    float             m_fps;
    unsigned long     m_nFrames;
    Buffer<Frame>     m_frames;

    struct BoneInfo;
    struct AnimationData;

    void ReadBone(ChunkReader& reader, int version, const Model& model, BoneInfo& info, AnimationData& data, size_t dataOffset, bool checkOnly);
    void ConstructTransforms(const Model& model, const std::vector<BoneInfo>& bones, const AnimationData& data);

public:
    float         GetFPS()       const { return m_fps; }
    unsigned long GetNumFrames() const { return m_nFrames; }

    Matrix GetFrame(size_t bone, float t) const;
    bool  IsVisible(size_t bone, float t) const;

    // Returns the first time in [from,to] where the bone was (in)visible.
    // Returns -1 if there is no such time.
    float GetVisibleEvent  (size_t bone, float from, float to) const;
    float GetInvisibleEvent(size_t bone, float from, float to) const;

    Animation(ptr<IFile> file, const Model& model, bool checkOnly = false);
};

}

#endif