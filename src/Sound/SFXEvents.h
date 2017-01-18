#ifndef SFXEVENTS_H
#define SFXEVENTS_H

#include "General/Utils.h"
#include <vector>

namespace Alamo {

struct SFXEvent
{
    enum Type { SFX_2D, SFX_3D, SFX_GUI };

    Type           type;
    int            priority;
    int            maxInstances;
    int            playCount;
    float          probability;
    bool           localize;
    bool           playSequentially;
    bool           killsPreviousObjectSFX;
    float          volumeSaturationDistance;
    float          loopFadeOutSeconds;
    float          loopFadeInSeconds;
    std::string    chainedSFXEvent;
    range<float>   volume;
    range<float>   pitch;
    range<float>   predelay;
    std::vector<std::string> samples;
    std::vector<std::string> preSamples;
    std::vector<std::string> postSamples;
};

namespace SFXEvents {

void Initialize();
void Uninitialize();

const SFXEvent* GetEvent(const std::string& name);

}
}

#endif