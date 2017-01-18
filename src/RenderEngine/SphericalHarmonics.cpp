#include "RenderEngine/SphericalHarmonics.h"
using namespace Alamo;

//
// Spherical Harmonics Lighting Calculations
// Based on http://graphics.stanford.edu/papers/envmap/
//
void SphericalHarmonics::Calculate_Matrices(Matrix* matrices, const DirectionalLight* lights, int nLights, const Color& ambient)
{
    // Calculate the 9 Spherical Harmonic function coefficients per color
    float coefficients[3][9] = {0};
    for (int i = 0; i < nLights; i++)
	{
        float coeffs[9] = {0};

        // We flip the Z value because of the wanted coord. system handedness.
		D3DXSHEvalDirection(coeffs, 3, &(lights[i].m_direction * Vector3(1,1,-1)));
        for (int j = 0; j < 9; j++)
        {
            coefficients[0][j] += lights[i].m_color.r * lights[i].m_color.a * coeffs[j];
            coefficients[1][j] += lights[i].m_color.g * lights[i].m_color.a * coeffs[j];
            coefficients[2][j] += lights[i].m_color.b * lights[i].m_color.a * coeffs[j];
        }
	}

	// Compose the 9 coefficients per color into three matrices.
    // See the paper for details.
	float c1 = 0.429043f, c2 = 0.511664f;
	float c3 = 0.743125f, c4 = 0.886227f, c5 = 0.247708f;

	memset(matrices, 0, 3 * sizeof(Matrix));
	for (int col = 0 ; col < 3; col++)
	{
		const float* coeffs = coefficients[col];
		matrices[col]._11 = c1*coeffs[8];  // c1 L_{22} 
		matrices[col]._12 = c1*coeffs[4];  // c1 L_{2-2}
		matrices[col]._13 = c1*coeffs[7];  // c1 L_{21} 
		matrices[col]._14 = c2*coeffs[3];  // c2 L_{11} 

		matrices[col]._21 = c1*coeffs[4];  // c1 L_{2-2}
		matrices[col]._22 = -c1*coeffs[8]; //-c1 L_{22} 
		matrices[col]._23 = c1*coeffs[5];  // c1 L_{2-1}
		matrices[col]._24 = c2*coeffs[1];  // c2 L_{1-1}

		matrices[col]._31 = c1*coeffs[7];  // c1 L_{21} 
		matrices[col]._32 = c1*coeffs[5];  // c1 L_{2-1}
		matrices[col]._33 = c3*coeffs[6];  // c3 L_{20} 
		matrices[col]._34 = c2*coeffs[2];  // c2 L_{10} 

		matrices[col]._41 = c2*coeffs[3];  // c2 L_{11} 
		matrices[col]._42 = c2*coeffs[1];  // c2 L_{1-1}
		matrices[col]._43 = c2*coeffs[2];  // c2 L_{10} 
		matrices[col]._44 = c4*coeffs[0] - c5*coeffs[6] ; // c4 L_{00} - c5 L_{20}
	}

    // Add ambience
    matrices[0]._44 += ambient.r * ambient.a;
	matrices[1]._44 += ambient.g * ambient.a;
	matrices[2]._44 += ambient.b * ambient.a;
}
