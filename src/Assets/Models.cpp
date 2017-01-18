#include <algorithm>
#include <stack>
#include "Assets/Models.h"
#include "General/Exceptions.h"
#include "General/Utils.h"
using namespace std;

namespace Alamo
{

void Model::ReadBone(ChunkReader& reader, Bone& bone)
{
    // Read bone name
    Verify(reader.next() == 0x203);
    bone.name = Uppercase(reader.readString());

    // Read bone data
    ChunkType type = reader.next();
    Verify(type == 0x205 || type == 0x206);

    long parent = reader.readInteger();
    bone.parent    = (parent >= 0) ? &m_bones[parent] : NULL;
    bone.visible   = (reader.readInteger() != 0);
    bone.billboard = BBT_DISABLE;
    if (type == 0x206)
    {
        bone.billboard = (BillboardType)reader.readInteger();
    }
    
    // Read relative transform
    bone.relTransform = Matrix::Identity;
    for (int i = 0; i < 12; i++)
    {
        bone.relTransform[i] = reader.readFloat();
    }
    bone.relTransform.transpose();

    // Calculate absolute transform
    bone.absTransform = (bone.parent != NULL)
        ? bone.relTransform * bone.parent->absTransform
        : bone.relTransform;
    bone.invAbsTransform = bone.absTransform.inverse();

    Verify(reader.next() == -1);
}

void Model::ReadSkeleton(ChunkReader& reader)
{
    Verify(reader.next() == 0x200);
    Verify(reader.next() == 0x201);
    m_bones.resize(reader.readInteger());
    
    vector<size_t> siblings(m_bones.size(), -1);
    size_t rootSibling = -1;

    for (size_t i = 0; i < m_bones.size(); i++)
    {
        Verify(reader.next() == 0x202);
        ReadBone(reader, m_bones[i]);
        m_bones[i].index   = i;
    }
    Verify(reader.next() == -1);
}

void Model::ReadSubMesh(ChunkReader& reader, SubMesh& mesh)
{
    Verify(reader.next() == 0x10100);
    
    // Read shader name
    Verify(reader.next() == 0x10101);
    mesh.shader = reader.readString();
    
    // Read shader parameters
    ChunkType type;
    while ((type = reader.next()) != -1)
    {
        mesh.parameters.push_back(ShaderParameter());
        ShaderParameter& param = mesh.parameters.back();

        Verify(reader.nextMini() == 1);
        param.m_name = reader.readString();
        param.m_colorize = (mesh.mesh->name.find("FC_") == 0 && _stricmp(param.m_name.c_str(),"Color") == 0);

        Verify(reader.nextMini() == 2);
        switch (type)
        {
        case 0x10102: param.m_type = SPT_INT;     param.m_int     = reader.readInteger(); break;
        case 0x10103: param.m_type = SPT_FLOAT;   param.m_float   = reader.readFloat(); break;
        case 0x10104: param.m_type = SPT_FLOAT3;  param.m_float3  = reader.readVector3(); break;
        case 0x10105: param.m_type = SPT_TEXTURE; param.m_texture = reader.readString(); break;
        case 0x10106: param.m_type = SPT_FLOAT4;  param.m_float4  = reader.readVector4(); break;
        }

        Verify(reader.nextMini() == -1);
    }

    Verify(reader.next() == 0x10000);
    
    // Read vertex and primitive count
    Verify(reader.next() == 0x10001);
    mesh.vertices.resize(reader.readInteger());
    mesh.indices .resize(reader.readInteger() * 3);

    // Read vertex format
    Verify(reader.next() == 0x10002);
    mesh.vertexFormat = reader.readString();

    type = reader.next();
    Verify(type == 0x10007 || type == 0x10005);
    if (type == 0x10007)
    {
        reader.read(mesh.vertices, mesh.vertices.size() * sizeof(MASTER_VERTEX));
    }
    else
    {
        #pragma pack(1)
        struct OldVertex
        {
            Vector3 Position;
            Vector3 Normal;
            Vector2 TexCoord[4];
            Vector3 Tangent;
            Vector3 Binormal;
            Color   Color;
            DWORD   BoneIndices[4];
            float   BoneWeights[4];
        };
        #pragma pack()

        // Read and convert old format
        Buffer<OldVertex> vertices(mesh.vertices.size());
        reader.read(vertices, vertices.size() * sizeof(OldVertex));
        for (size_t i = 0; i < vertices.size(); i++)
        {
            mesh.vertices[i].Position = vertices[i].Position;
            mesh.vertices[i].Normal   = vertices[i].Normal;
            mesh.vertices[i].Tangent  = vertices[i].Tangent;
            mesh.vertices[i].Binormal = vertices[i].Binormal;
            mesh.vertices[i].Color    = vertices[i].Color;
            for (int j = 0; j < 4; j++)
            {
                mesh.vertices[i].TexCoord[j]    = vertices[i].TexCoord[j];
                mesh.vertices[i].BoneIndices[j] = vertices[i].BoneIndices[j];
                mesh.vertices[i].BoneWeights[j] = vertices[i].BoneWeights[j];
            }
        }
    }

    Verify(reader.next() == 0x10004);
    reader.read(mesh.indices, mesh.indices.size() * sizeof(uint16_t));

    // Read skin mapping
    type = reader.next();
    mesh.nSkinBones = 0;
    if (type == 0x10006)
    {
        while (reader.tell() != reader.size() && mesh.nSkinBones < MAX_NUM_SKIN_BONES)
        {
            mesh.skin[mesh.nSkinBones++] = reader.readInteger();
        }
        type = reader.next();
    }

    if (type == 0x1200)
    {
        // Read collision tree
        reader.skip();
        type = reader.next();
    }

    Verify(type == -1);
}

Model::Mesh* Model::ReadMesh(ChunkReader& reader)
{
    m_meshes.push_back(NULL);
    Mesh* mesh = m_meshes.back() = new Mesh();

    // Read mesh name
    Verify(reader.next() == 0x401);
    mesh->name = reader.readString();
    
    // Read mesh info
    Verify(reader.next() == 0x402);
    mesh->subMeshes.resize( reader.readInteger() );
    mesh->bounds.min   = reader.readVector3();
    mesh->bounds.max   = reader.readVector3();
    reader.readInteger();
    mesh->isVisible    = (reader.readInteger() == 0);
    mesh->isCollidable = (reader.readInteger() != 0);
    mesh->firstSubMesh = m_numSubMeshes;
    mesh->nVertices    = 0;

    // Read sub-meshes
    for (size_t i = 0; i < mesh->subMeshes.size(); i++)
    {
        mesh->subMeshes[i].mesh = mesh;
        ReadSubMesh(reader, mesh->subMeshes[i]);
        mesh->nVertices += mesh->subMeshes[i].vertices.size();
        m_numSubMeshes++;
    }

    Verify(reader.next() == -1);
    return mesh;
}

Model::Light* Model::ReadLight(ChunkReader& reader)
{
    m_lights.push_back(NULL);
    Light* light = m_lights.back() = new Light();
    
    // Read light name
    Verify(reader.next() == 0x1301);
    light->name = reader.readString();

    // Read light info
    Verify(reader.next() == 0x1302);
    light->type                = (LightType)reader.readInteger();
    light->color               = reader.readColorRGB();
    light->intensity           = reader.readFloat();
    light->farAttenuationEnd   = reader.readFloat();
  	light->farAttenuationStart = reader.readFloat();
  	light->hotspotSize         = reader.readFloat();
  	light->falloffSize         = reader.readFloat();
    
    Verify(reader.next() == -1);
    return light;
}

unsigned long Model::ReadDazzle(ChunkReader& reader, Dazzle& dazzle)
{
    unsigned long bone;
    Verify(reader.next() == 0);
    Verify(reader.nextMini() ==  0); dazzle.color     = reader.readColorRGB();
    Verify(reader.nextMini() ==  1); dazzle.position  = reader.readVector3();
    Verify(reader.nextMini() ==  2); dazzle.radius    = reader.readFloat();
    Verify(reader.nextMini() ==  3); dazzle.texX      = reader.readInteger();
    Verify(reader.nextMini() ==  4); dazzle.texY      = reader.readInteger();
    Verify(reader.nextMini() ==  6); dazzle.texSize   = reader.readInteger();
    Verify(reader.nextMini() ==  5); dazzle.texture   = reader.readString();
    Verify(reader.nextMini() ==  7); dazzle.frequency = reader.readFloat(); 
    Verify(reader.nextMini() ==  8); dazzle.phase     = reader.readFloat();
    Verify(reader.nextMini() ==  9); dazzle.nightOnly = reader.readInteger() != 0;
    Verify(reader.nextMini() == 10); bone             = reader.readInteger();
    Verify(reader.nextMini() == 11); dazzle.name      = reader.readString();
    Verify(reader.nextMini() == 12); dazzle.isVisible = reader.readInteger() == 0;
    
    ChunkType type = reader.nextMini();
    dazzle.bias = 1.0f;
    if (type == 13)
    {
        dazzle.bias = reader.readFloat();
        type = reader.nextMini();
    }

    dazzle.colorize = (dazzle.name.find("FC_") == 0);

    Verify(type == -1); 
    Verify(reader.next() == -1);
    return bone;
}

void Model::ReadConnections(ChunkReader& reader, const std::vector<Attachable*>& objects)
{
    Verify(reader.next() == 0x601);
    Verify(reader.nextMini() ==  1); unsigned long nConns   = reader.readInteger();
    Verify(reader.nextMini() ==  4); unsigned long nProxies = reader.readInteger();
    
    // Universe at War: Earth Assault added dazzles to the format
    unsigned long nDazzles = 0;
    ChunkType type = reader.nextMini();
    if (type == 9)
    {
        nDazzles = reader.readInteger();
        type = reader.nextMini();
    }
    Verify(type == -1);
    
    // Read connections
    for (unsigned long i = 0; i < nConns; i++)
    {
        Verify(reader.next() == 0x602);
        Verify(reader.nextMini() ==  2); unsigned long object = reader.readInteger();
        Verify(reader.nextMini() ==  3); unsigned long bone   = reader.readInteger();
        Verify(reader.nextMini() == -1);
        
        // Connect the object to the bone
        objects[object]->bone = &m_bones[bone];
    }

    // Read proxies
    m_proxies.resize(nProxies);
    for (unsigned long i = 0; i < nProxies; i++)
    {
        Proxy& proxy = m_proxies[i];
        proxy.isVisible             = true;
        proxy.altDecreaseStayHidden = false;

        Verify(reader.next() == 0x603);
        Verify(reader.nextMini() ==  5); proxy.name = reader.readString();
        Verify(reader.nextMini() ==  6); proxy.bone = &m_bones[reader.readInteger()];
        ChunkType type = reader.nextMini();
        if (type == 7)
        {
            proxy.isVisible = (reader.readInteger() == 0);
            type = reader.nextMini();
        }
        if (type == 8)
        {
            proxy.altDecreaseStayHidden = (reader.readInteger() != 0);
            type = reader.nextMini();
        }

        proxy.mesh = NULL;
        if (proxy.bone->parent != NULL)
        {
            // See if any mesh exists to which the proxy is attached
            for (size_t j = 0; j < m_meshes.size(); j++)
            {
                if (proxy.bone->parent == m_meshes[j]->bone)
                {
                    // Yup
                    proxy.mesh = m_meshes[j];
                    break;
                }
            }
        }

        Verify(type == -1);
    }

    m_dazzles.resize(nDazzles);
    for (unsigned long i = 0; i < nDazzles; i++)
    {
        Verify(reader.next() == 0x604);
        Dazzle& dazzle = m_dazzles[i];

        dazzle.bone = &m_bones[ReadDazzle(reader, dazzle)];
    }

    Verify(reader.next() == -1);
}

Model::Model(ptr<IFile> file) : m_numSubMeshes(0)
{
    Log::WriteInfo("Loading model %ls\n", file->name().c_str() );

    ChunkReader reader(file);
    ReadSkeleton(reader);

    try
    {
        vector<Attachable*> objects;
        ChunkType type = reader.next();
        while (type == 0x400 || type == 0x1300)
        {
            if (type == 0x400) {
                objects.push_back( ReadMesh(reader) );
            } else {
                objects.push_back( ReadLight(reader) );
            }
            type = reader.next();
        }

        Verify(type == 0x600);
        ReadConnections(reader, objects);
    }
    catch (...)
    {
        Cleanup();
        throw;
    }
}

void Model::Cleanup()
{
    for (size_t i = 0; i < m_meshes.size(); i++) {
        delete m_meshes[i];
    }

    for (size_t i = 0; i < m_lights.size(); i++) {
        delete m_lights[i];
    }
}

Model::~Model()
{
    Cleanup();
}

size_t Model::GetBone(std::string name) const
{
    transform(name.begin(), name.end(), name.begin(), toupper);
    for (size_t i = 0; i < m_bones.size(); i++)
    {
        if (m_bones[i].name == name)
        {
            return i;
        }
    }
    return -1;
}

}