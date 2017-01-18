string _ALAMO_RENDER_PHASE = "Opaque";
string _ALAMO_VERTEX_PROC = "Mesh";
string _ALAMO_VERTEX_TYPE = "alD3dVertN";

// Matrices
float4x4 m_world      : WORLD;
float4x4 m_view       : VIEW;
float4x4 m_projection : PROJECTION;
float4x4 m_worldView  : WORLDVIEW;

float Intensity = 1.0f;

// material parameters
float4 m_solidColor  = {0.0f, 1.0f, 0.0f, 1.0f};

technique TNoShaderSkin
<
	string LOD = "FIXEDFUNCTION";
>
{
    pass P0
    {
        // transforms
        WorldTransform[0]   = (m_world);
        ViewTransform       = (m_view);
        ProjectionTransform = (m_projection);

        Lighting       = FALSE;
        
        // texture stages
        ColorOp[0]   = SELECTARG1;
        ColorArg1[0] = TFACTOR;
        ColorArg2[0] = TFACTOR;
        AlphaOp[0]   = SELECTARG1;
        AlphaArg1[0] = TFACTOR;
        AlphaArg2[0] = TFACTOR;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
			
        TextureFactor = (m_solidColor * Intensity);
	AlphaBlendEnable = FALSE;
	DestBlend = ZERO;
	SrcBlend = ONE;
		
        // shaders
        VertexShader = NULL;
        PixelShader  = NULL;
    }
}
