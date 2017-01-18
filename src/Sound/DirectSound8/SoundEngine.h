#ifndef RENDERENGINE_DS8_H
#define RENDERENGINE_DS8_H

#include "Sound/SoundEngine.h"
#include <windows.h>
#include <dsound.h>
#include <stack>
#include <vector>

namespace Alamo {
namespace DirectSound8 {

class SoundEngine : public ISoundEngine
{
    friend DWORD WINAPI ThreadFunc(LPVOID);

    struct Sound;
    typedef std::map<std::string, IDirectSoundBuffer*> SoundCache;
    enum SoundType { PRE, MAIN, POST };

    struct SoundTypeInfo;
    typedef std::map<const SFXEvent*, SoundTypeInfo> SoundTypeMap;

    IDirectSound*       m_pDirectSound;
    HANDLE              m_hThread;
    HANDLE              m_hAccess;
    std::vector<HANDLE> m_hEvents;
    std::vector<Sound*> m_sounds;
    SoundTypeMap        m_soundTypes;
    std::stack<size_t>  m_freeEvents;
    SoundCache          m_cache;
    bool                m_paused;
    float               m_volume;
    const Camera*       m_camera;

    size_t GetEvent();
    void   ReleaseEvent(size_t e);
    Sound* CreateSound(const SFXEvent& e, const std::string& sample, SoundType type);
    void   DestroySound(size_t i);
    void   OnSoundCompleted(size_t i);
    
    void   SuspendCleanupThread();
    void   ResumeCleanupThread();

    void ThreadFunc();

    void PlaySoundEvent(const SFXEvent& e);
    void SetMasterVolume(float volume);
    void Pause();
    void Play();
    void Clear();

    ~SoundEngine();
public:
    SoundEngine(HWND hWnd);
};

}
}

#endif
