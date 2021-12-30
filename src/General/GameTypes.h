#ifndef GAMETYPES_H
#define GAMETYPES_H

#include "General/3DTypes.h"
#include <string>
#include <vector>

namespace Alamo
{

typedef int ShaderDetail;
static const ShaderDetail SHADERDETAIL_FIXEDFUNCTION = 0;
static const ShaderDetail SHADERDETAIL_DX8           = 1;
static const ShaderDetail SHADERDETAIL_DX8ATI        = 2;
static const ShaderDetail SHADERDETAIL_DX9           = 3;
static const ShaderDetail NUM_SHADER_DETAIL_LEVELS   = 4;

struct RenderSettings
{
    unsigned long m_screenWidth;
    unsigned long m_screenHeight;
    unsigned long m_screenRefresh;
    bool          m_softShadows;
    bool          m_antiAlias;
    ShaderDetail  m_shaderDetail;
    bool          m_bloom;
    bool          m_heatDistortion;
    bool          m_heatDebug;
    bool          m_shadowDebug;
    Color         m_modelColor;
};

struct Range
{
	float min;
	float max;
};

struct DirectionalLight
{
	Color   m_color;
	Vector3 m_direction;
};

struct Camera
{
    Vector3 m_position;
    Vector3 m_target;
    Vector3 m_up;
};

enum MapLightType
{
	LT_SUN,
	LT_FILL1,
	LT_FILL2,
    NUM_LIGHTS
};

struct Wind
{
	float speed;
	float heading;
};

struct Environment
{
	DirectionalLight m_lights[3];
	Color            m_specular;
	Color            m_ambient;
	Color			 m_shadow;
	Wind			 m_wind;
    Color            m_clearColor;
    Vector3          m_gravity;
};

struct BoundingBox
{
    Vector3 min;
    Vector3 max;
};

enum BillboardType
{
    BBT_DISABLE,
    BBT_PARALLEL,
    BBT_FACE,
    BBT_ZAXIS_VIEW,
    BBT_ZAXIS_LIGHT,
    BBT_ZAXIS_WIND,
    BBT_SUNLIGHT_GLOW,
    BBT_SUN
};

enum ShaderParameterType
{
    SPT_INT,
    SPT_FLOAT,
    SPT_FLOAT3,
    SPT_FLOAT4,
    SPT_TEXTURE
};

struct ShaderParameter
{
    std::string m_name;
    bool        m_colorize;
    ShaderParameterType m_type;
    int         m_int;
    float       m_float;
    Vector3     m_float3;
    Vector4     m_float4;
    std::string m_texture;
};

static const int MAX_NUM_SKIN_BONES = 24;

enum LightType
{
    LIGHT_OMNI,
    LIGHT_DIRECTIONAL,
    LIGHT_SPOT
};

static const int NUM_LODS = 10;
static const int NUM_ALTS = 10;

// Master vertex type, as stored in the files
#pragma pack(1)
struct MASTER_VERTEX
{
    Vector3 Position;
    Vector3 Normal;
    Vector2 TexCoord[4];
    Vector3 Tangent;
    Vector3 Binormal;
    Color   Color;
    Vector4 Unused;
    DWORD   BoneIndices[4];
    float   BoneWeights[4];
};
#pragma pack()

enum LightFieldSourceType {
    LFT_POINT = 0,
    LFT_HCONE,
    LFT_DUAL_HCONE,
    LFT_DUAL_OPPOSED_HCONE,
    LFT_LINE
};

enum LightFieldSourceFadeType {
    LFFT_TIME = 0,
    LFFT_PARTICLES
};

struct LightFieldSource
{
    LightFieldSourceType     m_type;
    LightFieldSourceFadeType m_fadeType;
    Vector3       m_position;
    Color         m_diffuse;
    float         m_intensity;
    float         m_width;
    float         m_length;
    float         m_height;
    float         m_autoDestructTime;
    float         m_autoDestructFadeTime;
    float         m_intensityNoiseScale;
    float         m_intensityNoiseTimeScale;
    float         m_angularVelocity;
    bool          m_affectedByGlobalIntensity;
    unsigned long m_particles;
};

}
#endif
