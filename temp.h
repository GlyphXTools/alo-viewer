enum RenderPhase
{
	TerrainMesh,
	Heat,
	Occluded,
	Opaque,
	Transparent,
	Shadow
};

typedef void (*VertexProc)();

struct BoundingBox
{
	float minX, minY, minZ;
	float maxX, maxY, maxZ;
};

struct Bone
{
	Bone*      parent;
	D3DXMATRIX transform;
};

struct Model
{
	unsigned int nBones;
	Bone*        bones;
};

struct Mesh
{
	Mesh* next;
	Mesh* prev;

	BoundingBox boundingBox;

	Model* model;

	Bone* bone;

	IDirect3DVertexBuffer9* vertexBuffer;
	IDirect3DIndexBuffer9*  indexBuffer;

	Effect* effect;

	unsigned int nParameters;
	Parameter*   parameters;
};

struct GlobalParameters
{
	// World, View, Projection, and all used combinations
	D3DXHANDLE hWorld;
	D3DXHANDLE hView;
	D3DXHANDLE hProjection;
	D3DXHANDLE hWorldView;
	D3DXHANDLE hViewProjection;
	D3DXHANDLE hWorldViewProjection;
	D3DXHANDLE hWorldInverse;
	D3DXHANDLE hViewInverse;
	D3DXHANDLE hWorldViewInverse;

	// Animation
	D3DXHANDLE hSkinMatrix;

	// Lighting (directional)
	D3DXHANDLE hGlobalAmbient;
	D3DXHANDLE hDirLightVec;
	D3DXHANDLE hDirLightObjVec;
	D3DXHANDLE hDirLightDiffuse;
	D3DXHANDLE hDirLightSpecular;

	D3DXHANDLE hSphLightAll;
	D3DXHANDLE hSphLightFill;

	// Miscellaneous
	D3DXHANDLE hResolutionConstants;
	D3DXHANDLE hTime;
};

struct Effect
{
	ID3DXEffect* effect;

	RenderPhase	renderPhase;
	VertexProc  vertexProc;
	bool		tangentSpace;
	bool		shadowVolume;
	bool		z_Sort;

	GlobalParameters parameters;
};

enum ParameterType
{
	PT_FLOAT,
	PT_FLOAT3,
	PT_FLOAT4,
	PT_TEXTURE,
};

struct Parameter
{
	D3DXHANDLE    hParameter;
	ParameterType type;
	enum
	{
		float m_float;
		float m_float3[3];
		float m_float4[4];
		IDirect3DBaseTexture9* m_texture;
	} value;

	~Parameter()
	{
		if (type == PT_TEXTURE && value.m_texture != NULL)
		{
			value.m_texture->Release();
		}
	}
};

struct Phase
{
	Effect* effect;
	Mesh*   meshes;
};
