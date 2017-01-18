#ifndef LIGHTSOURCES_H
#define LIGHTSOURCES_H

#include "General/GameTypes.h"

namespace Alamo {
namespace LightSources {

void Initialize();
void Uninitialize();

}

const LightFieldSource* GetLightFieldSource(std::string name);
}

#endif