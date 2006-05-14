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
		FLOAT,
		FLOAT3,
		FLOAT4,
		TEXTURE,
		MATRIX,
	};

	std::string name;			// Name of the parameter
	Type        type;			// Type of the parameter

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
	D3DXVECTOR2 TexCoord;			//  24  18
	float       filler1[6];			//  32	20
	D3DXVECTOR3 Tangent;			//  56	38
	D3DXVECTOR3 Binormal;			//  68  44
	float       filler2[8];			//  80  50
	DWORD       BoneIndices[4];		// 112	70
	float       BoneWeights[4];		// 128  80
};
#pragma pack()

// Describes a camera
struct Camera
{
	D3DXVECTOR3 Position;
	D3DXVECTOR3 Target;
	D3DXVECTOR3 Up;
};

class IMesh
{
public:
	// Get the vertex and index buffer, respectively
	virtual const Vertex*      getVertexBuffer()    const = 0;
	virtual const uint16_t*    getIndexBuffer()     const = 0;
	virtual unsigned long      getNumVertices()     const = 0;
	virtual unsigned long      getNumTriangles()    const = 0;
	virtual const std::string& getVertexFormat()    const = 0;
	virtual D3DFILLMODE        getFillMode()        const = 0;
	virtual D3DCULL            getCulling()         const = 0;
	virtual unsigned int       getNumEffects()      const = 0;
	virtual const Effect*      getEffect(int index) const = 0;
	virtual unsigned long      getBoneMapping(int i) const = 0;
	virtual unsigned long      getNumBoneMappings()  const = 0;
	virtual ~IMesh() {}
};

struct RENDERINFO
{
	bool     showBones;
	bool     showBoneNames;
	bool     useColor;
	COLORREF color;
};

class Engine
{
private:
	class EngineImpl;
	EngineImpl* pimpl;

public:
	// Getters
	const Camera& getCamera() const;

	// Setters
	void       setCamera( const Camera& camera );
	void       setModel(Model* model);
	void       applyAnimation(const Animation* animation);
	Model*     getModel() const;
	Animation* getAnimation() const;
	void       enableMesh(unsigned int index, bool enabled);

	// Actions
	bool render(unsigned int frame, const RENDERINFO& ri);
	void reinitialize( HWND hWnd, int width, int height );

	Engine(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager);
	~Engine();
};

#endif