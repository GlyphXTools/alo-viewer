#include <iostream>
#include <set>
#include "engine.h"
#include "exceptions.h"
using namespace std;

class Engine::EngineImpl
{
	friend class Engine;

	class AnimatedModel
	{
		D3DXMATRIX*  Matrices;
		unsigned int numFrames;

	public:
		const D3DXMATRIX& getAnimatedBone(unsigned int bone, unsigned int frame) const
		{
			return Matrices[bone * numFrames + frame];
		}

		AnimatedModel(const Animation* anim, const Model& model);
		~AnimatedModel();
	};

	struct D3DXMaterial
	{
		ID3DXEffect*		  pEffect;
		ID3DXMesh*            pMesh;
		vector<unsigned long> boneMapping;

		D3DXMaterial()
		{
			pMesh   = NULL;
			pEffect = NULL;
		}

		D3DXMaterial& operator =(const D3DXMaterial& mat)
		{
			if (pMesh != NULL) pMesh->Release();
			pMesh       = mat.pMesh;
			pEffect     = mat.pEffect;
			boneMapping = mat.boneMapping;
			if (pMesh != NULL) pMesh->AddRef();
			return *this;
		}

		D3DXMaterial(const D3DXMaterial& mat)
		{
			pMesh       = mat.pMesh;
			pEffect     = mat.pEffect;
			boneMapping = mat.boneMapping;
			if (pMesh != NULL) pMesh->AddRef();
		}

		~D3DXMaterial()
		{
			if (pMesh != NULL) pMesh->Release();
		}
	};

	class MeshInfo
	{
		bool				 enabled;
		const IMesh*         pMesh;
		IDirect3DDevice9*    pDevice;
		const Model*         pModel;
		IEffectManager*      effectManager;
		vector<D3DXMaterial> materials;

		void checkMaterial(int i);

	public: 
		bool                isEnabled() const         { return enabled; }
		void                enable(bool enabled)      { this->enabled = enabled; }
		unsigned int        getNumMaterials()         { return (unsigned int)materials.size(); }
		const D3DXMaterial* getMaterial(int i)        { checkMaterial(i); return &materials[i]; }
		const IMesh*        getIMesh() const          { return pMesh; }

		void invalidate();

		MeshInfo(IEffectManager* effectManager, IDirect3DDevice9* pDevice, const Model* model, const IMesh* mesh, bool enabled = false);
		~MeshInfo();
	};

	struct Settings
	{
		D3DXMATRIX  World;
		D3DXMATRIX  View;
		D3DXMATRIX  Projection;
		Camera      Eye;
		RenderMode  renderMode;
		D3DLIGHT9   Lights[1];
	};

	D3DPRESENT_PARAMETERS        dpp;
	Settings                     settings;

	IDirect3D9*					 pD3D;
	IDirect3DDevice9*			 pDevice;

	ITextureManager* textureManager;
	IEffectManager*  effectManager;

	Model*         model;
	AnimatedModel* animatedModel;

	std::vector<MeshInfo> meshes;

	void composeParametersTable(std::map<std::string,Parameter>& parameters);
	void setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param);
	void setCamera( const Camera& camera );

	bool render(unsigned int frame, const RENDERINFO& ri);
	void reinitialize( HWND hWnd, int width, int height );

	EngineImpl(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager);
	~EngineImpl();
};

Engine::EngineImpl::AnimatedModel::AnimatedModel(const Animation* anim, const Model& model)
{
	unsigned int numBones     = model.getNumBones();
	unsigned int numAnimBones = (anim == NULL) ? 0 : anim->getNumBoneAnimations();
				 numFrames    = (anim == NULL) ? 1 : anim->getNumFrames();

	Matrices = new D3DXMATRIX[numBones * numFrames];

	// Keep track of what bones were animated
	set<unsigned int> animatedBones;

	for (unsigned int i = 0; i < numAnimBones; i++)
	{
		const BoneAnimation& ba = anim->getBoneAnimation(i);
		unsigned int iBone = ba.getBoneIndex();
		for (unsigned int frame = 0; frame < numFrames; frame++)
		{
			D3DXMATRIX& transform = Matrices[iBone * numFrames + frame];
			D3DXMATRIX  translation;
			D3DXVECTOR3 trans = ba.getTranslationOffset();

			D3DXMatrixRotationQuaternion(&transform, &ba.getQuaternion( (ba.getNumQuaternions() > 1) ? frame : 0 ));
			if (ba.getNumTranslations() > 0)
			{
				trans += ba.getTranslation(frame);
			}
			D3DXMatrixTranslation(&translation, trans.x, trans.y, trans.z);
			D3DXMatrixMultiply(&transform, &transform, &translation);
		}
		animatedBones.insert( iBone );
	}

	for (unsigned int i = 0; i < numBones; i++)
	{
		// Fill everything else that isn't animated with default bone values
		if (animatedBones.find(i) == animatedBones.end())
		{
			const D3DXMATRIX& transform = model.getBone(i)->matrix;
			for (unsigned int frame = 0; frame < numFrames; frame++)
			{
				Matrices[i * numFrames + frame] = transform;
			}
		}

		// Multiply with parent matrix to create absolute matrix
		unsigned int parent = model.getBone(i)->parent;
		if (parent != -1)
		{
			for (unsigned int frame = 0; frame < numFrames; frame++)
			{
				D3DXMatrixMultiply(&Matrices[i * numFrames + frame], &Matrices[i * numFrames + frame], &Matrices[parent * numFrames + frame] );
			}
		}

	}
}

Engine::EngineImpl::AnimatedModel::~AnimatedModel()
{
	delete[] Matrices;
}

//
// Engine::EngineImpl::MeshInfo class
//

// Invalidate the mesh
void Engine::EngineImpl::MeshInfo::invalidate()
{
	for (size_t i = 0; i < materials.size(); i++)
	{
		materials[i].pEffect = NULL;
		if (materials[i].pMesh != NULL)
		{
			materials[i].pMesh->Release();
			materials[i].pMesh = NULL;
		}
	}
}

void Engine::EngineImpl::MeshInfo::checkMaterial(int i)
{
	const Material& material = pMesh->getMaterial(i);

	if (materials[i].pEffect == NULL)
	{
		// Recreate effect
		materials[i].pEffect = effectManager->getEffect( pDevice, material.effect.name );
		if (materials[i].pEffect != NULL)
		{
			ID3DXEffect* pEffect  = materials[i].pEffect;
			D3DXHANDLE hTechnique = NULL;
			D3DXEFFECT_DESC desc;
			pEffect->GetDesc(&desc);

			const char* LODs[] = {"DX9", "DX8", "DX8ATI", "FIXEDFUNCTION"};
			for (unsigned int lod = 0; lod < 4 && hTechnique == NULL; lod++)
			{
				for (unsigned int i = 0; i < desc.Techniques && hTechnique == NULL; i++)
				{
					D3DXTECHNIQUE_DESC tdesc;
					D3DXHANDLE hTech = pEffect->GetTechnique(i);
					pEffect->GetTechniqueDesc(hTech, &tdesc );
					for (unsigned int j = 0; j < tdesc.Annotations; j++)
					{
						D3DXPARAMETER_DESC adesc;
						D3DXHANDLE hAnn = pEffect->GetAnnotation(hTech, j);
						pEffect->GetParameterDesc(hAnn, &adesc);
						if (strcmp(adesc.Name,"LOD") == 0)
						{
							const char* value;
							pEffect->GetString(hAnn, &value);
							if (strcmp(value, LODs[lod]) == 0)
							{
								hTechnique = hTech;
								break;
							}
						}
					}
				}
			}

			pEffect->SetTechnique( hTechnique );
		}
	}

	if (materials[i].pMesh == NULL && material.nVertices > 0 && material.nTriangles > 0)
	{
		// Recreate mesh
		D3DVERTEXELEMENT9 elements[] = {
			{0,   0, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_POSITION, 0},
			{0,  12, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_NORMAL  , 0},
			{0,  28, D3DDECLTYPE_FLOAT2,   0, D3DDECLUSAGE_TEXCOORD, 0},
			{0,  36, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_TANGENT,  0},
			{0,  48, D3DDECLTYPE_FLOAT3,   0, D3DDECLUSAGE_BINORMAL, 0},
			D3DDECL_END()
		};

		#pragma pack(1)
		struct RSkinVertex
		{
			D3DXVECTOR3 Position;
			D3DXVECTOR4 Normal;
			D3DXVECTOR2 TexCoord;
			D3DXVECTOR3 Tangent;
			D3DXVECTOR3 Binormal;
		};
		#pragma pack()

		const string& format = material.vertexFormat;

		HRESULT hErr;
		if (FAILED(hErr = D3DXCreateMesh(material.nTriangles, material.nVertices, D3DXMESH_WRITEONLY, elements, pDevice, &materials[i].pMesh)))
		{
			throw D3DException(hErr);
		}

		// FIXME: This is a hack, figure out how it *should* be done
		float texScale = (format == "alD3dVertRSkinNU2" || format == "alD3dVertB4I4NU2") ? 4096.0f : 1.0f;

		RSkinVertex* data;
		materials[i].pMesh->LockVertexBuffer(D3DLOCK_DISCARD, (void**)&data);
		const Vertex* v = material.vertices;
		for (unsigned int j = 0; j < material.nVertices; j++, data++, v++ )
		{
			data->Position = v->Position;
			data->Normal   = D3DXVECTOR4(v->Normal.x, v->Normal.y, v->Normal.z, (float)v->BoneIndices[0] );
			data->TexCoord = v->TexCoord * texScale;
			data->Tangent  = v->Tangent;
			data->Binormal = v->Binormal;
		}
		materials[i].pMesh->UnlockVertexBuffer();

		materials[i].pMesh->LockIndexBuffer(D3DLOCK_DISCARD, (void**)&data);
		memcpy( data, material.indices, material.nTriangles * 3 * sizeof(uint16_t) );
		materials[i].pMesh->UnlockIndexBuffer();
	}
}

Engine::EngineImpl::MeshInfo::MeshInfo(IEffectManager* effectManager, IDirect3DDevice9* pDevice, const Model* model, const IMesh* mesh, bool enabled)
{
	pMesh            = mesh;
	this->pModel     = model;
	this->pDevice    = pDevice;
	this->enabled    = enabled;
	this->effectManager = effectManager;

	unsigned int nMaterials = mesh->getNumMaterials();
	materials.resize(nMaterials);
	for (unsigned int i = 0; i < nMaterials; i++)
	{
		materials[i].boneMapping = mesh->getMaterial(i).boneMapping;
	}
}

Engine::EngineImpl::MeshInfo::~MeshInfo()
{
	invalidate();
}

//
// Engine class
//

// Create effect parameters based on current engine settings
void Engine::EngineImpl::composeParametersTable(map<string,Parameter>& parameters)
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
	param.m_float4[0] = settings.Lights[0].Position.x;
	param.m_float4[1] = settings.Lights[0].Position.y;
	param.m_float4[2] = settings.Lights[0].Position.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("DIR_LIGHT_VEC_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Position.x;
	param.m_float4[1] = settings.Lights[0].Position.y;
	param.m_float4[2] = settings.Lights[0].Position.z;
	param.m_float4[3] = 1.0f;
	parameters.insert(make_pair("DIR_LIGHT_OBJ_VEC_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Specular.r;
	param.m_float4[1] = settings.Lights[0].Specular.g;
	param.m_float4[2] = settings.Lights[0].Specular.b;
	param.m_float4[3] = settings.Lights[0].Specular.a;
	parameters.insert(make_pair("DIR_LIGHT_SPECULAR_0", param));

	param.type = Parameter::FLOAT4;
	param.m_float4[0] = settings.Lights[0].Diffuse.r;
	param.m_float4[1] = settings.Lights[0].Diffuse.g;
	param.m_float4[2] = settings.Lights[0].Diffuse.b;
	param.m_float4[3] = settings.Lights[0].Diffuse.a;
	parameters.insert(make_pair("DIR_LIGHT_DIFFUSE_0", param));

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
	param.m_float = GetTickCount() / 1000.0f;
	parameters.insert(make_pair("TIME", param));
}

// Set the parameter for this effect
void Engine::EngineImpl::setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param)
{
	switch (param.type)
	{
		case Parameter::INT:
			pEffect->SetInt(hParam, param.m_int);
			break;

		case Parameter::FLOAT:
			pEffect->SetFloat(hParam, param.m_float);
			break;

		case Parameter::FLOAT3:
			pEffect->SetFloatArray(hParam, param.m_float3, 3);
			break;

		case Parameter::FLOAT4:
			pEffect->SetFloatArray(hParam, param.m_float4, 4);
			break;

		case Parameter::TEXTURE:
		{
			IDirect3DTexture9* pTexture = textureManager->getTexture(pDevice, param.m_texture);
			if (pTexture != NULL)
			{
				pEffect->SetTexture(hParam, pTexture);
			}
			break;
		}

		case Parameter::MATRIX:
			pEffect->SetMatrix(hParam, &param.m_matrix);
			break;
	}
}

void Engine::EngineImpl::setCamera( const Camera& camera )
{
	settings.Eye = camera;
	D3DXMatrixLookAtRH(&settings.View, &camera.Position, &camera.Target, &camera.Up );
}

bool Engine::EngineImpl::render(unsigned int frame, const RENDERINFO& ri)
{
	// See if we can render
	switch (pDevice->TestCooperativeLevel())
	{
		case D3DERR_DEVICELOST:
			return true;

		case D3DERR_DEVICENOTRESET:
			return false;
	}

	// Prepare SPH_LIGHT_FILL matrices
	D3DXMATRIX SPH_Light_Fill[3];
	memset(SPH_Light_Fill, 0, sizeof(D3DXMATRIX)*3);
	SPH_Light_Fill[0]._44 = settings.Lights[0].Ambient.r;
	SPH_Light_Fill[1]._44 = settings.Lights[0].Ambient.g;
	SPH_Light_Fill[2]._44 = settings.Lights[0].Ambient.b;

	// Prepare SPH_LIGHT_ALL matrices
	D3DXMATRIX SPH_Light_All[3];
	memset(SPH_Light_All, 0, sizeof(D3DXMATRIX)*3);
	SPH_Light_All[0]._44 = settings.Lights[0].Ambient.r;
	SPH_Light_All[1]._44 = settings.Lights[0].Ambient.g;
	SPH_Light_All[2]._44 = settings.Lights[0].Ambient.b;

	// Yes, now clear
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0, 1.0f, 0);
	pDevice->BeginScene();

	DWORD fillMode = (settings.renderMode == RM_WIREFRAME) ? D3DFILL_WIREFRAME : D3DFILL_SOLID;

	// Render all the meshes
	for (int i = (int)meshes.size() - 1; i >= 0 ; i--)
	{
		MeshInfo& meshinfo = meshes[i];
		unsigned int numMaterials = meshinfo.getNumMaterials();
		if (!meshinfo.isEnabled() || numMaterials == 0)
		{
			continue;
		}

		pDevice->SetRenderState(D3DRS_CULLMODE, meshinfo.getIMesh()->getCulling()  );
		pDevice->SetRenderState(D3DRS_FILLMODE, meshinfo.getIMesh()->getFillMode() );

		// Set world transformation
		settings.World = animatedModel->getAnimatedBone( model->getConnection(i), frame);

		for (unsigned int m = 0; m < numMaterials; m++)
		{
			map<string,Parameter> parameters;
			composeParametersTable(parameters);

			const D3DXMaterial* material = meshinfo.getMaterial(m);
			if (material->pEffect != NULL)
			{
				// We have an effect, so use it
				D3DXEFFECT_DESC effdesc;
				ID3DXEffect* pEffect = material->pEffect;
				pEffect->GetDesc(&effdesc);

				D3DXMATRIX skinArray[24];
				skinArray[0] = settings.World;

				// Set the parameters with semantics from the effects file
				for (unsigned int iParam = 0; iParam < effdesc.Parameters; iParam++)
				{
					D3DXHANDLE hParam = pEffect->GetParameter(NULL, iParam);
					D3DXPARAMETER_DESC desc;
					pEffect->GetParameterDesc( hParam, &desc );
					if (desc.Semantic != NULL)
					{
						if (strcmp(desc.Semantic,"SKINMATRIXARRAY") == 0)
						{
							const IMesh* mesh = meshinfo.getIMesh();
							unsigned int numMappings = (unsigned int)material->boneMapping.size();
							for (unsigned long m = 0; m < numMappings; m++)
							{
								unsigned long bone = material->boneMapping[m];
								
								D3DXMATRIX transform;
								model->getBoneTransformation(bone, transform);
								D3DXMatrixInverse(&transform, NULL, &transform);

								skinArray[m] = animatedModel->getAnimatedBone(bone, frame);
								D3DXMatrixMultiply(&skinArray[m], &transform, &skinArray[m]);
							}

							pEffect->SetMatrixArray(hParam, skinArray, max(1,numMappings));
						}
						else if (strcmp(desc.Semantic,"SPH_LIGHT_FILL") == 0)
						{
							pEffect->SetMatrixArray(hParam, SPH_Light_Fill, 3);
						}
						else if (strcmp(desc.Semantic,"SPH_LIGHT_ALL") == 0)
						{
							pEffect->SetMatrixArray(hParam, SPH_Light_All, 3);
						}
						else
						{
							map<string,Parameter>::const_iterator p = parameters.find(desc.Semantic);
							if (p != parameters.end())
							{
								setParameter(pEffect, hParam, p->second);
							}
						}
					}

					if (desc.Name != NULL && ri.useColor && strcmp(desc.Name, "Colorization") == 0)
					{
						float color[4] = { GetRValue(ri.color) / 255.0f, GetGValue(ri.color) / 255.0f, GetBValue(ri.color) / 255.0f, 1.0f};
						pEffect->SetFloatArray(hParam, color, 4);
					}
				}

				// Set the parameters from the mesh file
				const Effect& effect = meshinfo.getIMesh()->getMaterial(m).effect;
				for (vector<Parameter>::const_iterator p = effect.parameters.begin(); p != effect.parameters.end(); p++)
				{
					if (p->name != "Colorization" || !ri.useColor)
					{
						D3DXHANDLE hParam = pEffect->GetParameterByName( NULL, p->name.c_str() );
						if (hParam != NULL)
						{
							setParameter(pEffect, hParam, *p);
						}
					}
				}
		
				// Use the effect to render the mesh
				unsigned int nPasses;
				pEffect->Begin(&nPasses, 0);
				for (unsigned int iPass = 0; iPass < nPasses; iPass++)
				{	
					pEffect->BeginPass(iPass);
					pEffect->CommitChanges();
					pDevice->SetRenderState(D3DRS_FILLMODE, fillMode);
					material->pMesh->DrawSubset(0);
					pEffect->EndPass();
				}
				pEffect->End();
			}
		}
	}

	if (ri.showBones && model != NULL)
	{
		//
		// Render the model's bones
		//
		struct BoneVertex
		{
				D3DXVECTOR3 pos;
				D3DCOLOR    color;
		};

		unsigned int numBones = model->getNumBones();
		BoneVertex* bonePoints = new BoneVertex[numBones];
		BoneVertex* boneLines  = new BoneVertex[numBones*2];

		// Create bone array
		for (unsigned int i = 0; i < numBones; i++)
		{
			D3DXMATRIX transform;

			// Create transformation matrix for this joint
			transform = animatedModel->getAnimatedBone(i, frame);

			D3DXVec3TransformCoord(&bonePoints[i].pos, &D3DXVECTOR3(0,0,0), &transform);
			bonePoints[i].color = D3DCOLOR_XRGB(255,255,0);

			boneLines[2*i+0].pos   = bonePoints[i].pos;
			boneLines[2*i+0].color = D3DCOLOR_XRGB(255,255,0);
			const Bone* bone = model->getBone(i);
			if (bone->parent != -1)
			{
				// Create transformation matrix for parent joint
				transform = animatedModel->getAnimatedBone(bone->parent, frame);
			}
			D3DXVec3TransformCoord(&boneLines[2*i+1].pos, &D3DXVECTOR3(0,0,0), &transform);
			boneLines[2*i+1].color = D3DCOLOR_XRGB(255,255,0);
		}

		// Render bones
		float PointSize = 4.0;
		pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
		pDevice->SetRenderState(D3DRS_POINTSIZE, *(DWORD*)&PointSize);
		D3DXMATRIX identity;
		D3DXMatrixIdentity(&identity);
		pDevice->SetTransform(D3DTS_WORLD, &identity);
		pDevice->SetTransform(D3DTS_VIEW, &settings.View);
		pDevice->SetTransform(D3DTS_PROJECTION, &settings.Projection);

		pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, numBones, bonePoints, sizeof(BoneVertex));
		pDevice->DrawPrimitiveUP(D3DPT_LINELIST,  numBones, boneLines,  sizeof(BoneVertex));

		delete[] boneLines;
		delete[] bonePoints;
	}
	pDevice->EndScene();
	pDevice->Present(NULL, NULL, NULL, NULL);

	if (ri.showBones && ri.showBoneNames && model != NULL)
	{
		// Show bone names
		HDC hDC = GetDC(dpp.hDeviceWindow);
		SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
		SetBkMode(hDC, TRANSPARENT);
		SetTextColor(hDC, RGB(255,255,0));
		SetTextAlign(hDC, TA_BASELINE | TA_CENTER);

		D3DVIEWPORT9 viewport;
		pDevice->GetViewport(&viewport);

		D3DXMATRIX identity;
		D3DXMatrixIdentity(&identity);

		unsigned int numBones = model->getNumBones();
		for (unsigned int i = 0; i < numBones; i++)
		{
			const Bone* bone = model->getBone(i);

			// Get 3D bone position
			D3DXMATRIX  transform;
			D3DXVECTOR3 position;
			transform = animatedModel->getAnimatedBone(i, frame);
			D3DXVec3TransformCoord(&position, &D3DXVECTOR3(0,0,0), &transform);

			// Project unto screen
			D3DXVec3Project(&position, &position, &viewport, &settings.Projection, &settings.View, &identity );

			if (position.z > 0 && position.z < 1)
			{
				int x = (int)position.x;
				int y = (int)position.y;

				// Draw bone name
				TextOut(hDC, x, y - 5, bone->name.c_str(), (int)bone->name.length() );
			}
		}

		ReleaseDC(dpp.hDeviceWindow, hDC);
	}

	return true;
}

void Engine::EngineImpl::reinitialize( HWND hWnd, int width, int height )
{
	//
	// Cleanup
	//
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
	pDevice->SetRenderState(D3DRS_ZENABLE,  TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	if (width > 0 && height > 0)
	{
		D3DXMatrixPerspectiveFovRH(&settings.Projection, D3DXToRadian(45), (float)width / height, 1.0f, 10000.0f );
	}
}

Engine::EngineImpl::EngineImpl(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager)
{
	HRESULT hErr;

	this->textureManager = textureManager;
	this->effectManager  = effectManager;
	this->model          = NULL;
	this->settings.renderMode = RM_SOLID;

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

	pDevice->SetRenderState(D3DRS_ZENABLE,  TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// Initialize projection matrix with perspective
	RECT client;
	GetClientRect( hWnd, &client );
	D3DXMatrixPerspectiveFovRH(&settings.Projection, D3DXToRadian(45), (float)client.right / client.bottom, 1.0f, 10000.0f );

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
		{   0.5f,  0.5f,  0.5f,  1.0f},	// Diffuse
		{   0.0f,  0.0f,  0.0f,  1.0f},	// Specular
		{   0.25f, 0.25f, 0.25f, 1.0f},	// Ambient
		{1500.0f,  0.0f,  1000.0f},		// Position
		{  -0.83f, 0.0f, -0.55f},		// Direction
	};
	settings.Lights[0] = light;
}

Engine::EngineImpl::~EngineImpl()
{
	pDevice->Release();
	pD3D->Release();
}

// Get the current camera
RenderMode Engine::getRenderMode() const
{
	return pimpl->settings.renderMode;
}

void Engine::setRenderMode(RenderMode mode)
{
	pimpl->settings.renderMode = mode;
}

const Camera& Engine::getCamera() const
{
	return pimpl->settings.Eye;
}

void Engine::setCamera( const Camera& camera )
{
	pimpl->setCamera( camera );
}

void Engine::enableMesh(unsigned int i, bool enabled)
{
	if (i >= 0 && i < pimpl->meshes.size())
	{
		pimpl->meshes[i].enable( enabled );
	}
}

Model* Engine::getModel() const
{
	return pimpl->model;
}

void Engine::setModel(Model* model)
{
	delete pimpl->animatedModel;

	pimpl->model         = model;
	pimpl->animatedModel = new EngineImpl::AnimatedModel(NULL, *model);

	pimpl->meshes.clear();
	for (unsigned int i = 0; i < model->getNumMeshes(); i++)
	{
		pimpl->meshes.push_back( EngineImpl::MeshInfo(pimpl->effectManager, pimpl->pDevice, model, (const IMesh*)model->getMesh(i)) );
	}
}

void Engine::applyAnimation(const Animation* animation)
{
	delete pimpl->animatedModel;
	pimpl->animatedModel = new EngineImpl::AnimatedModel(animation, *pimpl->model);
}

bool Engine::render(unsigned int frame, const RENDERINFO& ri)
{
	return pimpl->render(frame, ri);
}

void Engine::reinitialize( HWND hWnd, int width, int height )
{
	pimpl->reinitialize(hWnd, width, height);
}

Engine::Engine(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager)
	: pimpl( new EngineImpl(hWnd, textureManager, effectManager ))
{
	pimpl->animatedModel = NULL;
}

Engine::~Engine()
{
	delete pimpl->animatedModel;
	delete pimpl;
}