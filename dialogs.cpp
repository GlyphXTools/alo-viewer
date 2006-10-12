#include <algorithm>
#include <sstream>
#include <iomanip>

#include "dialogs.h"
#include "model.h"
#include "mesh.h"
using namespace std;

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <commctrl.h>
#include <afxres.h>
#include "resource.h"

struct SELECTFILEPARAM
{
	const MegaFile* megaFile;
	const Model*    model;
};

static bool isSuitedAnimation(const Animation* anim, const Model* model)
{
	unsigned int numAnims = anim->getNumBoneAnimations();
	unsigned int numBones = model->getNumBones();
	
	if (numAnims > numBones)
	{
		// Obviously we can't animate more bones than there are in the model
		return false;
	}

	for (unsigned int i = 0; i < numAnims; i++)
	{
		const BoneAnimation& ba = anim->getBoneAnimation(i);
		if (ba.getBoneIndex() >= numBones)
		{
			return false;
		}

		const Bone* bone = model->getBone( ba.getBoneIndex() );
		if (bone->name != ba.getName())
		{
			return false;
		}
	}

	return true;
}

// Dialog box window procedure for "Select file" dialog
INT_PTR CALLBACK SelectFileDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	SELECTFILEPARAM* sfp = (SELECTFILEPARAM*)lParam;
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hList = GetDlgItem(hWnd, IDC_LIST1);
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)sfp);
			for (unsigned int i = 0; i < sfp->megaFile->getNumFiles(); i++)
			{
				string name = sfp->megaFile->getFilename(i);
				if (name != "")
				{
					size_t p = name.find_last_of(".");
					if (p != string::npos)
					{
						string ext = name.substr(p + 1);
						transform(ext.begin(), ext.end(), ext.begin(), toupper);
						bool show = false;
						if (sfp->model == NULL)
						{
							show = (ext == "ALO");
						}
						else if (ext == "ALA")
						{
							File* file = sfp->megaFile->getFile(i);
							try
							{
								Animation* anim = new Animation( file, true );
								show = isSuitedAnimation(anim, sfp->model);
								delete anim;
							}
							catch (...)
							{
								show = false;
							}
							file->release();
						}

						if (show)
						{
							int index = (int)SendMessage(hList, LB_ADDSTRING, 0, (LPARAM)name.c_str() );
							SendMessage(hList, LB_SETITEMDATA, index, i );
						}
					}
				}
			}
			SetFocus( hList );
			return FALSE;
		}

		case WM_COMMAND:
		{
			WORD code = HIWORD(wParam);
			WORD id   = LOWORD(wParam);
			if (code == LBN_DBLCLK || code == BN_CLICKED)
			{
				int index = -1;
				if ((code == BN_CLICKED && id == IDOK) || (code == LBN_DBLCLK && id == IDC_LIST1))
				{
					HWND hList = GetDlgItem(hWnd, IDC_LIST1);
					index = (int)SendMessage(hList, LB_GETCURSEL, 0, 0 );
					if (index == LB_ERR)
					{
						return TRUE;
					}
					index = (int)SendMessage(hList, LB_GETITEMDATA, index, 0 );
				}
				EndDialog( hWnd, index );
			}
			return TRUE;
		}
	}
	return FALSE;
}

// Do the whole "Open File" dialog and load and set
// the model accordingly.
Model* DlgOpenModel( ApplicationInfo *info )
{
	Model* model = NULL;

	try
	{
		char filename[MAX_PATH];
		filename[0] = '\0';

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize  = sizeof(OPENFILENAME);
		ofn.hwndOwner    = info->hMainWnd;
		ofn.hInstance    = info->hInstance;
		ofn.lpstrFilter  = "Alamo Files (*.alo, *.meg)\0*.ALO; *.MEG\0All Files (*.*)\0*.*\0\0";
		ofn.nFilterIndex = 0;
		ofn.lpstrFile    = filename;
		ofn.nMaxFile     = MAX_PATH;
		ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileName( &ofn ) != 0)
		{
			string ext = &filename[ofn.nFileExtension];
			transform(ext.begin(), ext.end(), ext.begin(), toupper);
			File* file = new PhysicalFile(filename);
			if (ext == "MEG")
			{
				// Load it through a MegaFile
				MegaFile megafile( file );
				
				SELECTFILEPARAM sfp;
				sfp.megaFile = &megafile;
				sfp.model    = NULL;

				int index = (int)DialogBoxParam( info->hInstance, MAKEINTRESOURCE(IDD_DIALOG2), info->hMainWnd, SelectFileDlgProc, (LPARAM)&sfp );
				if (index >= 0)
				{
					// We selected a subfile, try to load it
					File* subfile = megafile.getFile(index);
					model = new Model( subfile );
					subfile->release();
				}
			}
			else
			{
				// Load the model file directly
				model = new Model(file);
			}
			file->release();
		}
	}
	catch (...)
	{
		MessageBox(NULL, "Unable to open the specified model", NULL, MB_OK | MB_ICONHAND );
		model = NULL;
	}
	return model;
}

// Do the whole "Open Animation" dialog and load and set
// the model accordingly.
Animation* DlgOpenAnimation( ApplicationInfo *info, const Model* model )
{
	Animation* anim = NULL;

	try
	{
		char filename[MAX_PATH];
		filename[0] = '\0';

		OPENFILENAME ofn;
		memset(&ofn, 0, sizeof(OPENFILENAME));
		ofn.lStructSize  = sizeof(OPENFILENAME);
		ofn.hwndOwner    = info->hMainWnd;
		ofn.hInstance    = info->hInstance;
		ofn.lpstrFilter  = "Alamo Files (*.ala, *.meg)\0*.ALA; *.MEG\0All Files (*.*)\0*.*\0\0";
		ofn.nFilterIndex = 0;
		ofn.lpstrFile    = filename;
		ofn.nMaxFile     = MAX_PATH;
		ofn.Flags        = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		if (GetOpenFileName( &ofn ) != 0)
		{
			string ext = &filename[ofn.nFileExtension];
			transform(ext.begin(), ext.end(), ext.begin(), toupper);
			File* file = new PhysicalFile(filename);
			if (ext == "MEG")
			{
				// Load it through a MegaFile
				MegaFile megafile( file );

				SELECTFILEPARAM sfp;
				sfp.megaFile = &megafile;
				sfp.model    = model;

				int index = (int)DialogBoxParam( info->hInstance, MAKEINTRESOURCE(IDD_DIALOG2), info->hMainWnd, SelectFileDlgProc, (LPARAM)&sfp );
				if (index >= 0)
				{
					// We selected a subfile, try to load it
					File* subfile = megafile.getFile(index);
					anim = new Animation( subfile );
					subfile->release();
				}
			}
			else
			{
				// Load the model file directly
				anim = new Animation(file);
			}
			file->release();

			if (anim != NULL && !isSuitedAnimation(anim, model))
			{
				MessageBox(NULL, "That animation is incompatible with the loaded model", NULL, MB_OK | MB_ICONHAND );
				anim = NULL;
			}
		}
	}
	catch (...)
	{
		MessageBox(NULL, "Unable to open the specified animation", NULL, MB_OK | MB_ICONHAND );
		anim = NULL;
	}
	return anim;
}

static const char* crlf = "\r\n";

static const int SHIFT_MESH		= 26;
static const int MASK_MESH		= 0x3F;
static const int MESH_BONE		= MASK_MESH;

static const int SHIFT_BONE     = 0;
static const int MASK_BONE		= 0x03FFFFFF;
static const int BONE_INVALID	= MASK_BONE;

static const int SHIFT_MATERIAL     = 22;
static const int MASK_MATERIAL      = 0xF;
static const int MATERIAL_INVALID   = MASK_MATERIAL;

static const int SHIFT_MESHPART       = 19;
static const int MASK_MESHPART        = 0x7;
static const int MESHPART_INFORMATION = 0;
static const int MESHPART_BOUNDINGBOX = 1;
static const int MESHPART_INVALID     = 7;

static const int SHIFT_MATERIALPART = 19;
static const int MASK_MATERIALPART  = 0x7;
static const int MATERIALPART_EFFECT    = 0;
static const int MATERIALPART_VERTICES  = 1;
static const int MATERIALPART_TRIANGLES = 2;
static const int MATERIALPART_MAPPING   = 3;
static const int MATERIALPART_COLLISION = 4;
static const int MATERIALPART_INVALID   = 7;

static const int SHIFT_PARAMETER   = 0;
static const int MASK_PARAMETER    = 0xFFFF;
static const int PARAMETER_INVALID = 0xFFFF;


#define MAKE_BONE(i)				((MESH_BONE << SHIFT_MESH) | ((i) & MASK_BONE))
#define MAKE_MATERIAL(m,a)			((((m) & MASK_MESH) << SHIFT_MESH) | (((a) & MASK_MATERIAL) << SHIFT_MATERIAL))
#define MAKE_MATERIAL_PART(m,a,p)	(MAKE_MATERIAL(m,a) | (((p) & MASK_MATERIALPART) << SHIFT_MATERIALPART))
#define MAKE_MESH_PART(m,p)			(MAKE_MATERIAL(m,MATERIAL_INVALID) | (((p) & MASK_MESHPART) << SHIFT_MESHPART))
#define MAKE_PARAMETER(m,a,p)		((((m) & MASK_MESH) << SHIFT_MESH) | (((a) & MASK_MATERIAL) << SHIFT_MATERIAL) | (MATERIALPART_EFFECT << SHIFT_MATERIALPART) | (((p) & MASK_PARAMETER) << SHIFT_PARAMETER) )
#define MAKE_EFFECT(m,a)			MAKE_PARAMETER(m,a,PARAMETER_INVALID)
#define INVALID_ID					MAKE_BONE(BONE_INVALID)

static void printBone(HWND hEdit, const Model* model, int iBone, stringstream& str)
{
	const Bone* bone = model->getBone(iBone);
	if (bone != NULL)
	{
		// Dump bone info to edit screen
		str << fixed << setprecision(3);
		str << "Index:  " << iBone << crlf;
		str << "Name:   " << bone->name << crlf;
		str << "Parent: " << bone->parent << crlf;
		str << "Matrix: " << setw(8) << bone->matrix._11 << " " << setw(8) << bone->matrix._12 << " " << setw(8) << bone->matrix._13 << " " << setw(8) << bone->matrix._14 << crlf;
		str << "        " << setw(8) << bone->matrix._21 << " " << setw(8) << bone->matrix._22 << " " << setw(8) << bone->matrix._23 << " " << setw(8) << bone->matrix._24 << crlf;
		str << "        " << setw(8) << bone->matrix._31 << " " << setw(8) << bone->matrix._32 << " " << setw(8) << bone->matrix._33 << " " << setw(8) << bone->matrix._34 << crlf;
		str << "        " << setw(8) << bone->matrix._41 << " " << setw(8) << bone->matrix._42 << " " << setw(8) << bone->matrix._43 << " " << setw(8) << bone->matrix._44 << crlf;
	}
}

static void printParameter(HWND hEdit, int iParam, const Parameter& param, stringstream& str)
{
	str << "Index: " << iParam << crlf;
	str << "Name:  " << param.name << crlf;
	str << "Type:  ";
	switch (param.type)
	{
		case Parameter::FLOAT:   str << "FLOAT"; break;
		case Parameter::FLOAT3:  str << "FLOAT3"; break;
		case Parameter::FLOAT4:  str << "FLOAT4"; break;
		case Parameter::TEXTURE: str << "TEXTURE"; break;
	}
	str << crlf;

	str << "Value: " << fixed << setprecision(3);
	switch (param.type)
	{
		case Parameter::FLOAT:   str << setw(8) << param.m_float; break;
		case Parameter::FLOAT3:  str << setw(8) << param.m_float3[0] << setw(8) << param.m_float3[1] << setw(8) << param.m_float3[2]; break;
		case Parameter::FLOAT4:  str << setw(8) << param.m_float4[0] << setw(8) << param.m_float4[1] << setw(8) << param.m_float4[2] << setw(8) << param.m_float4[3]; break;
		case Parameter::TEXTURE: str << param.m_texture; break;
	}
	str << crlf;
}

static void OnModelTreeSelect(HWND hTree, HWND hEdit, const TVITEM& item, const Model* model)
{
	stringstream str;
	unsigned int id = (unsigned int)item.lParam;
	if (id != INVALID_ID)
	{
		unsigned int iMesh = (id >> SHIFT_MESH) & MASK_MESH;
		if (iMesh == MESH_BONE)
		{
			// It's actually a bone
			printBone(hEdit, model, (id >> SHIFT_BONE) & MASK_BONE, str);
		}
		else
		{
			// It's part of a mesh
			const Mesh* mesh  = model->getMesh(iMesh);
			unsigned int mat = (id >> SHIFT_MATERIAL) & MASK_MATERIAL;
			if (mat == MATERIAL_INVALID)
			{
				unsigned int part = (id >> SHIFT_MESHPART) & MASK_MESHPART;
				switch (part)
				{
				case MESHPART_INFORMATION:
				{
					unsigned int iBone = model->getConnection(iMesh);
					const Bone* bone   = model->getBone(iBone);

					str << "Index:          " << iMesh << crlf;
					str << "Name:           " << mesh->getName() << crlf;
					str << "Attached to:    bone #" << iBone << " (" << bone->name << ")" << crlf;
					str << "Hidden:         " << (mesh->isHidden() ? "yes" : "no") << crlf;
					str << "Collision mesh: " << (mesh->isCollisionEnabled() ? "yes" : "no") << crlf;
					break;
				}

				case MESHPART_BOUNDINGBOX:
				{
					D3DXVECTOR3 bb1, bb2;
					mesh->getBoundingBox(bb1, bb2);
					str << fixed << setprecision(3);
					str << "Min X: " << setw(8) << bb1.x << ",   Max X: " << setw(8) << bb2.x << crlf;
					str << "Min Y: " << setw(8) << bb1.y << ",   Max Y: " << setw(8) << bb2.y << crlf;
					str << "Min Z: " << setw(8) << bb1.z << ",   Max Z: " << setw(8) << bb2.z << crlf;
					break;
				}
				}
			}
			else
			{
				const Material& material = mesh->getMaterial(mat);
				unsigned int part = (id >> SHIFT_MATERIALPART) & MASK_MATERIALPART;
				switch (part)
				{
				case MATERIALPART_INVALID:
					// It's the effect itself
					str << "Index: " << mat << crlf;
					break;

				case MATERIALPART_EFFECT:
				{
					int iParam  = (id >> SHIFT_PARAMETER) & MASK_PARAMETER;
					if (iParam == PARAMETER_INVALID)
					{
						// It's the effect itself
						str << "File:  " << material.effect.name << crlf;
					}
					else
					{
						printParameter(hEdit, iParam, material.effect.parameters[iParam], str);
					}
					break;
				}

				case MATERIALPART_VERTICES:
				{
					const Vertex* v = material.vertices;
					str << fixed << setprecision(3);

					str << " Index |          Position          |           Normal           |   Texture Coords  |          Tangent           |          Binormal          | Skin Indices |       Skin Weights      " << crlf;
					str << "-------+----------------------------+----------------------------+-------------------+----------------------------+----------------------------+--------------+-------------------------" << crlf;
					for (unsigned int i = 0; i < material.nVertices; i++, v++)
					{
						str << setw(6) << right << i << " | "; 
						str << setw(8) << v->Position.x << " " << setw(8) << v->Position.y << " " << setw(8) << v->Position.z << " | ";
						str << setw(8) << v->Normal.x   << " " << setw(8) << v->Normal.y   << " " << setw(8) << v->Normal.z   << " | ";
						str << setw(8) << v->TexCoords[0].x << " " << setw(8) << v->TexCoords[0].y                            << " | ";
						str << setw(8) << v->Tangent.x  << " " << setw(8) << v->Tangent.y  << " " << setw(8) << v->Tangent.z  << " | ";
						str << setw(8) << v->Binormal.x << " " << setw(8) << v->Binormal.y << " " << setw(8) << v->Binormal.z << " | ";
						str << setw(2) << v->BoneIndices[0] << " " << setw(2) << v->BoneIndices[1] << " " << setw(2) << v->BoneIndices[2] << " " << setw(2) << v->BoneIndices[3] << "  | ";
						str << setw(5) << v->BoneWeights[0] << " " << setw(5) << v->BoneWeights[1] << " " << setw(5) << v->BoneWeights[2] << " " << setw(5) << v->BoneWeights[3]; 
						str << crlf;
					}
					break;
				}

				case MATERIALPART_TRIANGLES:
				{
					const uint16_t* t = material.indices;
					str << " Index |    Vertex indices   " << crlf;
					str << "-------+---------------------" << crlf;
					for (unsigned int i = 0; i < material.nTriangles; i++, t += 3)
					{
						str << setw(6) << i << " | " << setw(6) << t[0] << " "  << setw(6) << t[1] << " "  << setw(6) << t[2] << crlf;
					}
					break;
				}

				case MATERIALPART_MAPPING:
				{
					str << " Skin | Bone | Bone Name" << crlf;
					str << "------+------+----------------" << crlf;
					for (unsigned int i = 0; i < material.boneMapping.size(); i++)
					{
						unsigned int iBone = material.boneMapping[i];
						const Bone* bone   = model->getBone(iBone);
						str << setw(5) << i << " | " << setw(4) << iBone << " | " << bone->name << crlf;
					}
					break;
				}

				case MATERIALPART_COLLISION:
					break;
				}
			}
		}
	}
	string value = str.str();
	SetWindowText( hEdit, value.c_str() );
}

static void addNode(HWND hTree, const Model* model, HTREEITEM parent, unsigned int idx, const vector<vector<unsigned int> >& nodes )
{
	TVINSERTSTRUCT tvi;
	tvi.hInsertAfter     = TVI_LAST;
	tvi.itemex.mask      = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
	tvi.hParent          = parent;
	tvi.itemex.lParam    = idx;
	tvi.itemex.cChildren = (nodes[idx].size() > 0) ? 1 : 0;
	tvi.itemex.pszText   = (char*)model->getBone(idx)->name.c_str();
	HTREEITEM hNode = TreeView_InsertItem(hTree, &tvi);
	for (vector<unsigned int>::const_iterator i = nodes[idx].begin(); i != nodes[idx].end(); i++)
	{
		addNode(hTree, model, hNode, *i, nodes );
	}
	TreeView_Expand(hTree, hNode, TVE_EXPAND);
}

INT_PTR CALLBACK ModelDetailsDlgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hTab   = GetDlgItem(hWnd, IDC_TAB1);
			HWND hTree1 = GetDlgItem(hWnd, IDC_TREE1);
			HWND hEdit  = GetDlgItem(hWnd, IDC_EDIT1);
			HWND hTree2 = GetDlgItem(hWnd, IDC_TREE2);

			SendMessage(hEdit, WM_SETFONT, (WPARAM)GetStockObject(SYSTEM_FIXED_FONT), TRUE );

			// Initialize tab control
			TCITEM tci;
			tci.mask    = TCIF_TEXT;
			tci.pszText = "File structure"; TabCtrl_InsertItem(hTab, 0, &tci);
			tci.pszText = "Bone hierarchy"; TabCtrl_InsertItem(hTab, 1, &tci);

			// Initialize file structure tree view
			TVINSERTSTRUCT tvi;
			tvi.hParent          = NULL;
			tvi.hInsertAfter     = TVI_LAST;
			tvi.itemex.mask      = TVIF_TEXT | TVIF_CHILDREN | TVIF_PARAM;
			
			tvi.hParent          = NULL;
			tvi.itemex.lParam    = INVALID_ID;
			tvi.itemex.cChildren = 1;
			tvi.itemex.pszText   = "Bones";
			HTREEITEM hBones = TreeView_InsertItem(hTree1, &tvi);

			const Model* model = (const Model*)lParam;
			SetWindowLongPtr(hWnd, GWL_USERDATA, (LONG)(LONG_PTR)model );

			for (unsigned int i = 0; i < model->getNumBones(); i++)
			{
				const Bone* bone = model->getBone(i);
				tvi.hParent          = hBones;
				tvi.itemex.lParam    = MAKE_BONE(i);
				tvi.itemex.cChildren = 0;
				string name = "Bone: " + bone->name;
				tvi.itemex.pszText = (char*)name.c_str();
				HTREEITEM hBone = TreeView_InsertItem(hTree1, &tvi);
			}

			for (unsigned int i = 0; i < model->getNumMeshes(); i++)
			{
				const Mesh* mesh = model->getMesh(i);

				tvi.hParent          = NULL;
				tvi.itemex.lParam    = MAKE_MESH_PART(i, MESHPART_INFORMATION);
				tvi.itemex.cChildren = 1;
				string name = "Mesh: " + mesh->getName();
				tvi.itemex.pszText = (char*)name.c_str();
				HTREEITEM hMesh = TreeView_InsertItem(hTree1, &tvi);

				// Add bounding box
				tvi.hParent = hMesh;
				tvi.itemex.lParam    = MAKE_MESH_PART(i, MESHPART_BOUNDINGBOX);
				tvi.itemex.cChildren = 0;
				tvi.itemex.pszText   = "Bounding Box";
				TreeView_InsertItem(hTree1, &tvi);

				// Add effects
				for (unsigned int j = 0; j < mesh->getNumMaterials(); j++)
				{
					const Material& material = mesh->getMaterial(j);
					tvi.hParent          = hMesh;
					tvi.itemex.cChildren = 1;
					tvi.itemex.lParam    = MAKE_MATERIAL(i,j);
					tvi.itemex.pszText   = "Material";
					HTREEITEM hMaterial = TreeView_InsertItem(hTree1, &tvi);

					name = "Effect: "+ material.effect.name;
					tvi.hParent = hMaterial;
					tvi.itemex.lParam  = MAKE_EFFECT(i,j);
					tvi.itemex.pszText = (char*)name.c_str();
					HTREEITEM hEffect = TreeView_InsertItem(hTree1, &tvi);

					// Add parameters
					for (size_t p = 0; p < material.effect.parameters.size(); p++)
					{
						const Parameter& param = material.effect.parameters[p];

						tvi.hParent          = hEffect;
						tvi.itemex.lParam    = MAKE_PARAMETER(i,j,p);
						tvi.itemex.cChildren = 0;
						name = "Parameter: " + param.name + " (";
						switch (param.type)
						{
							case Parameter::FLOAT:   name += "FLOAT"; break;
							case Parameter::FLOAT3:  name += "FLOAT3"; break;
							case Parameter::FLOAT4:  name += "FLOAT4"; break;
							case Parameter::TEXTURE: name += "TEXTURE"; break;
						}
						name += ")";

						tvi.itemex.pszText = (char*)name.c_str();
						HTREEITEM hParam = TreeView_InsertItem(hTree1, &tvi);
					}
					TreeView_Expand(hTree1, hEffect, TVE_EXPAND);

					// Add vertex format
					name = "Mesh data: " + material.vertexFormat;
					tvi.hParent          = hMaterial;
					tvi.itemex.lParam    = INVALID_ID;
					tvi.itemex.cChildren = 1;
					tvi.itemex.pszText   = (char*)name.c_str();
					HTREEITEM hData = TreeView_InsertItem(hTree1, &tvi);

					stringstream str1;
					str1 << "Vertices (" << material.nVertices << ")";

					tvi.hParent          = hData;
					tvi.itemex.lParam    = MAKE_MATERIAL_PART(i, j, MATERIALPART_VERTICES);
					tvi.itemex.cChildren = 0;
					name = str1.str();
					tvi.itemex.pszText = (char*)name.c_str();
					TreeView_InsertItem(hTree1, &tvi);

					stringstream str2;
					str2 << "Triangles (" << material.nTriangles << ")";
					name = str2.str();
					tvi.itemex.lParam  = MAKE_MATERIAL_PART(i, j, MATERIALPART_TRIANGLES);
					tvi.itemex.pszText = (char*)name.c_str();
					TreeView_InsertItem(hTree1, &tvi);

					if (material.boneMapping.size() > 0)
					{
						stringstream str3;
						str3 << "Bone mappings (" << (unsigned int)material.boneMapping.size() << ")";
						name = str3.str();
						tvi.itemex.lParam    = MAKE_MATERIAL_PART(i, j, MATERIALPART_MAPPING);
						tvi.itemex.pszText = (char*)name.c_str();
						TreeView_InsertItem(hTree1, &tvi);
					}

					// Add collision tree
					if (material.hasCollisionTree)
					{
						tvi.hParent = hData;
						tvi.itemex.cChildren = 0;
						tvi.itemex.lParam    = MAKE_MATERIAL_PART(i, j, MATERIALPART_COLLISION);
						tvi.itemex.pszText   = "Collision tree";
						TreeView_InsertItem(hTree1, &tvi);
					}
					TreeView_Expand(hTree1, hData, TVE_EXPAND);
					TreeView_Expand(hTree1, hMaterial, TVE_EXPAND);
				}
				TreeView_Expand(hTree1, hMesh, TVE_EXPAND);
			}
			TreeView_Expand(hTree1, hBones, TVE_EXPAND);

			// Determine parent-child relations
			vector< vector<unsigned int> > bones(model->getNumBones());
			unsigned int root = -1;
			
			for (unsigned int i = 0; i < model->getNumBones(); i++)
			{
				const Bone* bone = model->getBone(i);
				if (bone->parent == -1) root = i;
				else bones[ bone->parent ].push_back(i);
			}

			// Initialize bone structure window
			if (root != -1)
			{
				addNode(hTree2, model, NULL, root, bones );
			}

			// Resize windows
			SendMessage(hWnd, WM_SIZE, 0, 0);
			return FALSE;
		}

		case WM_NOTIFY:
		{
			NMHDR* nmhdr = (NMHDR*)lParam;
			switch (nmhdr->code)
			{
			case TCN_SELCHANGE:
				{
					HWND hTree1 = GetDlgItem(hWnd, IDC_TREE1);
					HWND hTree2 = GetDlgItem(hWnd, IDC_TREE2);
					int sel = TabCtrl_GetCurSel(nmhdr->hwndFrom);
					ShowWindow(hTree1, (sel == 0) ? SW_SHOW : SW_HIDE);
					ShowWindow(hTree2, (sel == 0) ? SW_HIDE : SW_SHOW);
				}
				break;

			case TVN_SELCHANGED:
				{
					NMTREEVIEW* nmtv = (NMTREEVIEW*)lParam;
					HWND hTree1  = GetDlgItem(hWnd, IDC_TREE1);
					HWND hTree2  = GetDlgItem(hWnd, IDC_TREE2);
					HWND hEdit   = GetDlgItem(hWnd, IDC_EDIT1);
					Model* model = (Model*)(LONG_PTR)GetWindowLongPtr(hWnd, GWL_USERDATA);	
					if (nmhdr->hwndFrom == hTree1)
					{
						OnModelTreeSelect(hTree1, hEdit, nmtv->itemNew, model);
					}
					else
					{
						int iBone = (int)nmtv->itemNew.lParam;
						stringstream str;
						printBone(hEdit, model, iBone, str);
						string value = str.str();
						SetWindowText( hEdit, value.c_str() );
					}
				}
				break;
			}
		}

		case WM_SIZE:
		{
			HWND hTab   = GetDlgItem(hWnd, IDC_TAB1);
			HWND hTree1 = GetDlgItem(hWnd, IDC_TREE1);
			HWND hTree2 = GetDlgItem(hWnd, IDC_TREE2);
			HWND hEdit  = GetDlgItem(hWnd, IDC_EDIT1);
			HWND hBtn   = GetDlgItem(hWnd, IDCANCEL);
			
			RECT window, button, tab;
			GetClientRect(hWnd, &window);
			GetClientRect(hBtn, &button);
			MoveWindow(hTab, 8, 8, window.right / 2 - 8, window.bottom - 8 - button.bottom * 2, TRUE );
			MoveWindow(hBtn, (window.right - button.right) / 2, window.bottom - button.bottom - button.bottom / 2, button.right, button.bottom, TRUE );

			GetClientRect(hTab, &tab);

			RECT display = tab;
			TabCtrl_AdjustRect(hTab, FALSE, &display);
			
			POINT pos = {display.left, display.top};
			ClientToScreen(hTab, &pos);
			ScreenToClient(hWnd, &pos);

			MoveWindow(hEdit, window.right / 2 + 4, pos.y, window.right / 2 - 12, tab.bottom - display.top, TRUE );

			MoveWindow(hTree1, pos.x + 4, pos.y + 4, tab.right - display.left - 12, tab.bottom - display.top - 12, TRUE);
			MoveWindow(hTree2, pos.x + 4, pos.y + 4, tab.right - display.left - 12, tab.bottom - display.top - 12, TRUE);
			break;
		}

		case WM_COMMAND:
		{
			WORD code = HIWORD(wParam);
			WORD id   = LOWORD(wParam);
			if (code == BN_CLICKED && (id == IDOK || id == IDCANCEL))
			{
				EndDialog(hWnd, 0);
			}
			return TRUE;
		}
	}
	return FALSE;
}
