#ifndef SURFACEFX_H
#define SURFACEFX_H

#include <string>

namespace Alamo {
namespace SurfaceFX {

struct Effect
{
    std::string sound;
};

void Initialize();
void Uninitialize();

const Effect* GetEffect(const std::string& name);

}
}
#endif