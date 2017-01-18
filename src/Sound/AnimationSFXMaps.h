#ifndef ANIMATIONSFXMAPS_H
#define ANIMATIONSFXMAPS_H

#include <string>
#include <map>

namespace Alamo {
namespace AnimationSFXMaps {

struct Event
{
    enum Type { SOUND, SURFACE };

    Type        type;
    std::string name;
    std::string bone;   // Only valid for SURFACE events
};

typedef std::multimap<float, Event> SFXMap;

void Initialize();
void Uninitialize();

const SFXMap* GetEvents(const std::string& animation);

}
}

#endif