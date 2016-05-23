//Stuff all/most of McFX shared shaders need
NAMESPACE_ENTER(MFX)
#define MFX_SETTINGS_DEF "ReShade/McFX_settings.cfg"
#define MFX_SETTINGS_UNDEF "ReShade/McFX_settings.undef" 

#include MFX_SETTINGS_DEF

#if( HDR_MODE == 0)
 #define RENDERMODE RGBA8
#elif( HDR_MODE == 1)
 #define RENDERMODE RGBA16F
#else
 #define RENDERMODE RGBA32F
#endif

//global vars
#define ScreenSize 	float4(BUFFER_WIDTH, BUFFER_RCP_WIDTH, float(BUFFER_WIDTH) / float(BUFFER_HEIGHT), float(BUFFER_HEIGHT) / float(BUFFER_WIDTH)) //x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
#define PixelSize  	float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT)
#define PI 		3.1415972
#define PIOVER180 	0.017453292
#define AUTHOR 		MartyMcFly
#define FOV 		75
#define MFX_LumCoeff 	float3(0.212656, 0.715158, 0.072186)
#define zFarPlane 	1
#define zNearPlane 	0.001		//I know, weird values but ReShade's depthbuffer is ... odd
#define aspect          (BUFFER_RCP_HEIGHT/BUFFER_RCP_WIDTH)
#define InvFocalLen 	float2(tan(0.5f*radians(FOV)) / (float)BUFFER_RCP_HEIGHT * (float)BUFFER_RCP_WIDTH, tan(0.5f*radians(FOV)))

//textures
texture   texHDR1 	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; MipLevels = 8; Format = RENDERMODE;};	
texture   texHDR2 	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; MipLevels = 8; Format = RENDERMODE;};

//samplers
sampler2D SamplerHDR1
{
	Texture = texHDR1;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerHDR2
{
	Texture = texHDR2;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Functions														     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float GetLinearDepth(float depth)
{
	#if RFX_LogDepth 
		return depth = (exp(depth * log(2f)) - (1f-0.0045*(1f-pow(abs(depth * log(300f)),100))))/0.056f;
	#else 
		return 1 / ((depth * ((zFarPlane - zNearPlane) / (-zFarPlane * zNearPlane)) + zFarPlane / (zFarPlane * zNearPlane)));
	#endif
}

void PS_Init(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdrT : SV_Target0) 
{
	hdrT = tex2D(RFX_originalColor, texcoord.xy);
}

float4 PS_Overlay(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 color = tex2D(SamplerHDR2, texcoord.xy);
	return color;
}

#if USE_AMBIENTOCCLUSION || USE_DEPTHOFFIELD
technique Init_Tech  < enabled = true; >
{
	pass Init_HDR1						//later, numerous DOF shaders have different passnumber but later passes depend
	{							//on fixed HDR1 HDR2 HDR1 HDR2... sequence so a 2 pass DOF outputs HDR1 in pass 1 and 	
		VertexShader = RFX_VS_PostProcess;			//HDR2 in second pass, a 3 pass DOF outputs HDR2, HDR1, HDR2 so last pass outputs always HDR2
		PixelShader = PS_Init;
		RenderTarget = texHDR1;
	}

	pass Init_HDR2						//later, numerous DOF shaders have different passnumber but later passes depend
	{							//on fixed HDR1 HDR2 HDR1 HDR2... sequence so a 2 pass DOF outputs HDR1 in pass 1 and 	
		VertexShader = RFX_VS_PostProcess;			//HDR2 in second pass, a 3 pass DOF outputs HDR2, HDR1, HDR2 so last pass outputs always HDR2
		PixelShader = PS_Init;
		RenderTarget = texHDR2;
	}
}
#endif

#include MFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()

#pragma message "McFX 1.1.190 by Marty McFly\n\n"