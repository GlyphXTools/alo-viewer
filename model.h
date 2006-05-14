#ifndef MODEL_H
#define MODEL_H

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0400
#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>

#include <string>
#include <istream>
#include <vector>
#include <map>

#include "types.h"
#include "managers.h"

struct Parameter
{
	enum Type
	{
		FLOAT,
		TEXTURE,
		FLOAT4,
		MATRIX,
	};

	std::string name;			// Name of the parameter
	Type        type;			// Type of the parameter

	float       m_float;		// Valid if type is FLOAT
	float		m_float4[4];	// Valid if type is FLOAT4
	D3DXMATRIX  m_matrix;		// Valid if type is MATRIX
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
	D3DXVECTOR3 position;
	D3DXVECTOR3 normal;
	float		u, v;
	float filler[28];
};
#pragma pack()

class IMesh
{
public:
	// Get the vertex and index buffer, respectively
	virtual const Vertex* getVertexBuffer()  const = 0;
	virtual const uint16_t* getIndexBuffer() const = 0;
	virtual unsigned long getNumVertices()   const = 0;
	virtual unsigned long getNumTriangles()  const = 0;
	virtual const Effect* getEffect() const = 0;
	virtual const std::string& getVertexFormat() const = 0;
};

class Mesh : public IMesh
{
	friend class Model;
private:
	int             index;
	std::string		name;
	std::string     vertexFormat;
	Effect 			effect;
	bool            hasEffect;
	unsigned long	nTriangles;
	unsigned long	nVertices;
	Vertex*			vertices;
	uint16_t*		indices;

public:
	// Get the vertex and index buffer, respectively
	const Vertex* getVertexBuffer()  const { return vertices;   }
	const uint16_t* getIndexBuffer() const { return indices;    }
	unsigned long getNumVertices()   const { return nVertices;  }
	unsigned long getNumTriangles()  const { return nTriangles; }

	// Returns the effect for this mesh, or NULL if it does not have an effect
	const Effect* getEffect() const
	{
		return (hasEffect) ? &effect : NULL;
	}

	// Returns the name of this mesh
	const std::string& getName() const
	{
		return name;
	}

	// Returns the string describing the vertex format used for this mesh
	const std::string& getVertexFormat() const
	{
		return vertexFormat;
	}

	// Returns the index of this mesh in the model file
	int getIndex() const
	{
		return index;
	}

	Mesh(const Mesh& mesh);	// Declared but not implemented as a safeguard. Don't copy meshes!
	Mesh();
	~Mesh();
};

struct Bone
{
	unsigned int parent;
	D3DXMATRIX   matrix;
};

class Model
{
	std::vector<Mesh*>                   meshes;
	std::map<std::string,size_t>         meshMap;
	std::vector<Bone>                    bones;
	std::map<std::string,int>            boneMap;
	std::map<unsigned int, unsigned int> connections;
	unsigned long rootBone;

	void readModel(File* input);
	void readMesh(File* input);
	void readConnections(File* input);
	void readBone(File* input);
	void readBones(File* input);
	void readVertexBuffer(Mesh* mesh, File* input);
	void readEffectInfo(Effect& effect, File* input);

public:
	// Returns a mesh, or NULL if the mesh does not exist
	unsigned int getNumMeshes() const { return (unsigned int)meshes.size(); }
	const Mesh*  getMesh(std::string name) const;
	const Mesh*  getMesh(unsigned int mesh) const;

	// Returns a bone, or NULL if the bone does not exist
	unsigned int getNumBones() const { return (unsigned int)bones.size(); }
	const Bone*  getBone(std::string name) const;
	const Bone*  getBone(int bone) const;

	// Returns a transformation matrix for the specified mesh.
	// This matrix translates the mesh from its local origin to its proper location
	// based on the bone its connected to.
	bool getTransformation(std::string name, D3DXMATRIX& matrix) const;
	bool getTransformation(int mesh, D3DXMATRIX& matrix) const;

	// Returns the index of the bone this mesh is connected to
	int getConnection(unsigned int mesh) const;

	Model(File* file);
	~Model();
};

#endif