#ifndef ENGINE_H
#define ENGINE_H

#include <string>
#include <vector>

#include "animation.h"
#include "model.h"
#include "managers.h"

struct Parameter
{
	enum Type
	{
		INT,
		FLOAT,
		FLOAT3,
		FLOAT4,
		TEXTURE,
		MATRIX,
	};

	std::string name;			// Name of the parameter
	Type        type;			// Type of the parameter

	int32_t     m_int;			// Valid if type is INT
	float       m_float;		// Valid if type is FLOAT
	float		m_float3[3];	// Valid if type is FLOAT3
	float		m_float4[4];	// Valid if type is FLOAT4
	D3DXMATRIX  m_matrix;		// Valud if type is MATRIX
	std::string m_texture;		// Valid if type is TEXTURE
};

struct Effect
{
	std::string				name;
	std::vector<Parameter>	parameters;
};

#pragma pack(1)
struct Vertex
{
	D3DXVECTOR3 Position;			//   0	00
	D3DXVECTOR3 Normal;				//  12	0C
	D3DXVECTOR2 TexCoords[4];		//  24  18
	D3DXVECTOR3 Tangent;			//  56	38
	D3DXVECTOR3 Binormal;			//  68  44
	D3DXVECTOR4 Color;				//  80  50
	float       filler[4];			//  96  60
	DWORD       BoneIndices[4];		// 112	70
	float       BoneWeights[4];		// 128  80
};

struct Vertex2
{
	D3DXVECTOR3 Position;			//   0	00
	D3DXVECTOR3 Normal;				//  12	0C
	D3DXVECTOR2 TexCoord;			//  24  18
	float       filler1[24];		//  32	20
};
#pragma pack()

// Describes a camera
struct Camera
{
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Target;
	D3DXVECTOR3 Up;
};

struct Material
{
	Effect					   effect;
	std::string				   vertexFormat;
	unsigned long			   nTriangles;
	unsigned long			   nVertices;
	Vertex*					   vertices;
	uint16_t*	               indices;
	std::vector<unsigned long> boneMapping;
	bool					   hasCollisionTree;
};

class IMesh
{
public:
	// Get the vertex and index buffer, respectively
	virtual const Material&    getMaterial(int i) const = 0;
	virtual unsigned int       getNumMaterials()  const = 0;
	virtual ~IMesh() {}
};

struct RENDERINFO
{
	bool     showBones;
	bool     showBoneNames;
	bool     useColor;
	COLORREF color;
};

enum RenderMode
{
	RM_SOLID,
	RM_WIREFRAME,
};

enum LightType
{
	LT_SUN,
	LT_FILL1,
	LT_FILL2,
};

struct LIGHT
{
	D3DXVECTOR4 Diffuse;
	D3DXVECTOR4 Specular;
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Direction;
};

class Engine
{
private:
	class EngineImpl;
	EngineImpl* pimpl;

public:
	// Getters
	const Camera& getCamera() const;
	Model*        getModel() const;
	Animation*    getAnimation() const;
	RenderMode    getRenderMode() const;
	COLORREF      getBackground() const;

	// Setters
	void setRenderMode( RenderMode mode );
	void setCamera( const Camera& camera );
	void setModel(Model* model);
	void applyAnimation(const Animation* animation);
	void enableMesh(unsigned int index, bool enabled);
	void setGroundVisibility(bool visible);
	
	void setBackground(COLORREF color);
	void setLight(LightType which, const LIGHT& light);
	void setAmbient(const D3DXVECTOR4& color);
	void setShadow(const D3DXVECTOR4& color);
	void setWind(const D3DXVECTOR3& wind);

	// Actions
	bool render(unsigned int frame, const RENDERINFO& ri);
	void reinitialize( HWND hWnd, int width, int height );

	Engine(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager);
	~Engine();
};

#endif