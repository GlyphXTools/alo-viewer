#ifndef SOUNDENGINE_H
#define SOUNDENGINE_H

#include "General/Objects.h"
#include "Sound/SFXEvents.h"

namespace Alamo
{

// Abstract sound engine
class ISoundEngine : public IObject
{
public:
    virtual void PlaySoundEvent(const SFXEvent& e) = 0;
    virtual void SetMasterVolume(float volume) = 0;
    virtual void Pause() = 0;
    virtual void Play() = 0;
    virtual void Clear() = 0;
};

}

#endif