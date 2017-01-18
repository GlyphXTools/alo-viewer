#include "Dialogs/Dialogs.h"
#include "Assets/Models.h"
#include "General/Utils.h"
#include "General/WinUtils.h"
#include <commctrl.h>
#include "resource.h"
#include <sstream>
using namespace Alamo;
using namespace std;

// ModelPath is a class that encodes a path in the model into a single 
// integer. This way it can be set as user-data for tree nodes.
class ModelPath
{
    unsigned long m_value;
public:
    // Model path type
    static const int NOTHING = 0;
    static const int BONE    = 1;
    static const int MESH    = 2;
    static const int LIGHT   = 3;static const int PROXY   = 4;
    static const int DAZZLE  = 5;

    static const int EFFECT      = 1;
    static const int VERTICES    = 3;
    static const int FACES       = 4;
    static const int COLLISION   = 5;
    static const int BONEMAPPING = 6;

    int GetType()                     const { return m_value & 7; }
    unsigned long GetBone()           const { return m_value >> 3; }
    unsigned long GetProxy()          const { return m_value >> 3; }
    unsigned long GetDazzle()         const { return m_value >> 3; }
    unsigned long GetLight()          const { return m_value >> 3; }
    unsigned long GetMesh()           const { return (m_value >>  3) & 0x1FFF; }
    unsigned long GetSubMesh()        const { return (m_value >> 16) & 0x3F; }
    bool          IsSubMesh()         const { return GetSubMesh() != 0x3F; }
    int           GetMeshObjectType() const { return (m_value >> 22) & 7; }
    unsigned long GetParameter()      const { return (m_value >> 25) & 0x7F; }

    ModelPath& SetBone(unsigned long v)      { m_value = (v << 3) | BONE;   return *this; }
    ModelPath& SetProxy(unsigned long v)     { m_value = (v << 3) | PROXY;  return *this; }
    ModelPath& SetDazzle(unsigned long v)    { m_value = (v << 3) | DAZZLE; return *this; }
    ModelPath& SetLight(unsigned long v)     { m_value = (v << 3) | LIGHT;  return *this; }
    ModelPath& SetMesh(unsigned long v)      { m_value = (0x3F << 16) | (v << 3) | MESH;  return *this; }
    ModelPath& SetSubMesh(unsigned long v)   { m_value = (m_value & ~(0x3F << 16)) | (v << 16); return *this; }
    ModelPath& SetMeshObjectType(int t)      { m_value |= t << 22; return *this; }
    ModelPath& SetParameter(unsigned long v) { m_value |= (v << 25) | (EFFECT << 22); return *this; }

    unsigned long GetValue() { return m_value; }
    ModelPath(unsigned long val = 0) : m_value(val) {}
};

struct ColumnInfo
{
    wstring name;
    int     width;
};

static INT_PTR CALLBACK StaticDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return FALSE;
}

static INT_PTR CALLBACK FluidDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            RECT client;
            GetClientRect(hWnd, &client);
            SetWindowLongPtr(hWnd, GWLP_USERDATA, MAKELONG(client.right, client.bottom));
            for (HWND hChild = GetWindow(hWnd, GW_CHILD); hChild != NULL; hChild = GetNextWindow(hChild, GW_HWNDNEXT))
            {
                TCHAR name[64];
                if (GetClassName(hChild, name, 64) != 0 && wcscmp(name,WC_LISTVIEW) == 0)
                {
                    ListView_SetExtendedListViewStyle(hChild, ListView_GetExtendedListViewStyle(hChild) | LVS_EX_FULLROWSELECT);
                }
            }

            // Resize to container size
            GetClientRect(GetParent(hWnd), &client);
            SetWindowPos(hWnd, NULL, 0, 0, client.right, client.bottom, SWP_NOZORDER | SWP_NOMOVE);
            break;
        }

        case WM_SIZE:
        {
            LONG old = GetWindowLongPtr(hWnd, GWLP_USERDATA);
            int dx = (int)LOWORD(lParam) - (int)LOWORD(old);
            int dy = (int)HIWORD(lParam) - (int)HIWORD(old);
            for (HWND hChild = GetWindow(hWnd, GW_CHILD); hChild != NULL; hChild = GetNextWindow(hChild, GW_HWNDNEXT))
            {
                // Resize control
                RECT size;
                GetWindowRect(hChild, &size);
                ScreenToClient(hWnd, &size);
                MoveWindow(hChild, size.left, size.top, size.right - size.left + dx, size.bottom - size.top + dy, TRUE);
            }
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)lParam);
            break;
        }
    }
    return FALSE;
}


template <typename T>
static wstring val2str(const T& val)
{
    wstringstream ss;
    ss << val;
    return ss.str();
}

template <>
static wstring val2str(const Vector2& val)
{
    wstringstream ss;
    ss.precision(1);
    ss << fixed << L"[" << val.x << L", " << val.y << L"]";
    return ss.str();
}

template <>
static wstring val2str(const Vector3& val)
{
    wstringstream ss;
    ss.precision(1);
    ss << fixed << L"[" << val.x << L", " << val.y << L", " << val.z << L"]";
    return ss.str();
}

template <>
static wstring val2str(const Vector4& val)
{
    wstringstream ss;
    ss.precision(1);
    ss << fixed << L"[" << val.x << L", " << val.y << L", " << val.z << L", " << val.w << L"]";
    return ss.str();
}

static wstring val2str(const Color& val)
{
    return val2str(Vector4(val));
}

static wstring val2str(const DWORD* val)
{
    wstringstream ss;
    ss << fixed << L"[" << val[0] << L", " << val[1] << L", " << val[2] << L", " << val[3] << L"]";
    return ss.str();
}

static HWND ShowBone(HWND hWnd, const Model::Bone& bone)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_BONE), hContainer, StaticDialogProc, NULL);

    stringstream parent, index, rtr[4], atr[4];
    index << bone.index;
    for (int i = 0; i < 4; i++)
    {
        rtr[i].precision(3);
        Vector4 row = bone.relTransform.row(i);
        rtr[i] << fixed << "[" << row.x << ", " << row.y << ", " << row.z << ", " << row.w << "]";
        
        atr[i].precision(3);
        row = bone.absTransform.row(i);
        atr[i] << fixed << "[" << row.x << ", " << row.y << ", " << row.z << ", " << row.w << "]";
    }

    if (bone.parent != NULL) {
        parent << bone.parent->name << " (#" << bone.parent->index << ")";
    } else {
        parent << "-";
    }

    const wstring str_yes = LoadString(IDS_YES);
    const wstring str_no  = LoadString(IDS_NO);

    string temp = index.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_INDEX),   (LPCSTR)temp.c_str());
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME),    bone.name.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_VISIBLE), bone.visible ? str_yes.c_str() : str_no.c_str());
    temp = parent.str(); SetWindowTextA(GetDlgItem(hDlg, IDC_PARENT),         (LPCSTR)temp.c_str());
    temp = rtr[0].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_REL_TRANSFORM1), (LPCSTR)temp.c_str());
    temp = rtr[1].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_REL_TRANSFORM2), (LPCSTR)temp.c_str());
    temp = rtr[2].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_REL_TRANSFORM3), (LPCSTR)temp.c_str());
    temp = rtr[3].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_REL_TRANSFORM4), (LPCSTR)temp.c_str());
    temp = atr[0].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_ABS_TRANSFORM1), (LPCSTR)temp.c_str());
    temp = atr[1].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_ABS_TRANSFORM2), (LPCSTR)temp.c_str());
    temp = atr[2].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_ABS_TRANSFORM3), (LPCSTR)temp.c_str());
    temp = atr[3].str();  SetWindowTextA(GetDlgItem(hDlg, IDC_ABS_TRANSFORM4), (LPCSTR)temp.c_str());
    return hDlg;
}

static HWND ShowMesh(HWND hWnd, const Model::Mesh& mesh)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_MESH), hContainer, StaticDialogProc, NULL);

    stringstream bone, bounds;
    if (mesh.bone != NULL) {
        bone << mesh.bone->name << " (#" << mesh.bone->index << ")";
    } else {
        bone << "-";
    }
    bounds.precision(3);
    bounds << fixed << "[" << mesh.bounds.min.x << ", " << mesh.bounds.min.y << ", " << mesh.bounds.min.z << "] - [" << mesh.bounds.max.x << ", " << mesh.bounds.max.y << ", " << mesh.bounds.max.z << "]";

    const wstring str_yes = LoadString(IDS_YES);
    const wstring str_no  = LoadString(IDS_NO);

    string temp = bone.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME),       mesh.name.c_str());
    SetWindowTextA(GetDlgItem(hDlg, IDC_BONE),       (LPCSTR)temp.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_VISIBLE),    mesh.isVisible    ? str_yes.c_str() : str_no.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_COLLIDABLE), mesh.isCollidable ? str_yes.c_str() : str_no.c_str());
    temp = bounds.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_BOUNDINGBOX), (LPCSTR)temp.c_str());
    return hDlg;
}

static HWND ShowParameter(HWND hWnd, const ShaderParameter& param)
{
    const wstring ParamTypes[] = {
        LoadString(IDS_DETAILS_PARAM_INTEGER),
        LoadString(IDS_DETAILS_PARAM_FLOAT),
        LoadString(IDS_DETAILS_PARAM_FLOAT3),
        LoadString(IDS_DETAILS_PARAM_FLOAT4),
        LoadString(IDS_DETAILS_PARAM_TEXTURE)
    };
    
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_PARAMETER), hContainer, StaticDialogProc, NULL);
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME), param.m_name.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_TYPE), ParamTypes[param.m_type].c_str());
    
    stringstream value;
    switch (param.m_type)
    {
    case SPT_INT:     value << param.m_int; break;
    case SPT_FLOAT:   value << param.m_float; break;
    case SPT_FLOAT3:  value << "[" << param.m_float3.x << ", " << param.m_float3.y << ", " << param.m_float3.z << "]"; break;
    case SPT_FLOAT4:  value << "[" << param.m_float4.x << ", " << param.m_float4.y << ", " << param.m_float4.z << ", " << param.m_float4.w << "]"; break;
    case SPT_TEXTURE: value << param.m_texture; break;
    }

    string temp = value.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_VALUE), temp.c_str());
    return hDlg;
}

static HWND ShowVertices(HWND hWnd, const Model::SubMesh& submesh)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_VERTICES), hContainer, FluidDialogProc, NULL);

    const ColumnInfo columns[] = {
        {L"#",                                       30},
        {LoadString(IDS_DETAILS_VERT_POSITION),     130},
        {LoadString(IDS_DETAILS_VERT_NORMAL),       100},
        {LoadString(IDS_DETAILS_VERT_TEXCOORDS, 1), 100},
        {LoadString(IDS_DETAILS_VERT_TEXCOORDS, 2),  60},
        {LoadString(IDS_DETAILS_VERT_TEXCOORDS, 3),  60},
        {LoadString(IDS_DETAILS_VERT_TEXCOORDS, 4),  60},
        {LoadString(IDS_DETAILS_VERT_TANGENT),      100},
        {LoadString(IDS_DETAILS_VERT_BINORMAL),     100},
        {LoadString(IDS_DETAILS_VERT_COLOR),        110},
        {LoadString(IDS_DETAILS_VERT_INDICES),       80},
        {LoadString(IDS_DETAILS_VERT_WEIGHTS),      110},
        {L"", 0}
    };

    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Create columns
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    for (int i = 0; columns[i].width > 0; i++)
    {
        LVCOLUMN column;
        column.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText  = (LPWSTR)columns[i].name.c_str();
        column.cx       = columns[i].width;
        column.iSubItem = i;
        ListView_InsertColumn(hList, i, &column);
    }

    // Fill the list
    LV_ITEM item;
    item.mask = LVIF_TEXT;
    for (item.iItem = 0; item.iItem < (int)submesh.vertices.size(); item.iItem++)
    {
        const MASTER_VERTEX& v = submesh.vertices[item.iItem];
        wstring temp;
        temp = val2str(item.iItem); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 0; ListView_InsertItem(hList, &item);
        temp = val2str(v.Position); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 1; ListView_SetItem(hList, &item);
        temp = val2str(v.Normal);   item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 2; ListView_SetItem(hList, &item);
        temp = val2str(v.TexCoord[0]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 3; ListView_SetItem(hList, &item);
        temp = val2str(v.TexCoord[1]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 4; ListView_SetItem(hList, &item);
        temp = val2str(v.TexCoord[2]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 5; ListView_SetItem(hList, &item);
        temp = val2str(v.TexCoord[3]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 6; ListView_SetItem(hList, &item);
        temp = val2str(v.Tangent);  item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 7; ListView_SetItem(hList, &item);
        temp = val2str(v.Binormal); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 8; ListView_SetItem(hList, &item);
        temp = val2str(Vector4(v.Color));       item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 9; ListView_SetItem(hList, &item);
        temp = val2str(v.BoneIndices);          item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 10; ListView_SetItem(hList, &item);
        temp = val2str(Vector4(v.BoneWeights)); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 11; ListView_SetItem(hList, &item);
    }
    SetCursor(hOldCursor);

    return hDlg; 
}

static HWND ShowBoneMapping(HWND hWnd, const Model::SubMesh& submesh, const Model& model)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_BONEMAPPING), hContainer, FluidDialogProc, NULL);

    const ColumnInfo columns[] = {
        {L"#",                                  30},
        {LoadString(IDS_DETAILS_MAPPING_BONE),  40},
        {LoadString(IDS_DETAILS_MAPPING_NAME), 200},
        {L"", 0}
    };

    // Create columns
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    for (int i = 0; columns[i].width > 0; i++)
    {
        LVCOLUMN column;
        column.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText  = (LPWSTR)columns[i].name.c_str();
        column.cx       = columns[i].width;
        column.iSubItem = i;
        ListView_InsertColumn(hList, i, &column);
    }

    // Fill the list
    LV_ITEM item;
    item.mask = LVIF_TEXT;
    for (item.iItem = 0; item.iItem < (int)submesh.nSkinBones; item.iItem++)
    {
        wstring temp;
        temp = val2str(item.iItem);                                      item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 0; ListView_InsertItem(hList, &item);
        temp = val2str(submesh.skin[item.iItem]);                        item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 1; ListView_SetItem(hList, &item);
        temp = AnsiToWide(model.GetBone(submesh.skin[item.iItem]).name); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 2; ListView_SetItem(hList, &item);
    }

    return hDlg; 
}

static HWND ShowFaces(HWND hWnd, const Model::SubMesh& submesh)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_FACES), hContainer, FluidDialogProc, NULL);

    const ColumnInfo columns[] = {
        {L"#",  30},
        {LoadString(IDS_DETAILS_FACES_VERT, 1), 40},
        {LoadString(IDS_DETAILS_FACES_VERT, 2), 40},
        {LoadString(IDS_DETAILS_FACES_VERT, 3), 40},
        {L"", 0}
    };

    HCURSOR hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Create columns
    HWND hList = GetDlgItem(hDlg, IDC_LIST1);
    for (int i = 0; columns[i].width > 0; i++)
    {
        LVCOLUMN column;
        column.mask     = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText  = (LPWSTR)columns[i].name.c_str();
        column.cx       = columns[i].width;
        column.iSubItem = i;
        ListView_InsertColumn(hList, i, &column);
    }

    // Fill the list
    LV_ITEM item;
    item.mask = LVIF_TEXT;
    for (item.iItem = 0; item.iItem < (int)submesh.indices.size() / 3; item.iItem++)
    {
        wstring temp;
        temp = val2str(item.iItem);                        item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 0; ListView_InsertItem(hList, &item);
        temp = val2str(submesh.indices[3*item.iItem + 0]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 1; ListView_SetItem(hList, &item);
        temp = val2str(submesh.indices[3*item.iItem + 1]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 2; ListView_SetItem(hList, &item);
        temp = val2str(submesh.indices[3*item.iItem + 2]); item.pszText = (LPWSTR)temp.c_str(); item.iSubItem = 3; ListView_SetItem(hList, &item);
    }
    SetCursor(hOldCursor);

    return hDlg; 
}

static HWND ShowLight(HWND hWnd, const Model::Light& light)
{
    const wstring LightTypes[] = {
        LoadString(IDS_DETAILS_LIGHT_OMNI),
        LoadString(IDS_DETAILS_LIGHT_DIRECTIONAL),
        LoadString(IDS_DETAILS_LIGHT_SPOT)
    };

    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_LIGHT), hContainer, StaticDialogProc, NULL);
 
    stringstream  bone, intensity, color, farAtten;
    wstringstream falloff, hotspot;
    if (light.bone != NULL) {
        bone << light.bone->name << " (#" << light.bone->index << ")";
    } else {
        bone << "-";
    }

    farAtten.precision(2);
    intensity << (int)(light.intensity * 100) << "%";
    color     << "[" << (int)(light.color.r * 255) << ", " << (int)(light.color.g * 255) << ", " << (int)(light.color.b * 255) << "]";
    falloff   << ToDegrees(light.falloffSize) << L"\x00B0";
    hotspot   << ToDegrees(light.hotspotSize) << L"\x00B0";
    farAtten  << fixed << light.farAttenuationStart << " - " << light.farAttenuationEnd;

    string temp;
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME), light.name.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_TYPE), (LPWSTR)LightTypes[light.type].c_str());
    temp = bone.str();      SetWindowTextA(GetDlgItem(hDlg, IDC_BONE),           (LPCSTR)temp.c_str());
    temp = color.str();     SetWindowTextA(GetDlgItem(hDlg, IDC_COLOR),          (LPCSTR)temp.c_str());
    temp = intensity.str(); SetWindowTextA(GetDlgItem(hDlg, IDC_INTENSITY),      (LPCSTR)temp.c_str());
    temp = farAtten.str();  SetWindowTextA(GetDlgItem(hDlg, IDC_FARATTENUATION), (LPCSTR)temp.c_str());
    wstring wtemp;
    wtemp = falloff.str();  SetWindowTextW(GetDlgItem(hDlg, IDC_FALLOFFSIZE),    (LPWSTR)wtemp.c_str());
    wtemp = hotspot.str();  SetWindowTextW(GetDlgItem(hDlg, IDC_HOTSPOTSIZE),    (LPWSTR)wtemp.c_str());
    return hDlg;
}

static HWND ShowProxy(HWND hWnd, const Model::Proxy& proxy)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_PROXY), hContainer, StaticDialogProc, NULL);
 
    stringstream bone;
    if (proxy.bone != NULL) {
        bone << proxy.bone->name << " (#" << proxy.bone->index << ")";
    } else {
        bone << "-";
    }

    const wstring str_yes = LoadString(IDS_YES);
    const wstring str_no  = LoadString(IDS_NO);

    string temp = bone.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME),             proxy.name.c_str());
    SetWindowTextA(GetDlgItem(hDlg, IDC_BONE),             (LPCSTR)temp.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_VISIBLE),          proxy.isVisible             ? str_yes.c_str() : str_no.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_ALTDECSTAYHIDDEN), proxy.altDecreaseStayHidden ? str_yes.c_str() : str_no.c_str());
    return hDlg;
}

static HWND ShowDazzle(HWND hWnd, const Model::Dazzle& dazzle)
{
    HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);
    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS_DAZZLE), hContainer, StaticDialogProc, NULL);
 
    stringstream bone, texture;
    wstringstream timing;
    if (dazzle.bone != NULL) {
        bone << dazzle.bone->name << " (#" << dazzle.bone->index << ")";
    } else {
        bone << "-";
    }
    texture << dazzle.texture << "; (" <<
        (dazzle.texX + 0.0f) / dazzle.texSize << "," << (dazzle.texY + 0.0f) / dazzle.texSize << ") - (" << 
        (dazzle.texX + 1.0f) / dazzle.texSize << "," << (dazzle.texY + 1.0f) / dazzle.texSize << ")";
    timing << "(" << dazzle.frequency << " Hz + " << dazzle.phase << L" \x03BB) < " << dazzle.bias * 100 << "%";

    const wstring str_yes = LoadString(IDS_YES);
    const wstring str_no  = LoadString(IDS_NO);

    string temp = bone.str();
    SetWindowTextA(GetDlgItem(hDlg, IDC_NAME),     dazzle.name.c_str());
    SetWindowTextA(GetDlgItem(hDlg, IDC_BONE),     (LPCSTR)temp.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_VISIBLE),   dazzle.isVisible ? str_yes.c_str() : str_no.c_str());
    SetWindowText(GetDlgItem(hDlg, IDC_NIGHTONLY), dazzle.nightOnly ? str_yes.c_str() : str_no.c_str());
    temp = texture.str(); SetWindowTextA(GetDlgItem(hDlg, IDC_TEXTURE),  (LPCSTR)temp.c_str());
    wstring wtemp;
    wtemp = timing.str();             SetWindowText(GetDlgItem(hDlg, IDC_TIMING),   (LPTSTR)wtemp.c_str());
    wtemp = val2str(dazzle.position); SetWindowText(GetDlgItem(hDlg, IDC_POSITION), (LPTSTR)wtemp.c_str());
    wtemp = val2str(dazzle.radius);   SetWindowText(GetDlgItem(hDlg, IDC_RADIUS),   (LPTSTR)wtemp.c_str());
    wtemp = val2str(dazzle.color);    SetWindowText(GetDlgItem(hDlg, IDC_COLOR),    (LPTSTR)wtemp.c_str());
    return hDlg;
}

static HWND ShowModelItem(HWND hWnd, const Model& model, const ModelPath& path)
{
    switch (path.GetType())
    {
    case ModelPath::BONE:
        return ShowBone(hWnd, model.GetBone(path.GetBone()));

    case ModelPath::PROXY:
        return ShowProxy(hWnd, model.GetProxy(path.GetProxy()));

    case ModelPath::DAZZLE:
        return ShowDazzle(hWnd, model.GetDazzle(path.GetDazzle()));

    case ModelPath::LIGHT:
        return ShowLight(hWnd, model.GetLight(path.GetLight()));

    case ModelPath::MESH:
        if (!path.IsSubMesh())
        {
            return ShowMesh(hWnd, model.GetMesh(path.GetMesh()));
        }
        
        switch (path.GetMeshObjectType())
        {
        case ModelPath::VERTICES:
            return ShowVertices(hWnd, model.GetMesh(path.GetMesh()).subMeshes[path.GetSubMesh()]);
        
        case ModelPath::FACES:
            return ShowFaces(hWnd, model.GetMesh(path.GetMesh()).subMeshes[path.GetSubMesh()]);

        case ModelPath::BONEMAPPING:
            return ShowBoneMapping(hWnd, model.GetMesh(path.GetMesh()).subMeshes[path.GetSubMesh()], model);

        case ModelPath::EFFECT:
            return ShowParameter(hWnd, model.GetMesh(path.GetMesh()).subMeshes[path.GetSubMesh()].parameters[path.GetParameter()]);
        }
        break;
    }
    return NULL;
}

static void FillSubMesh(HWND hTree, HTREEITEM hParent, const Model::SubMesh& submesh, const ModelPath& parent)
{
    TVINSERTSTRUCT tvis;
    tvis.hParent        = hParent;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvis.item.cChildren = 0;

    for (unsigned long i = 0; i < submesh.parameters.size(); i++)
    {
        wstring   name = LoadString(IDS_DETAILS_PARAMETER, submesh.parameters[i].m_name.c_str());
        ModelPath path = ModelPath(parent).SetParameter(i);

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.lParam    = path.GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }

    wstring temp = LoadString(IDS_DETAILS_VERTICES, submesh.vertices.size());
    tvis.item.pszText   = (LPWSTR)temp.c_str();
    tvis.item.lParam    = ModelPath(parent).SetMeshObjectType(ModelPath::VERTICES).GetValue();
    TreeView_InsertItem(hTree, &tvis);

    temp = LoadString(IDS_DETAILS_FACES, submesh.indices.size() / 3);
    tvis.item.pszText   = (LPWSTR)temp.c_str();
    tvis.item.lParam    = ModelPath(parent).SetMeshObjectType(ModelPath::FACES).GetValue();
    TreeView_InsertItem(hTree, &tvis);

    if (submesh.nSkinBones > 0)
    {
        wstringstream skinbones;
        skinbones << LoadString(IDS_DETAILS_BONE_MAPPING, submesh.nSkinBones);

        temp = skinbones.str();
        tvis.item.pszText = (LPWSTR)temp.c_str();
        tvis.item.lParam  = ModelPath(parent).SetMeshObjectType(ModelPath::BONEMAPPING).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }
}

static void FillMesh(HWND hTree, HTREEITEM hParent, const Model::Mesh& mesh, const ModelPath& parent)
{
    TVINSERTSTRUCT tvis;
    tvis.hParent        = hParent;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvis.item.cChildren = 1;

    // Create a node for each sub mesh
    for (unsigned long i = 0; i < mesh.subMeshes.size(); i++)
    {
        wstring   name = LoadString(IDS_DETAILS_MATERIAL, mesh.subMeshes[i].shader.c_str());
        ModelPath path = ModelPath(parent).SetSubMesh(i);

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 1;
        tvis.item.lParam    = path.GetValue();
        HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
        FillSubMesh(hTree, hItem, mesh.subMeshes[i], path);
        TreeView_Expand(hTree, hItem, TVE_EXPAND);
    }
}

static void FillSkeleton(HWND hTree, HTREEITEM hParent, const Model& model)
{
    TVINSERTSTRUCT tvis;
    tvis.hParent        = hParent;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvis.item.cChildren = 0;

    // Create a node for each bone
    for (unsigned long i = 0; i < model.GetNumBones(); i++)
    {
        wstring name = LoadString(IDS_DETAILS_BONE, model.GetBone(i).name.c_str());

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.lParam    = ModelPath().SetBone(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }
}

static void FillTreeStructure(HWND hTree, const Model& model)
{
    wstring temp = LoadString(IDS_DETAILS_SKELETON);

    TreeView_DeleteAllItems(hTree);

    // Create a node for the skeleton
    TVINSERTSTRUCT tvis;
    tvis.hParent        = NULL;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvis.item.pszText   = (LPWSTR)temp.c_str();
    tvis.item.cChildren = 1;
    tvis.item.lParam    = ModelPath().GetValue();
    
    HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
    FillSkeleton(hTree, hItem, model);
    TreeView_Expand(hTree, hItem, TVE_EXPAND);

    // Create a node for each mesh
    for (unsigned long i = 0; i < model.GetNumMeshes(); i++)
    {
        wstring   name = LoadString(IDS_DETAILS_MESH, model.GetMesh(i).name.c_str());
        ModelPath path = ModelPath().SetMesh(i);

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 1;
        tvis.item.lParam    = ModelPath().SetMesh(i).GetValue();
        HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
        FillMesh(hTree, hItem, model.GetMesh(i), path);
        TreeView_Expand(hTree, hItem, TVE_EXPAND);
    }

    // Create a node for each light
    for (unsigned long i = 0; i < model.GetNumLights(); i++)
    {
        wstring name = LoadString(IDS_DETAILS_LIGHT, model.GetLight(i).name.c_str());

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetLight(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }

    // Create a node for each proxy
    for (unsigned long i = 0; i < model.GetNumProxies(); i++)
    {
        wstring name = LoadString(IDS_DETAILS_PROXY, model.GetProxy(i).name.c_str());

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetProxy(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }

    // Create a node for each dazzle
    for (unsigned long i = 0; i < model.GetNumDazzles(); i++)
    {
        wstring name = LoadString(IDS_DETAILS_DAZZLE, model.GetDazzle(i).name.c_str());

        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetDazzle(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }
}

static void FillTreeHierarchy(HWND hTree, const Model& model)
{
    TreeView_DeleteAllItems(hTree);

    // Determine which bones have children
    vector<bool> children(model.GetNumBones(), false);
    for (size_t i = 0; i < model.GetNumBones(); i++) {
        if (model.GetBone(i).parent != NULL) {
            children[model.GetBone(i).parent->index] = true;
        }
    }

    for (size_t i = 0; i < model.GetNumMeshes(); i++) {
        children[model.GetMesh(i).bone->index] = true;
    }

    for (size_t i = 0; i < model.GetNumProxies(); i++) {
        children[model.GetProxy(i).bone->index] = true;
    }

    for (size_t i = 0; i < model.GetNumDazzles(); i++) {
        children[model.GetDazzle(i).bone->index] = true;
    }

    for (size_t i = 0; i < model.GetNumLights(); i++) {
        children[model.GetLight(i).bone->index] = true;
    }

    TVINSERTSTRUCT tvis;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_CHILDREN;
    tvis.item.cChildren = 0;

    // Create a node for each bone
    vector<HTREEITEM> hBones(model.GetNumBones());
    for (unsigned long i = 0; i < model.GetNumBones(); i++)
    {
        const Model::Bone& bone = model.GetBone(i);
        wstring name = LoadString(IDS_DETAILS_BONE, bone.name.c_str());

        tvis.hParent        = (bone.parent != NULL) ? hBones[bone.parent->index] : NULL;
        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.lParam    = ModelPath().SetBone(i).GetValue();
        tvis.item.cChildren = (children[i] ? 1 : 0);
        hBones[i] = TreeView_InsertItem(hTree, &tvis);
    }

    // Create a node for each mesh
    for (unsigned long i = 0; i < model.GetNumMeshes(); i++)
    {
        const Model::Mesh& mesh = model.GetMesh(i);
        wstring   name = LoadString(IDS_DETAILS_MESH, mesh.name.c_str());
        ModelPath path = ModelPath().SetMesh(i);

        tvis.hParent        = hBones[mesh.bone->index];
        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 1;
        tvis.item.lParam    = ModelPath().SetMesh(i).GetValue();
        HTREEITEM hItem = TreeView_InsertItem(hTree, &tvis);
        FillMesh(hTree, hItem, mesh, path);
        TreeView_Expand(hTree, hItem, TVE_EXPAND);
    }

    // Create a node for each light
    for (unsigned long i = 0; i < model.GetNumLights(); i++)
    {
        const Model::Light& light = model.GetLight(i);
        wstring name = LoadString(IDS_DETAILS_LIGHT, light.name.c_str());

        tvis.hParent        = hBones[light.bone->index];
        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetLight(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }

    // Create a node for each proxy
    for (unsigned long i = 0; i < model.GetNumProxies(); i++)
    {
        const Model::Proxy& proxy = model.GetProxy(i);
        wstring name = LoadString(IDS_DETAILS_PROXY, proxy.name.c_str());

        tvis.hParent        = hBones[proxy.bone->index];
        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetProxy(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }
    
    // Create a node for each dazzle
    for (unsigned long i = 0; i < model.GetNumDazzles(); i++)
    {
        const Model::Dazzle& dazzle = model.GetDazzle(i);
        wstring name = LoadString(IDS_DETAILS_DAZZLE, dazzle.name.c_str());

        tvis.hParent        = hBones[dazzle.bone->index];
        tvis.item.pszText   = (LPWSTR)name.c_str();
        tvis.item.cChildren = 0;
        tvis.item.lParam    = ModelPath().SetDazzle(i).GetValue();
        TreeView_InsertItem(hTree, &tvis);
    }
    
    // Expand all bones
    for (unsigned long i = 0; i < model.GetNumBones(); i++)
    {
        TreeView_Expand(hTree, hBones[i], TVE_EXPAND);
    }
}

struct DetailsInfo
{
    const Model* model;
    POINT        minSize;
    HWND         hItemDialog;
};

static HWND g_DetailsWindow = NULL;

static INT_PTR CALLBACK DetailsProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DetailsInfo* info = (DetailsInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        DetailsInfo* info = new DetailsInfo;
        info->model       = (const Model*)lParam;
        info->hItemDialog = NULL;
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)info);

        // Add tabs
        HWND hTabs = GetDlgItem(hWnd, IDC_TAB1);
        TCITEM item;
        item.mask    = TCIF_TEXT;
        wstring temp;
        temp = LoadString(IDS_DETAILS_STRUCTURE); item.pszText = (LPWSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 0, &item);
        temp = LoadString(IDS_DETAILS_HIERARCHY); item.pszText = (LPWSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 1, &item);

        HWND hTree = GetDlgItem(hWnd, IDC_TREE1);
        FillTreeStructure(hTree, *info->model);
        TreeView_EnsureVisible(hTree, TreeView_GetRoot(hTree));
        
        hTree = GetDlgItem(hWnd, IDC_TREE2);
        FillTreeHierarchy(hTree, *info->model);
        TreeView_EnsureVisible(hTree, TreeView_GetRoot(hTree));
        ShowWindow(hTree, SW_HIDE);

        // Remember current size as minimum size
        RECT size;
        GetWindowRect(hWnd, &size);
        info->minSize.x = size.right  - size.left;
        info->minSize.y = size.bottom - size.top;

        // Trigger WM_SIZE to move all components into their positions
        SetWindowPos(hWnd, NULL, 0, 0, info->minSize.x, info->minSize.y, SWP_NOZORDER | SWP_NOMOVE);
        return TRUE;
    }

    case WM_NOTIFY:
    {
        NMHDR* hdr = (NMHDR*)lParam;
        switch (hdr->code)
        {
        case TVN_SELCHANGING:
            // Destroy the old item dialog
            DestroyWindow(info->hItemDialog);
            info->hItemDialog = NULL;
            break;

        case TVN_SELCHANGED:
        {
            // Create the new item dialog
            NMTREEVIEW* nmtv = (NMTREEVIEW*)hdr;
            info->hItemDialog = ShowModelItem(hWnd, *info->model, ModelPath((unsigned long)nmtv->itemNew.lParam) );
            ShowWindow(info->hItemDialog, SW_SHOWNORMAL);
            RedrawWindow(hdr->hwndFrom, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
            break;
        }

        case TCN_SELCHANGE:
            // Select the right tab
            if (TabCtrl_GetCurSel(hdr->hwndFrom) == 1) {
                ShowWindow(GetDlgItem(hWnd, IDC_TREE1), SW_HIDE);
                ShowWindow(GetDlgItem(hWnd, IDC_TREE2), SW_SHOWNORMAL);
            } else {
                ShowWindow(GetDlgItem(hWnd, IDC_TREE1), SW_SHOWNORMAL);
                ShowWindow(GetDlgItem(hWnd, IDC_TREE2), SW_HIDE);
            }
            break;
        }
        break;
    }

	case WM_SIZING:
	{
        // Restrict the size to at least the minimum sizes
		RECT* rect = (RECT*)lParam;
		bool left  = (wParam == WMSZ_BOTTOMLEFT) || (wParam == WMSZ_LEFT) || (wParam == WMSZ_TOPLEFT);
		bool top   = (wParam == WMSZ_TOPLEFT)    || (wParam == WMSZ_TOP)  || (wParam == WMSZ_TOPRIGHT);
        if (rect->right - rect->left < info->minSize.x)
		{
			if (left) rect->left  = rect->right - info->minSize.x;
			else      rect->right = rect->left  + info->minSize.x;
		}
        if (rect->bottom - rect->top < info->minSize.y)
		{
			if (top) rect->top    = rect->bottom - info->minSize.y;
			else     rect->bottom = rect->top    + info->minSize.y;
		}
		break;
	}

    case WM_SIZE:
    {
        HWND hClose     = GetDlgItem(hWnd, IDOK);
        HWND hTab       = GetDlgItem(hWnd, IDC_TAB1);
        HWND hTree1     = GetDlgItem(hWnd, IDC_TREE1);
        HWND hTree2     = GetDlgItem(hWnd, IDC_TREE2);
        HWND hContainer = GetDlgItem(hWnd, IDC_CONTAINER);

        RECT close, client;
        GetClientRect(hWnd,   &client);
        GetWindowRect(hClose, &close);

        close.bottom -= close.top;
        close.right  -= close.left;
        
        MoveWindow(hClose,
                 (client.right  - close.right) / 2,
            (int)(client.bottom - close.bottom * 1.5f),
            close.right, close.bottom, TRUE);
        
        RECT tab = {8, 8, client.right - 8, client.bottom - close.bottom * 2};
        MoveWindow(hTab, tab.left, tab.top, tab.right - tab.left, tab.bottom - tab.top, TRUE);
        
        int split = client.right * 2 / 5;

        TabCtrl_AdjustRect(hTab, FALSE, &tab);
        MoveWindow(hTree1, tab.left + 4, tab.top + 4, split - tab.left - 8,  tab.bottom - tab.top - 8, TRUE);
        MoveWindow(hTree2, tab.left + 4, tab.top + 4, split - tab.left - 8,  tab.bottom - tab.top - 8, TRUE);
        MoveWindow(hContainer, split + 4, tab.top + 4, tab.right - split - 8,  tab.bottom - tab.top - 8, TRUE);
        
        // Also move all children of hContainer
        for (HWND hChild = GetWindow(hContainer, GW_CHILD); hChild != NULL; hChild = GetNextWindow(hChild, GW_HWNDNEXT))
        {
            MoveWindow(hChild, 0, 0, tab.right - split - 8,  tab.bottom - tab.top - 8, TRUE);
        }
        break;
    }

    case WM_CLOSE:
        delete info;
        DestroyWindow(hWnd);
        g_DetailsWindow = NULL;
        break;

    case WM_COMMAND:
        if (lParam != NULL)
        {
            // Control
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                // A button was clicked
                switch (LOWORD(wParam))
                {
                    case IDOK:
                    case IDCANCEL:
                        delete info;
                        DestroyWindow(hWnd);
                        g_DetailsWindow = NULL;
                        break;
                }
            }
        }
        break;
    }
    return FALSE;
}

void Dialogs::ShowDetailsDialog(HWND hWndParent, const Model& model)
{
    if (g_DetailsWindow == NULL)
    {
        g_DetailsWindow = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_DETAILS), NULL, DetailsProc, (LPARAM)&model);
    }
    SetWindowPos(g_DetailsWindow, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

void Dialogs::CloseDetailsDialog()
{
    if (g_DetailsWindow != NULL)
    {
        DestroyWindow(g_DetailsWindow);
        g_DetailsWindow = NULL;
    }
}