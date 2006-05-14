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

//#include "mesh.h"
#include "files.h"
#include "types.h"

class Mesh;

struct Bone
{
	std::string name;
	int         parent;
	D3DXMATRIX  matrix;
	std::vector<D3DXMATRIX> skin;
};

class Model
{
	class ModelImpl;
	
	ModelImpl* pimpl;

public:
	// Returns a mesh, or NULL if the mesh does not exist
	unsigned int getNumMeshes() const;
	const Mesh*  getMesh(std::string name) const;
	const Mesh*  getMesh(unsigned int mesh) const;

	// Returns a bone, or NULL if the bone does not exist
	unsigned int getNumBones() const;
	const Bone*  getBone(std::string name) const;
	const Bone*  getBone(int bone) const;

	// Returns a transformation matrix for the specified mesh.
	// This matrix translates the mesh from its local origin to its proper location
	// based on the bone its connected to.
	bool getTransformation(std::string name, D3DXMATRIX& matrix) const;
	bool getTransformation(int mesh, D3DXMATRIX& matrix) const;
	bool getBoneTransformation(int bone, D3DXMATRIX& matrix) const;

	// Returns the index of the bone this mesh is connected to
	int getConnection(unsigned int mesh) const;

	Model(File* file);
	~Model();
};

#endif