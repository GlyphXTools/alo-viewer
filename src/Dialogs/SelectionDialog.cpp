#include "Dialogs\Dialogs.h"
#include "General\Exceptions.h"
#include "General\WinUtils.h"
#include "General\Utils.h"
#include <commctrl.h>
#include "resource.h"
using namespace Alamo;
using namespace std;

namespace Dialogs
{

struct SELECTION_INFO
{
    ISelectionCallback*     callback;
    IRenderObject*          object;
    vector<ANIMATION_INFO>  animations;
    int                     maxAlt;
    int                     maxLod;
};

// Extracts the alt and lod level from a name like "NAME_ALT1_LOD0"
// and returns the name without the suffixes. If any of the two suffixes
// aren't present, they're set to -1.
static string ParseName(string name, int* alt, int *lod)
{
    int* const values[2] = {alt, lod};
    static const char* names[2] = {"_ALT", "_LOD"};

    for (int i = 0; i < 2; i++)
    {
        *values[i] = -1;
        string::size_type pos = name.find(names[i]);
        if (pos != string::npos)
        {
            // Extract value
            string tail = name.substr(pos + 4);
            char* endptr;
            int val = strtoul(tail.c_str(), &endptr, 10);
            string::size_type n = endptr - tail.c_str();
            if (n > 0)
            {
                // We have at least one number
                *values[i] = max(0, val);
            }
            name = name.substr(0, pos) + tail.substr(n);
        }
    }
    return name;
}

static void CheckSelection(HWND hWnd)
{
    bool selected = false;
    switch (TabCtrl_GetCurSel(GetDlgItem(hWnd, IDC_TAB1)))
    {
        case 0: break;
        case 1: selected = (TreeView_GetSelection(GetDlgItem(hWnd, IDC_TREE1)) != NULL); break;
        case 2: selected = (ListView_GetSelectedCount(GetDlgItem(hWnd, IDC_LIST2)) != 0); break;
        case 3: selected = (ListView_GetSelectedCount(GetDlgItem(hWnd, IDC_LIST3)) != 0); break;
        case 4: selected = (ListView_GetSelectedCount(GetDlgItem(hWnd, IDC_LIST4)) != 0); break;
    }
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_SHOW),   selected);
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_TOGGLE), selected);
    EnableWindow(GetDlgItem(hWnd, IDC_BUTTON_HIDE),   selected);
}

static void ChangeSelection(SELECTION_INFO* info, HWND hWnd, WORD button)
{
    int tab = TabCtrl_GetCurSel(GetDlgItem(hWnd, IDC_TAB1));
    switch (tab)
    {
        case 0: break; // Animations don't use this
        case 1: break; // Bones don't use this

        case 2: // Meshes
        case 3: // Proxies
        case 4: // Dazzles
        {
            LVITEM item;
            item.mask      = LVIF_PARAM | LVIF_STATE;
            item.stateMask = LVIS_STATEIMAGEMASK;
            item.iItem     = -1;
            item.iSubItem  = 0;

            HWND hList = GetDlgItem(hWnd, tab == 2 ? IDC_LIST2 : (tab == 3 ? IDC_LIST3 : IDC_LIST4) );
            while ((item.iItem = ListView_GetNextItem(hList, item.iItem, LVIS_SELECTED)) != -1)
            {
                ListView_GetItem(hList, &item);
                int index = (int)item.lParam;
                switch (button)
                {
                case IDC_BUTTON_SHOW:
                    ListView_SetCheckState(hList, item.iItem, TRUE);
                         if (tab == 2) info->object->ShowMesh(index);
                    else if (tab == 3) info->object->ShowProxy(index);
                    else if (tab == 4) info->object->ShowDazzle(index);
                    break;

                case IDC_BUTTON_TOGGLE:
                    if (item.state == INDEXTOSTATEIMAGEMASK(2) )
                    {
                        // Item was checked; uncheck
                        ListView_SetCheckState(hList, item.iItem, FALSE);
                             if (tab == 2) info->object->HideMesh(index);
                        else if (tab == 3) info->object->HideProxy(index);
                        else if (tab == 4) info->object->HideDazzle(index);
                    }
                    else
                    {
                        // Item was unchecked; check
                        ListView_SetCheckState(hList, item.iItem, TRUE);
                             if (tab == 2) info->object->ShowMesh(index);
                        else if (tab == 3) info->object->ShowProxy(index);
                        else if (tab == 4) info->object->ShowDazzle(index);
                    }
                    break;

                case IDC_BUTTON_HIDE:
                    ListView_SetCheckState(hList, item.iItem, FALSE);
                         if (tab == 2) info->object->HideMesh(index);
                    else if (tab == 3) info->object->HideProxy(index);
                    else if (tab == 4) info->object->HideDazzle(index);
                    break;
                }
            }
        }
    }
}

void AddToAnimationList(HWND hWnd, ptr<ANIMATION_INFO> pai, AnimationType animType)
{
    SELECTION_INFO* psi = (SELECTION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    static HWND hAnimList = NULL;
    if (hAnimList == NULL)
    {
        hAnimList = GetDlgItem(hWnd, IDC_LIST1);
    }
    psi->animations.push_back(*pai);

    LV_ITEM item;
    item.mask     = LVIF_TEXT | LVIF_PARAM | LVIF_GROUPID;
    item.pszText  = (LPWSTR)pai->name.c_str();
    item.lParam   = (int)psi->animations.size() - 1;
    item.iItem    = (int)item.lParam;
    item.iSubItem = 0;
    item.iGroupId = animType;
    ListView_InsertItem(hAnimList, &item);
}

static void PlayAnimation(SELECTION_INFO* info, int index, bool loop)
{
    if (index >= 0)
    {
        ANIMATION_INFO* pai = &info->animations[index];
        if (pai->animation == NULL)
        {
            pai->animation = new Animation(pai->file, *info->object->GetTemplate()->GetModel());
        }
        info->callback->OnAnimationSelected(pai->animation, pai->filename, loop);
    }
    else
    {
        info->callback->OnAnimationSelected(NULL, L"", loop);
    }
}

static void SelectAltLod(SELECTION_INFO* info, HWND hWnd, int alt, int lod, bool altDec)
{
    info->object->SetLOD(lod);
    info->object->SetALT(alt);

    const Model& model = *info->object->GetTemplate()->GetModel();

    LV_ITEM item;
    item.mask      = LVIF_PARAM;
    item.iSubItem  = 0;

    // Update meshes
    HWND hMeshList = GetDlgItem(hWnd, IDC_LIST2);
    int nItems = ListView_GetItemCount(hMeshList);
    for (item.iItem = 0; item.iItem < nItems; item.iItem++)
    {
        ListView_GetItem(hMeshList, &item);
        size_t index = (size_t)item.lParam;
        ListView_SetCheckState(hMeshList, item.iItem, info->object->IsMeshVisible(index));
    }

    // Update proxies
    HWND hProxyList = GetDlgItem(hWnd, IDC_LIST3);
    nItems = ListView_GetItemCount(hProxyList);
    for (item.iItem = 0; item.iItem < nItems; item.iItem++)
    {
        ListView_GetItem(hProxyList, &item);
        size_t index = (size_t)item.lParam;
        ListView_SetCheckState(hProxyList, item.iItem, info->object->IsProxyVisible(index));
    }
}

static INT_PTR CALLBACK SelectionDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    SELECTION_INFO* info = (SELECTION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hTabs       = GetDlgItem(hWnd, IDC_TAB1);
            HWND hAnimList   = GetDlgItem(hWnd, IDC_LIST1);
            HWND hBoneTree   = GetDlgItem(hWnd, IDC_TREE1);
            HWND hMeshList   = GetDlgItem(hWnd, IDC_LIST2);
            HWND hProxyList  = GetDlgItem(hWnd, IDC_LIST3);
            HWND hDazzleList = GetDlgItem(hWnd, IDC_LIST4);
            HWND hLODSpin    = GetDlgItem(hWnd, IDC_SPIN1);
            HWND hALTSpin    = GetDlgItem(hWnd, IDC_SPIN2);

            wstring temp;
            TCITEM item = {TCIF_TEXT};
            temp = LoadString(IDS_SELECTION_ANIMATIONS); item.pszText = (LPTSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 0, &item);
            temp = LoadString(IDS_SELECTION_BONES);      item.pszText = (LPTSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 1, &item);
            temp = LoadString(IDS_SELECTION_MESHES);     item.pszText = (LPTSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 2, &item);
            temp = LoadString(IDS_SELECTION_PROXIES);    item.pszText = (LPTSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 3, &item);
            temp = LoadString(IDS_SELECTION_DAZZLES);    item.pszText = (LPTSTR)temp.c_str(); TabCtrl_InsertItem(hTabs, 4, &item);
            TabCtrl_SetCurSel(hTabs, 2);

            RECT list;
            GetClientRect(hMeshList, &list);

            LVCOLUMN column;
            column.mask = LVCF_WIDTH;
            column.cx   = list.right;
            ListView_InsertColumn(hAnimList,   0, &column);
            ListView_InsertColumn(hMeshList,   0, &column);
            ListView_InsertColumn(hProxyList,  0, &column);
            ListView_InsertColumn(hDazzleList, 0, &column);
            ListView_SetExtendedListViewStyle(hAnimList,   LVS_EX_FULLROWSELECT);
            ListView_SetExtendedListViewStyle(hMeshList,   LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
            ListView_SetExtendedListViewStyle(hProxyList,  LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);
            ListView_SetExtendedListViewStyle(hDazzleList, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

            ListView_EnableGroupView(hAnimList, TRUE);

            wstring modelAnimationsText    = LoadString(IDS_ANIMATIONS_MODEL);
            wstring extraAnimationsText    = LoadString(IDS_ANIMATIONS_EXTRA);
            wstring overrideAnimationsText = LoadString(IDS_ANIMATIONS_OVERRIDE);
            LVGROUP group;
            group.cbSize    = sizeof(LVGROUP);
            group.mask      = LVGF_HEADER | LVGF_GROUPID | LVGF_STATE;
            group.pszHeader = (LPWSTR)modelAnimationsText.c_str();
            group.iGroupId  = AnimationType::INHERENT_ANIM;
            group.state     = LVGS_COLLAPSIBLE;
            ListView_InsertGroup(hAnimList, -1, &group);

            group.pszHeader = (LPWSTR)extraAnimationsText.c_str();
            group.iGroupId  = AnimationType::ADDITIONAL_ANIM;
            ListView_InsertGroup(hAnimList, -1, &group);

            group.pszHeader = (LPWSTR)overrideAnimationsText.c_str();
            group.iGroupId  = AnimationType::OVERRIDE_ANIM;
            ListView_InsertGroup(hAnimList, -1, &group);
            
            SetWindowLong(hBoneTree, GWL_STYLE, GetWindowLong(hBoneTree, GWL_STYLE) | TVS_CHECKBOXES);

            SendMessage(hLODSpin, UDM_SETRANGE32, 0, 0);
            SendMessage(hALTSpin, UDM_SETRANGE32, 0, 0);
            SendMessage(hLODSpin, UDM_SETPOS32, 0, 0);
            SendMessage(hALTSpin, UDM_SETPOS32, 0, 0);

            info = new SELECTION_INFO;
            info->callback = (ISelectionCallback*)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)info);
            return TRUE;
        }

        case WM_DESTROY:
            delete info;
            break;

        case WM_SIZE:
        {
            RECT tab, list;
            GetParentRect(GetDlgItem(hWnd, IDC_TAB1),  &tab);
            GetParentRect(GetDlgItem(hWnd, IDC_LIST2), &list);
            MoveWindow(GetDlgItem(hWnd, IDC_TAB1),  tab .left, tab.top,  tab.right  - tab.left,  HIWORD(lParam) - tab.top, TRUE);
            MoveWindow(GetDlgItem(hWnd, IDC_TREE1), list.left, list.top, list.right - list.left, HIWORD(lParam) - list.top - (tab.bottom - list.bottom), TRUE);
            MoveWindow(GetDlgItem(hWnd, IDC_LIST1), list.left, list.top, list.right - list.left, HIWORD(lParam) - list.top - (tab.bottom - list.bottom), TRUE);
            MoveWindow(GetDlgItem(hWnd, IDC_LIST2), list.left, list.top, list.right - list.left, HIWORD(lParam) - list.top - (tab.bottom - list.bottom), TRUE);
            MoveWindow(GetDlgItem(hWnd, IDC_LIST3), list.left, list.top, list.right - list.left, HIWORD(lParam) - list.top - (tab.bottom - list.bottom), TRUE);
            MoveWindow(GetDlgItem(hWnd, IDC_LIST4), list.left, list.top, list.right - list.left, HIWORD(lParam) - list.top - (tab.bottom - list.bottom), TRUE);
            break;
        }

        case WM_COMMAND:
            if (lParam != 0 && HIWORD(wParam) == BN_CLICKED)
            {
                // Button was clicked
                switch (LOWORD(wParam))
                {
                case IDC_BUTTON_SHOW:
                case IDC_BUTTON_TOGGLE:
                case IDC_BUTTON_HIDE:
                    ChangeSelection(info, hWnd, LOWORD(wParam));
                    break;
                }
            }
            break;

        case WM_NOTIFY:
        {
            NMHDR* hdr = (NMHDR*)lParam;
            switch (hdr->code)
            {
                case TCN_SELCHANGE:
                {
                    // Tab selection changed
                    int tab = TabCtrl_GetCurSel(hdr->hwndFrom);
                    ShowWindow(GetDlgItem(hWnd, IDC_LIST1), tab == 0);
                    ShowWindow(GetDlgItem(hWnd, IDC_TREE1), tab == 1);
                    ShowWindow(GetDlgItem(hWnd, IDC_LIST2), tab == 2);
                    ShowWindow(GetDlgItem(hWnd, IDC_LIST3), tab == 3);
                    ShowWindow(GetDlgItem(hWnd, IDC_LIST4), tab == 4);
                    CheckSelection(hWnd);
                    break;
                }

                case UDN_DELTAPOS:
                    if (info != NULL && info->object != NULL)
                    {
                        NMUPDOWN* nmud = (NMUPDOWN*)lParam;
                        
                        bool changed, altDec = false;
                        int alt, lod, pos = max(0, nmud->iPos + nmud->iDelta);
                        if (hdr->idFrom == IDC_SPIN1) {
                            // ALT has changed
                            pos = min(info->maxAlt, pos);
                            alt = pos;
                            lod = (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_GETPOS32, 0, NULL);
                            changed = (alt != (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_GETPOS32, 0, NULL));
                            altDec = (nmud->iDelta < 0);
                        } else {
                            // LOD has changed
                            pos = min(info->maxLod, pos);
                            alt = (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_GETPOS32, 0, NULL);
                            lod = pos;
                            changed = (lod != (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_GETPOS32, 0, NULL));
                        }

                        if (changed)
                        {
                            SelectAltLod(info, hWnd, alt, lod, altDec);
                        }
                    }
                    break;

                case LVN_KEYDOWN:
                {
                    NMLVKEYDOWN* nmkd = (NMLVKEYDOWN*)lParam;
                    if (hdr->idFrom == IDC_LIST1 && (nmkd->wVKey == VK_SPACE || nmkd->wVKey == VK_CONTROL))
                    {
                        int focus = ListView_GetNextItem(hdr->hwndFrom, -1, LVIS_FOCUSED);
                        if (nmkd->wVKey == VK_SPACE && focus != -1)
                        {
                            // Get LPARAM for item
                            LVITEM item;
                            item.mask     = LVIF_PARAM;
                            item.iItem    = focus;
                            item.iSubItem = 0;
                            ListView_GetItem(hdr->hwndFrom, &item);

                            // Play the animation; looped if CTRL held
                            PlayAnimation(info, (int)item.lParam, (GetKeyState(VK_CONTROL) & 0x8000) != 0);
                        }
                        return TRUE;
                    }
                    break;
                }

                case TVN_KEYDOWN:
                {
                    NMTVKEYDOWN* nmkd = (NMTVKEYDOWN*)lParam;
                    if (hdr->idFrom == IDC_TREE1 && nmkd->wVKey == VK_SPACE && info != NULL && info->object != NULL)
                    {
                        // Space was hit, this changed the checkbox state on selected items
                        TVITEM item;
                        item.mask = TVIF_PARAM;
                        if ((item.hItem = TreeView_GetSelection(hdr->hwndFrom)) != NULL)
                        {
                            // An item was selected
                            TreeView_GetItem(hdr->hwndFrom, &item);

                            // Get old state; new state is opposite that
                            if (TreeView_GetCheckState(hdr->hwndFrom, item.hItem) == 1) {
                                info->object->HideBone( (size_t)item.lParam );
                            } else {
                                info->object->ShowBone( (size_t)item.lParam );
                            }
                        }
                    }
                    break;
                }

                case NM_CLICK:
                    if (hdr->idFrom == IDC_TREE1 && info != NULL && info->object != NULL)
                    {
                        // Click in the bone tree, check if it hit the checkbox
                        // (TVN_ITEMCHANGED is only available for Vista; so we do this the hard way)
                        TVHITTESTINFO hti;
                        GetCursorPos(&hti.pt);
                        ScreenToClient(hdr->hwndFrom, &hti.pt);

                        if (TreeView_HitTest(hdr->hwndFrom, &hti) != NULL && hti.flags & TVHT_ONITEMSTATEICON)
                        {
                            // Checkbox was hit
                            // Get LPARAM (bone index in model)
                            TVITEM item;
                            item.mask  = TVIF_PARAM;
                            item.hItem = hti.hItem;
                            TreeView_GetItem(hdr->hwndFrom, &item);

                            // Get old state; new state is opposite that
                            if (TreeView_GetCheckState(hdr->hwndFrom, hti.hItem) == 1) {
                                info->object->HideBone( (size_t)item.lParam );
                            } else {
                                info->object->ShowBone( (size_t)item.lParam );
                            }
                        }
                    }

                case NM_DBLCLK:
                    if (hdr->idFrom == IDC_LIST1)
                    {
                        // Animation has been double-clicked
                        NMITEMACTIVATE* nmia = (NMITEMACTIVATE*)lParam;
                        
                        // See which item was hit
                        LVHITTESTINFO lvhi;
                        lvhi.pt = nmia->ptAction;
                        if (ListView_SubItemHitTest(hdr->hwndFrom, &lvhi) != -1)
                        {
                            // Get LPARAM for item
                            LVITEM item;
                            item.mask     = LVIF_PARAM;
                            item.iItem    = lvhi.iItem;
                            item.iSubItem = 0;
                            ListView_GetItem(hdr->hwndFrom, &item);

                            // Play the animation; looped if double-clicked
                            PlayAnimation(info, (int)item.lParam, (hdr->code == NM_DBLCLK));
                        }
                        else
                        {
                            PlayAnimation(info, -1, false);
                        }
                    }
                    break;

                case LVN_ITEMCHANGED:
                {
                    NMLISTVIEW* nmlv = (NMLISTVIEW*)hdr;
                    if (info != NULL && info->object != NULL && nmlv->uChanged & LVIF_STATE)
                    {
                        if ((nmlv->uOldState ^ nmlv->uNewState) & LVIS_STATEIMAGEMASK)
                        {
                            // Check state has changed; forward notification to parent
                            if (hdr->hwndFrom == GetDlgItem(hWnd, IDC_LIST2))
                            {
                                // Mesh visibility changed
                                size_t mesh = (size_t)nmlv->lParam;
                                if (ListView_GetCheckState(hdr->hwndFrom, nmlv->iItem)) {
                                    info->object->ShowMesh(mesh);
                                } else {
                                    info->object->HideMesh(mesh);
                                }
                            }
                            else if (hdr->idFrom == IDC_LIST3)
                            {
                                // Proxy visibility changed
                                size_t proxy = (size_t)nmlv->lParam;
                                if (ListView_GetCheckState(hdr->hwndFrom, nmlv->iItem)) {
                                    info->object->ShowProxy(proxy);
                                } else {
                                    info->object->HideProxy(proxy);
                                }
                            }
                            else if (hdr->idFrom == IDC_LIST4)
                            {
                                // Dazzle visibility changed
                                size_t dazzle = (size_t)nmlv->lParam;
                                if (ListView_GetCheckState(hdr->hwndFrom, nmlv->iItem)) {
                                    info->object->ShowDazzle(dazzle);
                                } else {
                                    info->object->HideDazzle(dazzle);
                                }
                            }
                        }
                        else if ((nmlv->uOldState ^ nmlv->uNewState) & LVIS_SELECTED)
                        {
                            // Mesh or proxy selection changed
                            if (hdr->idFrom == IDC_LIST2)
                            {
                                // Mesh
                                size_t mesh = (size_t)nmlv->lParam;
                                if (nmlv->uNewState & LVIS_SELECTED) {
                                    info->object->SelectMesh(mesh, true);
                                } else {
                                    info->object->SelectMesh(mesh, false);
                                }
                            }
                            else if (hdr->idFrom == IDC_LIST4)
                            {
                                // Mesh
                                size_t dazzle = (size_t)nmlv->lParam;
                                if (nmlv->uNewState & LVIS_SELECTED) {
                                    info->object->SelectDazzle(dazzle, true);
                                } else {
                                    info->object->SelectDazzle(dazzle, false);
                                }
                            }
                            CheckSelection(hWnd);
                        }
                    }
                    break;
                }
            }
            break;
        }
    }
    return FALSE;
}

static void LoadAnimationList(vector<ANIMATION_INFO>& animations, const Model& model, const MegaFile* megaFile, wstring wModelFilename, bool isOverride)
{
    if (megaFile != NULL)
    {
        // Read them from the mega file

        // Extract model filename
        string modelFilename = WideToAnsi(wModelFilename);
        string::size_type ofs = modelFilename.find_last_of("|");
        if (ofs != string::npos)
        {
            modelFilename = modelFilename.substr(ofs+ 1);
        }

        // Remove extension
        if ((ofs = modelFilename.find_last_of(".")) != string::npos)
        {
            modelFilename = modelFilename.substr(0, ofs);
        }
        modelFilename = modelFilename + "_";

        for (size_t i = 0; i < megaFile->GetNumFiles(); i++)
        {
            string filename = megaFile->GetFilename(i);
            
            // Get extension of subfile
            if ((ofs = filename.find_last_of(".")) != string::npos)
            {
                if (filename.substr(ofs + 1) == "ALA")
                {
                    // It's an animation, get the base name
                    string name = filename.substr(0, ofs);

                    // Must have same base as model file
                    if (name.find(modelFilename) == 0)
                    {
                        try
                        {
                            // Load file to check only
                            ANIMATION_INFO info;
                            info.filename  = AnsiToWide(filename);
                            if (isOverride)
                            {
                                // If loading from an override, do not remove the model file name
                                info.name  = AnsiToWide(name);
                            }
                            else
                            {
                                info.name  = AnsiToWide(name.substr(modelFilename.length()));
                            }
                            info.file      = megaFile->GetFile(i);
                            info.animation = NULL;

                            ptr<Animation> anim = new Animation(info.file, model, true);
                            
                            // Animation matches, reset file and add it
                            info.file->seek(0);
                            animations.push_back(info);
                        }
                        catch (wexception&)
                        {
                            Log::WriteError("Failed to load animation: %s", name.c_str());
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Read them from the directory
        wstring::size_type ofs = wModelFilename.find_last_of(L"\\/");
        wstring dir = L"";
        if (ofs != wstring::npos)
        {
            dir = wModelFilename.substr(0, ofs + 1);
            wModelFilename = wModelFilename.substr(ofs + 1);
        }

        if ((ofs = wModelFilename.find_last_of(L".")) != wstring::npos)
        {
            wModelFilename = wModelFilename.substr(0, ofs);
        }

        wstring filter = dir + wModelFilename + L"_*.ALA";
        WIN32_FIND_DATA wfd;
        HANDLE hFind = FindFirstFile(filter.c_str(), &wfd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                try
                {
                    // Load file to check only
                    ANIMATION_INFO info;
                    info.filename  = wfd.cFileName;
                    if (isOverride)
                    {
                        // If loading from an override, do not remove the model file name
                        info.name  = info.filename;
                    }
                    else
                    {
                        info.name  = info.filename.substr(wModelFilename.length() + 1);
                    }
                    info.name      = info.name.substr(0, info.name.length() - 4);    // Strip extension
                    info.file      = new PhysicalFile(dir + wfd.cFileName);
                    info.animation = NULL;

                    ptr<Animation> anim = new Animation(info.file, model, true);
                    
                    // Animation matches, reset file and add it
                    info.file->seek(0);
                    animations.push_back(info);
                }
                catch (wexception&)
                {
                    Log::WriteError("Failed to load animation: %s", WideToAnsi(wfd.cFileName).c_str());
                }

            } while (FindNextFile(hFind, &wfd));
            FindClose(hFind);
        }
    }

    if (animations.size() == 0 && isOverride)
    {
        Log::WriteError("No valid animations found for model %s", WideToAnsi(wModelFilename).c_str());
    }
}

void LoadModelAnimations(HWND hWnd, const Model* model, const MegaFile* megaFile, wstring filename, AnimationType animType)
{
    SELECTION_INFO* psi = (SELECTION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    static HWND hAnimList = NULL;
    if (hAnimList == NULL)
    {
        hAnimList = GetDlgItem(hWnd, IDC_LIST1);
    }
    vector<ANIMATION_INFO> animations;
    LoadAnimationList(animations, *model, megaFile, filename, animType != AnimationType::INHERENT_ANIM);

    for (size_t i = 0; i < animations.size(); i++)
    {
        psi->animations.push_back(animations[i]);
        LV_ITEM item;
        item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_GROUPID;
        item.pszText = (LPWSTR)animations[i].name.c_str();
        item.lParam = psi->animations.size() - 1;
        item.iItem = (int)psi->animations.size() - 1;
        item.iSubItem = 0;
        item.iGroupId = animType;
        ListView_InsertItem(hAnimList, &item);
    }
}

void Selection_ResetObject(HWND hWnd, IRenderObject* object)
{
    SELECTION_INFO* info = (SELECTION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (info->object != NULL)
    {    
        info->object = object;

        int alt = (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_GETPOS32, 0, NULL);
        int lod = (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_GETPOS32, 0, NULL);
        SelectAltLod(info, hWnd, alt, lod, false);
    }
}

int Selection_GetALT(HWND hWnd)
{
    return (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_GETPOS32, 0, NULL);
}

int Selection_GetLOD(HWND hWnd)
{
    return (int)SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_GETPOS32, 0, NULL);
}

void Selection_SetObject(HWND hWnd, IRenderObject* object, ptr<MegaFile> megaFile, const wstring& filename)
{
    HWND hAnimList   = GetDlgItem(hWnd, IDC_LIST1);
    HWND hBoneTree   = GetDlgItem(hWnd, IDC_TREE1);
    HWND hMeshList   = GetDlgItem(hWnd, IDC_LIST2);
    HWND hProxyList  = GetDlgItem(hWnd, IDC_LIST3);
    HWND hDazzleList = GetDlgItem(hWnd, IDC_LIST4);

    // Remove old info
    SELECTION_INFO* info = (SELECTION_INFO*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    info->object = NULL;

    info->maxAlt = -1;
    info->maxLod = -1;
    info->animations.clear();

    // Clear lists
    ListView_DeleteAllItems(hAnimList);
    TreeView_DeleteAllItems(hBoneTree);
    ListView_DeleteAllItems(hMeshList);
    ListView_DeleteAllItems(hProxyList);
    ListView_DeleteAllItems(hDazzleList);

    if (object != NULL)
    {
        const Model* model = object->GetTemplate()->GetModel();
        
        // Fill bones list
        vector<HTREEITEM> hBones(model->GetNumBones(), NULL);
        for (size_t i = 0; i < model->GetNumBones(); i++)
        {
            const Model::Bone& bone = model->GetBone(i);
            wstring name = AnsiToWide(bone.name);
            
            TVINSERTSTRUCT tvis;
            if (bone.parent == NULL) {
                tvis.hParent = NULL;
            } else {
                tvis.hParent = hBones[bone.parent->index];
            }
            tvis.hInsertAfter   = TVI_LAST;
            tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_STATE;
            tvis.item.pszText   = (LPWSTR)name.c_str();
            tvis.item.lParam    = i;
            tvis.item.state     = TVIS_EXPANDED | INDEXTOSTATEIMAGEMASK(2);
            tvis.item.stateMask = TVIS_EXPANDED | TVIS_STATEIMAGEMASK;
            HTREEITEM hItem = TreeView_InsertItem(hBoneTree, &tvis);
            hBones[i] = hItem;
        }

        // Fill mesh list
        for (size_t i = 0; i < model->GetNumMeshes(); i++)
        {           
            int alt, lod;
            ParseName(model->GetMesh(i).name, &alt, &lod);
            info->maxAlt = max(alt, info->maxAlt);
            info->maxLod = max(lod, info->maxLod);

            wstring name = AnsiToWide(model->GetMesh(i).name);
            LV_ITEM item;
            item.mask      = LVIF_TEXT | LVIF_PARAM;
            item.pszText   = (LPWSTR)name.c_str();
            item.lParam    = i;
            item.iItem     = (int)i;
            item.iSubItem  = 0;
            int index = ListView_InsertItem(hMeshList, &item);
            ListView_SetCheckState(hMeshList, index, object->IsMeshVisible(i) );
        }

        // Fill proxy list
        for (size_t i = 0; i < model->GetNumProxies(); i++)
        {
            int alt, lod;
            ParseName(model->GetProxy(i).name, &alt, &lod);
            info->maxAlt = max(alt, info->maxAlt);
            info->maxLod = max(lod, info->maxLod);

            wstring name = AnsiToWide(model->GetProxy(i).name);
            LV_ITEM item;
            item.mask     = LVIF_TEXT | LVIF_PARAM;
            item.pszText  = (LPWSTR)name.c_str();
            item.lParam   = i;
            item.iItem    = (int)i;
            item.iSubItem = 0;
            int index = ListView_InsertItem(hProxyList, &item);
            ListView_SetCheckState(hProxyList, index, object->IsProxyVisible(i) );
        }

        // Fill dazzle list
        for (size_t i = 0; i < model->GetNumDazzles(); i++)
        {
            wstring name = AnsiToWide(model->GetDazzle(i).name);
            LV_ITEM item;
            item.mask     = LVIF_TEXT | LVIF_PARAM;
            item.pszText  = (LPWSTR)name.c_str();
            item.lParam   = i;
            item.iItem    = (int)i;
            item.iSubItem = 0;
            int index = ListView_InsertItem(hDazzleList, &item);
            ListView_SetCheckState(hDazzleList, index, object->IsDazzleVisible(i) );
        }

        // Fill animation list
        LoadModelAnimations(hWnd, model, megaFile, filename, AnimationType::INHERENT_ANIM);

        info->object = object;

        // Start at full health (alt), max detail (lod)
        SelectAltLod(info, hWnd, object->GetALT(), object->GetLOD(), false);
    }

    SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETRANGE32, 0, max(0, info->maxAlt));
    SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_SETRANGE32, 0, max(0, info->maxLod));
    SendMessage(GetDlgItem(hWnd, IDC_SPIN1), UDM_SETPOS32, 0, 0);
    SendMessage(GetDlgItem(hWnd, IDC_SPIN2), UDM_SETPOS32, 0, max(0, info->maxLod));
    EnableWindow(GetDlgItem(hWnd, IDC_SPIN1),      (info->object != NULL && info->maxAlt >= 0));
    EnableWindow(GetDlgItem(hWnd, IDC_SPIN2),      (info->object != NULL && info->maxLod >= 0));
    EnableWindow(GetDlgItem(hWnd, IDC_SELECT_ALT), (info->object != NULL && info->maxAlt >= 0));
    EnableWindow(GetDlgItem(hWnd, IDC_SELECT_LOD), (info->object != NULL && info->maxLod >= 0));
}

HWND CreateSelectionDialog(HWND hWndParent, ISelectionCallback& callback)
{
    return CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_SELECTION), hWndParent, SelectionDialogProc, (LPARAM)&callback);
}

}