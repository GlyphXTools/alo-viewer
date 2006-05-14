#include <vector>

#include "mesh.h"
using namespace std;

class Mesh::MeshImpl
{
	friend Mesh;

	int					  index;
	std::string			  name;
	std::string			  vertexFormat;
	vector<Effect>		  effects;
	bool				  hasEffect;
	unsigned long		  nTriangles;
	unsigned long		  nVertices;
	Vertex*				  vertices;
	uint16_t*	   		  indices;
	D3DXVECTOR3           boundingBox[2];
	vector<unsigned long> boneMapping;
	bool                  hasCollisionTree;

	MeshImpl();
	~MeshImpl();
};

Mesh::MeshImpl::MeshImpl()
{
	nTriangles = ULONG_MAX;
	nVertices  = ULONG_MAX;
	vertices   = NULL;
	indices    = NULL;
	hasCollisionTree = false;
}

Mesh::MeshImpl::~MeshImpl()
{
	delete[] vertices;
	delete[] indices;
}

//
// Mesh functions
//

const Vertex*      Mesh::getVertexBuffer()     const { return pimpl->vertices;   }
const uint16_t*    Mesh::getIndexBuffer()      const { return pimpl->indices;    }
unsigned long      Mesh::getNumVertices()      const { return pimpl->nVertices;  }
unsigned long      Mesh::getNumTriangles()     const { return pimpl->nTriangles; }
D3DFILLMODE        Mesh::getFillMode()         const { return D3DFILL_SOLID; }
D3DCULL            Mesh::getCulling()          const { return D3DCULL_CW;    }
const std::string& Mesh::getName()             const { return pimpl->name;   }
int                Mesh::getIndex()            const { return pimpl->index;  }
const std::string& Mesh::getVertexFormat()     const { return pimpl->vertexFormat; }
unsigned int       Mesh::getNumEffects()       const { return (unsigned int)pimpl->effects.size(); }
unsigned long      Mesh::getBoneMapping(int i) const { return pimpl->boneMapping[i]; }
unsigned long      Mesh::getNumBoneMappings()  const { return (unsigned long)pimpl->boneMapping.size(); }
bool               Mesh::getCollisionTree()    const { return pimpl->hasCollisionTree; }

const Effect* Mesh::getEffect(int index) const
{
	return &pimpl->effects[index];
}

void Mesh::getBoundingBox(D3DXVECTOR3& v1, D3DXVECTOR3& v2) const
{
	v1 = pimpl->boundingBox[0];
	v2 = pimpl->boundingBox[1];
}

void Mesh::setVertexData(Vertex* vbuffer, uint16_t* ibuffer, unsigned long vertnum, unsigned long primnum, const string& format)
{
	pimpl->vertices     = vbuffer;
	pimpl->indices      = ibuffer;
	pimpl->nVertices    = vertnum;
	pimpl->nTriangles   = primnum;
	pimpl->vertexFormat = format;
}

void Mesh::addBoneMapping(unsigned long mapping)
{
	pimpl->boneMapping.push_back(mapping);
}

void Mesh::setName(const std::string& name)
{
	pimpl->name = name;
}

void Mesh::setCollisionTree()
{
	pimpl->hasCollisionTree = true;
}

void Mesh::setBoundingBox(const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
{
	pimpl->boundingBox[0] = v1;
	pimpl->boundingBox[1] = v2;
}

void Mesh::addEffect(const Effect& effect)
{
	pimpl->effects.push_back(effect);
}

Mesh::Mesh() : pimpl(new MeshImpl())
{
}

Mesh::~Mesh()
{
	delete pimpl;
}