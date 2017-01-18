#include "Dialogs/Dialogs.h"
#include "General/WinUtils.h"
#include "UI/UI.h"
#include "resource.h"
#include "config.h"
#include <commctrl.h>
#include <map>
#include <sstream>
using namespace Alamo;
using namespace std;

// Name of the registry key that stores the camera predefineds
static const wchar_t* CAMERA_SUBKEY_NAME = L"Cameras";

class CameraInfo
{
    struct CAMERA_INFO
    {
        Vector3 position;
        Vector3 target;
    };

    bool                 m_changed;
    Camera&              m_camera;
    map<wstring, Camera> m_predefineds;

public:
    const map<wstring, Camera> GetPredefineds() const { return m_predefineds; }
    Camera&                    GetCamera()      const { return m_camera; }

    void SetPredefined(const wstring& name, const Camera& camera) {
        m_predefineds[name] = camera;
        m_changed = true;
    }

    void DeletePredefined(const wstring& name) {
        m_predefineds.erase(name);
        m_changed = true;
    }

    CameraInfo(Camera* _camera) : m_camera(*_camera)
    {
        // Read the predefineds from the registry
        HKEY hKey;
        wstring path = wstring(Config::REGISTRY_BASE_PATH) + L"\\" + CAMERA_SUBKEY_NAME;
        if (RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS)
        {
            TCHAR valname[16383];
            DWORD type;
            DWORD size = 0, valsize = 16383;
            for (DWORD index = 0; RegEnumValue(hKey, index, valname, &valsize, NULL, &type, NULL, &size) == ERROR_SUCCESS; index++, size = 0, valsize = 16383)
            {
                // Load a camera
                CAMERA_INFO ci;
                if (type == REG_BINARY && size == sizeof(CAMERA_INFO) && RegQueryValueEx(hKey, valname, NULL, NULL, (BYTE*)&ci, &size) == ERROR_SUCCESS)
                {
                    Camera camera;
                    camera.m_position = ci.position;
                    camera.m_target   = ci.target;
                    camera.m_up       = Vector3(0,0,1);
                    m_predefineds[valname] = camera;
                }
            }
            RegCloseKey(hKey);
        }
        m_changed = false;
    }

    ~CameraInfo()
    {
        if (m_changed)
        {
            HKEY hKey;
            wstring path = wstring(Config::REGISTRY_BASE_PATH) + L"\\" + CAMERA_SUBKEY_NAME;
            if (m_predefineds.empty())
            {
                // Remove the registry key
                RegDeleteKey(HKEY_CURRENT_USER, path.c_str());
            }
            // Create the key
            else if (RegCreateKeyEx(HKEY_CURRENT_USER, path.c_str(), 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS)
            {
                // Store the cameras
                for (map<wstring,Camera>::const_iterator p = m_predefineds.begin(); p != m_predefineds.end(); p++)
                {
                    CAMERA_INFO ci;
                    ci.position = p->second.m_position;
                    ci.target   = p->second.m_target;
                    RegSetValueEx(hKey, p->first.c_str(), 0, REG_BINARY, (BYTE*)&ci, sizeof ci);
                }
                RegCloseKey(hKey);
            }
        }
    }
};

static void AddCameraToList(HWND hWnd, const wstring& name, const Camera& camera)
{
    // See if the item already exists
    LVFINDINFO lvfi;
    lvfi.flags = LVFI_STRING;
    lvfi.psz   = name.c_str();

    LV_ITEM item;
    item.iItem    = ListView_FindItem(hWnd, -1, &lvfi);
    item.mask     = LVIF_TEXT;
    item.pszText  = (LPWSTR)name.c_str();
    item.iSubItem = 0;
    if (item.iItem == -1) {
        item.iItem = ListView_InsertItem(hWnd, &item);
    } else {
        ListView_SetItem(hWnd, &item);
    }

    wstringstream position, target;
    position << L"[" << camera.m_position.x << L", " << camera.m_position.y << L", " << camera.m_position.z << "]";
    target   << L"[" << camera.m_target.x   << L", " << camera.m_target.y   << L", " << camera.m_target.z   << "]";

    wstring temp = position.str();
    item.pszText  = (LPWSTR)temp.c_str();
    item.iSubItem = 1;
    ListView_SetItem(hWnd, &item);

    temp = target.str();
    item.pszText  = (LPWSTR)temp.c_str();
    item.iSubItem = 2;
    ListView_SetItem(hWnd, &item);
}

static void SetCamera(HWND hWnd, const wstring& name, const Camera& camera)
{
    SetUIFloat(hWnd, IDC_SPINNER1, camera.m_position.x);
    SetUIFloat(hWnd, IDC_SPINNER2, camera.m_position.y);
    SetUIFloat(hWnd, IDC_SPINNER3, camera.m_position.z);
    SetUIFloat(hWnd, IDC_SPINNER4, camera.m_target.x);
    SetUIFloat(hWnd, IDC_SPINNER5, camera.m_target.y);
    SetUIFloat(hWnd, IDC_SPINNER6, camera.m_target.z);
    SetWindowText(GetDlgItem(hWnd, IDC_EDIT1), name.c_str());
}

static void LoadSelectedItem(HWND hWnd, const CameraInfo* ci)
{
    TCHAR   name[1024];
    LV_ITEM item;
    item.mask       = LVIF_TEXT;
    item.pszText    = name;
    item.cchTextMax = 1024;
    item.iSubItem   = 0;

    HWND hListView = GetDlgItem(hWnd, IDC_LIST1);
    if ((item.iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) != -1)
    {
        ListView_GetItem(hListView, &item);
        const map<wstring,Camera>& predefineds = ci->GetPredefineds();
        map<wstring,Camera>::const_iterator p = predefineds.find(item.pszText);
        if (p != predefineds.end())
        {
            SetCamera(hWnd, p->first, p->second);
        }
    }
}

static void DeleteSelectedItem(HWND hWnd, CameraInfo* ci)
{
    TCHAR   name[1024];
    LV_ITEM item;
    item.mask       = LVIF_TEXT;
    item.pszText    = name;
    item.cchTextMax = 1024;
    item.iSubItem   = 0;

    HWND hListView = GetDlgItem(hWnd, IDC_LIST1);
    if ((item.iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED)) != -1)
    {
        ListView_GetItem(hListView, &item);
        ci->DeletePredefined(item.pszText);
        ListView_DeleteItem(hListView, item.iItem);
    }
}

static void CloseDialog(HWND hWnd, CameraInfo* ci, bool useCamera)
{
    Camera& camera = ci->GetCamera();
    camera.m_position = Vector3(GetUIFloat(hWnd, IDC_SPINNER1), GetUIFloat(hWnd, IDC_SPINNER2),GetUIFloat(hWnd, IDC_SPINNER3));
    camera.m_target   = Vector3(GetUIFloat(hWnd, IDC_SPINNER4), GetUIFloat(hWnd, IDC_SPINNER5),GetUIFloat(hWnd, IDC_SPINNER6));
    delete ci;
    EndDialog(hWnd, useCamera);
}

INT_PTR CALLBACK SetCameraProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CameraInfo* ci = (CameraInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        ci = new CameraInfo( (Camera*)lParam );
        SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)ci);

        // Fill the "Camera properties" group
        SPINNER_INFO si;
        si.IsFloat     = true;
        si.Mask        = SPIF_RANGE | SPIF_INCREMENT;
        si.f.Increment = 1.0f;
        si.f.MaxValue  =  FLT_MAX;
        si.f.MinValue  = -FLT_MAX;
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER1), &si);
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER2), &si);
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER3), &si);
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER4), &si);
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER5), &si);
        Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER6), &si);

        SetCamera(hWnd, L"", ci->GetCamera());

        // Initialize the "predefined camera" group
        HWND hListView = GetDlgItem(hWnd, IDC_LIST1);
        ListView_DeleteAllItems(hListView);
        ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT);
        LVCOLUMN column;
        
        const wstring ColumnNames[3] = {
            LoadString(IDS_CAMERA_NAME),
            LoadString(IDS_CAMERA_POSITION),
            LoadString(IDS_CAMERA_TARGET),
        };

        column.mask = LVCF_WIDTH | LVCF_TEXT |LVCF_SUBITEM;
        column.cx =  65; column.iSubItem = 0; column.pszText = (LPTSTR)ColumnNames[0].c_str(); ListView_InsertColumn(hListView, 0, &column);
        column.cx = 125; column.iSubItem = 1; column.pszText = (LPTSTR)ColumnNames[1].c_str(); ListView_InsertColumn(hListView, 1, &column);
        column.cx = 100; column.iSubItem = 2; column.pszText = (LPTSTR)ColumnNames[2].c_str(); ListView_InsertColumn(hListView, 2, &column);

        // Fill the "predefined camera" group
        const map<wstring,Camera>& predefineds = ci->GetPredefineds();
        for (map<wstring,Camera>::const_iterator p = predefineds.begin(); p != predefineds.end(); p++)
        {
            AddCameraToList(hListView, p->first, p->second);
        }
        return TRUE;
    }

    case WM_NOTIFY:
    {
        NMHDR* hdr = (NMHDR*)lParam;
        switch (hdr->code)
        {
            case LVN_ITEMCHANGED:
            {
                bool selected = (ListView_GetSelectedCount(hdr->hwndFrom) != 0);
                EnableWindow(GetDlgItem(hWnd, IDC_LOAD),   selected);
                EnableWindow(GetDlgItem(hWnd, IDC_DELETE), selected);
                break;
            }

            case NM_DBLCLK:
                if (ListView_GetSelectedCount(hdr->hwndFrom) != 0)
                {
                    LoadSelectedItem(hWnd, ci);
                    CloseDialog(hWnd, ci, true);
                }
                break;
        }
        break;
    }

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
                    case IDOK:     CloseDialog(hWnd, ci, true);  break;
                    case IDCANCEL: CloseDialog(hWnd, ci, false); break;
                
                    case IDC_SAVE:
                    {
                        wstring name = GetWindowText(GetDlgItem(hWnd, IDC_EDIT1));
                        Camera camera;
                        camera.m_position = Vector3(GetUIFloat(hWnd, IDC_SPINNER1), GetUIFloat(hWnd, IDC_SPINNER2),GetUIFloat(hWnd, IDC_SPINNER3));
                        camera.m_target   = Vector3(GetUIFloat(hWnd, IDC_SPINNER4), GetUIFloat(hWnd, IDC_SPINNER5),GetUIFloat(hWnd, IDC_SPINNER6));
                        ci->SetPredefined(name, camera);
                        AddCameraToList(GetDlgItem(hWnd, IDC_LIST1), name, camera);
                        break;
                    }

                    case IDC_LOAD:
                        LoadSelectedItem(hWnd, ci);
                        break;

                    case IDC_DELETE:
                        DeleteSelectedItem(hWnd, ci);
                        break;
                }
                break;
            
                case EN_CHANGE:
                {
                    // An edit box was changed; toggle Save button
                    bool empty = (GetWindowTextLength(GetDlgItem(hWnd, IDC_EDIT1)) == 0);
                    EnableWindow(GetDlgItem(hWnd, IDC_SAVE), !empty);
                    break;
                }
            }
        }
    }
    return FALSE;
}

void Dialogs::ShowCameraDialog(HWND hWndParent, Camera& camera)
{
    Camera copy = camera;
    if (DialogBoxParam(NULL, MAKEINTRESOURCE(IDD_CAMERA), hWndParent, SetCameraProc, (LPARAM)&copy))
    {
        // The user exited the dialog with OK
        camera = copy;
    }
}