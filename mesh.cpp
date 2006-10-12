#include <vector>

#include "mesh.h"
using namespace std;

class Mesh::MeshImpl
{
	friend Mesh;

	bool				  hidden;
	bool				  collisionEnabled;
	int					  index;
	std::string			  name;
	vector<Material>	  materials;
	D3DXVECTOR3           boundingBox[2];

	MeshImpl();
	~MeshImpl();
};

Mesh::MeshImpl::MeshImpl()
{
}

Mesh::MeshImpl::~MeshImpl()
{
	for (size_t i = 0; i < materials.size(); i++)
	{
		delete[] materials[i].vertices;
		delete[] materials[i].indices;
	}
}

//
// Mesh functions
//

D3DFILLMODE        Mesh::getFillMode()          const { return D3DFILL_SOLID; }
D3DCULL            Mesh::getCulling()           const { return D3DCULL_CW;    }
const std::string& Mesh::getName()              const { return pimpl->name;   }
int                Mesh::getIndex()             const { return pimpl->index;  }
const Material&    Mesh::getMaterial(int index) const { return pimpl->materials[index]; }
unsigned int       Mesh::getNumMaterials()      const { return (unsigned int)pimpl->materials.size(); }
bool			   Mesh::isHidden()             const { return pimpl->hidden; }
bool			   Mesh::isCollisionEnabled()   const { return pimpl->collisionEnabled; }

void Mesh::getBoundingBox(D3DXVECTOR3& v1, D3DXVECTOR3& v2) const
{
	v1 = pimpl->boundingBox[0];
	v2 = pimpl->boundingBox[1];
}

void Mesh::addMaterial(const Material& material)
{
	pimpl->materials.push_back(material);
}

void Mesh::setName(const std::string& name)				{ pimpl->name = name; }
void Mesh::setHidden(bool hidden)						{ pimpl->hidden = hidden; }
void Mesh::setCollisionEnabled(bool collisionEnabled)	{ pimpl->collisionEnabled = collisionEnabled; }

void Mesh::setBoundingBox(const D3DXVECTOR3& v1, const D3DXVECTOR3& v2)
{
	pimpl->boundingBox[0] = v1;
	pimpl->boundingBox[1] = v2;
}

Mesh::Mesh() : pimpl(new MeshImpl())
{
}

Mesh::~Mesh()
{
	delete pimpl;
}