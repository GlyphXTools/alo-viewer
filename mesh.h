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
	unsigned int       getNumMaterials()      const;
	const Material&    getMaterial(int index) const;

	// Getters
	bool			   isHidden() const;
	bool			   isCollisionEnabled() const;
	const std::string& getName()  const;
	int                getIndex() const;
	void               getBoundingBox(D3DXVECTOR3& v1, D3DXVECTOR3& v2) const;

	// Setters
	void addMaterial(const Material& material);
	void setName(const std::string& name);
	void setHidden(bool hidden);
	void setCollisionEnabled(bool collisionEnabled);
	void setBoundingBox(const D3DXVECTOR3& v1, const D3DXVECTOR3& v2);
	void addEffect(const Effect& effect);
	
	Mesh();
	~Mesh();
};

