//
// Default shader file for the Alamo Viewer, by Mike Lankamp
// Shows the model animated, in green.
// Use this when the wanted shader cannot be found.
//
string _ALAMO_RENDER_PHASE = "Opaque";
string _ALAMO_VERTEX_PROC = "RSkin";
string _ALAMO_VERTEX_TYPE = "alD3dVertRSkinNU2";
bool _ALAMO_TANGENT_SPACE = false;
bool _ALAMO_SHADOW_VOLUME = false;
int _ALAMO_BONES_PER_VERTEX = 1;

static const int MAX_BONES = 24;
float4x3 m_skinMatrixArray[MAX_BONES] : SKINMATRIXARRAY;
float4x4 m_worldViewProj              : WORLDVIEWPROJECTION;
float4x4 m_viewProj                   : VIEWPROJECTION;

float4 vs_main(float4 Pos : POSITION, float4 Normal : NORMAL) : POSITION
{
    float4x3 transform = m_skinMatrixArray[Normal.w];
	return mul(float4(mul(Pos, transform), 1), m_viewProj);
}

float4 ps_main() : COLOR
{
	return float4(0,1,0,1);
}

vertexshader vs_main_bin = compile vs_1_1 vs_main();
pixelshader  ps_main_bin = compile ps_1_1 ps_main();

technique t0
<
	string LOD="DX8";
>
{
    pass t0_p0
    {
   		ZWriteEnable = TRUE;
   		ZFunc = LESSEQUAL;
   		DestBlend = INVSRCALPHA;
   		SrcBlend = SRCALPHA;

        VertexShader = (vs_main_bin);
        PixelShader = (ps_main_bin);
    }
}
