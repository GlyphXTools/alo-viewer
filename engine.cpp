#include <iostream>
#include <set>
#include <list>
#include "log.h"
#include "engine.h"
#include "exceptions.h"
#include "SphericalHarmonics.h"
using namespace std;

class Engine::EngineImpl
{
	friend class Engine;

	typedef int RenderPhase;
	static const RenderPhase PHASE_TERRAIN_MESH	= 0;
	static const RenderPhase PHASE_OPAQUE		= 1;
	static const RenderPhase PHASE_TRANSPARENT	= 2;
	static const RenderPhase PHASE_OCCLUDED		= 3;
	static const RenderPhase PHASE_HEAT			= 4;
	static const RenderPhase PHASE_SHADOW		= 5;
	static const RenderPhase NUM_PHASES			= 6;

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

	class MeshInfo;

	struct D3DXMaterial
	{
		D3DXMaterial*		  next;

		ID3DXEffect*		  pEffect;
		int					  index;		// Index in the mesh
		int					  iMeshInfo;	// Index of MeshInfo
		RenderPhase			  phase;
		bool				  zSort;
		ID3DXMesh*            pMesh;
		vector<unsigned long> boneMapping;

		D3DXMaterial()
		{
			pMesh     = NULL;
			pEffect   = NULL;
		}

		D3DXMaterial& operator =(const D3DXMaterial& mat)
		{
			if (pMesh != NULL) pMesh->Release();
			pMesh       = mat.pMesh;
			index       = mat.index;
			iMeshInfo   = mat.iMeshInfo;
			pEffect     = mat.pEffect;
			phase       = mat.phase;
			zSort       = mat.zSort;
			boneMapping = mat.boneMapping;
			if (pMesh != NULL) pMesh->AddRef();
			return *this;
		}

		D3DXMaterial(const D3DXMaterial& mat)
		{
			pMesh = NULL;
			*this = mat;
		}

		~D3DXMaterial()
		{
			if (pMesh != NULL) pMesh->Release();
		}
	};

	class MeshInfo
	{
		bool				 enabled;
		int					 index;		// Index in the model
		bool				 fixedFunction;
		const IMesh*         pMesh;
		IDirect3DDevice9*    pDevice;
		const Model*         pModel;
		IEffectManager*      effectManager;
		vector<D3DXMaterial> materials;

		void checkMaterial(int i);

	public: 
		int			  getIndex() const	   { return index; }
		bool          isEnabled() const    { return enabled; }
		void          enable(bool enabled) { this->enabled = enabled; }
		unsigned int  getNumMaterials()    { return (unsigned int)materials.size(); }
		D3DXMaterial* getMaterial(int i)   { checkMaterial(i); return &materials[i]; }
		const IMesh*  getIMesh() const     { return pMesh; }

		void invalidate();

		MeshInfo& operator =(const MeshInfo& meshinfo);
		MeshInfo(const MeshInfo& meshinfo);
		MeshInfo(int index, IEffectManager* effectManager, IDirect3DDevice9* pDevice, const Model* model, const IMesh* mesh, bool enabled = false);
		~MeshInfo();
	};

	struct Settings
	{
		D3DXMATRIX	View;
		D3DXMATRIX	Projection;
		D3DXMATRIX  ViewProjection;
		Camera		Eye;
		RenderMode	renderMode;
		bool		isGroundVisible;
		
		LIGHT		Lights[3];
		COLORREF	Background;
		D3DXVECTOR3 Wind;
		D3DXVECTOR4 Ambient;
		D3DXVECTOR4 Shadow;
		D3DXMATRIX  SPH_Light_Fill[3];
		D3DXMATRIX  SPH_Light_All [3];
	};

	D3DPRESENT_PARAMETERS        PresentationParameters;
	Settings                     settings;

	IDirect3D9*					 pD3D;
	IDirect3DDevice9*			 pDevice;

	ITextureManager* textureManager;
	IEffectManager*  effectManager;

	Model*         model;
	AnimatedModel* animatedModel;

	std::vector<MeshInfo> meshes;

	void setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param);
	void setCamera( const Camera& camera );
	void doBillboard(const Bone* pBone, D3DXMATRIX& world, D3DXMATRIX& viewInv);

	bool render(unsigned int frame, const RENDERINFO& ri);
	void reinitialize( HWND hWnd, int width, int height );

	D3DFORMAT GetDepthStencilFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat);
	void	  GetMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT DisplayFormat);

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

			if (ba.getNumQuaternions() > 0)
			{
				D3DXMatrixRotationQuaternion(&transform, &ba.getQuaternion( (ba.getNumQuaternions() > 1) ? frame : 0 ));
			}
			else
			{
				D3DXMatrixIdentity(&transform);
			}

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

			// Read some properties (phase & Z-Sort)
			LPCSTR strPhase;
			BOOL  zSort;
			pEffect->GetString("_ALAMO_RENDER_PHASE", &strPhase);
			pEffect->GetBool("_ALAMO_Z_SORT", &zSort);
			static const char* Phases[NUM_PHASES] = {"TerrainMesh", "Opaque", "Transparent", "Occluded", "Heat", "Shadow"};
			int phase;
			for (phase = 0; phase < NUM_PHASES && _stricmp(strPhase, Phases[phase]) != 0; phase++);
			materials[i].phase = (phase < NUM_PHASES) ? phase : 0;
			materials[i].zSort = (zSort != FALSE);

			// Read techniques
			const char* LODs[] = {"DX9", "DX8", "DX8ATI", "FIXEDFUNCTION"};
			for (unsigned int lod = 0; lod < 4 && hTechnique == NULL; lod++)
			{
				for (unsigned int i = 0; i < desc.Techniques && hTechnique == NULL; i++)
				{
					D3DXTECHNIQUE_DESC tdesc;
					D3DXHANDLE hTech = pEffect->GetTechnique(i);
					if (SUCCEEDED(pEffect->ValidateTechnique(hTech)))
					{
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
			{0,  60, D3DDECLTYPE_FLOAT4,   0, D3DDECLUSAGE_COLOR   , 0},
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
			D3DXVECTOR4 Color;
		};
		#pragma pack()

		HRESULT hErr;
		if (FAILED(hErr = D3DXCreateMesh(material.nTriangles, material.nVertices, D3DXMESH_WRITEONLY, elements, pDevice, &materials[i].pMesh)))
		{
			throw D3DException(hErr);
		}

		// FIXME: This is a hack, figure out how it *should* be done
		const string& format = material.vertexFormat;
		float texScale = (format == "alD3dVertRSkinNU2" || format == "alD3dVertB4I4NU2") ? 4096.0f : 1.0f;

		RSkinVertex* data;
		materials[i].pMesh->LockVertexBuffer(D3DLOCK_DISCARD, (void**)&data);
		const Vertex* v = material.vertices;
		for (unsigned int j = 0; j < material.nVertices; j++, data++, v++ )
		{
			data->Position = v->Position;
			data->Normal   = D3DXVECTOR4(v->Normal.x, v->Normal.y, v->Normal.z, (float)v->BoneIndices[0] );
			data->TexCoord = v->TexCoords[0] * texScale;
			data->Tangent  = v->Tangent;
			data->Binormal = v->Binormal;
			data->Color    = v->Color;
		}
		materials[i].pMesh->UnlockVertexBuffer();

		materials[i].pMesh->LockIndexBuffer(D3DLOCK_DISCARD, (void**)&data);
		memcpy( data, material.indices, material.nTriangles * 3 * sizeof(uint16_t) );
		materials[i].pMesh->UnlockIndexBuffer();
	}
}

Engine::EngineImpl::MeshInfo& Engine::EngineImpl::MeshInfo::operator =(const MeshInfo& meshinfo)
{
	enabled			= meshinfo.enabled;
	index			= meshinfo.index;
	pMesh			= meshinfo.pMesh;
	pDevice			= meshinfo.pDevice;
	pModel			= meshinfo.pModel;
	effectManager	= meshinfo.effectManager;
	materials		= meshinfo.materials;
	return *this;
}

Engine::EngineImpl::MeshInfo::MeshInfo(const MeshInfo& meshinfo)
{
	*this = meshinfo;
}

Engine::EngineImpl::MeshInfo::MeshInfo(int index, IEffectManager* effectManager, IDirect3DDevice9* pDevice, const Model* model, const IMesh* mesh, bool enabled)
{
	pMesh            = mesh;
	this->index      = index;
	this->pModel     = model;
	this->pDevice    = pDevice;
	this->enabled    = enabled;
	this->effectManager = effectManager;

	unsigned int nMaterials = mesh->getNumMaterials();
	materials.resize(nMaterials);
	for (unsigned int i = 0; i < nMaterials; i++)
	{
		materials[i].index       = i;
		materials[i].iMeshInfo   = index;
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
void Engine::EngineImpl::setCamera( const Camera& camera )
{
	settings.Eye = camera;
	D3DXMatrixLookAtRH(&settings.View, &camera.Position, &camera.Target, &camera.Up );
	D3DXMatrixMultiply(&settings.ViewProjection, &settings.View, &settings.Projection);
}

// Set the parameter for this effect
void Engine::EngineImpl::setParameter(ID3DXEffect* pEffect, D3DXHANDLE hParam, const Parameter& param)
{
	switch (param.type)
	{
		case Parameter::INT:	pEffect->SetInt(hParam, param.m_int); break;
		case Parameter::FLOAT:	pEffect->SetFloat(hParam, param.m_float); break;
		case Parameter::FLOAT3:	pEffect->SetFloatArray(hParam, param.m_float3, 3); break;
		case Parameter::FLOAT4:	pEffect->SetFloatArray(hParam, param.m_float4, 4); break;
		case Parameter::MATRIX: pEffect->SetMatrix(hParam, &param.m_matrix); break;
		case Parameter::TEXTURE:
		{
			IDirect3DTexture9* pTexture = textureManager->getTexture(pDevice, param.m_texture);
			if (pTexture != NULL)
			{
				pEffect->SetTexture(hParam, pTexture);
			}
			break;
		}
	}
}

void Engine::EngineImpl::doBillboard(const Bone* pBone, D3DXMATRIX& world, D3DXMATRIX& viewInv)
{
	if (pBone->billboardType != BT_DISABLE)
	{
		D3DXMATRIX transform;
		D3DXMATRIX rotation;
		D3DXMatrixRotationX(&rotation, D3DXToRadian(-90));
		switch (pBone->billboardType)
		{
			case BT_PARALLEL:
			case BT_FACE:
				D3DXMatrixMultiply(&transform, &rotation, &viewInv);
				D3DXMatrixMultiply(&world, &transform, &world);
				break;

			case BT_ZAXIS_LIGHT:
			{
				D3DXMatrixRotationZ(&transform, atan2(-settings.Lights[0].Direction.y, -settings.Lights[0].Direction.x) - D3DXToRadian(90));
				D3DXMatrixMultiply(&world, &transform, &world);
				break;
			}

			case BT_ZAXIS_VIEW:
			{
				D3DXMatrixRotationZ(&transform, atan2(settings.Eye.Position.y, settings.Eye.Position.x) - D3DXToRadian(90) );
				D3DXMatrixMultiply(&world, &transform, &world);
				break;
			}

			case BT_ZAXIS_WIND:
			{
				D3DXMatrixRotationZ(&transform, atan2(-settings.Wind.y, -settings.Wind.x) - D3DXToRadian(90) );
				D3DXMatrixMultiply(&world, &transform, &world);
				break;
			}

			case BT_SUNLIGHT_GLOW:
				if (pBone != NULL)
				{
					D3DXMatrixInverse(&transform, NULL, &pBone->matrix);
					D3DXMatrixMultiply(&transform, &transform, &rotation);
					D3DXMatrixMultiply(&transform, &transform, &pBone->matrix);
					D3DXMatrixMultiply(&transform, &transform, &viewInv);
					D3DXMatrixMultiply(&world, &world, &transform);
				}
				break;

			case BT_SUN:
				if (pBone != NULL)
				{
					D3DXMatrixInverse(&transform, NULL, &pBone->matrix);
					D3DXMatrixMultiply(&transform, &transform, &rotation);
					D3DXMatrixMultiply(&transform, &transform, &viewInv);

					D3DXMATRIX tmp;
					float len = sqrt(settings.Lights[0].Position.x * settings.Lights[0].Position.x + settings.Lights[0].Position.y * settings.Lights[0].Position.y);
					D3DXMatrixRotationY(&tmp, -atan2(settings.Lights[0].Position.z, len));
					D3DXMatrixRotationZ(&rotation, atan2(settings.Lights[0].Position.y, settings.Lights[0].Position.x));
					D3DXMatrixMultiply(&rotation, &tmp, &rotation);

					D3DXMatrixMultiply(&tmp, &pBone->matrix, &rotation);
					tmp._12 = tmp._13 = tmp._14 = tmp._21 = tmp._23 = tmp._24 = tmp._31 = tmp._32 = tmp._34 = 0.0f;
					tmp._11 = tmp._22 = tmp._33 = 1.0f;

					D3DXMatrixMultiply(&transform, &transform, &tmp);
					D3DXMatrixMultiply(&world, &world, &transform);
				}
				break;
		}
	}
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

	//
	// Calculate global parameters
	//
	float time = GetTickCount() / 1000.0f;
	D3DXVECTOR4 eye(settings.Eye.Position.x, settings.Eye.Position.y, settings.Eye.Position.z, 1.0f);
	
	D3DXMATRIX shadowTranslate;
	D3DXMatrixTranslation(&shadowTranslate, settings.Lights[0].Direction.x, settings.Lights[0].Direction.y, settings.Lights[0].Direction.z);

	D3DXMATRIX identity;
	D3DXMatrixIdentity(&identity);
	pDevice->SetTransform(D3DTS_VIEW, &settings.View);
	pDevice->SetTransform(D3DTS_PROJECTION, &settings.Projection);

	// Set global states
	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);

	// Set Fixed Function states
	D3DLIGHT9 D3DLight =
	{
		D3DLIGHT_DIRECTIONAL,
		{settings.Lights[0].Diffuse.x,  settings.Lights[0].Diffuse.y,  settings.Lights[0].Diffuse.z,  settings.Lights[0].Diffuse.w },
		{settings.Lights[0].Specular.x, settings.Lights[0].Specular.y, settings.Lights[0].Specular.z, settings.Lights[0].Specular.w},
		{settings.Ambient.x, settings.Ambient.y, settings.Ambient.z, settings.Ambient.w},
		{0,0,0},
		{settings.Lights[0].Direction.x, settings.Lights[0].Direction.y, settings.Lights[0].Direction.z}
	};

	D3DXMATRIX billboard;
	D3DXMatrixInverse(&billboard, NULL, &settings.View);
	billboard._41 = billboard._42 = billboard._43 = 0.0f;	// Clear transpose, rotation only

	D3DLIGHT9 D3DShadow = D3DLight;
	D3DShadow.Diffuse .r *= settings.Shadow.x; D3DShadow.Diffuse .g *= settings.Shadow.y; D3DShadow.Diffuse .b *= settings.Shadow.z; D3DShadow.Diffuse .a *= settings.Shadow.w;
	D3DShadow.Specular.r *= settings.Shadow.x; D3DShadow.Specular.g *= settings.Shadow.y; D3DShadow.Specular.b *= settings.Shadow.z; D3DShadow.Specular.a *= settings.Shadow.w;
	D3DShadow.Ambient .r *= settings.Shadow.x; D3DShadow.Ambient .g *= settings.Shadow.y; D3DShadow.Ambient .b *= settings.Shadow.z; D3DShadow.Ambient .a *= settings.Shadow.w;

	pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
	pDevice->LightEnable(0, TRUE);
	pDevice->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_RGBA(
		(int)(settings.Ambient.x * 255),
		(int)(settings.Ambient.y * 255),
		(int)(settings.Ambient.z * 255),
		(int)(settings.Ambient.w * 255)) );

	D3DMATERIAL9 material = {{1,1,1,1}, {1,1,1,1}, {1,1,1,1}, {0,0,0,0}, 0};
	pDevice->SetMaterial(&material);

	//
	// Create a per-phase list of all the meshes
	//
	D3DXMaterial* phases[NUM_PHASES] = {NULL};
	for (size_t i = 0; i < meshes.size(); i++)
	{
		if (meshes[i].isEnabled())
		{
			unsigned int numMaterials = meshes[i].getNumMaterials();
			for (unsigned int m = 0; m < numMaterials; m++)
			{
				D3DXMaterial* material = meshes[i].getMaterial(m);
				material->next = phases[material->phase];
				phases[material->phase] = material;
			}
		}
	}

	//
	// Now render the scene
	//
	static const int STENCIL_INITIAL_VALUE = 8;

	D3DCOLOR background = D3DCOLOR_XRGB(GetRValue(settings.Background), GetGValue(settings.Background), GetBValue(settings.Background));
	pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, background, 1.0f, STENCIL_INITIAL_VALUE);
	pDevice->BeginScene();

	bool nolight = false;
	for (int phase = 0; phase < NUM_PHASES; phase++)
	{
		pDevice->SetLight(0, (nolight) ? &D3DShadow : &D3DLight);

		if (settings.isGroundVisible && phase == PHASE_OPAQUE)
		{
			//
			// Render the ground plane
			//
			struct GROUNDVERTEX
			{
				D3DXVECTOR3 Position;
				D3DXVECTOR3 Normal;
				D3DCOLOR	Color;
			};

			static GROUNDVERTEX GroundPlane[4] = {
				{D3DXVECTOR3(-10000,-10000,0), D3DXVECTOR3(0,0,1), 0},
				{D3DXVECTOR3( 10000,-10000,0), D3DXVECTOR3(0,0,1), 0},
				{D3DXVECTOR3(-10000, 10000,0), D3DXVECTOR3(0,0,1), 0},
				{D3DXVECTOR3( 10000, 10000,0), D3DXVECTOR3(0,0,1), 0},
			};

			D3DCOLOR color = D3DCOLOR_RGBA(GetRValue(settings.Background), GetGValue(settings.Background), GetBValue(settings.Background), 255);
			GroundPlane[0].Color = GroundPlane[1].Color = GroundPlane[2].Color = GroundPlane[3].Color = color;

			pDevice->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
			pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
			pDevice->SetTransform(D3DTS_WORLD, &identity);
			pDevice->SetFVF(D3DFVF_DIFFUSE | D3DFVF_NORMAL | D3DFVF_XYZ);
			pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, GroundPlane, sizeof(GROUNDVERTEX));
		}

		pDevice->SetRenderState(D3DRS_FILLMODE, (settings.renderMode == RM_WIREFRAME) ? D3DFILL_WIREFRAME : D3DFILL_SOLID);
		pDevice->SetRenderState(D3DRS_CULLMODE, (phase == PHASE_TRANSPARENT) ? D3DCULL_NONE : D3DCULL_CW);

		// Render all materials in this phase
		for (const D3DXMaterial* material = phases[phase]; material != NULL; material = material->next)
		{
			const MeshInfo& meshinfo = meshes[material->iMeshInfo];

			if (material->pEffect == NULL)
			{
				// No effect? No rendering!
				continue;
			}

			// Set world transformation
			int bone = model->getConnection(meshinfo.getIndex());
			D3DXMATRIX world = animatedModel->getAnimatedBone(bone, frame);
			doBillboard( (bone != -1) ? model->getBone(bone) : NULL, world, billboard);
			if (phase == PHASE_SHADOW)
			{
				// Transpose the shadow mesh slightly in the direction of the light to avoid coplanar Z-fighting
				D3DXMatrixMultiply(&world, &world, &shadowTranslate);
			}

			// For some reason, we need to rotate light0 in opposite direction of the mesh's rotation if
			// the lighting and shadows are to be calculated correctly.
			D3DXVECTOR4 light0_pos(settings.Lights[0].Position.x, settings.Lights[0].Position.y, settings.Lights[0].Position.z, 1.0f);
			D3DXMATRIX  lightRotationInv;
			D3DXMatrixInverse(&lightRotationInv, NULL, &world);
			lightRotationInv._41 = lightRotationInv._42 = lightRotationInv._43 = 0.0f;	// Clear translation
			D3DXVec4Transform(&light0_pos, &light0_pos, &lightRotationInv);

			// Set the parameters from the mesh file
			ID3DXEffect* pEffect = material->pEffect;
			const Effect& effect = meshinfo.getIMesh()->getMaterial(material->index).effect;
			for (vector<Parameter>::const_iterator p = effect.parameters.begin(); p != effect.parameters.end(); p++)
			{
				setParameter(pEffect, p->name.c_str(), *p);
			}

			// Set the transforms for the Fixed Function pipeline, in case it uses it
			D3DXMATRIX worldView;
			D3DXMATRIX worldViewProjection;
			D3DXMatrixMultiply(&worldView,           &world, &settings.View);
			D3DXMatrixMultiply(&worldViewProjection, &world, &settings.ViewProjection);
			pDevice->SetTransform(D3DTS_WORLD,       &world);

			// Set the parameters with semantics from the effects file
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "WORLD"),				  &world);
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "VIEW"),				  &settings.View );
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "WORLDVIEW"),			  &worldView);
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "PROJECTION"),			  &settings.Projection);
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "VIEWPROJECTION"),		  &settings.ViewProjection);
			pEffect->SetMatrix(pEffect->GetParameterBySemantic(NULL, "WORLDVIEWPROJECTION"),  &worldViewProjection);
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "GLOBAL_AMBIENT"),       &settings.Ambient );
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "DIR_LIGHT_VEC_0"),      &light0_pos );
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "DIR_LIGHT_OBJ_VEC_0"),  &light0_pos );
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "DIR_LIGHT_DIFFUSE_0"),  &settings.Lights[0].Diffuse);
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "DIR_LIGHT_SPECULAR_0"), &settings.Lights[0].Specular);
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "LIGHT_SCALE"),      (nolight) ? &settings.Shadow : &D3DXVECTOR4(1,1,1,1));
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "EYE_POSITION"),     &eye);
			pEffect->SetVector(pEffect->GetParameterBySemantic(NULL, "EYE_OBJ_POSITION"), &eye);
			pEffect->SetFloat(pEffect->GetParameterBySemantic(NULL, "TIME"), time);
			pEffect->SetMatrixArray(pEffect->GetParameterBySemantic(NULL, "SPH_LIGHT_FILL"), settings.SPH_Light_Fill, 3);
			pEffect->SetMatrixArray(pEffect->GetParameterBySemantic(NULL, "SPH_LIGHT_ALL"),  settings.SPH_Light_All,  3);
			if (ri.useColor)
			{
				D3DXVECTOR4 color(GetRValue(ri.color) / 255.0f, GetGValue(ri.color) / 255.0f, GetBValue(ri.color) / 255.0f, 1.0f);
				pEffect->SetVector("Colorization", &color);
			}
			pEffect->SetFloat("EdgeBrightness", 2.0f);	// For the shields to show up properly

			D3DXHANDLE hSkinMatrixArray = pEffect->GetParameterBySemantic(NULL, "SKINMATRIXARRAY");
			if (hSkinMatrixArray != NULL)
			{
				unsigned int numMappings = (unsigned int)material->boneMapping.size();
				if (numMappings == 0)
				{
					pEffect->SetMatrix(hSkinMatrixArray, &world);
				}
				else
				{
					D3DXMATRIX skinArray[24];
					for (unsigned long map = 0; map < numMappings; map++)
					{
						unsigned long bone = material->boneMapping[map];
						D3DXMATRIX transform;
						model->getBoneTransformation(bone, transform);
						D3DXMatrixInverse(&transform, NULL, &transform);
						if (phase == PHASE_SHADOW)
						{
							D3DXMatrixMultiply(&transform, &transform, &shadowTranslate);
						}
						skinArray[map] = animatedModel->getAnimatedBone(bone, frame);
						D3DXMatrixMultiply(&skinArray[map], &transform, &skinArray[map]);
					}
					pEffect->SetMatrixArray(hSkinMatrixArray, skinArray, numMappings);
				}
			}

			// Use the effect to render the mesh
			unsigned int nPasses;
			pEffect->Begin(&nPasses, 0);
			for (unsigned int iPass = 0; iPass < nPasses; iPass++)
			{	
				pEffect->BeginPass(iPass);
				material->pMesh->DrawSubset(0);
				pEffect->EndPass();
			}
			pEffect->End();
		}

		if (nolight && phase == PHASE_TRANSPARENT)
		{
			break;
		}
		else if (phase == PHASE_SHADOW && !nolight)
		{
			phase = -1;
			nolight = true;
			pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
			pDevice->SetRenderState(D3DRS_STENCILENABLE, TRUE);
			pDevice->SetRenderState(D3DRS_STENCILREF,    STENCIL_INITIAL_VALUE);
			pDevice->SetRenderState(D3DRS_STENCILFUNC,   D3DCMP_LESS);
			pDevice->SetRenderState(D3DRS_STENCILPASS,	 D3DSTENCILOP_KEEP);
			pDevice->SetRenderState(D3DRS_STENCILFAIL,   D3DSTENCILOP_KEEP);
		}
	}
	pDevice->SetRenderState(D3DRS_STENCILENABLE, FALSE);

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
		BoneVertex* boneLines  = new BoneVertex[numBones*8];
		
		// Create bone array
		for (unsigned int i = 0; i < numBones; i++)
		{
			const Bone* bone = model->getBone(i);
			D3DXMATRIX transform;

			// Create transformation matrix for this joint
			transform = animatedModel->getAnimatedBone(i, frame);
			doBillboard(bone, transform, billboard);

			D3DXVec3TransformCoord(&bonePoints[i].pos, &D3DXVECTOR3(0,0,0), &transform);
			bonePoints[i].color = D3DCOLOR_XRGB(255,255,0);

			BoneVertex* lines = &boneLines[8*i];

			// Create local coordinate system lines
			transform._41 = transform._42 = transform._43 = 0.0; // No translation, only rotation

			D3DXVec3TransformCoord( &lines[3].pos, &D3DXVECTOR3(2,0,0), &transform);
			lines[2].color = lines[3].color = D3DCOLOR_XRGB(255,0,0);
			lines[2].pos   = bonePoints[i].pos;
			lines[3].pos  += bonePoints[i].pos;
			D3DXVec3TransformCoord( &lines[5].pos, &D3DXVECTOR3(0,2,0), &transform);
			lines[4].color = lines[5].color = D3DCOLOR_XRGB(0,255,0);
			lines[4].pos   = bonePoints[i].pos;
			lines[5].pos  += bonePoints[i].pos;
			D3DXVec3TransformCoord( &lines[7].pos, &D3DXVECTOR3(0,0,2), &transform);
			lines[6].color = lines[7].color = D3DCOLOR_XRGB(0,0,255);
			lines[6].pos   = bonePoints[i].pos;
			lines[7].pos  += bonePoints[i].pos;

			// Create parent-child connection line
			lines[0].color = D3DCOLOR_XRGB(255,255,0);
			lines[0].pos   = bonePoints[i].pos;
			if (bone->parent != -1)
			{
				// Create transformation matrix for parent joint
				transform = animatedModel->getAnimatedBone(bone->parent, frame);
				D3DXVec3TransformCoord(&lines[1].pos, &D3DXVECTOR3(0,0,0), &transform);
				lines[1].color = D3DCOLOR_XRGB(255,255,0);
			}
			else
			{
				lines[1] = lines[0];
			}
		}

		// Render bones
		float PointSize = 4.0;
		pDevice->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
		pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
		pDevice->SetRenderState(D3DRS_POINTSIZE, *(DWORD*)&PointSize);
		D3DXMATRIX identity;
		D3DXMatrixIdentity(&identity);
		pDevice->SetTransform(D3DTS_WORLD, &identity);
		pDevice->SetTransform(D3DTS_VIEW, &settings.View);
		pDevice->SetTransform(D3DTS_PROJECTION, &settings.Projection);

		pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER, 0, 1.0f, 0);
		pDevice->DrawPrimitiveUP(D3DPT_POINTLIST, numBones, bonePoints, sizeof(BoneVertex));
		pDevice->DrawPrimitiveUP(D3DPT_LINELIST,  numBones * 4, boneLines,  sizeof(BoneVertex));

		delete[] boneLines;
		delete[] bonePoints;
	}
	pDevice->EndScene();
	pDevice->Present(NULL, NULL, NULL, NULL);

	if (ri.showBones && ri.showBoneNames && model != NULL)
	{
		// Show bone names
		HDC hDC = GetDC(PresentationParameters.hDeviceWindow);
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
			doBillboard(bone, transform, billboard);

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

		ReleaseDC(PresentationParameters.hDeviceWindow, hDC);
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
	PresentationParameters.BackBufferWidth  = 0;
    PresentationParameters.BackBufferHeight = 0;
	PresentationParameters.BackBufferCount  = 1;
    PresentationParameters.hDeviceWindow    = hWnd;
    PresentationParameters.Windowed         = true;

	HRESULT hErr;
	if ((hErr = pDevice->Reset( &PresentationParameters )) != D3D_OK)
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
		// http://www.gamedev.net/columns/hardcore/shadowvolume/page4.asp
		float n = 1.0f;
		D3DXMatrixPerspectiveFovRH(&settings.Projection, D3DXToRadian(45), (float)width / height, n, 1000.0f );
		settings.Projection._33 = -1.0f;
		settings.Projection._43 = -2 * n;

		D3DXMatrixMultiply(&settings.ViewProjection, &settings.View, &settings.Projection);
	}
}

D3DFORMAT Engine::EngineImpl::GetDepthStencilFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat)
{
	static const D3DFORMAT Formats[3] = { D3DFMT_D24S8, D3DFMT_D24FS8, D3DFMT_D24X4S4 };

	for (int i = 0; i < 3; i++)
	{
		if (SUCCEEDED(pD3D->CheckDeviceFormat     (Adapter, DeviceType, AdapterFormat, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE, Formats[i])))
		if (SUCCEEDED(pD3D->CheckDepthStencilMatch(Adapter, DeviceType, AdapterFormat, AdapterFormat, Formats[i])))
		{
			return Formats[i];
		}
	}

	return D3DFMT_UNKNOWN;
}

void Engine::EngineImpl::GetMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT DisplayFormat)
{
	D3DMULTISAMPLE_TYPE MultiSampleTypes[16] = {
		D3DMULTISAMPLE_16_SAMPLES, D3DMULTISAMPLE_15_SAMPLES, D3DMULTISAMPLE_14_SAMPLES, D3DMULTISAMPLE_13_SAMPLES,
		D3DMULTISAMPLE_12_SAMPLES, D3DMULTISAMPLE_11_SAMPLES, D3DMULTISAMPLE_10_SAMPLES, D3DMULTISAMPLE_9_SAMPLES,
		D3DMULTISAMPLE_8_SAMPLES, D3DMULTISAMPLE_7_SAMPLES, D3DMULTISAMPLE_6_SAMPLES, D3DMULTISAMPLE_5_SAMPLES,
		D3DMULTISAMPLE_4_SAMPLES, D3DMULTISAMPLE_3_SAMPLES, D3DMULTISAMPLE_2_SAMPLES, D3DMULTISAMPLE_NONE
	};

	int i;
	for (i = 0; i < 16; i++)
	{
		if (SUCCEEDED(pD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, DisplayFormat,                                 PresentationParameters.Windowed, MultiSampleTypes[i], &PresentationParameters.MultiSampleQuality)))
		if (SUCCEEDED(pD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, PresentationParameters.AutoDepthStencilFormat, PresentationParameters.Windowed, MultiSampleTypes[i], &PresentationParameters.MultiSampleQuality)))
		{
			PresentationParameters.MultiSampleQuality--;
			PresentationParameters.MultiSampleType = MultiSampleTypes[i];
			break;
		}
	}

	if (i == 16)
	{
		PresentationParameters.MultiSampleQuality = 0;
		PresentationParameters.MultiSampleType    = D3DMULTISAMPLE_NONE;
	}
}

Engine::EngineImpl::EngineImpl(HWND hWnd, ITextureManager* textureManager, IEffectManager* effectManager)
{
	HRESULT hErr;

	this->textureManager = textureManager;
	this->effectManager  = effectManager;
	this->model          = NULL;
	this->settings.isGroundVisible = false;
	this->settings.renderMode      = RM_SOLID;
	this->settings.Background      = RGB(0,0,0);
	this->settings.Wind            = D3DXVECTOR3(1,0,0);

	UINT       Adapter    = D3DADAPTER_DEFAULT;
	D3DDEVTYPE DeviceType = D3DDEVTYPE_HAL;

	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
        throw D3DException(E_FAIL);
	}

	//
	// Create the device
	//
	ZeroMemory(&PresentationParameters, sizeof PresentationParameters);
    PresentationParameters.BackBufferFormat   = D3DFMT_UNKNOWN;
	PresentationParameters.BackBufferWidth    = 0;
	PresentationParameters.BackBufferHeight   = 0;
    PresentationParameters.BackBufferCount    = 1;
    PresentationParameters.SwapEffect         = D3DSWAPEFFECT_DISCARD;
    PresentationParameters.hDeviceWindow      = hWnd;
    PresentationParameters.Windowed           = TRUE;
    PresentationParameters.Flags              = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    PresentationParameters.EnableAutoDepthStencil     = TRUE;
    PresentationParameters.FullScreen_RefreshRateInHz = 0;
    PresentationParameters.PresentationInterval       = D3DPRESENT_INTERVAL_DEFAULT;

	// Get current display mode
	D3DDISPLAYMODE DisplayMode;
	if (FAILED(hErr = pD3D->GetAdapterDisplayMode(Adapter, &DisplayMode)))
	{
		pD3D->Release();
		throw D3DException( hErr );
	}

	// Determine best depth/stencil buffer format
	if ((PresentationParameters.AutoDepthStencilFormat = GetDepthStencilFormat(Adapter, DeviceType, DisplayMode.Format)) == D3DFMT_UNKNOWN)
	{
		Log::Write("Unable to find a matching depth buffer format\n");
		pD3D->Release();
		throw D3DException( E_FAIL );
	}

	// Get the best multisample type
	GetMultiSampleType(Adapter, DeviceType, DisplayMode.Format);

	//
	// Dump what we've got
	//
	Log::Write("Initializing Direct3D with:\n");
	Log::Write(" FSAA:     ");
	if (PresentationParameters.MultiSampleType == D3DMULTISAMPLE_NONE)  Log::Write("None\n");
	else Log::Write("%ux (q=%u)\n", PresentationParameters.MultiSampleType, PresentationParameters.MultiSampleQuality);

	const char* buffer = "Unknown";
	switch (PresentationParameters.AutoDepthStencilFormat)
	{
		case D3DFMT_D24S8:	 buffer = "D24/S8"; break;
		case D3DFMT_D24FS8:  buffer = "D24/FS8"; break;
		case D3DFMT_D24X4S4: buffer = "D24/X4/S4"; break;
	}
	Log::Write(" Buffer:   %s\n", buffer);

	//
	// Create device (HW first, then SW)
	//
	if (FAILED(hErr = pD3D->CreateDevice(Adapter, DeviceType, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &PresentationParameters, &pDevice)))
	if (FAILED(hErr = pD3D->CreateDevice(Adapter, DeviceType, NULL, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &PresentationParameters, &pDevice)))
	{
		pD3D->Release();
		throw D3DException( hErr );
	}
	else Log::Write(" Renderer: Software\n");
	else Log::Write(" Renderer: Hardware\n");

	//
	// Set the vertex declaration
	//

	pDevice->SetRenderState(D3DRS_ZENABLE,  TRUE);
	pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

	// Initialize projection matrix with perspective
	RECT client;
	GetClientRect( hWnd, &client );

	// http://www.gamedev.net/columns/hardcore/shadowvolume/page4.asp
	float n = 1.0f;
	D3DXMatrixPerspectiveFovRH(&settings.Projection, D3DXToRadian(45), (float)client.right / client.bottom, n, 10000.0f );
	settings.Projection._33 = -1.0f;
	settings.Projection._43 = -2 * n;

	// Initialize camera
	Camera camera = {
		D3DXVECTOR3(-500,-500,500),
		D3DXVECTOR3(0,0,0),
		D3DXVECTOR3(0,0,1)
	};
	setCamera( camera );

	memset(settings.Lights, 0, 4 * sizeof(D3DLIGHT9));
}

Engine::EngineImpl::~EngineImpl()
{
	pDevice->Release();
	pD3D->Release();
}

void Engine::setRenderMode(RenderMode mode)		{ pimpl->settings.renderMode = mode; }
void Engine::setCamera( const Camera& camera )	{ pimpl->setCamera( camera ); }
void Engine::setGroundVisibility(bool visible)	{ pimpl->settings.isGroundVisible = visible; }
void Engine::setBackground(COLORREF color)		{ pimpl->settings.Background = color; }
void Engine::setWind(const D3DXVECTOR3& wind)	{ pimpl->settings.Wind       = wind; }
RenderMode Engine::getRenderMode() const		{ return pimpl->settings.renderMode; }
const Camera& Engine::getCamera() const			{ return pimpl->settings.Eye; }
Model* Engine::getModel() const					{ return pimpl->model; }
COLORREF Engine::getBackground() const			{ return pimpl->settings.Background; }

void Engine::setLight(LightType which, const LIGHT& light)
{
	int index = 0;
	switch (which)
	{
		case LT_SUN:	index = 0; break;
		case LT_FILL1:	index = 1; break;
		case LT_FILL2:	index = 2; break;
	}
	pimpl->settings.Lights[index] = light;
	
	// Calculate direction from position
	D3DXVec3Normalize(&pimpl->settings.Lights[index].Direction, &-pimpl->settings.Lights[index].Position);

	// Recalculate Spherical Harmonics matrices
	SPH_Calculate_Matrices(pimpl->settings.SPH_Light_Fill,  &pimpl->settings.Lights[1], 2, pimpl->settings.Ambient);
	SPH_Calculate_Matrices(pimpl->settings.SPH_Light_All,   &pimpl->settings.Lights[0], 3, pimpl->settings.Ambient);
}

void Engine::setAmbient(const D3DXVECTOR4& color)
{
	pimpl->settings.Ambient = color;

	// Recalculate Spherical Harmonics matrices
	SPH_Calculate_Matrices(pimpl->settings.SPH_Light_Fill,  &pimpl->settings.Lights[1], 2, pimpl->settings.Ambient);
	SPH_Calculate_Matrices(pimpl->settings.SPH_Light_All,   &pimpl->settings.Lights[0], 3, pimpl->settings.Ambient);
}

void Engine::setShadow(const D3DXVECTOR4& color)
{
	pimpl->settings.Shadow = color;
}

void Engine::setModel(Model* model)
{
	delete pimpl->animatedModel;

	pimpl->model         = model;
	pimpl->animatedModel = new EngineImpl::AnimatedModel(NULL, *model);

	pimpl->meshes.clear();
	for (unsigned int i = 0; i < model->getNumMeshes(); i++)
	{
		pimpl->meshes.push_back( EngineImpl::MeshInfo(i, pimpl->effectManager, pimpl->pDevice, model, (const IMesh*)model->getMesh(i)) );
	}
}

void Engine::enableMesh(unsigned int i, bool enabled)
{
	if (i >= 0 && i < pimpl->meshes.size())
	{
		pimpl->meshes[i].enable( enabled );
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