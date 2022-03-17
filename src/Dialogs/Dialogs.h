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
    struct ANIMATION_INFO : IObject
    {
        std::wstring          name;
        std::wstring          filename;
        ptr<Alamo::IFile>     file;
        ptr<Alamo::Animation> animation;

        void SetAnimation(ptr<Alamo::Animation> anim, ptr<Alamo::IFile> f)
        {
            this->animation = anim;
            this->file = f;
            this->filename = this->file->name();
            wchar_t fName[_MAX_FNAME];
            if (_wsplitpath_s(this->filename.c_str(), NULL, 0, NULL, 0, fName, _MAX_FNAME, NULL, 0) == 0)
            {
                this->name = fName;
            }
        }
    };

    enum AnimationType
    {
        INHERENT_ANIM,   // Animations that start with the name of the model, and get loaded with the model
        ADDITIONAL_ANIM, // Animations loaded through the Load Animation menu item
        OVERRIDE_ANIM    // Animations loaded through the Load Animation Override menu item
    };

    typedef bool (*SSF_CALLBACK)(const std::string& name, ptr<Alamo::IFile> f, void* param);

    // Modal dialogs
    void ShowCameraDialog (HWND hWndParent, Alamo::Camera& camera);
    void ShowAboutDialog  (HWND hWndParent);
    ptr<Alamo::Model>     ShowOpenModelDialog(HWND hWndParent, GameMod mod, std::wstring* filename, ptr<Alamo::MegaFile>& meg);
    ptr<ANIMATION_INFO>   ShowOpenAnimationDialog(HWND hWndParent, GameMod mod, ptr<Alamo::Model> model);
    bool                  ShowOpenAnimationOverrideDialog(HWND hWndParent, GameMod mod, std::wstring* filename, ptr<Alamo::MegaFile>& meg);
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
    void AddToAnimationList(HWND hWnd, ptr<ANIMATION_INFO> pai, AnimationType animType);
    void LoadModelAnimations(HWND hWnd, const Alamo::Model* model, const Alamo::MegaFile* megaFile, std::wstring filename, AnimationType animType);
}
#endif