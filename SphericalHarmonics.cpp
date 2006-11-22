#include "SphericalHarmonics.h"

//
// Comment this to use the fallback classical method to encode
// the directional lights into the matrices
//
//#define USE_PROPER_SPH

//
// Spherical Harmonics Lighting Calculations
// Adapted from http://graphics.stanford.edu/papers/envmap/prefilter.c
//

void SPH_Calculate_Matrices(D3DXMATRIX* matrices, LIGHT* Lights, int nLights, const D3DXVECTOR4& ambient)
{
	memset(matrices, 0, 3 * sizeof(D3DXMATRIX));

#ifdef USE_PROPER_SPH
	float coeffs[3][9];
	memset(coeffs, 0, sizeof(float) * 3 * 9);

	for (int i = 0; i < nLights; i++)
	{
		float red[9] = {0}, green[9] = {0}, blue[9] = {0};
		D3DXVECTOR3 direction(Lights[i].Direction.x, Lights[i].Direction.y, -Lights[i].Direction.z);
		D3DXSHEvalDirectionalLight(3, &direction,
			Lights[i].Diffuse.x * Lights[i].Diffuse.w * 0.1f,
			Lights[i].Diffuse.y * Lights[i].Diffuse.w * 0.1f,
			Lights[i].Diffuse.z * Lights[i].Diffuse.w * 0.1f, red, green, blue);
		D3DXSHAdd(coeffs[0], 3, coeffs[0], red);
		D3DXSHAdd(coeffs[1], 3, coeffs[1], green);
		D3DXSHAdd(coeffs[2], 3, coeffs[2], blue);
	}

	// Form the quadratic form matrix
	float c1 = 0.429043f, c2 = 0.511664f;
	float c3 = 0.743125f, c4 = 0.886227f, c5 = 0.247708f;
	for (int col = 0 ; col < 3; col++)
	{
		const float* coeff = coeffs[col];
		matrices[col]._11 = c1*coeff[8];  // c1 L_{22} 
		matrices[col]._12 = c1*coeff[4];  // c1 L_{2-2}
		matrices[col]._13 = c1*coeff[7];  // c1 L_{21} 
		matrices[col]._14 = c2*coeff[3];  // c2 L_{11} 

		matrices[col]._21 = c1*coeff[4];  // c1 L_{2-2}
		matrices[col]._22 = -c1*coeff[8]; //-c1 L_{22} 
		matrices[col]._23 = c1*coeff[5];  // c1 L_{2-1}
		matrices[col]._24 = c2*coeff[1];  // c2 L_{1-1}

		matrices[col]._31 = c1*coeff[7];  // c1 L_{21} 
		matrices[col]._32 = c1*coeff[5];  // c1 L_{2-1}
		matrices[col]._33 = c3*coeff[6];  // c3 L_{20} 
		matrices[col]._34 = c2*coeff[2];  // c2 L_{10} 

		matrices[col]._41 = c2*coeff[3];  // c2 L_{11} 
		matrices[col]._42 = c2*coeff[1];  // c2 L_{1-1}
		matrices[col]._43 = c2*coeff[2];  // c2 L_{10} 
		matrices[col]._44 = c4*coeff[0] - c5*coeff[6] ; // c4 L_{00} - c5 L_{20}
	}
#endif

	for (int i = 0; i < nLights; i++)
	{
#ifndef USE_PROPER_SPH
		matrices[0]._41 += Lights[i].Diffuse.x * -Lights[i].Direction.x;
		matrices[0]._42 += Lights[i].Diffuse.x * -Lights[i].Direction.y;
		matrices[0]._43 += Lights[i].Diffuse.x * -Lights[i].Direction.z;
		matrices[1]._41 += Lights[i].Diffuse.y * -Lights[i].Direction.x;
		matrices[1]._42 += Lights[i].Diffuse.y * -Lights[i].Direction.y;
		matrices[1]._43 += Lights[i].Diffuse.y * -Lights[i].Direction.z;
		matrices[2]._41 += Lights[i].Diffuse.z * -Lights[i].Direction.x;
		matrices[2]._42 += Lights[i].Diffuse.z * -Lights[i].Direction.y;
		matrices[2]._43 += Lights[i].Diffuse.z * -Lights[i].Direction.z;
#endif
		matrices[0]._44 += ambient.x * ambient.w;
		matrices[1]._44 += ambient.y * ambient.w;
		matrices[2]._44 += ambient.z * ambient.w;
	}
}
