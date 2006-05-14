#include <climits>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "model.h"
#include "types.h"
#include "exceptions.h"

using namespace std;

Mesh::Mesh()
{
	hasEffect  = false;
	nTriangles = ULONG_MAX;
	nVertices  = ULONG_MAX;
	vertices   = NULL;
	indices    = NULL;
}

Mesh::~Mesh()
{
	delete[] vertices;
	delete[] indices;
}

// This represents a Chunk
class Chunk : public streambuf
{
    uint32_t type;
	uint32_t size;
	unsigned long start;

	File* stream;

public:
	bool isGroup() const           { return (size & 0x80000000) != 0; }
	unsigned long getSize() const  { return size & 0x7FFFFFFF; }
	unsigned long getType() const  { return type; }
	unsigned long getStart() const { return start; }

	char* getData()
	{
		char* data = new char[getSize()];
		stream->seek(0);
		if (stream->read(data, getSize()) != getSize())
		{
			delete[] data;
			throw IOException("Unable to read chunk data");
		}
		return data;
	}

	File* getStream()
	{
		return stream;
	}

	Chunk(File* input)
	{
		if (input->read( (char*)&type, sizeof type) != sizeof type ||
			input->read( (char*)&size, sizeof size) != sizeof size)
		{
			throw IOException("Unable to read chunk header");
		}
		type   = letohl(type);
		size   = letohl(size);
		start  = input->tell();
		stream = new SubFile(input, start, getSize());
	}

	~Chunk()
	{
		stream->release();
	}
};

int Model::getConnection(unsigned int mesh) const
{
	map<unsigned int, unsigned int>::const_iterator p = connections.find(mesh);
	return (p == connections.end()) ? -1 : p->second;
}

const Mesh* Model::getMesh(string name) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,size_t>::const_iterator p = meshMap.find(name);
	return (p == meshMap.end()) ? NULL : meshes[p->second];
}

const Mesh* Model::getMesh(unsigned int index) const
{
	return meshes[index];
}

const Bone* Model::getBone(string name) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,int>::const_iterator p = boneMap.find(name);
	return (p == boneMap.end()) ? NULL : getBone(p->second);
}

const Bone* Model::getBone(int bone) const
{
	return &bones[bone];
}

bool Model::getTransformation(string name, D3DXMATRIX& matrix) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,int>::const_iterator p = boneMap.find(name);
	return (p == boneMap.end()) ? false : getTransformation(p->second, matrix);
}

bool Model::getTransformation(int mesh, D3DXMATRIX& matrix ) const
{
	D3DXMATRIX tmp;
	D3DXMatrixIdentity( &tmp );

	int bone = getConnection(mesh);
	while (bone != -1)
	{
		matrix = tmp;
		D3DXMatrixMultiply( &tmp, &bones[bone].matrix, &matrix );
		bone = bones[bone].parent;
	}

	D3DXMatrixTranspose(&matrix, &tmp);
	return true;
}

//
// Reads vertex information from a Chunk with type 0x10000
//
void Model::readVertexBuffer(Mesh* mesh, File* input)
{
	unsigned long vert_num = ULONG_MAX;
	unsigned long indx_num = ULONG_MAX;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x10001:
			{
				uint32_t verts;
				uint32_t prims;
				if (stream->read( (char*)&verts, sizeof verts) != sizeof verts ||
				    stream->read( (char*)&prims, sizeof prims) != sizeof prims)
				{
					throw IOException("Unable to read chunk data");
				}
				mesh->nVertices  = letohl(verts);
				mesh->nTriangles = letohl(prims);

				// Skip the rest of the Chunk
				input->seek(input->tell() + chunk.getSize() - sizeof verts - sizeof prims);
				break;
			}

			case 0x10002:
			{
				// Vertex format identifier
				char* str = chunk.getData();
				mesh->vertexFormat = str;
				delete[] str;
				break;
			}

			case 0x10004:
				indx_num = chunk.getSize() / sizeof(uint16_t);
				mesh->indices = (uint16_t*)chunk.getData();
				break;

			case 0x10007:
				vert_num = chunk.getSize() / sizeof(Vertex);
				mesh->vertices = (Vertex*)chunk.getData();
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	// Sanity check; if buffer sizes are at least the indicates sizes
	if (vert_num < mesh->nVertices || indx_num < 3 * mesh->nTriangles)
	{
		throw BadFileException(input->getName());
	}
}

//
// Reads effect information from a Chunk with type 0x10100
//
void Model::readEffectInfo(Effect& effect, File* input)
{
	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType() & 0xFF)
		{
			case 1:
			{
				char* str = chunk.getData();
				effect.name = str;
				delete[] str;
				break;
			}

			case 3:
			{
				Parameter param;
				char* data = chunk.getData();
				int len1 = data[1];
				int len2 = data[3 + len1];
				if (len2 != sizeof(float))
				{
					throw BadFileException(input->getName());
				}
				param.name = &data[2];
				param.type = Parameter::FLOAT;
				memcpy( &param.m_float, &data[4 + len1], sizeof(float));
				delete[] data;
				effect.parameters.push_back(param);
				break;
			}

			case 5:
			{
				Parameter param;
				char* data = chunk.getData();
				int len1 = data[1];
				int len2 = data[3 + len1];
				param.name = &data[2];
				param.type = Parameter::TEXTURE;
				param.m_texture = &data[4 + len1];
				delete[] data;
				effect.parameters.push_back(param);
				break;
			}

			case 6:
			{
				Parameter param;
				char* data = chunk.getData();
				int len1 = data[1];
				int len2 = data[3 + len1];
				if (len2 != 4 * sizeof(float))
				{
					throw BadFileException(input->getName());
				}
				param.name = &data[2];
				param.type = Parameter::FLOAT4;
				memcpy( &param.m_float4, &data[4 + len1], len2 );
				delete[] data;
				effect.parameters.push_back(param);
				break;
			}
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

//
// Reads a mesh from a Chunk with type 0x0400
//
void Model::readMesh(File* input)
{
	Mesh* mesh = new Mesh();
	try
	{
		mesh->index = (int)meshes.size();
		string name;

		while (!input->eof())
		{
			Chunk chunk(input);
			File* stream = chunk.getStream();

			switch (chunk.getType())
			{
				case 0x00401:
				{
					// Mesh identifier string
					char* str = chunk.getData();
					name = str;
					delete[] str;
					break;
				}

				case 0x10000:
					// Mesh vertex and index buffer
					readVertexBuffer(mesh, stream);
					break;

				case 0x10100:
					// Effect info
					readEffectInfo(mesh->effect, stream);
					mesh->hasEffect = true;
					break;
			}
			input->seek(chunk.getStart() + chunk.getSize());
		}

		if (name != "")
		{
			mesh->name = name;
			transform(name.begin(), name.end(), name.begin(), toupper);
			meshMap.insert( make_pair(name, meshes.size()) );
		}
		meshes.push_back(mesh);
	}
	catch (...)
	{
		delete mesh;
		throw;
	}
}

void Model::readBone(File* input)
{
	Bone   bone;
	string name;
	int    valid = 0;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x0203:
			{
				// Bone identifier string
				char* str = chunk.getData();
				name = str;
				transform(name.begin(), name.end(), name.begin(), toupper);
				delete[] str;
				valid |= 1;
				break;
			}

			case 0x0206:
			{
				// The stored matrix appears to be a 4x3 matrix, so in order
				// to create a 4x4 Direct3D matrix, we must padd and transpose
				if (chunk.getSize() < 12)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				bone.parent = letohl(*(uint32_t*)&data[0]);
				memset(&bone.matrix, 0, sizeof(D3DXMATRIX));
				memcpy(&bone.matrix, data + 12, chunk.getSize() - 12);
				bone.matrix[15] = 1.0f;
				delete[] data;
				valid |= 2;
				break;
			}
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	if (valid == 3)
	{
		int index = (int)bones.size();
		bones.push_back(bone);
		boneMap.insert(make_pair(name, index));
	}
}

void Model::readBones(File* input)
{
	unsigned long nBones;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x0201:
			{
				// Bones count
				uint32_t count;
				if (input->read((char*)&count, sizeof count) != sizeof count)
				{
					throw IOException("Unable to read chunk data");
				}
				nBones = letohl(count);
				break;
			}

			case 0x0202:
				// Bone
				readBone(stream);
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize() );
	}
}

void Model::readConnections(File* input)
{
	unsigned long nConnections = 0;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x0601:
			{
				if (chunk.getSize() < 12)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				char *type1 = &data[0];
				int   len1  =  data[1];
				char *type2 = &data[2 + len1];
				int   len2  =  data[3 + len1];
				if (len1 != 4 || len2 != 4 || *type1 != 1 || *type2 != 4)
				{
					throw BadFileException(input->getName());
				}
				uint32_t* conns = (uint32_t*)&data[2];
				uint32_t* root  = (uint32_t*)&data[8];

				nConnections = letohl(*conns);
				rootBone     = letohl(*root);
				delete[] data;
				break;
			}

			case 0x0602:
			{
				if (chunk.getSize() < 12)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				char *type1 = &data[0];
				int   len1  =  data[1];
				char *type2 = &data[2 + len1];
				int   len2  =  data[3 + len1];
				if (len1 != 4 || len2 != 4 || *type1 != 2 || *type2 != 3)
				{
					throw BadFileException(input->getName());
				}
				uint32_t* mesh = (uint32_t*)&data[2];
				uint32_t* bone = (uint32_t*)&data[8];

				connections.insert(make_pair(letohl(*mesh), letohl(*bone)));
				delete[] data;
				break;
			}
		}
		input->seek( chunk.getStart() + chunk.getSize() );
	}

	if (nConnections != connections.size())
	{
		throw BadFileException(input->getName());
	}
}

void Model::readModel(File* input)
{
	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x0200:
				readBones(stream);
				break;

			case 0x0400:
				readMesh(stream);
				break;

			case 0x0600:
				readConnections(stream);
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

Model::Model(File* file)
{
	readModel(file);
}

Model::~Model()
{
	for (vector<Mesh*>::iterator i = meshes.begin(); i != meshes.end(); i++)
	{
		delete *i;
	}
}
