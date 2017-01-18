#ifndef VERTEXFORMATS_DX9_H
#define VERTEXFORMATS_DX9_H

#include "General/Math.h"
#include "General/ExactTypes.h"

namespace Alamo {
namespace DirectX9 {

// Available vertex formats
enum VertexFormat
{
    VF_MESH_N = 0,
    
    // Static meshes (0 bones/vertex)
    VF_MESH_NU2,
    VF_MESH_NU2C,
    VF_MESH_NU2U3U3,
    VF_MESH_NU2U3U3C,

    // Reduced-skin meshes (1 bone/vertex)
    VF_RSKIN_NU2,
    VF_RSKIN_NU2C,
    VF_RSKIN_NU2U3U3,
    VF_RSKIN_NU2U3U3C,

    // Full-skin meshes (4 bones/vertex)
    VF_SKIN_NU2,
    VF_SKIN_NU2C,
    VF_SKIN_NU2U3U3,
    VF_SKIN_NU2U3U3C,

    VF_GRASS,
    VF_BILLBOARD,

    VF_GENERIC,
    VF_TERRAIN,
    VF_WATER,
    VF_TRACK,
};

// The vertex structures
#pragma pack(1)
struct VERTEX_MESH_N
{
	Vector3 Position;
	Vector3 Normal;
};

struct VERTEX_MESH_NU2
{
	Vector3  Position;
	Vector3  Normal;
    Vector2  TexCoord;
};

struct VERTEX_MESH_NU2C
{
	Vector3  Position;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
};

struct VERTEX_MESH_NC
{
	Vector3  Position;
	Vector3  Normal;
    D3DCOLOR Color;
};

struct VERTEX_MESH_NU2U3U3
{
	Vector3  Position;
	Vector3  Normal;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_MESH_NU2U3U3C
{
	Vector3  Position;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_RSKIN_NU2_HW
{
	Vector3  Position;
	Vector4  Normal;
    Vector2  TexCoord;
};

struct VERTEX_RSKIN_NU2_SW
{
	Vector3  Position;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
};

struct VERTEX_RSKIN_NU2C_HW
{
	Vector3  Position;
	Vector4  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
};

struct VERTEX_RSKIN_NU2C_SW
{
	Vector3  Position;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
};

struct VERTEX_RSKIN_NU2U3U3_HW
{
	Vector3  Position;
	Vector4  Normal;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_RSKIN_NU2U3U3_SW
{
	Vector3  Position;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_RSKIN_NU2U3U3C_HW
{
	Vector3  Position;
	Vector4  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_RSKIN_NU2U3U3C_SW
{
	Vector3  Position;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_SKIN_NU2_HW
{
	Vector3  Position;
    Vector4  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
};

struct VERTEX_SKIN_NU2_SW
{
	Vector3  Position;
    Vector3  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
};

struct VERTEX_SKIN_NU2C_HW
{
	Vector3  Position;
    Vector4  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
};

struct VERTEX_SKIN_NU2C_SW
{
	Vector3  Position;
    Vector3  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
};

struct VERTEX_SKIN_NU2U3U3_HW
{
	Vector3  Position;
    Vector4  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_SKIN_NU2U3U3_SW
{
	Vector3  Position;
    Vector3  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_SKIN_NU2U3U3C_HW
{
	Vector3  Position;
    Vector4  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_SKIN_NU2U3U3C_SW
{
	Vector3  Position;
    Vector3  BlendWeights;
    DWORD    BlendIndices;
	Vector3  Normal;
    D3DCOLOR Color;
    Vector2  TexCoord;
    Vector3  Tangent;
    Vector3  Binormal;
};

struct VERTEX_GRASS
{
	Vector3  Position;
    Vector2  TexCoord;
    Vector2  ClumpCenter;
};

struct VERTEX_BILLBOARD
{
	Vector3 Position;
	Vector3 Normal;
    Vector2 TexCoord;
    Vector3 Center;
};

struct VERTEX
{
	Vector3 Position;
};

struct VERTEX_TERRAIN
{
	Vector3  Position;
	Vector3  Normal;
};

struct VERTEX_WATER
{
	Vector3  Position;
	Vector3  Normal;
	D3DCOLOR Color;
};

struct VERTEX_TRACK
{
	Vector3  Position;
	Vector3  Normal;
	D3DCOLOR Color;
    Vector2  TexCoord;
};

#pragma pack()

typedef void (*VertexProc)(void* dest, const void* src, DWORD num, bool isUaW);

struct VertexFormatResourceInfo
{
	size_t			             size;
	const D3DVERTEXELEMENT9*     elements;
    VertexProc                   proc;
    IDirect3DVertexDeclaration9* declaration;
    unsigned long                nReferences;
};

struct VertexFormatInfo
{
    VertexFormatResourceInfo hw;
    VertexFormatResourceInfo sw;
    unsigned long            nReferences;
};

struct VertexFormatNameInfo
{
    const char*  name;
    VertexFormat format;
};

extern VertexFormatInfo     VertexFormats[];
extern VertexFormatNameInfo VertexFormatNames[];

}
}
#endif