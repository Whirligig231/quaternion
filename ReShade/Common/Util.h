//Performance Check
#if RFX_SmartPerfCheck && (RFX_InitialStorage || RFX_DepthBufferCalc)
	#include "ReShade\Common\PerformanceCheck.h"
#else
	#define RFX_InitialStorageS 1
#endif

//Global Defines
#if defined(__RESHADE__) && __RESHADE__ >= 1700
	#define NAMESPACE_ENTER(name) namespace name {
	#define NAMESPACE_LEAVE() }
#else
	#define NAMESPACE_ENTER(name)
	#define NAMESPACE_LEAVE()
#endif

#define STR(name) #name
#define EFFECT(l,n) STR(ReShade/l/##n.h)

//Global Settings
#if RFX_Screenshot_Format != 2
	#pragma reshade screenshot_format bmp //ReShade_Screenshot_Format == 1
#else
	#pragma reshade screenshot_format png //ReShade_Screenshot_Format == 2
#endif

#if RFX_ShowStatistics == 1
  #pragma reshade statistics
#if RFX_ShowFPS == 1
  #pragma reshade showfps
#endif
#if RFX_ShowClock == 1
  #pragma reshade showclock
#endif
#endif

#if USE_RFX_ShowToggleMessage == 1
  #pragma reshade showtogglemessage
#endif

//Global Variables
#define RFX_pixelSize float2(1.0f/BUFFER_WIDTH, 1.0f/BUFFER_HEIGHT)
#define RFX_ScreenSize float2(BUFFER_WIDTH,BUFFER_HEIGHT)
#define RFX_ScreenSizeFull float4(BUFFER_WIDTH, BUFFER_RCP_WIDTH, float(BUFFER_WIDTH) / float(BUFFER_HEIGHT), float(BUFFER_HEIGHT) / float(BUFFER_WIDTH)) //x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
uniform float RFX_timer < string source = "timer"; >;
uniform float RFX_timeleft < string source = "timeleft"; >;
uniform float RFX_frametime < source = "frametime"; >;

//Global Textures
texture RFX_depthBufferTex : DEPTH;
texture2D RFX_backbufferTex : COLOR;

#if RFX_InitialStorage
#if RFX_InitialStorageS
texture2D RFX_originalTex { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA8; };
#else
texture2D RFX_originalTex : COLOR;
#endif
#else
texture2D RFX_originalTex : COLOR;
#endif

texture2D RFX_depthTex { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA32F; };
texture2D RFX_depthTexPing { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RGBA32F; };
 

//Global Samplers
sampler RFX_depthColor { Texture = RFX_depthBufferTex; };
sampler2D RFX_backbufferColor { Texture = RFX_backbufferTex; };
sampler2D RFX_originalColor { Texture = RFX_originalTex; };
sampler2D RFX_depthTexColor { Texture = RFX_depthTex; };
sampler2D RFX_depthTexPingColor { Texture = RFX_depthTexPing; };

//Vertex Shader
void RFX_VS_PostProcess(in uint id : SV_VertexID, out float4 pos : SV_Position, out float2 texcoord : TEXCOORD)
{
	texcoord.x = (id == 2) ? 2.0 : 0.0;
	texcoord.y = (id == 1) ? 2.0 : 0.0;
	pos = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
}

#if RFX_InitialStorage
#if RFX_InitialStorageS
//Store Original Shader
void RFX_PS_Original(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 RFX_originalT : SV_Target0)
{
	RFX_originalT = tex2D(RFX_backbufferColor, texcoord);
}

technique RFX_StoreOriginal_Tech < enabled = true; >
{
	pass
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_PS_Original;
		RenderTarget = RFX_originalTex;	
	}
}
#endif
#endif

#if RFX_DepthBufferCalc
#ifdef RFX_DepthBufferCalcS
//Prepare DepthTex
#define zFarPlane 	1
#define zNearPlane 	0.001
void RFX_CalcDepth(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0, out float3 color : SV_Target0)
{
	// Depth buffer access
	float depth = tex2D(RFX_depthColor, texcoord).x;

	// Linearize depth	
#if RFX_LogDepth 
	depth = (exp(depth * log(2f)) - (1f-0.0045*(1f-pow(abs(depth * log(300f)),100))))/0.056f;
#else
	depth = 1 / ((depth * ((zFarPlane - zNearPlane) / (-zFarPlane * zNearPlane)) + zFarPlane / (zFarPlane * zNearPlane)));
	//depth = (2.0 * RFX_Depth_z_near) / ( -(RFX_Depth_z_far - RFX_Depth_z_near) * depth + (RFX_Depth_z_far + RFX_Depth_z_near) );
#endif
	color.rgb = float3(depth.xxx);
}

//SweetFX DitherMethod#1
void RFX_StoreDepth(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0, out float4 depthTexR : SV_Target0)
{
   float4 colorInput = tex2D(RFX_depthTexPingColor,texcoord);
   float2 tex = texcoord;

   float3 color = colorInput.rgb;

   float dither_bit  = 8.0;  //Number of bits per channel. Should be 8 for most monitors.

     float grid_position = frac( dot(tex,(RFX_ScreenSize * float2(1.0/16.0,10.0/36.0))) + 0.25 );

     //Calculate how big the shift should be
     float dither_shift = (0.25) * (1.0 / (pow(2,dither_bit) - 1.0));

     //Shift the individual colors differently, thus making it even harder to see the dithering pattern
     float3 dither_shift_RGB = float3(dither_shift, -dither_shift, dither_shift); //subpixel dithering

     //modify shift acording to grid position.
     dither_shift_RGB = lerp(2.0 * dither_shift_RGB, -2.0 * dither_shift_RGB, grid_position); //shift acording to grid position.

     //shift the color by dither_shift
     color.rgb += dither_shift_RGB;

   colorInput.rgb = color.rgb;

   depthTexR = colorInput;
}

technique RFX_DepthTex_Tech < enabled = true; >
{
	pass CalcDepth
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_CalcDepth;
		RenderTarget0 = RFX_depthTexPing;
	}

	pass StoreDepth
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = RFX_StoreDepth;
		RenderTarget0 = RFX_depthTex;
	}
}
#endif
#endif