#include <climits>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <set>

#include "mesh.h"
#include "model.h"
#include "chunks.h"
#include "exceptions.h"

using namespace std;

class Model::ModelImpl
{
	friend class Model;

	string							name;
	unsigned long                   nLights;
	vector<Mesh*>                   meshes;
	map<string,size_t>              meshMap;
	vector<Bone>                    bones;
	map<string,size_t>              boneMap;
	map<unsigned int, unsigned int> connections;

	void readModel(File* input);
	void readBones(File* input);
	void readBone(Bone& bone, File* input);
	void readMesh(Mesh& mesh, File* input);
	void readMeshData(Material& material, File* input);
	void readEffectInfo(Effect& effect, File* input);
	void readEffectParameter(Parameter& param, File* input);
	void readConnections(File* input);
	void readConnectionsInfo(unsigned long& nConnections, unsigned long& nAttachments, File* input);
	void readConnection(unsigned long& mesh, unsigned long& bone, File* input);
	void readProxy(File* input);

	ModelImpl(File* file);
	~ModelImpl();
};

#pragma pack(1)
struct Node
{
	unsigned char x1, y1, z1;
	unsigned char x2, y2, z2;
	unsigned short type;
	unsigned short link;
};
#pragma pack()

static bool checkNode(const Node* nodes, int node, set<int>& visited)
{
	for (unsigned short i = 0; i < nodes[node].type; i++)
	{
		if (visited.insert(nodes[node].link + i).second == false) return false;
	}

	if (nodes[node].type == 0)
	{
		if (!checkNode(nodes, nodes[node].link + 0, visited )) return false;
		if (!checkNode(nodes, nodes[node].link + 1, visited )) return false;
	}
	return true;
}

static void readCollisionTree(Material& material, File* input)
{
	unsigned long nNodes = 0;
	unsigned long nTrans = 0;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1201:
			{
				MiniChunks chunks(chunk);
				unsigned char len1, len2;
				const char* data1 = chunks.getChunk(0x02, len1);
				const char* data2 = chunks.getChunk(0x03, len2);
				if (len1 != 4 || len2 != 4)
				{
					throw BadFileException(input->getName());
				}
				nNodes = letohl( *(uint32_t*)data1 );
				nTrans = letohl( *(uint32_t*)data2 );
				break;
			}

			case 0x1202:
				break;

			case 0x1203:
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
	material.hasCollisionTree = true;
}

//
// Reads vertex information from a Chunk with type 0x10000
//
void Model::ModelImpl::readMeshData(Material& material, File* input)
{
	material.nVertices  = 0;
	material.nTriangles = 0;
	material.vertices = NULL;
	material.indices  = NULL;
	material.hasCollisionTree = false;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x1200:
				// Collision kd-tree
				readCollisionTree(material, stream);
				break;

			case 0x10001:
			{
				// Mesh data chunk information
				uint32_t verts;
				uint32_t prims;
				if (stream->read( (char*)&verts, sizeof verts) != sizeof verts ||
				    stream->read( (char*)&prims, sizeof prims) != sizeof prims)
				{
					throw IOException("Unable to read chunk data");
				}
				material.nVertices  = letohl(verts);
				material.nTriangles = letohl(prims);
				break;
			}

			case 0x10002:
			{
				// Vertex format identifier
				char* str = chunk.getData();
				material.vertexFormat = str;
				delete[] str;
				break;
			}

			case 0x10004:
				// Index buffer
				delete[] material.indices;
				if (3 * material.nTriangles != chunk.getSize() / sizeof(uint16_t))
				{
					delete[] material.vertices;
					throw BadFileException(input->getName());
				}
				material.indices = (uint16_t*)chunk.getData();
				break;

			case 0x10007:
				// Vertex buffer
				delete[] material.vertices;
				if (material.nVertices != chunk.getSize() / sizeof(Vertex))
				{
					delete[] material.indices;
					throw BadFileException(input->getName());
				}
				material.vertices = (Vertex*)chunk.getData();
				break;

			case 0x10005:
			{
				// Vertex buffer
				delete[] material.vertices;
				if (material.nVertices != chunk.getSize() / sizeof(Vertex2))
				{
					delete[] material.indices;
					throw BadFileException(input->getName());
				}
				Vertex2* tmp = (Vertex2*)chunk.getData();
				material.vertices = new Vertex[material.nVertices];
				memset(material.vertices, 0, material.nVertices * sizeof(Vertex));
				for (unsigned int i = 0; i < material.nVertices; i++)
				{
					memcpy(&material.vertices[i], &tmp[i], offsetof(Vertex2, filler1));
				}
				delete[] tmp;
				break;
			}

			case 0x10006:
			{
				// Bone to skin-array mapping
				const unsigned long* data = (const unsigned long*)chunk.getData();
				unsigned long size = chunk.getSize() / 4;
				for (unsigned long i = 0; i < size; i++)
				{
					material.boneMapping.push_back(data[i]);
				}
				delete[] data;
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

//
// Parses mini-chunks into an effect parameter
//
void Model::ModelImpl::readEffectParameter(Parameter& param, File* input)
{
	int valid = 0;

	while (!input->eof())
	{
		MiniChunk chunk(input);
		File*     stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 1:
			{
				char* data = chunk.getData();
				param.name = data;
				delete[] data;
				valid |= 1;
				break;
			}

			case 2:
			{
				int len = chunk.getSize();
				if ((param.type == Parameter::INT    && len !=     sizeof(int32_t)) ||
					(param.type == Parameter::FLOAT  && len !=     sizeof(float)) ||
					(param.type == Parameter::FLOAT4 && len != 4 * sizeof(float)) ||
					(param.type == Parameter::FLOAT3 && len != 3 * sizeof(float)))
				{
					throw BadFileException(input->getName());
				}

				char* data = chunk.getData();
				switch (param.type)
				{
					case Parameter::INT:     memcpy(&param.m_int,    data, len); break;
					case Parameter::FLOAT:   memcpy(&param.m_float,  data, len); break;
					case Parameter::FLOAT3:  memcpy(&param.m_float3, data, len); break;
					case Parameter::FLOAT4:  memcpy(&param.m_float4, data, len); break;
					case Parameter::TEXTURE: param.m_texture = data; break;
				}
				delete[] data;
				valid |= 2;
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	if (valid != 3)
	{
		throw BadFileException(input->getName());
	}
}

//
// Reads effect information from a Chunk with type 0x10100
//
void Model::ModelImpl::readEffectInfo(Effect& effect, File* input)
{
	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType() & 0xFF)
		{
			case 1:
			{
				// Effect name
				char* str = chunk.getData();
				effect.name = str;
				delete[] str;
				break;
			}

			case 2:
			{
				// Effect parameter: INT
				Parameter param;
				param.type = Parameter::INT;
				readEffectParameter( param, stream );
				effect.parameters.push_back(param);
				break;
			}

			case 3:
			{
				// Effect parameter: FLOAT
				Parameter param;
				param.type = Parameter::FLOAT;
				readEffectParameter( param, stream );
				effect.parameters.push_back(param);
				break;
			}

			case 4:
			{
				// Effect parameter: FLOAT3
				Parameter param;
				param.type = Parameter::FLOAT3;
				readEffectParameter( param, stream );
				effect.parameters.push_back(param);
				break;
			}

			case 5:
			{
				// Effect parameter: TEXTURE
				Parameter param;
				param.type = Parameter::TEXTURE;
				readEffectParameter( param, stream );
				effect.parameters.push_back(param);
				break;
			}

			case 6:
			{
				// Effect parameter: FLOAT4
				Parameter param;
				param.type = Parameter::FLOAT4;
				readEffectParameter( param, stream );
				effect.parameters.push_back(param);
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

//
// Reads a mesh from a Chunk with type 0x0400
//
void Model::ModelImpl::readMesh(Mesh& mesh, File* input)
{
	unsigned long nMaterials = 0;
	Material material;

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
				mesh.setName(str);
				delete[] str;
				break;
			}

			case 0x00402:
			{
				// Mesh metadata
				if (chunk.getSize() < 40)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				nMaterials = letohl(*(uint32_t*)&data[0]);

				mesh.setHidden          (letohl(*(uint32_t*)&data[32]) != 0);
				mesh.setCollisionEnabled(letohl(*(uint32_t*)&data[36]) != 0);
				mesh.setBoundingBox     (
					D3DXVECTOR3(*(float*)&data[ 4], *(float*)&data[ 8], *(float*)&data[12]),
					D3DXVECTOR3(*(float*)&data[16], *(float*)&data[20], *(float*)&data[24]) );
				delete[] data;
				break;
			}

			case 0x10000:
				// Mesh vertex and index buffer
				readMeshData(material, stream);
				if (material.vertices != NULL && material.indices != NULL && material.vertexFormat != "" && material.effect.name != "")
				{
					mesh.addMaterial(material);
				}
				material = Material();
				break;

			case 0x10100:
				// Effect info
				readEffectInfo(material.effect, stream);
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

void Model::ModelImpl::readBone(Bone& bone, File* input)
{
	int valid = 0;

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
				bone.name = str;
				delete[] str;
				valid |= 1;
				break;
			}

			case 0x0205:
			{
				// Bone offset matrix
				if (chunk.getSize() < 56)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				bone.parent  = letohl(*(uint32_t*)&data[0]);
				bone.visible = letohl(*(uint32_t*)&data[4]) != 0;
				bone.billboardType = BT_DISABLE;
				memset(&bone.matrix, 0, sizeof(D3DXMATRIX));
				memcpy(&bone.matrix, data + 8, chunk.getSize() - 8);
				bone.matrix[15] = 1.0f;
				D3DXMatrixTranspose(&bone.matrix, &bone.matrix);

				delete[] data;
				valid |= 2;
				break;
			}

			case 0x0206:
			{
				// Bone offset matrix
				if (chunk.getSize() < 60)
				{
					throw BadFileException(input->getName());
				}
				char* data = chunk.getData();
				bone.parent = letohl(*(uint32_t*)&data[0]);
				bone.visible = letohl(*(uint32_t*)&data[4]) != 0;
				bone.billboardType = (BillboardType)letohl(*(uint32_t*)&data[8]);
				memset(&bone.matrix, 0, sizeof(D3DXMATRIX));
				memcpy(&bone.matrix, data + 12, chunk.getSize() - 12);
				bone.matrix[15] = 1.0f;
				D3DXMatrixTranspose(&bone.matrix, &bone.matrix);

				delete[] data;
				valid |= 2;
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}

	if (valid != 3)
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readBones(File* input)
{
	int nBones = 0;

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
			{
				// Bone
				Bone bone;
				readBone(bone, stream);
				if (bone.parent != -1 && (bone.parent < 0 || (unsigned int)bone.parent >= bones.size()))
				{
					throw BadFileException(input->getName());
				}
				string name = bone.name;
				transform(name.begin(), name.end(), name.begin(), toupper);
				boneMap.insert(make_pair(name, bones.size()));
				bones.push_back(bone);
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize() );
	}

	// Validate
	if (bones.size() != nBones)
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readConnectionsInfo(unsigned long& nConnections, unsigned long& nAttachments, File* input)
{
	int valid = 0;

	while (!input->eof())
	{
		MiniChunk chunk(input);
		File*     stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 1:
			{
				uint32_t conns;
				if (stream->read(&conns, sizeof(conns)) != sizeof(conns))
				{
					throw BadFileException(input->getName());
				}
				nConnections = letohl(conns);
				valid |= 1;
				break;
			}

			case 4:
			{
				uint32_t attachs;
				if (stream->read(&attachs, sizeof(attachs)) != sizeof(attachs))
				{
					throw BadFileException(input->getName());
				}
				nAttachments = letohl(attachs);
				valid |= 2;
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek( chunk.getStart() + chunk.getSize() );
	}

	if (valid != 3)
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readConnection(unsigned long& mesh, unsigned long& bone, File* input)
{
	int valid = 0;

	while (!input->eof())
	{
		MiniChunk chunk(input);
		File*     stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 2:
			{
				uint32_t _mesh;
				if (stream->read(&_mesh, sizeof(_mesh)) != sizeof(_mesh))
				{
					throw BadFileException(input->getName());
				}
				mesh = letohl(_mesh);
				valid |= 1;
				break;
			}

			case 3:
			{
				uint32_t _bone;
				if (stream->read(&_bone, sizeof(_bone)) != sizeof(_bone))
				{
					throw BadFileException(input->getName());
				}
				bone = letohl(_bone);
				valid |= 2;
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek( chunk.getStart() + chunk.getSize() );
	}

	if (valid != 3)
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readProxy(File* input)
{
	int valid = 0;

	string name;

	while (!input->eof())
	{
		MiniChunk chunk(input);
		File*     stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 5:
				{
				valid |= 1;
				char* data = chunk.getData();
				name = data;
				if (name.find("ALT") != string::npos && name.find("LOD") != string::npos)
				{
					printf("%s\n", name.c_str());
				}
				delete[] data;
				break;
				}

			case 6:	valid |= 2; break;
			case 7:	valid |= 4; break;
			case 8:	valid |= 8; break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek( chunk.getStart() + chunk.getSize() );
	}

	if ((valid & 3) != 3)
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readConnections(File* input)
{
	unsigned long nConnections = 0;
	unsigned long nAttachments = 0;

	while (!input->eof())
	{
		Chunk chunk(input);
		File* stream = chunk.getStream();

		switch (chunk.getType())
		{
			case 0x0601:
				readConnectionsInfo(nConnections, nAttachments, stream);
				break;

			case 0x0602:
			{
				unsigned long mesh, bone;
				readConnection(mesh, bone, stream);
				connections.insert(make_pair(mesh, bone));
				break;
			}

			case 0x0603:
			{
				readProxy(stream);
				break;
			}

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek( chunk.getStart() + chunk.getSize() );
	}

	if (nConnections != connections.size())
	{
		throw BadFileException(input->getName());
	}
}

void Model::ModelImpl::readModel(File* input)
{
	nLights = 0;

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
			{
				Mesh* mesh = new Mesh;
				try
				{
					readMesh(*mesh, stream);
					if (mesh->getName() != "")
					{
						string name = mesh->getName();
						transform(name.begin(), name.end(), name.begin(), toupper);
						meshMap.insert( make_pair(name, meshes.size()) );
						meshes.push_back(mesh);
					}
				}
				catch (...)
				{
					delete mesh;
					throw;
				}
				break;
			}

			case 0x0600:
				readConnections(stream);
				break;

			case 0x1300:
				// We ignore lights, but count it as part of the connectables
				nLights++;
				break;

			default:
				throw BadFileException(input->getName());
				break;
		}
		input->seek(chunk.getStart() + chunk.getSize());
	}
}

Model::ModelImpl::ModelImpl(File* file)
{
	name      = file->getName();
	readModel(file);
}

Model::ModelImpl::~ModelImpl()
{
	for (vector<Mesh*>::iterator i = meshes.begin(); i != meshes.end(); i++)
	{
		delete *i;
	}
}

//
// Public Model functions
//
int Model::getConnection(unsigned int mesh) const
{
	map<unsigned int, unsigned int>::const_iterator p = pimpl->connections.find(mesh + pimpl->nLights);
	return (p == pimpl->connections.end()) ? -1 : p->second;
}

const Mesh* Model::getMesh(string name) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,size_t>::const_iterator p = pimpl->meshMap.find(name);
	return (p == pimpl->meshMap.end()) ? NULL : pimpl->meshes[p->second];
}

unsigned int Model::getNumMeshes() const { return (unsigned int)pimpl->meshes.size(); }
unsigned int Model::getNumBones()  const { return (unsigned int)pimpl->bones.size();  }

const Mesh* Model::getMesh(unsigned int index) const
{
	return pimpl->meshes[index];
}

const Bone* Model::getBone(string name) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,size_t>::const_iterator p = pimpl->boneMap.find(name);
	return (p == pimpl->boneMap.end()) ? NULL : &pimpl->bones[p->second];
}

const Bone* Model::getBone(int bone) const
{
	return &pimpl->bones[bone];
}

bool Model::getTransformation(string name, D3DXMATRIX& matrix) const
{
	transform(name.begin(), name.end(), name.begin(), toupper);
	map<string,size_t>::const_iterator p = pimpl->boneMap.find(name);
	return (p == pimpl->boneMap.end()) ? false : getTransformation( (int)p->second, matrix);
}

bool Model::getTransformation(int mesh, D3DXMATRIX& matrix ) const
{
	return getBoneTransformation( getConnection(mesh), matrix );
}

bool Model::getBoneTransformation(int bone, D3DXMATRIX& matrix ) const
{
	D3DXMatrixIdentity( &matrix );
	while (bone != -1)
	{
		D3DXMatrixMultiply( &matrix, &matrix, &pimpl->bones[bone].matrix );
		bone = pimpl->bones[bone].parent;
	}
	return true;
}

const string& Model::getName() const
{
	return pimpl->name;
}

Model::Model(File* file) : pimpl(new ModelImpl(file))
{
}

Model::~Model()
{
	delete pimpl;
}
