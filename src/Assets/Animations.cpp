#include <algorithm>
#include <set>
#include "Assets/Animations.h"
#include "Assets/ChunkFile.h"
#include "General/Exceptions.h"
#include "General/Math.h"
#include "General/Log.h"
#include "General/Utils.h"
using namespace std;

namespace Alamo
{

#pragma pack(1)
struct PackedQuaternion
{
    int16_t x, y, z, w;
};

struct PackedVector
{
    uint16_t x, y, z;
};
#pragma pack()

struct Animation::BoneInfo
{
    size_t         index;
    Vector3        ofsTrans, scaleTrans;
    Vector3        ofsScale, scaleScale;
    unsigned short idxTrans, idxScale, idxRot;
    Quaternion     defRotation;
};

struct Animation::AnimationData
{
    size_t                   transBlockSize, rotBlockSize, scaleBlockSize;
    Buffer<PackedVector>     transData;
    Buffer<PackedVector>     scaleData;
    Buffer<PackedQuaternion> rotData;
};

static Quaternion UnpackQuaternion(const PackedQuaternion& pq)
{
    return Quaternion(pq.x / (float)INT16_MAX, pq.y / (float)INT16_MAX, pq.z / (float)INT16_MAX, pq.w / (float)INT16_MAX);
}

static Vector3 UnpackVector(const PackedVector& pv)
{
    return Vector3(pv.x, pv.y, pv.z);
}

static PackedQuaternion ReadPackedQuaternion(ChunkReader& reader)
{
    PackedQuaternion pq;
    pq.x = reader.readShort();
    pq.y = reader.readShort();
    pq.z = reader.readShort();
    pq.w = reader.readShort();
    return pq;
}

static PackedVector ReadPackedVector(ChunkReader& reader)
{
    PackedVector pv;
    pv.x = reader.readShort();
    pv.y = reader.readShort();
    pv.z = reader.readShort();
    return pv;
}

void Animation::ReadBone(ChunkReader& reader, int version, const Model& model, BoneInfo& info, AnimationData& data, size_t dataOffset, bool checkOnly)
{
    Verify(reader.next() == 0x1002);

    Verify(reader.next() == 0x1003);
    Verify(reader.nextMini() ==  4); string name = Uppercase(reader.readString());
    Verify(reader.nextMini() ==  5); info.index  = reader.readInteger();

    // Verify that this bone matches the model's bone
    Verify(info.index < model.GetNumBones() && _stricmp(model.GetBone(info.index).name.c_str(), name.c_str()) == 0);
    
    ChunkType type = reader.nextMini();
    if (type == 10)
    {
        // We ignore this anyway
        type = reader.nextMini();
    }

    Verify(type              ==  6); info.  ofsTrans = reader.readVector3();
    Verify(reader.nextMini() ==  7); info.scaleTrans = reader.readVector3();
    Verify(reader.nextMini() ==  8); info.  ofsScale = reader.readVector3();
    Verify(reader.nextMini() ==  9); info.scaleScale = reader.readVector3();

    if (version == 2)
    {
        Verify(reader.nextMini() ==  14); info.idxTrans = reader.readShort(); if (info.idxTrans != UINT16_MAX) info.idxTrans /= (sizeof(PackedVector)     / sizeof(uint16_t));
        Verify(reader.nextMini() ==  15); info.idxScale = reader.readShort(); if (info.idxScale != UINT16_MAX) info.idxScale /= (sizeof(PackedVector)     / sizeof(uint16_t));
        Verify(reader.nextMini() ==  16); info.idxRot   = reader.readShort(); if (info.idxRot   != UINT16_MAX) info.idxRot   /= (sizeof(PackedQuaternion) / sizeof(int16_t));
        Verify(reader.nextMini() ==  17); info.defRotation = UnpackQuaternion(ReadPackedQuaternion(reader));
    }
    else
    {
        info.idxTrans = UINT16_MAX;
        info.idxScale = UINT16_MAX;
        info.idxRot   = UINT16_MAX;
    }
    Verify(reader.nextMini() == -1);

    type = reader.next();
    if (version == 1)
    {
        if (type == 0x1004)
        {
            if (!checkOnly)
            {
                // Read translation data
                info.idxTrans = (unsigned short)dataOffset;
                for (unsigned long i = 0; i < m_nFrames; i++)
                {
                    data.transData[i * data.transBlockSize + info.idxTrans] = ReadPackedVector(reader);
                }
            }
            type = reader.next();
        }

        if (type == 0x1005)
        {
            if (!checkOnly)
            {
                // Read scale data
                info.idxScale = (unsigned short)dataOffset;
                for (unsigned long i = 0; i < m_nFrames; i++)
                {
                    data.scaleData[i * data.scaleBlockSize + info.idxScale] = ReadPackedVector(reader);
                }
            }
            type = reader.next();
        }

        if (type == 0x1006)
        {
            if (!checkOnly)
            {
                // Read rotation data
                if (reader.size() == sizeof(PackedQuaternion))
                {
                    info.defRotation = UnpackQuaternion(ReadPackedQuaternion(reader));
                }
                else
                {
                    info.idxRot = (unsigned short)dataOffset;
                    for (unsigned long i = 0; i < m_nFrames; i++)
                    {
                        data.rotData[i * data.rotBlockSize + info.idxRot] = ReadPackedQuaternion(reader);
                    }
                }
            }
            type = reader.next();
        }
    }
    
    if (type == 0x1007)
    {
        if (!checkOnly)
        {
            // Read visibility data
            Buffer<unsigned char> rawdata((m_nFrames + 7) / 8);
            reader.read(rawdata, rawdata.size());
            for (unsigned long i = 0; i < m_nFrames; i++)
            {
                m_frames[info.index * m_nFrames + i].visible = ((rawdata[i/8] >> (i % 8)) != 0);
            }
        }
        type = reader.next();
    }
    
    if (type == 0x1008)
    {
        // Skip this track
        type = reader.next();
    }

    Verify(type == -1);
}

Animation::Animation(ptr<IFile> file, const Model& model, bool checkOnly)
{
    ChunkReader reader(file);
    Verify(reader.next() == 0x1000);

    AnimationData    data;
    vector<BoneInfo> bones;
    int version = 1;

    // Read file information
    Verify(reader.next() == 0x1001);
    Verify(reader.nextMini() == 1); m_nFrames = reader.readInteger();
    Verify(reader.nextMini() == 2); m_fps     = reader.readFloat();
    Verify(reader.nextMini() == 3); bones.resize(reader.readInteger());

    if (!checkOnly)
    {
        Log::WriteInfo("Loading animation %ls (%u frames)\n", file->name().c_str(), m_nFrames - 1 );
    }

    ChunkType type = reader.nextMini();
    if (type == 11)
    {
        // File format version #2
        version = 2;                     data.rotBlockSize   = reader.readInteger() / (sizeof(PackedQuaternion) / sizeof(int16_t));
        Verify(reader.nextMini() == 12); data.transBlockSize = reader.readInteger() / (sizeof(PackedVector) / sizeof(uint16_t));
        Verify(reader.nextMini() == 13); data.scaleBlockSize = reader.readInteger() / (sizeof(PackedVector) / sizeof(uint16_t));
        type = reader.nextMini();
    }
    else
    {
        // File format version #1
        data.rotBlockSize   = bones.size();
        data.transBlockSize = bones.size();
        data.scaleBlockSize = bones.size();
    }
    Verify(type == -1);

    if (!checkOnly)
    {
        // Allocate buffers
        data.rotData  .resize(data.rotBlockSize   * m_nFrames);
        data.transData.resize(data.transBlockSize * m_nFrames);
        data.scaleData.resize(data.scaleBlockSize * m_nFrames);

        m_frames.resize(model.GetNumBones() * m_nFrames);
        for (size_t i = 0; i < m_frames.size(); i++)
        {
            m_frames[i].visible = true;
        }
    }

    // Read the bone animations
    for (size_t i = 0; i < bones.size(); i++)
    {
        ReadBone(reader, version, model, bones[i], data, i, checkOnly);
    }

    if (!checkOnly)
    {
        if (version == 2)
        {
            if (data.transBlockSize > 0)
            {
                // Read translation data
                Verify(reader.next() == 0x100A);
                reader.read(data.transData, data.transData.size() * sizeof(PackedVector));
            }
            
            if (data.rotBlockSize > 0)
            {
                // Read rotation data
                Verify(reader.next() == 0x1009);
                reader.read(data.rotData, data.rotData.size() * sizeof(PackedQuaternion));
            }
        }

        Verify(reader.next() == -1);
        
        ConstructTransforms(model, bones, data);

        // There are actually one less frames in the animation, the first is duplicated
        // at the end for easy looping.
        m_nFrames--;
    }
}

void Animation::ConstructTransforms(const Model& model, const vector<BoneInfo>& bones, const AnimationData& data)
{
    // Construct the transform matrices for every frame for every bone in the model
    Buffer<Matrix> transforms(model.GetNumBones() * m_nFrames);
    
    // Keep track of which bones are animated
    set<size_t> animated;
    for (size_t i = 0; i < bones.size(); i++)
    {
        const BoneInfo& bone = bones[i];

        for (unsigned long f = 0; f < m_nFrames; f++)
        {
            Vector3    trans(bone.ofsTrans), scale(bone.ofsScale);
            Quaternion rot(bone.defRotation);

            if (bone.idxTrans != UINT16_MAX)
            {
                trans += UnpackVector(data.transData[f * data.transBlockSize + bone.idxTrans]) * bone.scaleTrans;
            }

            if (bone.idxScale != UINT16_MAX)
            {
                scale += UnpackVector(data.scaleData[f * data.scaleBlockSize + bone.idxScale]) * bone.scaleScale;
            }

            if (bone.idxRot != UINT16_MAX)
            {
                rot = UnpackQuaternion(data.rotData[f * data.rotBlockSize + bone.idxRot]);
            }

            // Store the relative animation transform
            /*if (f == 0)
            {
                printf("%d: %.2f %.2f %.2f; %.2f %.2f %.2f %.2f; %.2f %.2f%.2f\n",
                    bone.index, scale.x, scale.y, scale.z,
                    rot.x, rot.y, rot.z, rot.w,
                    trans.x, trans.y, trans.z);
            }*/

            transforms[bone.index * m_nFrames + f] = Matrix(scale, rot, trans);
        }

        animated.insert(bone.index);
    }

    for (size_t i = 0; i < model.GetNumBones(); i++)
    {
        if (animated.find(i) == animated.end())
        {
            // Fill unanimated bones with default relative transforms
            for (unsigned long f = 0; f < m_nFrames; f++)
            {
                transforms[i * m_nFrames + f] = model.GetBone(i).relTransform;
            }
        }

        // Multiply transforms with parent transforms to get absolute transforms
        const Model::Bone* parent = model.GetBone(i).parent;
        if (parent != NULL)
        {
            for (unsigned long f = 0; f < m_nFrames; f++)
            {
                transforms[i * m_nFrames + f] = transforms[i * m_nFrames + f] * transforms[parent->index * m_nFrames + f];
            }
        }

        // Decompose animations into SRT components
        for (unsigned long f = 0; f < m_nFrames; f++)
        {
            Frame& frame = m_frames[i * m_nFrames + f];
            transforms[i * m_nFrames + f].decompose(&frame.scale, &frame.rotation, &frame.translation);
        }
    }
}

Matrix Animation::GetFrame(size_t bone, float t) const
{
    size_t f    = (m_nFrames > 0) ? ((size_t)(t * m_fps)) % m_nFrames : 0;
    size_t base = bone * (m_nFrames + 1);
    float  s = (t * m_fps) - f;
    f = base + f;
    return Matrix(
        lerp (m_frames[f].scale,       m_frames[f + 1].scale,       s),
        slerp(m_frames[f].rotation,    m_frames[f + 1].rotation,    s),
        lerp (m_frames[f].translation, m_frames[f + 1].translation, s));
}

bool Animation::IsVisible(size_t bone, float t) const
{
    size_t f    = (m_nFrames > 0) ? ((size_t)(t * m_fps)) % m_nFrames : 0;
    size_t base = bone * (m_nFrames + 1);
    return m_frames[base + f].visible;
}

float Animation::GetVisibleEvent(size_t bone, float from, float to) const
{
    assert(from <= to);
    size_t f1   = (size_t)ceil(from * m_fps);
    size_t base = bone * (m_nFrames + 1);
    if (m_nFrames != 0)
    {
        size_t n = (size_t)max(floor(to * m_fps), f1) - f1;
        for (size_t f = 0; f <= n; f++)
        {
            size_t i = (f1 + f);
            if (m_frames[base + (i % m_nFrames)].visible)
            {
                return i / m_fps;
            }
        }
    }
    else if (m_frames[base].visible)
    {
        return 0.0f;
    }
    return -1;
}

float Animation::GetInvisibleEvent(size_t bone, float from, float to) const
{
    assert(from <= to);
    size_t f1   = (size_t)ceil(from * m_fps);
    size_t base = bone * (m_nFrames + 1);
    if (m_nFrames != 0)
    {
        size_t n = (size_t)max(floor(to * m_fps), f1) - f1;
        for (size_t f = 0; f <= n; f++)
        {
            size_t i = (f1 + f);
            if (!m_frames[base + (i % m_nFrames)].visible)
            {
                return i / m_fps;
            }
        }
    }
    else if (!m_frames[base].visible)
    {
        return 0.0f;
    }
    return -1;
}

}