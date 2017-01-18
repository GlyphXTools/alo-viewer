#ifndef SPHERICALHARMONICS_H
#define SPHERICALHARMONICS_H

#include "General/GameTypes.h"

namespace Alamo {
namespace SphericalHarmonics {

/*
 * Calculates the three spherical harmonics matrices from the lights and ambient color
 *  @matrices: pointer to the three output matrices; red, green and blue, respectively.
 *  @lights:   pointer to the array of input lights.
 *  @nLights:  number of lights in the @lights array.
 *  @ambient:  ambient color to encode in the matrices.
 */
void Calculate_Matrices(Matrix* matrices, const DirectionalLight* lights, int nLights, const Color& ambient);

}
}

#endif