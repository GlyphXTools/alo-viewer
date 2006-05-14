#ifndef ENGINE_H
#define ENGINE_H

#include "types.h"
#include "model.h"

class Engine
{
public:
	struct Camera
	{
		D3DXVECTOR3 Position;
		D3DXVECTOR3 Target;
		D3DXVECTOR3 Up;
	};

private:
	class MeshInfo
	{
		bool					enabled;
		const IMesh*            pMesh;
		IDirect3DVertexBuffer9* pVertexBuffer;
		IDirect3DIndexBuffer9*  pIndexBuffer;
		IDirect3DDevice9*       pDevice;
		D3DXMATRIX				transformation;

		void checkVertexBuffer();
		void checkIndexBuffer();

	public: 
		bool              isEnabled() const         { return enabled; }
		void              enable(bool enabled)      { this->enabled = enabled; }
		const IMesh&      getMesh() const           { return *pMesh; }
		const D3DXMATRIX  getTransformation() const { return transformation; }
		IDirect3DVertexBuffer9* getVertexBuffer()   { checkVertexBuffer(); return pVertexBuffer; }
		IDirect3DIndexBuffer9*  getIndexBuffer()    { checkIndexBuffer();  return pIndexBuffer; }

		void invalidate();

		MeshInfo& operator=(const MeshInfo& meshinfo);

		MeshInfo(const MeshInfo& meshinfo);
		MeshInfo(IDirect3DDevice9* pDevice, const IMesh* mesh, const D3DXMATRIX& transformation, bool enabled = false);
		~MeshInfo();
	};

	struct Settings
	{
		D3DXMATRIX  World;
		D3DXMATRIX  View;
		D3DXMATRIX  Projection;
		Camera      Eye;
		D3DLIGHT9   Lights[1];
	};

	D3DPRESENT_PARAMETERS        dpp;
	Settings                     settings;

	IDirect3D9*					 pD3D;
	IDirect3DDevice9*			 pDevice;
	IDirect3DVertexDeclaration9* pDecl;

	ITextureManager* textureManager;
	IEffectManager*  effectManager;

	std::vector<MeshInfo> meshes;

	void composeParametersTable(std::map<std::string,Parameter>& parameters);
	void setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param, std::vector<IDirect3DTexture9*>& usedTextures);

public:
	Camera getCamera();
	void setCamera( Camera& camera );

	void clearMeshes();
	void addMesh(const IMesh* mesh, const D3DXMATRIX& transformation, bool enabled = false);
	void enableMesh(unsigned int index, bool enabled);

	bool render();
	void reinitialize( HWND hWnd, int width, int height );

	Engine(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager);
	~Engine();
};

#endif