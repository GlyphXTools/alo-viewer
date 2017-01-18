#include "Assets/Assets.h"
#include "Sound/DirectSound8/SoundEngine.h"
#include "Sound/DirectSound8/Exceptions.h"
#include "Sound/WaveFile.h"
#include "General/Math.h"
#include "General/Log.h"
#include <dsound.h>
using namespace std;

namespace Alamo {
namespace DirectSound8 {

struct SoundEngine::SoundTypeInfo
{
    int m_nInstances;
};

struct SoundEngine::Sound
{
    // Resource management
    IDirectSoundBuffer*  m_buffer;

    // Playback properties
    float           m_volume;
    DWORD           m_baseFreq;
    float           m_pitch;
    DWORD           m_playCount;
    const SFXEvent& m_event;
    SoundType       m_type;

    void SetVolume(double volume)
    {
        // 1e-10 corresponds to -100dB, the minimum
        volume = min(1.0, max(1e-10, m_volume * volume));

        // DirectSound wants the *attenuation* in 100ths of *decibels*, so to provide
        // a linear volume scale, we need to do some converting.
        LONG attenuation = (LONG)(1000 * log10(volume));
        m_buffer->SetVolume(attenuation);
    }

    void SetPitch(float pitch)
    {
        pitch = max(0.0001f, m_pitch * pitch);
        m_buffer->SetFrequency( (DWORD)(m_baseFreq * pitch) );
    }

    void Pause()
    {
        m_buffer->Stop();
    }

    void Play()
    {
        m_buffer->Play(0, 0, (m_playCount != 1) ? DSBPLAY_LOOPING : 0);
    }

    Sound(IDirectSoundBuffer* buffer, const SFXEvent& e, const SoundType type)
        : m_event(e), m_type(type)
    {
        m_buffer = buffer;
        
        // Set sound properties
        m_buffer->GetFrequency(&m_baseFreq);
        m_volume    = GetRandom(e.volume.min, e.volume.max);
        m_pitch     = GetRandom(e.pitch.min, e.pitch.max);
        m_playCount = e.playCount;
        
        SetVolume(1.0f);
        SetPitch(1.0f);
    }

    ~Sound()
    {
        SAFE_RELEASE(m_buffer);
    }
};

// Returns a free event ID
size_t SoundEngine::GetEvent()
{
    if (m_freeEvents.empty())
    {
        HANDLE hEvent;
        if ((hEvent = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
        {
            return -1;
        }
        m_freeEvents.push(m_hEvents.size());
        m_hEvents.push_back(hEvent);
    }
    size_t e = m_freeEvents.top();
    m_freeEvents.pop();
    return e;
}

// Recycles a free event ID
void SoundEngine::ReleaseEvent(size_t e)
{
    if (e != -1)
    {
        m_freeEvents.push(e);
    }
}

void SoundEngine::OnSoundCompleted(size_t i)
{
    Sound* sound = m_sounds[i];    
    if (sound->m_playCount > 1)
    {
        // Decrease loop count
        sound->m_playCount--;
        sound->Play();
    }
    else if (sound->m_playCount >= 0)
    {
        // Clean it up
        SoundTypeMap::iterator p = m_soundTypes.find(&sound->m_event);
        assert(p != m_soundTypes.end());
        if (--p->second.m_nInstances == 0)
        {
            m_soundTypes.erase(p);
        }
        DestroySound(i);
    }
}

SoundEngine::Sound* SoundEngine::CreateSound(const SFXEvent& e, const string& sample, const SoundType type)
{
    IDirectSoundBuffer* buffer;
    HRESULT             hRes;

    // First look in the cache
    SoundCache::iterator p = m_cache.find(sample);
    if (p == m_cache.end() || p->second == NULL)
    {
        // Not in cache, load the sound file
        ptr<IFile> file = Assets::LoadFile(sample, "Data/Audio/SFX/");
        if (file == NULL)
        {
            Log::WriteError("Unable to open sound file: %s\n", sample.c_str());
            return NULL;
        }

        ptr<WaveFile> wav = new WaveFile(file);
        if (wav == NULL)
        {
            Log::WriteError("The file is not a valid WAV file: %s\n", sample.c_str());
            return NULL;
        }

        // Create the sound buffer
        DSBUFFERDESC desc;
        desc.dwSize          = sizeof(DSBUFFERDESC);
        desc.dwFlags         = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLPOSITIONNOTIFY;
        desc.dwBufferBytes   = wav->GetSize();
        desc.dwReserved      = 0;
        desc.lpwfxFormat     = &wav->GetFormat();
        desc.guid3DAlgorithm = GUID_NULL;

        if (FAILED(hRes = m_pDirectSound->CreateSoundBuffer(&desc, &buffer, NULL)))
        {
            throw DirectSoundException(hRes);
        }

        // Fill the sound buffer
        void *data;
        DWORD size;
        buffer->Lock(0, 0, &data, &size, NULL, NULL, DSBLOCK_ENTIREBUFFER);
        memcpy(data, wav->GetData(), size);
        buffer->Unlock(data, size, NULL, 0);
        
        // Add to cache
        m_cache[sample] = buffer;
        buffer->AddRef();
    }
    else
    {       
        // Sound is in the cache, duplicate buffer
        if (FAILED(hRes = m_pDirectSound->DuplicateSoundBuffer(p->second, &buffer)))
        {
            throw DirectSoundException(hRes);
        }
        
        // Fix for DirectSound 'issue': always change volume after duplication
        buffer->SetVolume(0);
        buffer->SetVolume(1);
    }

    Sound* sound = new Sound(buffer, e, type);
    sound->SetVolume(m_volume);

    size_t index = -1;

    // We're going to modify lists, suspend the cleanup thread
    try
    {
        // Get an event for this sound's notification
        if ((index = GetEvent()) == -1)
        {
            throw SoundException(L"Unable to allocate resources");
        }

        // Add the sound to the sound list, same index as the event
        if (index >= m_sounds.size())
        {
            m_sounds.resize(index + 1, NULL);
        }
        m_sounds[index] = sound;

        // Set up DirectSound's end-of-buffer notification
        IDirectSoundNotify* notify;
        if (FAILED(hRes = sound->m_buffer->QueryInterface(IID_IDirectSoundNotify, (LPVOID*)&notify)))
        {
            throw DirectSoundException(hRes);
        }

        DSBPOSITIONNOTIFY dpn;
        dpn.dwOffset     = DSBPN_OFFSETSTOP;
        dpn.hEventNotify = m_hEvents[index];
        if (FAILED(hRes = notify->SetNotificationPositions(1, &dpn)))
        {
            SAFE_RELEASE(notify);
            throw DirectSoundException(hRes);
        }
        SAFE_RELEASE(notify);
    }
    catch (...)
    {
        // Cleanup the sound
        ReleaseEvent(index);
        delete sound;
        throw;
    }
    return sound;
}

void SoundEngine::PlaySoundEvent(const SFXEvent& e)
{
    // Select a sample from the list and create sound
    const vector<string>* samples = NULL;
    SoundType             type;

         if (!e.preSamples.empty())  { type = PRE;  samples = &e.preSamples;  }
    else if (!e.samples.empty())     { type = MAIN; samples = &e.samples;     }
    else if (!e.postSamples.empty()) { type = POST; samples = &e.postSamples; }

    if (samples != NULL)
    {
        SoundTypeMap::iterator p = m_soundTypes.find(&e);
        if (p == m_soundTypes.end() || p->second.m_nInstances < e.maxInstances)
        {
            // Something to play
            try
            {
                SuspendCleanupThread();
                int s = GetRandom(0, (int)samples->size());
                Sound* sound = CreateSound(e, (*samples)[s], type );
                if (sound != NULL)
                {
                    // Play sound
                    sound->Play();
                    
                    if (p == m_soundTypes.end())
                    {
                        SoundTypeInfo info;
                        info.m_nInstances = 0;
                        pair<SoundTypeMap::iterator, bool> q = m_soundTypes.insert(make_pair(&e, info));
                        p = q.first;
                    }
                    p->second.m_nInstances++;
                }
                ResumeCleanupThread();
            }
            catch (...)
            {
                ResumeCleanupThread();
                throw;
            }
        }
    }
}

void SoundEngine::SetMasterVolume(float volume)
{
    m_volume = volume;
    for (size_t i = 0; i < m_sounds.size(); i++)
    {
        if (m_sounds[i] != NULL)
        {
            m_sounds[i]->SetVolume(volume);
        }
    }
}

void SoundEngine::Pause()
{
    // m_paused prevents the cleanup thread from cleaning up the sounds
    // because of the Stop() notification
    SuspendCleanupThread();
    m_paused = true;
    ResumeCleanupThread();

    for (size_t i = 0; i < m_sounds.size(); i++)
    {
        if (m_sounds[i] != NULL)
        {
            m_sounds[i]->Pause();
        }
    }
}

void SoundEngine::Play()
{
    SuspendCleanupThread();
    m_paused = false;
    ResumeCleanupThread();

    for (size_t i = 0; i < m_sounds.size(); i++)
    {
        if (m_sounds[i] != NULL)
        {
            m_sounds[i]->Play();
        }
    }
}

void SoundEngine::DestroySound(size_t i)
{
    ReleaseEvent(i);
    delete m_sounds[i];
    m_sounds[i] = NULL;
}

void SoundEngine::Clear()
{
    SuspendCleanupThread();

    for (size_t i = 0; i < m_sounds.size(); i++)
    {
        DestroySound(i);
    }
    m_sounds.clear();
    m_soundTypes.clear();

    // Clear the cache
    for (SoundCache::const_iterator p = m_cache.begin(); p != m_cache.end(); p++)
    {
        p->second->Release();
    }
    m_cache.clear();

    // First two events are reserved, clear the rest
    for (size_t i = 2; i < m_hEvents.size(); i++)
    {
        CloseHandle(m_hEvents[i]);
    }
    m_hEvents.resize(2);

    while (!m_freeEvents.empty())
    {
        m_freeEvents.pop();
    }

    ResumeCleanupThread();
}

static DWORD WINAPI ThreadFunc(LPVOID lpParam)
{
    ((SoundEngine*)lpParam)->ThreadFunc();
    return 0;
}

void SoundEngine::ThreadFunc()
{
    DWORD res;
    WaitForSingleObject(m_hAccess, INFINITE);
    while ((res = WaitForMultipleObjects((DWORD)m_hEvents.size(), &m_hEvents[0], FALSE, INFINITE)) != WAIT_OBJECT_0)
    {
        if (res > WAIT_OBJECT_0 && res < WAIT_OBJECT_0 + m_hEvents.size())
        {
            // An event was signaled
            if (res == WAIT_OBJECT_0 + 1)
            {
                // The list of events wants to be changed
                ReleaseMutex(m_hAccess);
                // Wait until the change is done
                WaitForSingleObject(m_hEvents[1], INFINITE);
                WaitForSingleObject(m_hAccess,    INFINITE);
            }
            else if (!m_paused)
            {
                // Sound has reached end, notify
                OnSoundCompleted(res - WAIT_OBJECT_0);
            }
        }
    }
    ReleaseMutex(m_hAccess);
}

void SoundEngine::SuspendCleanupThread()
{
    SignalObjectAndWait(m_hEvents[1], m_hAccess, INFINITE, FALSE);
}

void SoundEngine::ResumeCleanupThread()
{
    SetEvent(m_hEvents[1]);
    ReleaseMutex(m_hAccess);
}

SoundEngine::SoundEngine(HWND hWnd)
{
    HRESULT hRes;
    if (FAILED(hRes = DirectSoundCreate(NULL, &m_pDirectSound, NULL)))
    {
        throw DirectSoundException(hRes);
    }

    if (FAILED(hRes = m_pDirectSound->SetCooperativeLevel(hWnd, DSSCL_NORMAL)))
    {
        throw DirectSoundException(hRes);
    }

    m_hEvents.resize(2);
    if ((m_hEvents[0] = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
        SAFE_RELEASE(m_pDirectSound);
        throw runtime_error("Unable to create event");
    }

    if ((m_hEvents[1] = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL)
    {
        CloseHandle(m_hEvents[0]);
        SAFE_RELEASE(m_pDirectSound);
        throw runtime_error("Unable to create event");
    }

    if ((m_hAccess = CreateMutex(NULL, FALSE, NULL)) == NULL)
    {
        CloseHandle(m_hEvents[1]);
        CloseHandle(m_hEvents[0]);
        SAFE_RELEASE(m_pDirectSound);
        throw runtime_error("Unable to create mutex");
    }

    DWORD ThreadID;
    if ((m_hThread = CreateThread(NULL, 0, Alamo::DirectSound8::ThreadFunc, this, 0, &ThreadID)) == NULL)
    {
        CloseHandle(m_hAccess);
        CloseHandle(m_hEvents[1]);
        CloseHandle(m_hEvents[0]);
        SAFE_RELEASE(m_pDirectSound);
        throw runtime_error("Unable to create thread");
    }

    m_paused = false;
    m_volume = 1.0f;
}

SoundEngine::~SoundEngine()
{   
    // Abort cleanup thread
    SetEvent(m_hEvents[0]);
    WaitForSingleObject(m_hThread, INFINITE);
    CloseHandle(m_hThread);
    
    // Cleanup resources
    Clear();
    CloseHandle(m_hAccess);
    CloseHandle(m_hEvents[1]);
    CloseHandle(m_hEvents[0]);
    SAFE_RELEASE(m_pDirectSound);
}

}
}