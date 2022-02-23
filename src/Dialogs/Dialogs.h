#ifndef DIALOGS_H
#define DIALOGS_H

#include "Games.h"
#include "General/GameTypes.h"
#include "RenderEngine/RenderEngine.h"
#include "Assets/Files.h"
#include "Assets/Models.h"
#include "Assets/MegaFile.h"
#include "Sound/SoundEngine.h"

namespace Dialogs
{
    typedef bool (*SSF_CALLBACK)(const std::string& name, ptr<Alamo::IFile> f, void* param);

    // Modal dialogs
    void ShowCameraDialog (HWND hWndParent, Alamo::Camera& camera);
    void ShowAboutDialog  (HWND hWndParent);
    ptr<Alamo::Model>     ShowOpenModelDialog(HWND hWndParent, GameMod mod, std::wstring* filename, ptr<Alamo::MegaFile>& meg);
    ptr<Alamo::Animation> ShowOpenAnimationDialog(HWND hWndParent, GameMod mod, ptr<Alamo::Model> model, std::wstring* filename);
    int                   ShowSelectSubFileDialog(HWND hWndParent, ptr<Alamo::MegaFile> meg, SSF_CALLBACK callback, void* userdata);

    // Modeless dialogs
    void ShowDetailsDialog(HWND hWndParent, const Alamo::Model& model);
    void CloseDetailsDialog();

    HWND CreateSettingsDialog(HWND hWndParent, const Alamo::Environment& environment, Alamo::ISoundEngine* sound);
    const Alamo::RenderOptions& Settings_GetRenderOptions(HWND hWnd);
    Alamo::Environment&         Settings_GetEnvironment(HWND hWnd);

    class ISelectionCallback
    {
    public:
        virtual void OnAnimationSelected(ptr<Alamo::Animation> animation, const std::wstring& filename, bool loop) = 0;
    };

    HWND CreateSelectionDialog(HWND hWndParent, ISelectionCallback& callback);
    void Selection_SetObject(HWND hWnd, Alamo::IRenderObject* object, ptr<Alamo::MegaFile> megaFile, const std::wstring& filename);
    void Selection_ResetObject(HWND hWnd, Alamo::IRenderObject* object);
    int Selection_GetALT(HWND hWnd);
    int Selection_GetLOD(HWND hWnd);
}
#endif