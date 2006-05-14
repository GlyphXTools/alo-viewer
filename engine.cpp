#include <iostream>
#include "engine.h"
#include "exceptions.h"
using namespace std;

//
// Engine::MeshInfo class
//

// Invalidate the mesh
void Engine::MeshInfo::invalidate()
{
	if (pIndexBuffer != NULL)  pIndexBuffer->Release();
	if (pVertexBuffer != NULL) pVertexBuffer->Release();
	pIndexBuffer  = NULL;
	pVertexBuffer = NULL;
}

// Make sure the vertex buffer is present
void Engine::MeshInfo::checkVertexBuffer()
{
	if (pVertexBuffer == NULL)
	{
		HRESULT hErr;
		if ((hErr = pDevice->CreateVertexBuffer( sizeof(Vertex) * pMesh->getNumVertices(), 0, 0, D3DPOOL_DEFAULT, &pVertexBuffer, NULL)) != D3D_OK)
		{
			throw D3DException(hErr);
		}

		void* data;
		pVertexBuffer->Lock(0, 0, &data, D3DLOCK_DISCARD);
		memcpy(data, pMesh->getVertexBuffer(), pMesh->getNumVertices() * sizeof(Vertex));
		pVertexBuffer->Unlock();
	}
}

// Make sure the index buffer is present
void Engine::MeshInfo::checkIndexBuffer()
{
	if (pIndexBuffer == NULL)
	{
		HRESULT hErr;
		if ((hErr = pDevice->CreateIndexBuffer( sizeof(uint16_t) * 3 * pMesh->getNumTriangles(), 0, D3DFMT_INDEX16, D3DPOOL_DEFAULT, &pIndexBuffer, NULL)) != D3D_OK)
		{
			pVertexBuffer->Release();
			throw D3DException(hErr);
		}

		void* data;
		pIndexBuffer->Lock(0, 0, &data, D3DLOCK_DISCARD);
		memcpy(data, pMesh->getIndexBuffer(), 3 * pMesh->getNumTriangles() * sizeof(uint16_t));
		pIndexBuffer->Unlock();
	}
}

Engine::MeshInfo& Engine::MeshInfo::operator=(const MeshInfo& meshinfo)
{
	enabled        = meshinfo.enabled;
	pMesh          = meshinfo.pMesh;
	pDevice        = meshinfo.pDevice;
	pVertexBuffer  = meshinfo.pVertexBuffer;
	pIndexBuffer   = meshinfo.pIndexBuffer;
	transformation = meshinfo.transformation;
	if (pVertexBuffer != NULL) pVertexBuffer->AddRef();
	if (pIndexBuffer != NULL)  pIndexBuffer->AddRef();
	return *this;
}

Engine::MeshInfo::MeshInfo(const MeshInfo& meshinfo)
{
	enabled        = meshinfo.enabled;
	pMesh          = meshinfo.pMesh;
	pDevice        = meshinfo.pDevice;
	pVertexBuffer  = meshinfo.pVertexBuffer;
	pIndexBuffer   = meshinfo.pIndexBuffer;
	transformation = meshinfo.transformation;
	if (pVertexBuffer != NULL) pVertexBuffer->AddRef();
	if (pIndexBuffer != NULL)  pIndexBuffer->AddRef();
}

Engine::MeshInfo::MeshInfo(IDirect3DDevice9* pDevice, const IMesh* mesh, const D3DXMATRIX& trans, bool enabled)
{
	pMesh          = mesh;
	pVertexBuffer  = NULL;
	pIndexBuffer   = NULL;
	transformation = trans;
	this->pDevice  = pDevice;
	this->enabled  = enabled;
}

Engine::MeshInfo::~MeshInfo()
{
	invalidate();
}

//
// Engine class
//

// Create effect parameters based on current engine settings
void Engine::composeParametersTable(map<string,Parameter>& parameters)
{
	Parameter param;

	param.type = Parameter::MATRIX;
	param.m_matrix = settings.World;
	parameters.insert(make_pair("WORLD", param));

	param.type = Parameter::MATRIX;
	param.m_matrix  = settings.View;
	parameters.insert(make_pair("VIEW", param));

	param.type = Parameter::MATRIX;
	param.m_matrix  = settings.Projection;
	parameters.insert(make_pair("PROJECTION", param));

	param.type = Parameter::MATRIX;
	D3DXMatrixMultiply( &param.m_matrix, &settings.World, &settings.View );
	parameters.insert(make_pair("WORLDVIEW", param));

	Parameter worldViewProj;
	worldViewProj.type = Parameter::MATRIX;
	D3DXMatrixMultiply( &worldViewProj.m_matrix, &param.m_matrix, &settings.Projection );
	parameters.insert(make_pair("WORLDVIEWPROJECTION", worldViewProj));

	param.type = Parameter::MATRIX;
	D3DXMatrixMultiply( &param.m_matrix, &settings.View, &settings.Projection );
	parameters.insert(make_pair("VIEWPROJECTION", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Direction.x;
	param.m_float4[1] = settings.Lights[0].Direction.y;
	param.m_float4[2] = settings.Lights[0].Direction.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("DIR_LIGHT_VEC_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Position.x;
	param.m_float4[1] = settings.Lights[0].Position.y;
	param.m_float4[2] = settings.Lights[0].Position.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("DIR_LIGHT_OBJ_VEC_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Diffuse.r;
	param.m_float4[1] = settings.Lights[0].Diffuse.g;
	param.m_float4[2] = settings.Lights[0].Diffuse.b;
	param.m_float4[3] = settings.Lights[0].Diffuse.a;
	parameters.insert(make_pair("DIR_LIGHT_DIFFUSE_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Specular.r;
	param.m_float4[1] = settings.Lights[0].Specular.g;
	param.m_float4[2] = settings.Lights[0].Specular.b;
	param.m_float4[3] = settings.Lights[0].Specular.a;
	parameters.insert(make_pair("DIR_LIGHT_SPECULAR_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = 1.0f;
	param.m_float4[1] = 1.0f;
	param.m_float4[2] = 1.0f;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("LIGHT_SCALE", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Eye.Position.x;
	param.m_float4[1] = settings.Eye.Position.y;
	param.m_float4[2] = settings.Eye.Position.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("EYE_POSITION", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Eye.Target.x;
	param.m_float4[1] = settings.Eye.Target.y;
	param.m_float4[2] = settings.Eye.Target.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("EYE_OBJ_POSITION", param));

	param.type = Parameter::FLOAT;
	param.m_float = 0.0f;
	parameters.insert(make_pair("TIME", param));
}

// Set the parameter for this effect
void Engine::setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param, vector<IDirect3DTexture9*>& usedTextures)
{
	switch (param.type)
	{
		case Parameter::FLOAT:
			pEffect->SetFloat(hParam, param.m_float);
			break;

		case Parameter::FLOAT4:
			pEffect->SetFloatArray(hParam, param.m_float4, 4);
			break;

		case Parameter::TEXTURE:
		{
			IDirect3DTexture9* pTexture = textureManager->getTexture(pDevice, param.m_texture);
			if (pTexture != NULL)
			{
				usedTextures.push_back(pTexture);
				pEffect->SetTexture(hParam, pTexture);
			}
			break;
		}

		case Parameter::MATRIX:
			pEffect->SetMatrix(hParam, &param.m_matrix);
			break;
	}
}

bool Engine::render()
{
	// See if we can render
	switch (pDevice->TestCooperativeLevel())
	{
		case D3DERR_DEVICELOST:
			return true;

		case D3DERR_DEVICENOTRESET:
			return false;
	}

	// Yes, now clear
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.0f, 0);
	pDevice->BeginScene();

	// Render all the meshes
	for (vector<MeshInfo>::iterator p = meshes.begin(); p != meshes.end(); p++)
	{
		if (!p->isEnabled())
		{
			continue;
		}

		// Set vertex and index buffer
		const IMesh& mesh = p->getMesh();
		pDevice->SetStreamSource(0, p->getVertexBuffer(), 0, sizeof(Vertex));
		pDevice->SetIndices(p->getIndexBuffer());

		// Set world transformation
		settings.World = p->getTransformation();

		map<string,Parameter> parameters;
		composeParametersTable(parameters);

		ID3DXEffect* pEffect = NULL;
		const Effect* effect = mesh.getEffect();
		if (effect != NULL)
		{
			pEffect = effectManager->getEffect( pDevice, effect->name );
		}

		if (pEffect != NULL)
		{
			// We have an effect, so use it
			vector<IDirect3DTexture9*> usedTextures;

			// FIXME: This may not work for all effects; so what determines what technique to use?
			D3DXHANDLE hTechnique = pEffect->GetTechnique(0);
			pEffect->SetTechnique( hTechnique );

			// Set the parameters with semantics from the effects file
			D3DXHANDLE hParam;
			for (int iParam = 0; (hParam = pEffect->GetParameter(NULL, iParam)) != NULL; iParam++)
			{
				if (pEffect->IsParameterUsed(hParam, hTechnique))
				{
					D3DXPARAMETER_DESC desc;
					pEffect->GetParameterDesc( hParam, &desc );
					if (desc.Semantic != NULL)
					{
						map<string,Parameter>::const_iterator p = parameters.find(desc.Semantic);
						if (p != parameters.end())
						{
							setParameter(pEffect, hParam, p->second, usedTextures);
						}
					}
				}
			}

			// Set the parameters from the mesh file
			for (vector<Parameter>::const_iterator p = effect->parameters.begin(); p != effect->parameters.end(); p++)
			{
				D3DXHANDLE hParam = pEffect->GetParameterByName( NULL, p->name.c_str() );
				if (hParam != NULL)
				{
					setParameter(pEffect, hParam, *p, usedTextures );
				}
			}
	
			// Use the effect to render the mesh
			unsigned int nPasses;
			pEffect->Begin(&nPasses, 0);
			for (unsigned int iPass = 0; iPass < nPasses; iPass++)
			{	
				pEffect->BeginPass(iPass);
				pEffect->CommitChanges();
				pDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, mesh.getNumVertices(), 0, mesh.getNumTriangles() );
				pEffect->EndPass();
			}
			pEffect->End();
			pEffect->Release();

			// Release the textures we used
			for (vector<IDirect3DTexture9*>::iterator p = usedTextures.begin(); p != usedTextures.end(); p++)
			{
				(*p)->Release();
			}
		}
	}

	pDevice->EndScene();
	pDevice->Present(NULL, NULL, NULL, NULL);
	return true;
}

// Get the current camera
Engine::Camera Engine::getCamera()
{
	return settings.Eye;
}

// Set the new camera
void Engine::setCamera( Engine::Camera& camera )
{
	settings.Eye = camera;
	// Create view matrix from camera settings
	D3DXMatrixLookAtLH(&settings.View, &settings.Eye.Position, &settings.Eye.Target, &settings.Eye.Up );
}

// Clear the mesh array
void Engine::clearMeshes()
{
	meshes.clear();
}

void Engine::addMesh( const IMesh* mesh, const D3DXMATRIX& transformation, bool enabled )
{
	meshes.push_back( MeshInfo(pDevice, mesh, transformation, enabled) );
}

// Enable/disable a mesh
void Engine::enableMesh(unsigned int i, bool enabled)
{
	if (i >= 0 && i < meshes.size())
	{
		meshes[i].enable( enabled );
	}
}

void Engine::reinitialize( HWND hWnd, int width, int height )
{
	//
	// Cleanup
	//
	pDecl->Release();
	for (vector<MeshInfo>::iterator i = meshes.begin(); i != meshes.end(); i++)
	{
		i->invalidate();
	}

	//
	// Reset device
	//
	dpp.BackBufferWidth  = 0;
    dpp.BackBufferHeight = 0;
	dpp.BackBufferCount  = 1;
    dpp.hDeviceWindow    = hWnd;
    dpp.Windowed         = true;

	HRESULT hErr;
	if ((hErr = pDevice->Reset( &dpp )) != D3D_OK)
	{
		throw D3DException( hErr );
	}

	//
	// Reset parameters
	//
	D3DVERTEXELEMENT9 elements[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END(),
	};

	if ((hErr = pDevice->CreateVertexDeclaration( elements, &pDecl )) != D3D_OK)
	{
		throw D3DException( hErr );
	}
	pDevice->SetVertexDeclaration( pDecl );

	pDevice->SetRenderState(D3DRS_ZENABLE,  TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	if (width > 0 && height > 0)
	{
		D3DXMatrixPerspectiveFovLH(&settings.Projection, D3DXToRadian(45), (float)width / height, 1.0f, 3000.0f );
	}
}

Engine::Engine(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager)
{
	HRESULT hErr;

	this->textureManager = textureManager;
	this->effectManager  = effectManager;

	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
        throw D3DException(D3D_OK);
	}

	//
	// Find highest possible Anti-Alias level
	//
	D3DDISPLAYMODE mode;
	if ((hErr = pD3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &mode)) != D3D_OK)
	{
		pD3D->Release();
		throw D3DException( hErr );
	}

	DWORD multisampleType;
	DWORD nQualityLevels;
	for (multisampleType = 16; multisampleType > 1; multisampleType--)
	{
		if ((hErr = pD3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, mode.Format, TRUE, (D3DMULTISAMPLE_TYPE)multisampleType, &nQualityLevels)) == D3D_OK)
		{
			break;
		}
		if (hErr != D3DERR_NOTAVAILABLE)
		{
			pD3D->Release();
			throw D3DException( hErr );
		}
	}

	if (multisampleType == 1)
	{
		multisampleType = D3DMULTISAMPLE_NONE;
	}
	nQualityLevels = max(1, nQualityLevels);

	//
	// Check if we can use hardware T&L
	//
	D3DCAPS9 caps;
	if ((hErr = pD3D->GetDeviceCaps(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, &caps)) != D3D_OK)
	{
		pD3D->Release();
		throw D3DException( hErr );
	}

	DWORD BehaviorFlags = (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?  D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	//
	// Create the device
	//
	ZeroMemory(&dpp, sizeof dpp);
	dpp.BackBufferWidth    = 0;
	dpp.BackBufferHeight   = 0;
    dpp.BackBufferFormat   = D3DFMT_UNKNOWN;
    dpp.BackBufferCount    = 1;
    dpp.MultiSampleType    = (D3DMULTISAMPLE_TYPE)multisampleType;
	dpp.MultiSampleQuality = max(0, nQualityLevels - 1);
    dpp.SwapEffect         = D3DSWAPEFFECT_DISCARD;
    dpp.hDeviceWindow      = hWnd;
    dpp.Windowed           = TRUE;
    dpp.EnableAutoDepthStencil = TRUE;
    dpp.AutoDepthStencilFormat = D3DFMT_D24S8;
    dpp.Flags                  = 0;
    dpp.FullScreen_RefreshRateInHz = 0;
    dpp.PresentationInterval       = D3DPRESENT_INTERVAL_DEFAULT;

	if ((hErr = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, BehaviorFlags, &dpp, &pDevice)) != D3D_OK)
	{
		pD3D->Release();
		throw D3DException( hErr );
	}

	//
	// Set the vertex declaration
	//
	D3DVERTEXELEMENT9 elements[] =
	{
		{0,  0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		{0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL,   0},
		{0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		D3DDECL_END(),
	};

	if ((hErr = pDevice->CreateVertexDeclaration( elements, &pDecl )) != D3D_OK)
	{
		pDevice->Release();
		pD3D->Release();
		throw D3DException( hErr );
	}
	pDevice->SetVertexDeclaration( pDecl );

	pDevice->SetRenderState(D3DRS_ZENABLE,  TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);

	// Initialize projection matrix with perspective
	RECT client;
	GetClientRect( hWnd, &client );
	D3DXMatrixPerspectiveFovLH(&settings.Projection, D3DXToRadian(45), (float)client.right / client.bottom, 1.0f, 3000.0f );

	// Initialize camera
	Camera camera = {
		D3DXVECTOR3(-500,-500,500),
		D3DXVECTOR3(0,0,0),
		D3DXVECTOR3(0,0,1)
	};
	setCamera( camera );

	// Create light
	D3DLIGHT9 light = 
	{
		D3DLIGHT_DIRECTIONAL,
		{  0.6f,  0.6f,   0.6f, 1.0f},	// Diffuse
		{  0.0f,  0.0f,   0.0f, 0.0f},	// Specular
		{  0.1f,  0.1f,   0.1f, 1.0f},	// Ambient
		{500.0f,  0.0f, 500.0f},		// Position
		{ -1.0f,  0.0f,  -1.0f},		// Direction
	};
	settings.Lights[0] = light;

}

Engine::~Engine()
{
	pDecl->Release();
	pDevice->Release();
	pD3D->Release();
}