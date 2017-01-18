string _ALAMO_RENDER_PHASE = "Opaque";
string _ALAMO_VERTEX_PROC = "Mesh";
string _ALAMO_VERTEX_TYPE = "alD3dVertN";

// Matrices
float4x4 m_world      : WORLD;
float4x4 m_view       : VIEW;
float4x4 m_projection : PROJECTION;
float4x4 m_worldView  : WORLDVIEW;

// material parameters
float4 m_solidColor  = {0.0f, 1.0f, 0.0f, 1.0f};

technique TNoShader
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

        // material (just using emissive)
	MaterialAmbient  = (m_solidColor); 
      	MaterialDiffuse  = (m_solidColor); 
      	MaterialSpecular = (m_solidColor);
      	MaterialEmissive = (m_solidColor); 
      	       
        Lighting       = TRUE;
        LightEnable[0] = FALSE;
        SpecularEnable = FALSE;
        
        // texture stages
        ColorOp[0]   = SELECTARG1;
        ColorArg1[0] = DIFFUSE;
        ColorArg2[0] = DIFFUSE;
        AlphaOp[0]   = SELECTARG1;
        AlphaArg1[0] = DIFFUSE;
        AlphaArg2[0] = DIFFUSE;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
			
	AlphaBlendEnable = FALSE;
	DestBlend = ZERO;
	SrcBlend = ONE;
		
        // shaders
        VertexShader = NULL;
        PixelShader  = NULL;
    }
}
