#include "RenderWindow.h"
#include "RenderEngine/DirectX9/RenderEngine.h"
#include "General/Log.h"
#include "General/Exceptions.h"
#include "config.h"
using namespace Alamo;

enum DragMode {
    DRAG_NONE,
    DRAG_ROTATE,
    DRAG_MOVE,
    DRAG_ZOOM
};

struct RenderWindowInfo
{
    IRenderEngine* engine;
    GameID         game;
    RenderOptions  options;
    DragMode       dragMode;
	long           dragXStart;
	long           dragYStart;
	Camera         dragStartCam;
};

struct RenderWindowCreateInfo
{
    GameID             game;
    const Environment* environment;
};

static void CreateEngine(RenderWindowInfo* info, HWND hWnd, const RenderSettings& settings, const Environment& env, GameID game)
{
    try
    {
        switch (game)
        {
        case GID_EAW:
        case GID_EAW_FOC:
            info->engine = new DirectX9::RenderEngine(hWnd, settings, env, false);
            break;

        case GID_UAW_EA:
            info->engine = new DirectX9::RenderEngine(hWnd, settings, env, true);
            break;
        }
        info->game = game;
    }
    catch (GraphicsException& e)
    {
        Log::WriteError("Unable to initialize Direct3D: %ls\n", e.what());
        info->game = GID_UNKNOWN;
    }
}

static LRESULT CALLBACK RenderWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    RenderWindowInfo* info = (RenderWindowInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    switch (uMsg)
    {
        case WM_CREATE:
        {
            info = new RenderWindowInfo;
            info->dragMode = DRAG_NONE;
            info->engine   = NULL;
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG)(LONG_PTR)info);

            CREATESTRUCT* pcs = (CREATESTRUCT*)lParam;
            const RenderWindowCreateInfo* rwci = (RenderWindowCreateInfo*)pcs->lpCreateParams;

            try
            {
                // Create engine
                CreateEngine(info, hWnd, Config::GetRenderSettings(), *rwci->environment, rwci->game);
            }
            catch (GraphicsException& e)
            {
                Log::WriteError("Unable to initialize Direct3D: %ls\n", e.what());
                info->engine = NULL;
            }
        }
        break;

        case WM_DESTROY:
            if (info->engine != NULL)
            {
                // Write persistent information to configuration
                Config::SetRenderSettings(info->engine->GetSettings());
                Config::SetEnvironment(info->engine->GetEnvironment());
                delete info->engine;
            }
            delete info;
            break;

        case WM_PAINT:
            if (info->engine != NULL)
            {
                info->engine->Render(info->options);
            }
            else
            {
                PAINTSTRUCT ps;
                HDC hDC = BeginPaint(hWnd, &ps);
                FillRect(hDC, &ps.rcPaint, (HBRUSH)GetStockObject(BLACK_BRUSH));
                EndPaint(hWnd, &ps);
            }
            break;

        case WM_SIZE:
            if (info->engine != NULL)
            {
                // Notify renderer of window resize
                RenderSettings settings = info->engine->GetSettings();
                settings.m_screenWidth  = LOWORD(lParam);
                settings.m_screenHeight = HIWORD(lParam);
                info->engine->SetSettings(settings);
            }
            break;

        case WM_SETFOCUS:
            // Yes, we want focus please
            return TRUE;

        case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
            if (info->engine != NULL)
            {
                // Start dragging, remember start settings
			    info->dragStartCam = info->engine->GetCamera();
			    info->dragXStart   = LOWORD(lParam);
			    info->dragYStart   = HIWORD(lParam);
                info->dragMode = (wParam & MK_CONTROL) ? DRAG_ZOOM : (uMsg == WM_LBUTTONDOWN) ? DRAG_MOVE : DRAG_ROTATE;
			    SetCapture(hWnd);
			    SetFocus(hWnd);
            }
			break;

		case WM_MOUSEMOVE:
			if (info->engine != NULL)
            {
                if (info->dragMode != DRAG_NONE)
			    {
				    // Yay, math time!
                    long x = (short)LOWORD(lParam) - info->dragXStart;
                    long y = (short)HIWORD(lParam) - info->dragYStart;

				    Camera  camera = info->dragStartCam;
				    Vector3 diff   = info->dragStartCam.m_position - info->dragStartCam.m_target;
    				
				    // Get the orthogonal vector
				    Vector3 orthVec = diff.cross(camera.m_up);
                    orthVec.normalize();

                    if (info->dragMode == DRAG_ROTATE)
				    {
					    // Lets rotate
					    D3DXMATRIX rotateXY, rotateZ, rotate;
					    D3DXMatrixRotationZ( &rotateZ, -ToRadians(x / 2.0f) );
					    D3DXMatrixRotationAxis( &rotateXY, &orthVec, ToRadians(y / 2.0f) );
					    D3DXMatrixMultiply( &rotate, &rotateXY, &rotateZ );
					    D3DXVec3TransformCoord( &camera.m_position, &diff, &rotate );
					    camera.m_position += camera.m_target;
				    }
				    else if (info->dragMode == DRAG_MOVE)
				    {
					    // Lets translate
					    Vector3 Up = orthVec.cross(diff);
                        Up.normalize();
    					
					    // The distance we move depends on the distance from the object
					    // Large distance: move a lot, small distance: move a little
					    float multiplier = diff.length() / 1000;

					    camera.m_target  += (float)x * multiplier * *(D3DXVECTOR3*)&orthVec;
					    camera.m_target  += (float)y * multiplier * Up;
					    camera.m_position = diff + camera.m_target;
				    }
				    else if (info->dragMode == DRAG_ZOOM)
				    {
					    // Lets zoom
					    // The amount we scroll in and out depends on the distance.
					    Vector3 diff = camera.m_position - camera.m_target;
					    float olddist = diff.length();
					    float newdist = (float)max(7.0f, olddist - sqrt(olddist) * -y);
					    camera.m_position = diff * (newdist / olddist) + camera.m_target;
				    }

				    info->engine->SetCamera( camera );
			    }
            }
			break;

		case WM_MOUSEWHEEL:
			if (info->dragMode == DRAG_NONE)
			{
				Camera camera = info->engine->GetCamera();

				// The amount we scroll in and out depends on the distance.
                Vector3 diff = camera.m_position - camera.m_target;
				float olddist = diff.length();
				float newdist = (float)max(10.0f, olddist - sqrt(olddist) * (SHORT)HIWORD(wParam) / WHEEL_DELTA);
				camera.m_position = diff * (newdist / olddist) + camera.m_target;
				info->engine->SetCamera(camera);
			}
			break;

        case WM_LBUTTONUP:
		case WM_RBUTTONUP:
			// Stop dragging
			info->dragMode = DRAG_NONE;
			ReleaseCapture();
			break;

        case WM_DROPFILES:
		    // User dropped a filename on the window
            // Forward to parent window
            SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
            break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

HWND CreateRenderWindow(HWND hWndParent, GameID game, const Environment& env, int x, int y, int w, int h)
{
    RenderWindowCreateInfo rwci;
    rwci.game        = game;
    rwci.environment = &env;

    HINSTANCE hInstance = (HINSTANCE)(LONG_PTR)GetWindowLongPtr(hWndParent, GWLP_HINSTANCE);
    return CreateWindowEx(WS_EX_CLIENTEDGE | WS_EX_ACCEPTFILES, L"AloRenderWindow", NULL, WS_CHILD | WS_VISIBLE, x, y, w, h, hWndParent, NULL, hInstance, &rwci);
}

IRenderEngine* GetRenderEngine(HWND hWnd)
{
    RenderWindowInfo* info = (RenderWindowInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (info != NULL)
    {
        return info->engine;
    }
    return NULL;
}

void SetRenderWindowOptions(HWND hWnd, const RenderOptions& options)
{
    RenderWindowInfo* info = (RenderWindowInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (info != NULL)
    {
        info->options = options;
    }
}

void SetRenderWindowGameEngine(HWND hWnd, GameID game)
{
    RenderWindowInfo* info = (RenderWindowInfo*)(LONG_PTR)GetWindowLongPtr(hWnd, GWLP_USERDATA);
    if (info != NULL && (info->game != game || info->engine == NULL))
    {
        RenderSettings settings;
        Environment    env;
        Camera         camera;

        bool replace = info->engine != NULL;
        if (replace) {
            // Read current environment
            settings = info->engine->GetSettings();
            env      = info->engine->GetEnvironment();
            camera   = info->engine->GetCamera();
        } else {
            // Get default environment
            settings = Config::GetRenderSettings();
            env      = Config::GetDefaultEnvironment();
            Config::GetEnvironment(&env);
        }

        // Delete old engine
        delete info->engine;
        info->engine = NULL;

        // Create new engine
        CreateEngine(info, hWnd, settings, env, game);
        if (replace && info->engine != NULL)
        {
            info->engine->SetCamera(camera);
        }
        RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}

bool RegisterRenderWindow(HINSTANCE hInstance)
{
	WNDCLASSEX wcx;
	wcx.cbSize        = sizeof wcx;
    wcx.style		  = CS_HREDRAW | CS_VREDRAW;
    wcx.lpfnWndProc	  = RenderWindowProc;
    wcx.cbClsExtra	  = 0;
    wcx.cbWndExtra    = 0;
    wcx.hInstance     = hInstance;
    wcx.hIcon         = NULL;
    wcx.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wcx.hbrBackground = NULL;
    wcx.lpszMenuName  = NULL;
    wcx.lpszClassName = L"AloRenderWindow";
    wcx.hIconSm       = NULL;
	return (RegisterClassEx(&wcx) != 0);
}

void UnregisterRenderWindow(HINSTANCE hInstance)
{
    UnregisterClass(L"AloRenderWindow", hInstance);
}
