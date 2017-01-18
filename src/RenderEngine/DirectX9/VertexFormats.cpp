#include "RenderEngine/DirectX9/VertexFormats.h"
#include "General/GameTypes.h"
#include <stddef.h>
using namespace std;

namespace Alamo {
namespace DirectX9 {

static const D3DVERTEXELEMENT9 elVertN[] = {
	{0, offsetof(VERTEX_MESH_N, Position),  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_MESH_N, Normal),    D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertNU2[] = {
	{0, offsetof(VERTEX_MESH_NU2, Position),  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_MESH_NU2, Normal),    D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, offsetof(VERTEX_MESH_NU2, TexCoord),  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertNU2C[] = {
	{0, offsetof(VERTEX_MESH_NU2C, Position),  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_MESH_NU2C, Normal),    D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
    {0, offsetof(VERTEX_MESH_NU2C, Color),     D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	{0, offsetof(VERTEX_MESH_NU2C, TexCoord),  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertNU2U3U3[] = {
	{0, offsetof(VERTEX_MESH_NU2U3U3, Position),  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_MESH_NU2U3U3, Normal),    D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, offsetof(VERTEX_MESH_NU2U3U3, TexCoord),  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0, offsetof(VERTEX_MESH_NU2U3U3, Tangent),   D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0},
    {0, offsetof(VERTEX_MESH_NU2U3U3, Binormal),  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertNU2U3U3C[] = {
	{0, offsetof(VERTEX_MESH_NU2U3U3C, Position),  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_MESH_NU2U3U3C, Normal),    D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
    {0, offsetof(VERTEX_MESH_NU2U3U3C, Color),     D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	{0, offsetof(VERTEX_MESH_NU2U3U3C, TexCoord),  D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    {0, offsetof(VERTEX_MESH_NU2U3U3C, Tangent),   D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,  0},
    {0, offsetof(VERTEX_MESH_NU2U3U3C, Binormal),  D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL, 0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2_HW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2_HW, Position),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
	{0, offsetof(VERTEX_RSKIN_NU2_HW, Normal),       D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2_HW, TexCoord),     D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2_SW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2_SW, Position),     D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_RSKIN_NU2_SW, BlendIndices), D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_RSKIN_NU2_SW, Normal),       D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2_SW, TexCoord),     D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2C_HW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2C_HW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_RSKIN_NU2C_HW, Normal),       D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2C_HW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_RSKIN_NU2C_HW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2C_SW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2C_SW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_RSKIN_NU2C_SW, BlendIndices), D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_RSKIN_NU2C_SW, Normal),       D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2C_SW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_RSKIN_NU2C_SW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2U3U3_HW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_HW, Position),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_HW, Normal),       D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_HW, TexCoord),     D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3_HW, Tangent),      D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,      0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3_HW, Binormal),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2U3U3_SW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_SW, Position),     D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3_SW, BlendIndices), D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_SW, Normal),       D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3_SW, TexCoord),     D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2U3U3C_HW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, Normal),       D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, Tangent),      D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,      0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3C_HW, Binormal),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertRSkinNU2U3U3C_SW[] = {
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_SW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3C_SW, BlendIndices), D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_SW, Normal),       D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
    {0, offsetof(VERTEX_RSKIN_NU2U3U3C_SW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	{0, offsetof(VERTEX_RSKIN_NU2U3U3C_SW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2_HW[] = {
	{0, offsetof(VERTEX_SKIN_NU2_HW, Position),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2_HW, BlendWeights), D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2_HW, BlendIndices), D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2_HW, Normal),       D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_SKIN_NU2_HW, TexCoord),     D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2_SW[] = {
	{0, offsetof(VERTEX_SKIN_NU2_SW, Position),     D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2_SW, BlendWeights), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2_SW, BlendIndices), D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2_SW, Normal),       D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_SKIN_NU2_SW, TexCoord),     D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2C_HW[] = {
	{0, offsetof(VERTEX_SKIN_NU2C_HW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, offsetof(VERTEX_SKIN_NU2C_HW, BlendWeights), D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2C_HW, BlendIndices), D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2C_HW, Normal),       D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
    {0, offsetof(VERTEX_SKIN_NU2C_HW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0},
	{0, offsetof(VERTEX_SKIN_NU2C_HW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2C_SW[] = {
	{0, offsetof(VERTEX_SKIN_NU2C_SW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2C_SW, BlendWeights), D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2C_SW, BlendIndices), D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2C_SW, Normal),       D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_SKIN_NU2C_SW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_SKIN_NU2C_SW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,        0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2U3U3_HW[] = {
	{0, offsetof(VERTEX_SKIN_NU2U3U3_HW, Position),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_HW, BlendWeights), D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_HW, BlendIndices), D3DDECLTYPE_UBYTE4N, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},    
	{0, offsetof(VERTEX_SKIN_NU2U3U3_HW, Normal),       D3DDECLTYPE_FLOAT4,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3_HW, TexCoord),     D3DDECLTYPE_FLOAT2,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_HW, Tangent),      D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,      0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_HW, Binormal),     D3DDECLTYPE_FLOAT3,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2U3U3_SW[] = {
	{0, offsetof(VERTEX_SKIN_NU2U3U3_SW, Position),     D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_SW, BlendWeights), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3_SW, BlendIndices), D3DDECLTYPE_UBYTE4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3_SW, Normal),       D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3_SW, TexCoord),     D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2U3U3C_HW[] = {
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, BlendWeights), D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, BlendIndices), D3DDECLTYPE_UBYTE4N,  D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},    
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, Normal),       D3DDECLTYPE_FLOAT4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, Tangent),      D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TANGENT,      0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_HW, Binormal),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BINORMAL,     0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertSkinNU2U3U3C_SW[] = {
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, Position),     D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION,     0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, BlendWeights), D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDWEIGHT,  0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, BlendIndices), D3DDECLTYPE_UBYTE4,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_BLENDINDICES, 0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, Normal),       D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,       0},
    {0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, Color),        D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	{0, offsetof(VERTEX_SKIN_NU2U3U3C_SW, TexCoord),     D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD,     0},
	D3DDECL_END()
};
static const D3DVERTEXELEMENT9 elVertGrass[] = {
	{0, offsetof(VERTEX_GRASS, Position),    D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_GRASS, TexCoord),    D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, offsetof(VERTEX_GRASS, ClumpCenter), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVert[] = {
	{0, offsetof(VERTEX, Position),  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertTerrain[] = {
	{0, offsetof(VERTEX_TERRAIN, Position),  D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_TERRAIN, Normal),    D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertWater[] = {
	{0, offsetof(VERTEX_WATER, Position), D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_WATER, Normal),   D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, offsetof(VERTEX_WATER, Color),    D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertTrack[] = {
	{0, offsetof(VERTEX_TRACK, Position), D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_TRACK, Normal),   D3DDECLTYPE_FLOAT3,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, offsetof(VERTEX_TRACK, TexCoord), D3DDECLTYPE_FLOAT2,   D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, offsetof(VERTEX_TRACK, Color),    D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR,    0},
	D3DDECL_END()
};

static const D3DVERTEXELEMENT9 elVertBillboard[] = {
	{0, offsetof(VERTEX_BILLBOARD, Position), D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
	{0, offsetof(VERTEX_BILLBOARD, Normal),   D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
	{0, offsetof(VERTEX_BILLBOARD, TexCoord), D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
	{0, offsetof(VERTEX_BILLBOARD, Center),   D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 1},
	D3DDECL_END()
};

static Vector2 Pack_UV(const Vector2& texCoord)
{
    return Vector2(texCoord.x * 4096, texCoord.y * 4096);
}

static void VertProcN(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_MESH_N* dest = (VERTEX_MESH_N*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
    }
}

static void VertProcNU2(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_MESH_NU2* dest = (VERTEX_MESH_NU2*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
        dest->TexCoord = src->TexCoord[0];
    }
}

static void VertProcNU2C(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_MESH_NU2C* dest = (VERTEX_MESH_NU2C*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
        dest->Color    = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
        dest->TexCoord = src->TexCoord[0];
    }
}

static void VertProcNU2U3U3(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_MESH_NU2U3U3* dest = (VERTEX_MESH_NU2U3U3*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
        dest->TexCoord = src->TexCoord[0];
        dest->Tangent  = src->Tangent;
        dest->Binormal = src->Binormal;
    }
}

static void VertProcNU2U3U3C(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_MESH_NU2U3U3C* dest = (VERTEX_MESH_NU2U3U3C*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
        dest->Color    = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
        dest->TexCoord = src->TexCoord[0];
        dest->Tangent  = src->Tangent;
        dest->Binormal = src->Binormal;
    }
}

static void VertProcRSkinNU2_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2_HW* dest = (VERTEX_RSKIN_NU2_HW*)_dest;
    VERTEX_RSKIN_NU2_SW* src  = (VERTEX_RSKIN_NU2_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = Vector4(src->Normal, (float)src->BlendIndices);
        dest->TexCoord = isUaW ? src->TexCoord : Pack_UV(src->TexCoord);
    }
}

static void VertProcRSkinNU2_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2_SW* dest = (VERTEX_RSKIN_NU2_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendIndices = src->BoneIndices[0];
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
    }
}

static void VertProcRSkinNU2C_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2C_HW* dest = (VERTEX_RSKIN_NU2C_HW*)_dest;
    VERTEX_RSKIN_NU2C_SW* src  = (VERTEX_RSKIN_NU2C_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = Vector4(src->Normal, (float)src->BlendIndices);
        dest->TexCoord = isUaW ? src->TexCoord : Pack_UV(src->TexCoord);
        dest->Color    = src->Color;
    }
}

static void VertProcRSkinNU2C_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2C_SW* dest = (VERTEX_RSKIN_NU2C_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendIndices = src->BoneIndices[0];
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
        dest->Color        = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
    }
}

static void VertProcRSkinNU2U3U3_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2U3U3_HW* dest = (VERTEX_RSKIN_NU2U3U3_HW*)_dest;
    VERTEX_RSKIN_NU2U3U3_SW* src  = (VERTEX_RSKIN_NU2U3U3_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = Vector4(src->Normal, (float)src->BlendIndices);
        dest->TexCoord = src->TexCoord;
        dest->Tangent  = src->Tangent;
        dest->Binormal = src->Binormal;
    }
}

static void VertProcRSkinNU2U3U3_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2U3U3_SW* dest = (VERTEX_RSKIN_NU2U3U3_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendIndices = src->BoneIndices[0];
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcRSkinNU2U3U3C_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2U3U3C_HW* dest = (VERTEX_RSKIN_NU2U3U3C_HW*)_dest;
    VERTEX_RSKIN_NU2U3U3C_SW* src  = (VERTEX_RSKIN_NU2U3U3C_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = Vector4(src->Normal, (float)src->BlendIndices);
        dest->Color    = src->Color;
        dest->TexCoord = src->TexCoord;
        dest->Tangent  = src->Tangent;
        dest->Binormal = src->Binormal;
    }
}

static void VertProcRSkinNU2U3U3C_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_RSKIN_NU2U3U3C_SW* dest = (VERTEX_RSKIN_NU2U3U3C_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendIndices = src->BoneIndices[0];
        dest->Normal       = src->Normal;
        dest->Color        = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
        dest->TexCoord     = src->TexCoord[0];
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcSkinNU2_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2_HW* dest = (VERTEX_SKIN_NU2_HW*)_dest;
    VERTEX_SKIN_NU2_SW* src  = (VERTEX_SKIN_NU2_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector4(src->BlendWeights.z, src->BlendWeights.y, src->BlendWeights.x, 1.0f - src->BlendWeights.x - src->BlendWeights.y - src->BlendWeights.z);
        dest->BlendIndices = src->BlendIndices;
        dest->Normal       = src->Normal;
        dest->TexCoord     = isUaW ? src->TexCoord : Pack_UV(src->TexCoord);
    }
}

static void VertProcSkinNU2_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2_SW* dest = (VERTEX_SKIN_NU2_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector3(src->BoneWeights[0], src->BoneWeights[1], src->BoneWeights[2]);
        dest->BlendIndices = (src->BoneIndices[0] << 0) | (src->BoneIndices[1] << 8) | (src->BoneIndices[2] << 16) | (src->BoneIndices[3] << 24);
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
    }
}

static void VertProcSkinNU2C_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2C_HW* dest = (VERTEX_SKIN_NU2C_HW*)_dest;
    VERTEX_SKIN_NU2C_SW* src  = (VERTEX_SKIN_NU2C_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector4(src->BlendWeights.z, src->BlendWeights.y, src->BlendWeights.x, 1.0f - src->BlendWeights.x - src->BlendWeights.y - src->BlendWeights.z);
        dest->BlendIndices = src->BlendIndices;
        dest->Normal       = src->Normal;
        dest->TexCoord     = isUaW ? src->TexCoord : Pack_UV(src->TexCoord);
        dest->Color        = src->Color;
    }
}

static void VertProcSkinNU2C_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2C_SW* dest = (VERTEX_SKIN_NU2C_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector3(src->BoneWeights[0], src->BoneWeights[1], src->BoneWeights[2]);
        dest->BlendIndices = (src->BoneIndices[0] << 0) | (src->BoneIndices[1] << 8) | (src->BoneIndices[2] << 16) | (src->BoneIndices[3] << 24);
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
        dest->Color        = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
    }
}

static void VertProcSkinNU2U3U3_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2U3U3_HW* dest = (VERTEX_SKIN_NU2U3U3_HW*)_dest;
    VERTEX_SKIN_NU2U3U3_SW* src  = (VERTEX_SKIN_NU2U3U3_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector4(src->BlendWeights.z, src->BlendWeights.y, src->BlendWeights.x, 1.0f - src->BlendWeights.x - src->BlendWeights.y - src->BlendWeights.z);
        dest->BlendIndices = src->BlendIndices;
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord;
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcSkinNU2U3U3_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2U3U3_SW* dest = (VERTEX_SKIN_NU2U3U3_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector3(src->BoneWeights[0], src->BoneWeights[1], src->BoneWeights[2]);
        dest->BlendIndices = (src->BoneIndices[0] << 0) | (src->BoneIndices[1] << 8) | (src->BoneIndices[2] << 16) | (src->BoneIndices[3] << 24);
        dest->Normal       = src->Normal;
        dest->TexCoord     = src->TexCoord[0];
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcSkinNU2U3U3C_HW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2U3U3C_HW* dest = (VERTEX_SKIN_NU2U3U3C_HW*)_dest;
    VERTEX_SKIN_NU2U3U3C_SW* src  = (VERTEX_SKIN_NU2U3U3C_SW*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector4(src->BlendWeights.z, src->BlendWeights.y, src->BlendWeights.x, 1.0f - src->BlendWeights.x - src->BlendWeights.y - src->BlendWeights.z);
        dest->BlendIndices = src->BlendIndices;
        dest->Normal       = src->Normal;
        dest->Color        = src->Color;
        dest->TexCoord     = src->TexCoord;
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcSkinNU2U3U3C_SW(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_SKIN_NU2U3U3C_SW* dest = (VERTEX_SKIN_NU2U3U3C_SW*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position     = src->Position;
        dest->BlendWeights = Vector3(src->BoneWeights[0], src->BoneWeights[1], src->BoneWeights[2]);
        dest->BlendIndices = (src->BoneIndices[0] << 0) | (src->BoneIndices[1] << 8) | (src->BoneIndices[2] << 16) | (src->BoneIndices[3] << 24);
        dest->Normal       = src->Normal;
        dest->Color        = D3DCOLOR_COLORVALUE(src->Color.r, src->Color.g, src->Color.b, src->Color.a);
        dest->TexCoord     = src->TexCoord[0];
        dest->Tangent      = src->Tangent;
        dest->Binormal     = src->Binormal;
    }
}

static void VertProcGrass(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_GRASS* dest = (VERTEX_GRASS*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position    = src->Position;
        dest->TexCoord    = Pack_UV(src->TexCoord[0]);
        dest->ClumpCenter = src->TexCoord[1];
    }
}

static void VertProcBillboard(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX_BILLBOARD* dest = (VERTEX_BILLBOARD*)_dest;
    MASTER_VERTEX*    src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
        dest->Normal   = src->Normal;
        dest->TexCoord = Pack_UV(src->TexCoord[0]);
        dest->Center   = Vector3(src->BoneWeights);
    }
}

static void VertProc(void* _dest, const void* _src, DWORD num, bool isUaW)
{
    VERTEX* dest = (VERTEX*)_dest;
    MASTER_VERTEX* src  = (MASTER_VERTEX*)_src;
    for (DWORD i = 0; i < num; i++, dest++, src++) {
        dest->Position = src->Position;
    }
}

// These are built-in types and have specialized vertices from the start
static void VertProcTerrain(void* _dest, const void* _src, DWORD num, bool isUaW) { memcpy(_dest, _src, num * sizeof(VERTEX_TERRAIN)); }
static void VertProcWater  (void* _dest, const void* _src, DWORD num, bool isUaW) { memcpy(_dest, _src, num * sizeof(VERTEX_WATER));   }
static void VertProcTrack  (void* _dest, const void* _src, DWORD num, bool isUaW) { memcpy(_dest, _src, num * sizeof(VERTEX_TRACK));   }

VertexFormatInfo VertexFormats[] = {
    {{sizeof(VERTEX_MESH_N),       elVertN,       NULL,            NULL},
     {sizeof(VERTEX_MESH_N),       elVertN,       VertProcN,       NULL}, 0},

    {{sizeof(VERTEX_MESH_NU2),      elVertNU2,      NULL,             NULL},
     {sizeof(VERTEX_MESH_NU2),      elVertNU2,      VertProcNU2,      NULL}, 0},
    {{sizeof(VERTEX_MESH_NU2C),     elVertNU2C,     NULL,             NULL},
     {sizeof(VERTEX_MESH_NU2C),     elVertNU2C,     VertProcNU2C,     NULL}, 0},
    {{sizeof(VERTEX_MESH_NU2U3U3),  elVertNU2U3U3,  NULL,             NULL},
     {sizeof(VERTEX_MESH_NU2U3U3),  elVertNU2U3U3,  VertProcNU2U3U3,  NULL}, 0},
    {{sizeof(VERTEX_MESH_NU2U3U3C), elVertNU2U3U3C, NULL,             NULL},
     {sizeof(VERTEX_MESH_NU2U3U3C), elVertNU2U3U3C, VertProcNU2U3U3C, NULL}, 0},

    {{sizeof(VERTEX_RSKIN_NU2_HW),      elVertRSkinNU2_HW,      VertProcRSkinNU2_HW,      NULL},
     {sizeof(VERTEX_RSKIN_NU2_SW),      elVertRSkinNU2_SW,      VertProcRSkinNU2_SW,      NULL}, 0},
    {{sizeof(VERTEX_RSKIN_NU2C_HW),     elVertRSkinNU2C_HW,     VertProcRSkinNU2C_HW,     NULL},
     {sizeof(VERTEX_RSKIN_NU2C_SW),     elVertRSkinNU2C_SW,     VertProcRSkinNU2C_SW,     NULL}, 0},
    {{sizeof(VERTEX_RSKIN_NU2U3U3_HW),  elVertRSkinNU2U3U3_HW,  VertProcRSkinNU2U3U3_HW,  NULL},
     {sizeof(VERTEX_RSKIN_NU2U3U3_SW),  elVertRSkinNU2U3U3_SW,  VertProcRSkinNU2U3U3_SW,  NULL}, 0},
    {{sizeof(VERTEX_RSKIN_NU2U3U3C_HW), elVertRSkinNU2U3U3C_HW, VertProcRSkinNU2U3U3C_HW, NULL},
     {sizeof(VERTEX_RSKIN_NU2U3U3C_SW), elVertRSkinNU2U3U3C_SW, VertProcRSkinNU2U3U3C_SW, NULL}, 0},

    {{sizeof(VERTEX_SKIN_NU2_HW),      elVertSkinNU2_HW,      VertProcSkinNU2_HW,      NULL},
     {sizeof(VERTEX_SKIN_NU2_SW),      elVertSkinNU2_SW,      VertProcSkinNU2_SW,      NULL}, 0},
    {{sizeof(VERTEX_SKIN_NU2C_HW),     elVertSkinNU2C_HW,     VertProcSkinNU2C_HW,     NULL},
     {sizeof(VERTEX_SKIN_NU2C_SW),     elVertSkinNU2C_SW,     VertProcSkinNU2C_SW,     NULL}, 0},
    {{sizeof(VERTEX_SKIN_NU2U3U3_HW),  elVertSkinNU2U3U3_HW,  VertProcSkinNU2U3U3_HW,  NULL},
     {sizeof(VERTEX_SKIN_NU2U3U3_SW),  elVertSkinNU2U3U3_SW,  VertProcSkinNU2U3U3_SW,  NULL}, 0},
    {{sizeof(VERTEX_SKIN_NU2U3U3C_HW), elVertSkinNU2U3U3C_HW, VertProcSkinNU2U3U3C_HW, NULL},
     {sizeof(VERTEX_SKIN_NU2U3U3C_SW), elVertSkinNU2U3U3C_SW, VertProcSkinNU2U3U3C_SW, NULL}, 0},

    {{sizeof(VERTEX_GRASS),     elVertGrass,     NULL,          NULL},
     {sizeof(VERTEX_GRASS),     elVertGrass,     VertProcGrass, NULL}, 0},
    
    {{sizeof(VERTEX_BILLBOARD), elVertBillboard, NULL,              NULL},
     {sizeof(VERTEX_BILLBOARD), elVertBillboard, VertProcBillboard, NULL}, 0},

    // The following formats can not be directly used from shaders
    {{sizeof(VERTEX),               elVert,             NULL,            NULL},
     {sizeof(VERTEX),               elVert,             VertProc,        NULL}, 0},
    {{sizeof(VERTEX_TERRAIN),       elVertTerrain,      NULL,            NULL},
     {sizeof(VERTEX_TERRAIN),       elVertTerrain,      VertProcTerrain, NULL}, 0},
    {{sizeof(VERTEX_WATER),         elVertWater,        NULL,            NULL},
     {sizeof(VERTEX_WATER),         elVertWater,        VertProcWater,   NULL}, 0},
    {{sizeof(VERTEX_TRACK),         elVertTrack,        NULL,            NULL},
     {sizeof(VERTEX_TRACK),         elVertTrack,        VertProcTrack,   NULL}, 0},
};

VertexFormatNameInfo VertexFormatNames[] = {
    {"ALD3DVERTN",             VF_MESH_N},
    {"ALD3DVERTNU2",           VF_MESH_NU2},
    {"ALD3DVERTNU2C",          VF_MESH_NU2C},
    {"ALD3DVERTNU2U3U3",       VF_MESH_NU2U3U3},
    {"ALD3DVERTNU2U3U3C",      VF_MESH_NU2U3U3C},
    {"ALD3DVERTRSKINNU2",      VF_RSKIN_NU2},
    {"ALD3DVERTRSKINNU2C",     VF_RSKIN_NU2C},
    {"ALD3DVERTRSKINNU2U3U3",  VF_RSKIN_NU2U3U3},
    {"ALD3DVERTRSKINNU2U3U3C", VF_RSKIN_NU2U3U3C},
    {"ALD3DVERTB4I4NU2",       VF_SKIN_NU2},
    {"ALD3DVERTB4I4NU2C",      VF_SKIN_NU2C},
    {"ALD3DVERTB4I4NU2U3U3",   VF_SKIN_NU2U3U3},
    {"ALD3DVERTB4I4NU2U3U3C",  VF_SKIN_NU2U3U3C},
    {"ALD3DVERTGRASS",         VF_GRASS},
    {"ALD3DVERTBILLBOARD",     VF_BILLBOARD},
    {NULL}
};

}
}