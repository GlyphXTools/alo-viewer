#include "engine.h"

class Mesh : public IMesh
{
private:
	class MeshImpl;
	
	MeshImpl* pimpl;

	// Don't copy meshes
	Mesh(const Mesh& mesh);
	void operator=(const Mesh& mesh);

public:
	// IMesh implementation
	const Vertex*      getVertexBuffer()     const;
	const uint16_t*    getIndexBuffer()      const;
	unsigned long      getNumVertices()      const;
	unsigned long      getNumTriangles()     const;
	D3DFILLMODE        getFillMode()         const;
	D3DCULL            getCulling()          const;
	unsigned int       getNumEffects()       const;
	const Effect*      getEffect(int index)  const;
	const std::string& getVertexFormat()     const;
	unsigned long      getBoneMapping(int i) const;
	unsigned long      getNumBoneMappings()  const;
	bool               getCollisionTree()    const;

	// Getters
	const std::string& getName()  const;
	int                getIndex() const;
	void               getBoundingBox(D3DXVECTOR3& v1, D3DXVECTOR3& v2) const;

	// Setters
	void setCollisionTree();
	void addBoneMapping(unsigned long mapping);
	void setVertexData(Vertex* vbuffer, uint16_t* ibuffer, unsigned long vertnum, unsigned long primnum, const std::string& format);
	void setName(const std::string& name);
	void setBoundingBox(const D3DXVECTOR3& v1, const D3DXVECTOR3& v2);
	void addEffect(const Effect& effect);
	
	Mesh();
	~Mesh();
};

