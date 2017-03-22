#include "Dialogs/Dialogs.h"
#include "UI/UI.h"
#include "Config.h"
#include <commctrl.h>
#include "resource.h"
using namespace Alamo;
using namespace std;

struct Settings
{
    RenderOptions options;
    Environment   environment;
    ISoundEngine* sound;
};

static void FillEnvironment(HWND hWnd, const Environment& env)
{
    SetUIFloat(hWnd, IDC_SPINNER2,  fmodf(ToDegrees((-env.m_lights[LT_SUN  ].m_direction).zAngle()) + 90 + 360, 360));
    SetUIFloat(hWnd, IDC_SPINNER5,  fmodf(ToDegrees((-env.m_lights[LT_FILL1].m_direction).zAngle()) + 90 + 360, 360));
    SetUIFloat(hWnd, IDC_SPINNER8,  fmodf(ToDegrees((-env.m_lights[LT_FILL2].m_direction).zAngle()) + 90 + 360, 360));
    SetUIFloat(hWnd, IDC_SPINNER12, env.m_wind.speed);
    SetUIFloat(hWnd, IDC_SPINNER10, fmodf(ToDegrees(env.m_wind.heading) + 360, 360));

    SetUIFloat(hWnd, IDC_SPINNER3, ToDegrees(-env.m_lights[LT_SUN  ].m_direction.tilt()));
    SetUIFloat(hWnd, IDC_SPINNER6, ToDegrees(-env.m_lights[LT_FILL1].m_direction.tilt()));
    SetUIFloat(hWnd, IDC_SPINNER9, ToDegrees(-env.m_lights[LT_FILL2].m_direction.tilt()));

    SetUIFloat(hWnd, IDC_SPINNER1, env.m_lights[LT_SUN]  .m_color.a);
    SetUIFloat(hWnd, IDC_SPINNER4, env.m_lights[LT_FILL1].m_color.a);
    SetUIFloat(hWnd, IDC_SPINNER7, env.m_lights[LT_FILL2].m_color.a);

    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR1), Color(env.m_lights[LT_SUN]  .m_color, 1.0f));
    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR2), Color(env.m_lights[LT_FILL1].m_color, 1.0f));
    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR3), Color(env.m_lights[LT_FILL2].m_color, 1.0f));
    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR4), env.m_shadow);
    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR5), env.m_specular);
    ColorButton_SetColor(GetDlgItem(hWnd, IDC_COLOR6), env.m_ambient);            
}

static INT_PTR CALLBACK SettingsDialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    Settings* settings = (Settings*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            // Get the application info from the create parameters
            settings = (Settings*)lParam;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)settings);
            settings->options.showBones     = false;
            settings->options.showBoneNames = false;
            settings->options.showGround    = false;
            settings->options.groundLevel   = 0.0f;
            settings->options.showWireframe = false;

            // Initialize dialog elements
            SPINNER_INFO si;
            si.IsFloat     = true;
            si.Mask        = SPIF_ALL;
            si.f.MinValue  =   0.0f;
            si.f.MaxValue  = 360.0f;
            si.f.Increment =   1.0f;
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER2), &si);
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER5), &si);
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER8), &si);
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER10), &si);

            si.f.MinValue = -90.0f;
            si.f.MaxValue =  90.0f;
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER3), &si);
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER6), &si);
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER9), &si);

            si.f.MinValue  = 0.00f;
            si.f.Increment = 0.01f;
            si.f.MaxValue  = 0.75f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER1), &si);
            si.f.MaxValue  = 0.50f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER4), &si);
            si.f.MaxValue  = 0.50f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER7), &si);
            si.f.MaxValue  = 10.0f; Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER12), &si);

            // Ground plane
            si.f.MinValue  = -10000.0f;
            si.f.Value     =  settings->options.groundLevel;
            si.f.Increment =  5.00f;
            si.f.MaxValue  = +10000.0f;
            Spinner_SetInfo(GetDlgItem(hWnd, IDC_SPINNER11), &si);
            FillEnvironment(hWnd, settings->environment);

            EnableWindow(GetDlgItem(hWnd, IDC_CHECK5), settings->sound != NULL);
            CheckDlgButton(hWnd, IDC_CHECK5, settings->sound != NULL ? BST_UNCHECKED : BST_CHECKED);
        }
        break;

        case WM_DESTROY:
            delete settings;
            break;

	    case WM_COMMAND:
		    WORD id = LOWORD(wParam);
            switch (HIWORD(wParam))
            {
            case BN_CLICKED:
                // A button/checkbox has been clicked
			    switch (id)
			    {
                    case IDC_BUTTON1:
                    {
                        // Reset environment to default (except clear color)
                        Color clear = settings->environment.m_clearColor;
                        settings->environment = Config::GetDefaultEnvironment();
                        settings->environment.m_clearColor = clear;
                        FillEnvironment(hWnd, settings->environment);
                    }
                    break;

				    case IDC_CHECK1:
                        settings->options.showBones = (IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
                        EnableWindow(GetDlgItem(hWnd, IDC_CHECK2), settings->options.showBones);
                        break;

				    case IDC_CHECK4:
                        settings->options.showGround = (IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
                        EnableWindow(GetDlgItem(hWnd, IDC_SPINNER11), settings->options.showGround);
                        break;

                    case IDC_CHECK2:
                        settings->options.showBoneNames = (IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
                        break;

                    case IDC_CHECK3:
                        settings->options.showWireframe = (IsDlgButtonChecked(hWnd, id) == BST_CHECKED);
                        break;

                    case IDC_CHECK5:
                        if (settings->sound != NULL)
                        {
                            float volume = (IsDlgButtonChecked(hWnd, id) == BST_CHECKED) ? 0.0f : 1.0f;
                            settings->sound->SetMasterVolume(volume);
                        }
                        break;
                }
                
                // Notify parent
                if (id == IDC_BUTTON1) {
                    SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hWnd), SN_CHANGE), (LPARAM)hWnd);
                } else {
                    SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
                }
                break;

            case SN_CHANGE:
            case CBN_CHANGE:
                // A spinner or color button has changed
                switch (id)
                {
                case IDC_SPINNER1: case IDC_COLOR1:   settings->environment.m_lights[LT_SUN]  .m_color = Color(Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR1))), GetUIFloat(hWnd, IDC_SPINNER1)); break;
                case IDC_SPINNER4: case IDC_COLOR2:   settings->environment.m_lights[LT_FILL1].m_color = Color(Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR2))), GetUIFloat(hWnd, IDC_SPINNER4)); break;
                case IDC_SPINNER7: case IDC_COLOR3:   settings->environment.m_lights[LT_FILL2].m_color = Color(Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR3))), GetUIFloat(hWnd, IDC_SPINNER7)); break;
                case IDC_SPINNER2: case IDC_SPINNER3: settings->environment.m_lights[LT_SUN]  .m_direction = -Vector3(ToRadians(GetUIFloat(hWnd, IDC_SPINNER2) - 90), ToRadians(GetUIFloat(hWnd, IDC_SPINNER3))); break;
                case IDC_SPINNER5: case IDC_SPINNER6: settings->environment.m_lights[LT_FILL1].m_direction = -Vector3(ToRadians(GetUIFloat(hWnd, IDC_SPINNER5) - 90), ToRadians(GetUIFloat(hWnd, IDC_SPINNER6))); break;
                case IDC_SPINNER8: case IDC_SPINNER9: settings->environment.m_lights[LT_FILL2].m_direction = -Vector3(ToRadians(GetUIFloat(hWnd, IDC_SPINNER8) - 90), ToRadians(GetUIFloat(hWnd, IDC_SPINNER9))); break;
                case IDC_COLOR4:    settings->environment.m_shadow   = Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR4))); break;
                case IDC_COLOR5:    settings->environment.m_specular = Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR5))); break;
                case IDC_COLOR6:    settings->environment.m_ambient  = Color(ColorButton_GetColor(GetDlgItem(hWnd, IDC_COLOR6))); break;
                case IDC_SPINNER10: settings->environment.m_wind.heading = ToRadians(GetUIFloat(hWnd, IDC_SPINNER10)); break;
                case IDC_SPINNER12: settings->environment.m_wind.speed   = GetUIFloat(hWnd, id); break;
                case IDC_SPINNER11: settings->options.groundLevel = GetUIFloat(hWnd, id); break;
                }
                
                if (id != IDC_SPINNER11) {
                    // Environment has changed; notify parent
                    SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hWnd), SN_CHANGE), (LPARAM)hWnd);
                } else {
                    // Render options have changed; notify parent
                    SendMessage(GetParent(hWnd), WM_COMMAND, MAKELONG(GetDlgCtrlID(hWnd), BN_CLICKED), (LPARAM)hWnd);
                }
                break;
            }
		    break;
    }
    return FALSE;
}

const RenderOptions& Dialogs::Settings_GetRenderOptions(HWND hWnd)
{
    Settings* settings = (Settings*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    assert(settings != NULL);
    return settings->options;
}

Environment& Dialogs::Settings_GetEnvironment(HWND hWnd)
{
    Settings* settings = (Settings*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    assert(settings != NULL);
    return settings->environment;
}

HWND Dialogs::CreateSettingsDialog(HWND hWndParent, const Environment& environment, ISoundEngine* sound)
{
    Settings* settings = new Settings;
    settings->environment = environment;
    settings->sound       = sound;

    HWND hDlg = CreateDialogParam(NULL, MAKEINTRESOURCE(IDD_SETTINGS), hWndParent, SettingsDialogProc, (LPARAM)settings);
    if (hDlg == NULL)
    {
        delete settings;
    }
    return hDlg;
}
