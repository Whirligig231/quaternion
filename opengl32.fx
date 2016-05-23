#include "MasterEffect.h"
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// END OF TWEAKING VARIABLES      											     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//global vars
#define ScreenSize 	float4(BUFFER_WIDTH, BUFFER_RCP_WIDTH, float(BUFFER_WIDTH) / float(BUFFER_HEIGHT), float(BUFFER_HEIGHT) / float(BUFFER_WIDTH)) //x=Width, y=1/Width, z=ScreenScaleY, w=1/ScreenScaleY
#define PixelSize  	float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT)
#define PI 		3.1415972
#define PIOVER180 	0.017453292
#define AUTHOR 		MartyMcFly
#define FOV 		75
#define LumCoeff 	float3(0.212656, 0.715158, 0.072186)
#define zFarPlane 	1
#define zNearPlane 	0.001		//I know, weird values but ReShade's depthbuffer is ... odd
#define aspect          (BUFFER_RCP_HEIGHT/BUFFER_RCP_WIDTH)
#define InvFocalLen 	float2(tan(0.5f*radians(FOV)) / (float)BUFFER_RCP_HEIGHT * (float)BUFFER_RCP_WIDTH, tan(0.5f*radians(FOV)))
uniform float4 Timer < source = "timer"; >;

#pragma message "MasterEffect ReBorn 1.1.287 by Marty McFly"

#if( USE_HDR_LEVEL == 0)
 #define RENDERMODE RGBA8
#endif
#if( USE_HDR_LEVEL == 1)
 #define RENDERMODE RGBA16F
#endif
#if( USE_HDR_LEVEL == 2)
 #define RENDERMODE RGBA32F
#endif

//textures
texture   texBloom1 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RENDERMODE;};
texture   texBloom2 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; Format = RENDERMODE;};
texture   texBloom3 { Width = BUFFER_WIDTH/2; Height = BUFFER_HEIGHT/2; Format = RENDERMODE;};
texture   texBloom4 { Width = BUFFER_WIDTH/4; Height = BUFFER_HEIGHT/4; Format = RENDERMODE;};
texture   texBloom5 { Width = BUFFER_WIDTH/8; Height = BUFFER_HEIGHT/8; Format = RENDERMODE;};

texture   texLens1 { Width = BUFFER_WIDTH/2; Height = BUFFER_HEIGHT/2; Format = RENDERMODE;};
texture   texLens2 { Width = BUFFER_WIDTH/2; Height = BUFFER_HEIGHT/2; Format = RENDERMODE;};

texture2D texLDR : COLOR;
texture   texHDR1 	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; MipLevels = 5; Format = RENDERMODE;};	
texture   texHDR2 	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT; MipLevels = 5; Format = RENDERMODE;};

texture   texOcclusion1 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16F;}; //MUST be at least 16, 8 gives heavy artifacts when blurring.
texture   texOcclusion2 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16F;}; //"Optimizations" can be done elsewhere, not here.

texture   texCoC	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  MipLevels = 5; Format = RGBA16F;};
texture2D texDepth : DEPTH;

texture   texNoise  < string source = "MasterEffect/internal/mcnoise.png";  	> {Width = BUFFER_WIDTH;Height = BUFFER_HEIGHT;Format = RGBA8;};
texture   texSprite < string source = "MasterEffect/mcsprite.png"; 	 	> {Width = BUFFER_WIDTH;Height = BUFFER_HEIGHT;Format = RGBA8;};
texture   texDirt   < string source = "MasterEffect/mcdirt.png";   		> {Width = BUFFER_WIDTH;Height = BUFFER_HEIGHT;Format = RGBA8;};
texture   texLUT    < string source = "MasterEffect/mclut.png";    		> {Width = 256; Height = 1;   Format = RGBA8;};
texture   texLUT3D  < string source = "MasterEffect/mclut3d.png";  		> {Width = 256; Height = 16;   Format = RGBA8;};
texture   texMask   < string source = "MasterEffect/mcmask.png";   		> {Width = BUFFER_WIDTH;Height = BUFFER_HEIGHT;Format = R8;};
texture   texHeat   < string source = "MasterEffect/internal/mcheat.png";   	> {Width = 512;Height = 512;Format = RGBA8;};

//samplers
sampler SamplerBloom1 { Texture = texBloom1; };
sampler SamplerBloom2 { Texture = texBloom2; };
sampler SamplerBloom3 { Texture = texBloom3; };
sampler SamplerBloom4 { Texture = texBloom4; };
sampler SamplerBloom5 { Texture = texBloom5; };

sampler SamplerLens1 { Texture = texLens1; };
sampler SamplerLens2 { Texture = texLens2; };

sampler2D SamplerLDR
{
	Texture = texLDR;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

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

sampler2D SamplerOcclusion1
{
	Texture = texOcclusion1;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerOcclusion2
{
	Texture = texOcclusion2;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerCoC
{
	Texture = texCoC;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerDepth
{
	Texture = texDepth;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerNoise
{
	Texture = texNoise;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = Wrap;
	AddressV = Wrap;
};

sampler2D SamplerSprite
{
	Texture = texSprite;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerDirt
{
	Texture = texDirt;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerLUT
{
	Texture = texLUT;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerLUT3D
{
	Texture = texLUT3D;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerMask
{
	Texture = texMask;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

sampler2D SamplerHeat
{
	Texture = texHeat;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Repeat;
	AddressV = Repeat;
};

struct VS_OUTPUT_POST
{
	float4 vpos : SV_Position;
	float2 txcoord : TEXCOORD0;
};

struct VS_INPUT_POST
{
	uint id : SV_VertexID;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Vertex Shaders													     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

VS_OUTPUT_POST VS_MasterEffect(VS_INPUT_POST IN)
{
	VS_OUTPUT_POST OUT;
	OUT.txcoord.x = (IN.id == 2) ? 2.0 : 0.0;
	OUT.txcoord.y = (IN.id == 1) ? 2.0 : 0.0;
	OUT.vpos = float4(OUT.txcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
	return OUT;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// SMAA													     		     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#include "MasterEffect/internal/mcsmaa.hlsl"

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Functions														     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float GetLinearDepth(float depth)
{
	return  1 / ((depth * ((zFarPlane - zNearPlane) / (-zFarPlane * zNearPlane)) + zFarPlane / (zFarPlane * zNearPlane)));
}

float3 GetNormalFromDepth(float fDepth, float2 vTexcoord) {
  
  	const float2 offset1 = float2(0.0,0.001);
  	const float2 offset2 = float2(0.001,0.0);
  
  	float depth1 = GetLinearDepth(tex2Dlod(SamplerDepth, float4(vTexcoord + offset1,0,0)).x);
  	float depth2 = GetLinearDepth(tex2Dlod(SamplerDepth, float4(vTexcoord + offset2,0,0)).x);
  
  	float3 p1 = float3(offset1, depth1 - fDepth);
  	float3 p2 = float3(offset2, depth2 - fDepth);
  
  	float3 normal = cross(p1, p2);
  	normal.z = -normal.z;
  
  	return normalize(normal);
}

float GetRandom(float2 co){
	return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

float3 GetRandomVector(float2 vTexCoord) {
  	return 2 * normalize(float3(GetRandom(vTexCoord - 0.5f),
				    GetRandom(vTexCoord + 0.5f),
				    GetRandom(vTexCoord))) - 1;
}

float4 GetAtlasTex(sampler tex, float2 coord, float spaltenzahl, float reihenzahl, float spalte, float reihe)
{
	return tex2Dlod(tex, float4(coord.xy/float2(spaltenzahl, reihenzahl)+float2((spalte-1)/spaltenzahl,(reihe-1)/reihenzahl),0,0));
}


float4 GaussBlur22(float2 coord, sampler tex, float mult, float lodlevel, bool isBlurVert) //texcoord, texture, blurmult in pixels, tex2dlod level, axis (0=horiz, 1=vert)
{
	float4 sum = 0;
	float2 axis = (isBlurVert) ? float2(0, 1) : float2(1, 0);
	float  weight[11] = {0.082607, 0.080977, 0.076276, 0.069041, 0.060049, 0.050187, 0.040306, 0.031105, 0.023066, 0.016436, 0.011254};

	for(int i=-10; i < 11; i++)
	{
		float currweight = weight[abs(i)];	
		sum	+= tex2Dlod(tex, float4(coord.xy + axis.xy * (float)i * PixelSize * mult,0,lodlevel)) * currweight;
	}

	return sum;
}

float4 GaussBlurGeneric(float2 coord, sampler tex, float mult, float lodlevel, bool isBlurVert, float Radius, float Quality) //texcoord, texture, blurmult in pixels, tex2dlod level, axis (0=horiz, 1=vert), offset count, quality
{
	float2 axis = (isBlurVert) ? float2(0, 1) : float2(1, 0);

	float4 Sigma = 0;
	
	Sigma.x				= 1.0f / ( sqrt( 2.0f * PI ) * Radius );
	Sigma.y				= exp( -0.5f / ( Radius * Radius ));
	Sigma.z				= Sigma.y * Sigma.y;
	Sigma.w				= Sigma.x;
	Sigma.xy			*= Sigma.yz;

	float4 sum = tex2Dlod(tex, float4(coord.xy,0,lodlevel))*Sigma.x;

	for(int i = 1; i < 150 && Sigma.w < Quality; i++) 
	{
		sum 	+= tex2Dlod(tex, float4(coord.xy + axis.xy * (float)i * PixelSize * mult,0,lodlevel)) * Sigma.x;
		sum 	+= tex2Dlod(tex, float4(coord.xy - axis.xy * (float)i * PixelSize * mult,0,lodlevel)) * Sigma.x;
		Sigma.w += 2.0 * Sigma.x;
		Sigma.xy *= Sigma.yz;
	}

	sum /= Sigma.w;

	return sum;
}

float smootherstep(float edge0, float edge1, float x)
{
   	x = clamp((x - edge0)/(edge1 - edge0), 0.0, 1.0);
   	return x*x*x*(x*(x*6 - 15) + 10);
}

float3 Hue(in float3 RGB)
{
   	// Based on work by Sam Hocevar and Emil Persson
   	float Epsilon = 1e-10;
   	float4 P = (RGB.g < RGB.b) ? float4(RGB.bg, -1.0, 2.0/3.0) : float4(RGB.gb, 0.0, -1.0/3.0);
   	float4 Q = (RGB.r < P.x) ? float4(P.xyw, RGB.r) : float4(RGB.r, P.yzx);
   	float C = Q.x - min(Q.w, Q.y);
   	float H = abs((Q.w - Q.y) / (6 * C + Epsilon) + Q.z);
   	return float3(H, C, Q.x);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Passes														     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float4 BorderPass( float4 colorInput, float2 tex )
{
  	float3 fBorderColor_float = fBorderColor;
	float2 screen_size = float2(BUFFER_WIDTH, BUFFER_HEIGHT);
	float  screen_ratio = (BUFFER_WIDTH/BUFFER_HEIGHT);
  	float2 fBorderWidth_variable = fBorderWidth;

  	// -- calculate the right fBorderWidth for a given fBorderRatio --
  	//if (!any(fBorderWidth)) //if fBorderWidth is not used
  	if (fBorderWidth.x == -fBorderWidth.y) //if fBorderWidth is not used
    		if (screen_ratio < fBorderRatio)
      			fBorderWidth_variable = float2(0.0, (screen_size.y - (screen_size.x / fBorderRatio)) * 0.5);
    		else
      			fBorderWidth_variable = float2((screen_size.x - (screen_size.y * fBorderRatio)) * 0.5, 0.0);

  	float2 border = (PixelSize.xy * fBorderWidth_variable); //Translate integer pixel width to floating point

  	float2 within_border = saturate((-tex * tex + tex) - (-border * border + border)); //becomes positive when inside the border and 0 when outside

  	colorInput.rgb = all(within_border) ?  colorInput.rgb : fBorderColor_float ; //if the pixel is within the border use the original color, if not use the fBorderColor

  	return colorInput; //return the pixel
}

float3 CartoonPass( float3 colorInput, float2 tex, float2 pixelsize, sampler colorsampler )
{
 
  	float diff1 = dot(LumCoeff,tex2D(colorsampler, tex + pixelsize).rgb);
  	diff1 = dot(float4(LumCoeff,-1.0),float4(tex2D(colorsampler, tex - pixelsize).rgb , diff1));
  
  	float diff2 = dot(LumCoeff,tex2D(colorsampler, tex +float2(pixelsize.x,-pixelsize.y)).rgb);
  	diff2 = dot(float4(LumCoeff,-1.0),float4(tex2D(colorsampler, tex +float2(-pixelsize.x,pixelsize.y)).rgb , diff2));
    
  	float edge = dot(float2(diff1,diff2),float2(diff1,diff2));
  
  	colorInput.rgb =  pow(edge,CartoonEdgeSlope) * -CartoonPower + colorInput.rgb;
	
	return saturate(colorInput);
}

float3 LevelsPass( float3 colorInput )
{
	#define black_point_float ( Levels_black_point / 255.0 )
	#define white_point_float ( 255.0 / (Levels_white_point - Levels_black_point)) 

 	colorInput.rgb = colorInput.rgb * white_point_float - (black_point_float *  white_point_float);
  	return colorInput;
}

float3 TechniPass_prod80(float3 colorInput)
{

	float3 colStrength = float3(ColStrengthR,ColStrengthG,ColStrengthB);
	float3 tsource = saturate(colorInput.rgb);
	float3 ttemp = 1 - tsource;
	float3 ttarget = ttemp.grg;
	float3 ttarget2 = ttemp.bbr;
	float3 ttemp2 = tsource.rgb * ttarget.rgb;
	ttemp2.rgb *= ttarget2.rgb;

	ttemp.rgb = ttemp2.rgb * colStrength;
	ttemp2.rgb *= TechniBrightness;

	ttarget.rgb = ttemp.grg;
	ttarget2.rgb = ttemp.bbr;

	ttemp.rgb = tsource.rgb - ttarget.rgb;
	ttemp.rgb += ttemp2.rgb;
	ttemp2.rgb = ttemp.rgb - ttarget2.rgb;

	colorInput.rgb = lerp(tsource.rgb, ttemp2.rgb, TechniStrength);

	colorInput.rgb = lerp(dot(colorInput.rgb, 0.333), colorInput.rgb, TechniSat); 
	
	return colorInput.rgb;

}

float3 TechnicolorPass( float3 colorInput )
{

	#define cyanfilter float3(0.0, 1.30, 1.0)
	#define magentafilter float3(1.0, 0.0, 1.05) 
	#define yellowfilter float3(1.6, 1.6, 0.05)

	#define redorangefilter float2(1.05, 0.620) //RG_
	#define greenfilter float2(0.30, 1.0)       //RG_
	#define magentafilter2 magentafilter.rb     //R_B

	float3 tcol = colorInput.rgb;
	
  	float2 rednegative_mul   = tcol.rg * (1.0 / (redNegativeAmount * TechniPower));
	float2 greennegative_mul = tcol.rg * (1.0 / (greenNegativeAmount * TechniPower));
	float2 bluenegative_mul  = tcol.rb * (1.0 / (blueNegativeAmount * TechniPower));
	
  	float rednegative   = dot( redorangefilter, rednegative_mul );
	float greennegative = dot( greenfilter, greennegative_mul );
	float bluenegative  = dot( magentafilter2, bluenegative_mul );
	
	float3 redoutput   = rednegative.rrr + cyanfilter;
	float3 greenoutput = greennegative.rrr + magentafilter;
	float3 blueoutput  = bluenegative.rrr + yellowfilter;
	
	float3 result = redoutput * greenoutput * blueoutput;
	colorInput.rgb = lerp(tcol, result, TechniAmount);
	return colorInput;
}

float3 DPXPass(float3 InputColor){


	float3x3 RGB =
	float3x3(
	2.67147117265996,-1.26723605786241,-0.410995602172227,
	-1.02510702934664,1.98409116241089,0.0439502493584124,
	0.0610009456429445,-0.223670750812863,1.15902104167061
	);

	float3x3 XYZ =
	float3x3(
	0.500303383543316,0.338097573222739,0.164589779545857,
	0.257968894274758,0.676195259144706,0.0658358459823868,
	0.0234517888692628,0.1126992737203,0.866839673124201
	);

	float DPXContrast = 0.1;
	float DPXGamma = 1.0;

	float RedCurve = DPXRed;
	float GreenCurve = DPXGreen;
	float BlueCurve = DPXBlue;
	
	float3 RGB_Curve = float3(DPXRed,DPXGreen,DPXBlue);
	float3 RGB_C = float3(DPXRedC,DPXGreenC,DPXBlueC);

	float3 B = InputColor.rgb;
	B = pow(B, 1.0/DPXGamma);
 	B = B * (1.0 - DPXContrast) + (0.5 * DPXContrast);

	float3 Btemp = (1.0 / (1.0 + exp(RGB_Curve / 2.0)));	  
	B = ((1.0 / (1.0 + exp(-RGB_Curve * (B - RGB_C)))) / (-2.0 * Btemp + 1.0)) + (-Btemp / (-2.0 * Btemp + 1.0));

    	float value = max(max(B.r, B.g), B.b);
	float3 color = B / value;
	color = saturate(color);
	color = pow(color, 1.0/DPXColorGamma);
	
	float3 c0 = color * value;
        c0 = mul(XYZ, c0);

	float luma = dot(c0, float3(0.30, 0.59, 0.11)); //Use BT 709 instead?
        c0 = (1.0 - DPXSaturation) * luma + DPXSaturation * c0;
	c0 = mul(RGB, c0);
	
	InputColor.rgb = lerp(InputColor.rgb, c0, DPXBlend);

	return InputColor;
}

float3 LiftGammaGainPass( float3 colorInput )
{
	// -- Get input --
	float3 color = colorInput.rgb;
	
	// -- Lift --
	color = color * (1.5-0.5 * RGB_Lift) + 0.5 * RGB_Lift - 0.5;
	color = saturate(color); //isn't strictly necessary, but doesn't cost performance.
	
	// -- Gain --
	color *= RGB_Gain; 
	
	// -- Gamma --
	colorInput.rgb = pow(color, 1.0 / RGB_Gamma); //Gamma
	
	// -- Return output --
	//return (colorInput);
	return saturate(colorInput);
}

float3 TonemapPass( float3 colorInput )
{
	float3 color = colorInput.rgb;

	color = saturate(color - Defog * FogColor); // Defog
	
	color *= pow(2.0f, Exposure); // Exposure
	
	color = pow(color, Gamma);    // Gamma -- roll into the first gamma correction in main.h ?
	
	float lum = dot(LumCoeff, color.rgb);
	
	float3 blend = lum.rrr; //dont use float3
	
	float L = saturate( 10.0 * (lum - 0.45) );
  	
	float3 result1 = 2.0f * color.rgb * blend;
	float3 result2 = 1.0f - 2.0f * (1.0f - blend) * (1.0f - color.rgb);
	
	float3 newColor = lerp(result1, result2, L);
	float3 A2 = Bleach * color.rgb; //why use a float for A2 here and then multiply by color.rgb (a float3)?
	float3 mixRGB = A2 * newColor;
	
	color.rgb += ((1.0f - A2) * mixRGB);
	
	float3 middlegray = dot(color,(1.0/3.0)); //1fps slower than the original on nvidia, 2 fps faster on AMD
	
	float3 diffcolor = color - middlegray; //float 3 here
	colorInput.rgb = (color + diffcolor * Saturation)/(1+(diffcolor*Saturation)); //saturation
	
	return colorInput;
}

float3 VibrancePass( float3 colorInput )
{
   	#define Vibrance_coeff float3(Vibrance_RGB_balance * Vibrance)

	float3 color = colorInput; //original input color
  	float3 lumCoeff = float3(0.212656, 0.715158, 0.072186);  //Values to calculate luma with

	float luma = dot(LumCoeff, color.rgb); //calculate luma (grey)

	float max_color = max(colorInput.r, max(colorInput.g,colorInput.b)); //Find the strongest color
	float min_color = min(colorInput.r, min(colorInput.g,colorInput.b)); //Find the weakest color

  	float color_saturation = max_color - min_color; //The difference between the two is the saturation

   	color.rgb = lerp(luma, color.rgb, (1.0 + (Vibrance_coeff * (1.0 - (sign(Vibrance_coeff) * color_saturation))))); //extrapolate between luma and original by 1 + (1-saturation) - current

 	return color; //return the result
}

float3 CurvesPass( float3 colorInput )
{
  float Curves_contrast_blend = Curves_contrast;


   /*-----------------------------------------------------------.
  /               Separation of Luma and Chroma                 /
  '-----------------------------------------------------------*/

  	// -- Calculate Luma and Chroma if needed --
  	#if Curves_mode != 2

    	//calculate luma (grey)
    	float luma = dot(LumCoeff, colorInput.rgb);

    	//calculate chroma
	float3 chroma = colorInput.rgb - luma;
  	#endif

  	// -- Which value to put through the contrast formula? --
  	// I name it x because makes it easier to copy-paste to Graphtoy or Wolfram Alpha or another graphing program
  	#if Curves_mode == 2
	float3 x = colorInput.rgb; //if the curve should be applied to both Luma and Chroma
	#elif Curves_mode == 1
	float3 x = chroma; //if the curve should be applied to Chroma
	x = x * 0.5 + 0.5; //adjust range of Chroma from -1 -> 1 to 0 -> 1
  	#else // Curves_mode == 0
    	float x = luma; //if the curve should be applied to Luma
  	#endif

   /*-----------------------------------------------------------.
  /                     Contrast formulas                       /
  '-----------------------------------------------------------*/

  	// -- Curve 1 --
  	#if Curves_formula == 1
    	x = sin(PI * 0.5 * x); // Sin - 721 amd fps, +vign 536 nv
    	x *= x;
    
    	//x = 0.5 - 0.5*cos(PI*x);
    	//x = 0.5 * -sin(PI * -x + (PI*0.5)) + 0.5;
  	#endif

  	// -- Curve 2 --
  	#if Curves_formula == 2
    	x = x - 0.5;  
    	x = ( x / (0.5 + abs(x)) ) + 0.5;
    
    	//x = ( (x - 0.5) / (0.5 + abs(x-0.5)) ) + 0.5;
  	#endif

  	// -- Curve 3 --
  	#if Curves_formula == 3
    	//x = smoothstep(0.0,1.0,x); //smoothstep
    	x = x*x*(3.0-2.0*x); //faster smoothstep alternative - 776 amd fps, +vign 536 nv
    	//x = x - 2.0 * (x - 1.0) * x* (x- 0.5);  //2.0 is contrast. Range is 0.0 to 2.0
  	#endif

  	// -- Curve 4 --
  	#if Curves_formula == 4
    	x = (1.0524 * exp(6.0 * x) - 1.05248) / (20.0855 + exp(6.0 * x)); //exp formula
  	#endif

  	// -- Curve 5 --
  	#if Curves_formula == 5
    	//x = 0.5 * (x + 3.0 * x * x - 2.0 * x * x * x); //a simplified catmull-rom (0,0,1,1) - btw smoothstep can also be expressed as a simplified catmull-rom using (1,0,1,0)
    	//x = (0.5 * x) + (1.5 -x) * x*x; //estrin form - faster version
    	x = x * (x * (1.5-x) + 0.5); //horner form - fastest version

    	Curves_contrast_blend = Curves_contrast * 2.0; //I multiply by two to give it a strength closer to the other curves.
  	#endif

 	// -- Curve 6 --
  	#if Curves_formula == 6
    	x = x*x*x*(x*(x*6.0 - 15.0) + 10.0); //Perlins smootherstep
  	#endif

	// -- Curve 7 --
  	#if Curves_formula == 7
    	//x = ((x-0.5) / ((0.5/(4.0/3.0)) + abs((x-0.5)*1.25))) + 0.5;
	x = x - 0.5;
	x = x / ((abs(x)*1.25) + 0.375 ) + 0.5;
	//x = ( (x-0.5) / ((abs(x-0.5)*1.25) + (0.5/(4.0/3.0))) ) + 0.5;
  	#endif

  	// -- Curve 8 --
  	#if Curves_formula == 8
    	x = (x * (x * (x * (x * (x * (x * (1.6 * x - 7.2) + 10.8) - 4.2) - 3.6) + 2.7) - 1.8) + 2.7) * x * x; //Techicolor Cinestyle - almost identical to curve 1
  	#endif

  	// -- Curve 9 --
  	#if Curves_formula == 9
    	x =  -0.5 * (x*2.0-1.0) * (abs(x*2.0-1.0)-2.0) + 0.5; //parabola
  	#endif

  	// -- Curve 10 --
  	#if Curves_formula == 10 //Half-circles

    	#if Curves_mode == 0
      	float xstep = step(x,0.5);
	float xstep_shift = (xstep - 0.5);
	float shifted_x = x + xstep_shift;
   	#else
      	float3 xstep = step(x,0.5);
	float3 xstep_shift = (xstep - 0.5);
	float3 shifted_x = x + xstep_shift;
    	#endif

	x = abs(xstep - sqrt(-shifted_x * shifted_x + shifted_x) ) - xstep_shift;

  	//x = abs(step(x,0.5)-sqrt(-(x+step(x,0.5)-0.5)*(x+step(x,0.5)-0.5)+(x+step(x,0.5)-0.5)))-(step(x,0.5)-0.5); //single line version of the above
    
  	//x = 0.5 + (sign(x-0.5)) * sqrt(0.25-(x-trunc(x*2))*(x-trunc(x*2))); //worse
  
  	/* // if/else - even worse
  	if (x-0.5)
  	x = 0.5-sqrt(0.25-x*x);
  	else
  	x = 0.5+sqrt(0.25-(x-1)*(x-1));
	*/

  	//x = (abs(step(0.5,x)-clamp( 1-sqrt(1-abs(step(0.5,x)- frac(x*2%1)) * abs(step(0.5,x)- frac(x*2%1))),0 ,1))+ step(0.5,x) )*0.5; //worst so far
	
	//TODO: Check if I could use an abs split instead of step. It might be more efficient
	
	Curves_contrast_blend = Curves_contrast * 0.5; //I divide by two to give it a strength closer to the other curves.
  	#endif

  	// -- Curve 11 --
  	#if Curves_formula == 11 //Cubic catmull
    	float a = 1.00; //control point 1
    	float b = 0.00; //start point
    	float c = 1.00; //endpoint
    	float d = 0.20; //control point 2
    	x = 0.5 * ((-a + 3*b -3*c + d)*x*x*x + (2*a -5*b + 4*c - d)*x*x + (-a+c)*x + 2*b); //A customizable cubic catmull-rom spline
  	#endif

  	// -- Curve 12 --
  	#if Curves_formula == 12 //Cubic Bezier spline
    	float a = 0.00; //start point
    	float b = 0.00; //control point 1
    	float c = 1.00; //control point 2
    	float d = 1.00; //endpoint

    	float r  = (1-x);
	float r2 = r*r;
	float r3 = r2 * r;
	float x2 = x*x;
	float x3 = x2*x;
	//x = dot(float4(a,b,c,d),float4(r3,3*r2*x,3*r*x2,x3));

	//x = a * r*r*r + r * (3 * b * r * x + 3 * c * x*x) + d * x*x*x;
	//x = a*(1-x)*(1-x)*(1-x) +(1-x) * (3*b * (1-x) * x + 3 * c * x*x) + d * x*x*x;
	x = a*(1-x)*(1-x)*(1-x) + 3*b*(1-x)*(1-x)*x + 3*c*(1-x)*x*x + d*x*x*x;
  	#endif

  	// -- Curve 13 --
  	#if Curves_formula == 13 //Cubic Bezier spline - alternative implementation.
    	float3 a = float3(0.00,0.00,0.00); //start point
    	float3 b = float3(0.25,0.15,0.85); //control point 1
    	float3 c = float3(0.75,0.85,0.15); //control point 2
    	float3 d = float3(1.00,1.00,1.00); //endpoint

    	float3 ab = lerp(a,b,x);           // point between a and b
    	float3 bc = lerp(b,c,x);           // point between b and c
    	float3 cd = lerp(c,d,x);           // point between c and d
    	float3 abbc = lerp(ab,bc,x);       // point between ab and bc
    	float3 bccd = lerp(bc,cd,x);       // point between bc and cd
    	float3 dest = lerp(abbc,bccd,x);   // point on the bezier-curve
    	x = dest;
  	#endif

  	// -- Curve 14 --
  	#if Curves_formula == 14
    	x = 1.0 / (1.0 + exp(-(x * 10.0 - 5.0))); //alternative exp formula
  	#endif

   /*-----------------------------------------------------------.
  /                 Joining of Luma and Chroma                  /
  '-----------------------------------------------------------*/

  	#if Curves_mode == 2 //Both Luma and Chroma
	float3 color = x;  //if the curve should be applied to both Luma and Chroma
	colorInput.rgb = lerp(colorInput.rgb, color, Curves_contrast_blend); //Blend by Curves_contrast

  	#elif Curves_mode == 1 //Only Chroma
	x = x * 2.0 - 1.0; //adjust the Chroma range back to -1 -> 1
	float3 color = luma + x; //Luma + Chroma
	colorInput.rgb = lerp(colorInput.rgb, color, Curves_contrast_blend); //Blend by Curves_contrast

  	#else // Curves_mode == 0 //Only Luma
    	x = lerp(luma, x, Curves_contrast_blend); //Blend by Curves_contrast
    	colorInput.rgb = x + chroma; //Luma + Chroma

  	#endif

  	//Return the result
  	return colorInput;
}

float3 SepiaPass( float3 colorInput )
{
	float3 sepia = colorInput.rgb;
	
	// calculating amounts of input, grey and sepia colors to blend and combine
	float grey = dot(sepia, LumCoeff);
	sepia *= ColorTone;
	
	float3 blend2 = (grey * GreyPower) + (colorInput.rgb / (GreyPower + 1));

	colorInput.rgb = lerp(blend2, sepia, SepiaPower);
	// returning the final color
	return colorInput;
}

float3 SkyrimTonemapPass( float3 color )
{
	float	grayadaptation = dot(color.xyz, LumCoeff);

	#if (POSTPROCESS==1)
	color.xyz =  color.xyz / (grayadaptation * EAdaptationMaxV1 + EAdaptationMinV1);
	float cgray = dot( color.xyz, LumCoeff);
	cgray = pow(cgray, EContrastV1);
	float3 poweredcolor = pow( abs(color.xyz), EColorSaturationV1);
	float newgray = dot(poweredcolor.xyz, LumCoeff);
	color.xyz = poweredcolor.xyz * cgray / (newgray + 0.0001);
	float3	luma =  color.xyz;
	float	lumamax = 300.0;
	color.xyz = ( color.xyz * (1.0 +  color.xyz / lumamax)) / ( color.xyz + EToneMappingCurveV1);	
	#endif

	#if (POSTPROCESS==2)
	color.xyz =  color.xyz / (grayadaptation * EAdaptationMaxV2 + EAdaptationMinV2);
	float3 xncol = normalize( color.xyz);
	float3 scl =  color.xyz / xncol.xyz;
	scl = pow(scl, EIntensityContrastV2);
	xncol.xyz = pow(xncol.xyz, EColorSaturationV2);
	color.xyz = scl*xncol.xyz;
	float	lumamax = EToneMappingOversaturationV2;
	color.xyz = ( color.xyz * (1.0 +  color.xyz / lumamax)) / ( color.xyz + EToneMappingCurveV2);
 	color.xyz*=4;
	#endif

	#if (POSTPROCESS==3)
	color.xyz *= 35;
	float	lumamax = EToneMappingOversaturationV3;
	color.xyz = ( color.xyz * (1.0 +  color.xyz / lumamax)) / ( color.xyz + EToneMappingCurveV3);
	#endif

	#if (POSTPROCESS == 4)
	color.xyz =  color.xyz / (grayadaptation * EAdaptationMaxV4 + EAdaptationMinV4);
	float Y = dot( color.xyz, float3(0.299, 0.587, 0.114)); //0.299 * R + 0.587 * G + 0.114 * B;
	float U = dot( color.xyz, float3(-0.14713, -0.28886, 0.436)); //-0.14713 * R - 0.28886 * G + 0.436 * B;
	float V = dot( color.xyz, float3(0.615, -0.51499, -0.10001)); //0.615 * R - 0.51499 * G - 0.10001 * B;
	Y = pow(Y, EBrightnessCurveV4);
	Y = Y * EBrightnessMultiplierV4;
	color.xyz = V * float3(1.13983, -0.58060, 0.0) + U * float3(0.0, -0.39465, 2.03211) + Y;
	color.xyz = max( color.xyz, 0.0);
	color.xyz =  color.xyz / ( color.xyz + EBrightnessToneMappingCurveV4);
	#endif

	#if (POSTPROCESS == 5)
	float hnd = 1;
	float2 hndtweak = float2( 3.1 , 1.5 );
        color.xyz *= lerp( hndtweak.x, hndtweak.y, hnd );
	float3 xncol = normalize( color.xyz);
	float3 scl =  color.xyz/xncol.xyz;
	scl = pow(scl, EIntensityContrastV5);
	xncol.xyz = pow(xncol.xyz, EColorSaturationV5);
	color.xyz = scl*xncol.xyz;
	color.xyz *= HCompensateSatV5; // compensate for darkening caused my EcolorSat above
	color.xyz =  color.xyz / ( color.xyz + EToneMappingCurveV5);
	color.xyz *= 4;
	#endif

	#if (POSTPROCESS==6)
	//Postprocessing V6 by Kermles
	//tuned by the master himself for ME 1.4, thanks man!!!
	//hd6/ppv2///////////////////////////////////////////
	float 	EIntensityContrastV6 = EIntensityContrastV6Day;
	float 	EColorSaturationV6 = EColorSaturationV6Day;
	float 	HCompensateSatV6 = HCompensateSatV6Day;
	float 	EToneMappingCurveV6 = EToneMappingCurveV6Day;
	float 	EBrightnessV6 = EBrightnessV6Day;
	float 	EToneMappingOversaturationV6 = EToneMappingOversaturationV6Day;
	float 	EAdaptationMaxV6 = EAdaptationMaxV6Day;
	float 	EAdaptationMinV6 = EAdaptationMinV6Day;
	float	lumamax = EToneMappingOversaturationV6;
	//kermles////////////////////////////////////////////
	float4 	ncolor;					//temporary variable for color adjustments		
	//begin pp code/////////////////////////////////////////////////
	//ppv2 modified by kermles//////////////////////////////////////
		
	grayadaptation = clamp(grayadaptation, 0, 50);
	color.xyz *= EBrightnessV6;
	float3 xncol = normalize( color.xyz);
	float3 scl =  color.xyz/xncol.xyz;
	scl = pow(saturate(scl), EIntensityContrastV6);
	xncol.xyz = pow(xncol.xyz, EColorSaturationV6);
	color.xyz = scl*xncol.xyz;
	color.xyz *= HCompensateSatV6;
	color.xyz = ( color.xyz * (1.0 +  color.xyz/lumamax))/( color.xyz + EToneMappingCurveV6);
	color.xyz /= grayadaptation*EAdaptationMaxV6+EAdaptationMinV6;
	//rerun ppv2////////////////////////////////////////////////////
	color.xyz *= EBrightnessV6;
	xncol = normalize( color.xyz);
	scl =  color.xyz/xncol.xyz;
	scl = saturate(scl);
	scl = pow(scl, EIntensityContrastV6);
	xncol.xyz = pow(xncol.xyz, EColorSaturationV6);
	color.xyz = scl*xncol.xyz;
	color.xyz *= HCompensateSatV6;
	color.xyz = ( color.xyz * (1.0 +  color.xyz/lumamax))/( color.xyz + EToneMappingCurveV6);
	#endif

	return color;

}

float3 MoodPass( float3 colorInput )
{
	float3 colInput = colorInput;
	float3 colMood = 1.0f;
	colMood.r = moodR;
	colMood.g = moodG;
	colMood.b = moodB;
	float fLum = ( colInput.r + colInput.g + colInput.b ) / 3;
	colMood = lerp(0, colMood, saturate(fLum * 2.0));
	colMood = lerp(colMood, 1, saturate(fLum - 0.5) * 2.0);
	float3 colOutput = lerp(colInput, colMood, saturate(fLum * fRatio));
	colorInput=max(0, colOutput);
	return colorInput;
}

float3 CrossPass(float3 color)
{
	float2 CrossMatrix [3] = {
		float2 (1.03, 0.04),
		float2 (1.09, 0.01),
		float2 (0.78, 0.13),
 		};

	float3 image1 = color;
	float3 image2 = color;
	float gray = dot(float3(0.5,0.5,0.5), image1);  
	image1 = lerp (gray, image1,CrossSaturation);
	image1 = lerp (0.35, image1,CrossContrast);
	image1 +=CrossBrightness;
	image2.r = image1.r * CrossMatrix[0].x + CrossMatrix[0].y;
	image2.g = image1.g * CrossMatrix[1].x + CrossMatrix[1].y;
	image2.b = image1.b * CrossMatrix[2].x + CrossMatrix[2].y;
	color = lerp(image1, image2, CrossAmount);
	return color;
}

float3 FilmPass(float3 B)
{
	float3 G = B;
	float3 H = 0.01;
 
	B = saturate(B);
	B = pow(B, Linearization);
	B = lerp(H, B, Contrast);
 
	float A = dot(B.rgb, LumCoeff);
	float3 D = A;
 
	B = pow(B, 1.0 / BaseGamma);
 
	float a = FRedCurve;
	float b = FGreenCurve;
	float c = FBlueCurve;
	float d = BaseCurve;
 
	float y = 1.0 / (1.0 + exp(a / 2.0));
	float z = 1.0 / (1.0 + exp(b / 2.0));
	float w = 1.0 / (1.0 + exp(c / 2.0));
	float v = 1.0 / (1.0 + exp(d / 2.0));
 
	float3 C = B;
 
	D.r = (1.0 / (1.0 + exp(-a * (D.r - 0.5))) - y) / (1.0 - 2.0 * y);
	D.g = (1.0 / (1.0 + exp(-b * (D.g - 0.5))) - z) / (1.0 - 2.0 * z);
	D.b = (1.0 / (1.0 + exp(-c * (D.b - 0.5))) - w) / (1.0 - 2.0 * w);
 
	D = pow(D, 1.0 / EffectGamma);
 
	float3 Di = 1.0 - D;
 
	D = lerp(D, Di, FBleach);
 
	D.r = pow(abs(D.r), 1.0 / EffectGammaR);
	D.g = pow(abs(D.g), 1.0 / EffectGammaG);
	D.b = pow(abs(D.b), 1.0 / EffectGammaB);
 
	if (D.r < 0.5)
		C.r = (2.0 * D.r - 1.0) * (B.r - B.r * B.r) + B.r;
	else
		C.r = (2.0 * D.r - 1.0) * (sqrt(B.r) - B.r) + B.r;
 
	if (D.g < 0.5)
		C.g = (2.0 * D.g - 1.0) * (B.g - B.g * B.g) + B.g;
	else
		C.g = (2.0 * D.g - 1.0) * (sqrt(B.g) - B.g) + B.g;
 	//if (AgainstAllAutority) 
	if (D.b < 0.5)
		C.b = (2.0 * D.b - 1.0) * (B.b - B.b * B.b) + B.b;
	else
		C.b = (2.0 * D.b - 1.0) * (sqrt(B.b) - B.b) + B.b;
 
	float3 F = lerp(B, C, Strenght);
 
	F = (1.0 / (1.0 + exp(-d * (F - 0.5))) - v) / (1.0 - 2.0 * v);
 
	float r2R = 1.0 - FSaturation;
	float g2R = 0.0 + FSaturation;
	float b2R = 0.0 + FSaturation;
 
	float r2G = 0.0 + FSaturation;
	float g2G = (1.0 - Fade) - FSaturation;
	float b2G = (0.0 + Fade) + FSaturation;
 
	float r2B = 0.0 + FSaturation;
	float g2B = (0.0 + Fade) + FSaturation;
	float b2B = (1.0 - Fade) - FSaturation;
 
	float3 iF = F;
 
	F.r = (iF.r * r2R + iF.g * g2R + iF.b * b2R);
	F.g = (iF.r * r2G + iF.g * g2G + iF.b * b2G);
	F.b = (iF.r * r2B + iF.g * g2B + iF.b * b2B);
 
	float N = dot(F.rgb, LumCoeff);
	float3 Cn = F;
 
	if (N < 0.5)
		Cn = (2.0 * N - 1.0) * (F - F * F) + F;
	else
		Cn = (2.0 * N - 1.0) * (sqrt(F) - F) + F;
 
	Cn = pow(max(Cn,0), 1.0 / Linearization);
 
	float3 Fn = lerp(B, Cn, Strenght);
	return Fn;
}

float3 ReinhardToneMapping(in float3 x)
{
	const float W =  ReinhardWhitepoint;	// Linear White Point Value
    	const float K =  ReinhardScale;        // Scale

    	// gamma space or not?
    	return (1 + K * x / (W * W)) * x / (x + K);
}

float3 ReinhardLinearToneMapping(in float3 x)
{
    	const float W = ReinhardLinearWhitepoint;	        // Linear White Point Value
    	const float L = ReinhardLinearPoint;           // Linear point
    	const float C = ReinhardLinearSlope;           // Slope of the linear section
    	const float K = (1 - L * C) / C; // Scale (fixed so that the derivatives of the Reinhard and linear functions are the same at x = L)
    	float3 reinhard = L * C + (1 - L * C) * (1 + K * (x - L) / ((W - L) * (W - L))) * (x - L) / (x - L + K);

    	// gamma space or not?
    	return (x > L) ? reinhard : C * x;
}

float3 HaarmPeterDuikerFilmicToneMapping(in float3 x)
{
    	x = max( (float3)0.0f, x - 0.004f );
    	return pow( abs( ( x * ( 6.2f * x + 0.5f ) ) / ( x * ( 6.2f * x + 1.7f ) + 0.06 ) ), 2.2f );
}

float3 CustomToneMapping(in float3 x)
{
	const float A = 0.665f;
	const float B = 0.09f;
	const float C = 0.004f;
	const float D = 0.445f;
	const float E = 0.26f;
	const float F = 0.025f;
	const float G = 0.16f;//0.145f;
	const float H = 1.1844f;//1.15f;

    // gamma space or not?
	return (((x*(A*x+B)+C)/(x*(D*x+E)+F))-G) / H;
}

float3 ColorFilmicToneMapping(in float3 x)
{
	// Filmic tone mapping
	const float3 A = float3(0.55f, 0.50f, 0.45f);	// Shoulder strength
	const float3 B = float3(0.30f, 0.27f, 0.22f);	// Linear strength
	const float3 C = float3(0.10f, 0.10f, 0.10f);	// Linear angle
	const float3 D = float3(0.10f, 0.07f, 0.03f);	// Toe strength
	const float3 E = float3(0.01f, 0.01f, 0.01f);	// Toe Numerator
	const float3 F = float3(0.30f, 0.30f, 0.30f);	// Toe Denominator
	const float3 W = float3(2.80f, 2.90f, 3.10f);	// Linear White Point Value
	const float3 F_linearWhite = ((W*(A*W+C*B)+D*E)/(W*(A*W+B)+D*F))-(E/F);
	float3 F_linearColor = ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-(E/F);

    // gamma space or not?
	return pow(saturate(F_linearColor * 1.25 / F_linearWhite),1.25);
}

float3 ColormodPass( float3 color )
{
	color.xyz = (color.xyz - dot(color.xyz, 0.333)) * ColormodChroma + dot(color.xyz, 0.333);
	color.xyz = saturate(color.xyz);
	color.x = (pow(color.x, ColormodGammaR) - 0.5) * ColormodContrastR + 0.5 + ColormodBrightnessR;
	color.y = (pow(color.y, ColormodGammaG) - 0.5) * ColormodContrastG + 0.5 + ColormodBrightnessB;
	color.z = (pow(color.z, ColormodGammaB) - 0.5) * ColormodContrastB + 0.5 + ColormodBrightnessB;
	return color;	
}

float3 SphericalPass( float3 color )
{
	float3 signedColor = color.rgb * 2.0 - 1.0;
	float3 sphericalColor = sqrt(1.0 - signedColor.rgb * signedColor.rgb);
	sphericalColor = sphericalColor * 0.5 + 0.5;
	sphericalColor *= color.rgb;
	color.rgb += sphericalColor.rgb * sphericalAmount;
	color.rgb *= 0.95;
	return color;
}

float3 SincityPass(float3 color)
{
	float sinlumi = dot(color.rgb, float3(0.30f,0.59f,0.11f));
	if(color.r > (color.g + 0.2f) && color.r > (color.b + 0.025f))
	{
		color.rgb = float3(sinlumi, 0, 0)*1.5;
	}
	else
	{
		color.rgb = sinlumi;
	}
	return color;
}

float3 colorhuefx_prod80( float3 color )
{
	
	float3 fxcolor = saturate( color.xyz );
	float greyVal = dot( fxcolor.xyz, LumCoeff.xyz );
	float3 HueSat = Hue( fxcolor.xyz );
	float colorHue = HueSat.x;
	float colorInt = HueSat.z - HueSat.y * 0.5;
	float colorSat = HueSat.y / ( 1.0 - abs( colorInt * 2.0 - 1.0 ) * 1e-10 );

	//When color intensity not based on original saturation level
   	if ( USE_COLORSAT == 0 )   colorSat = 1.0f;

	float hueMin_1 = hueMid - hueRange;
	float hueMax_1 = hueMid + hueRange;
	float hueMin_2 = 0.0f;
	float hueMax_2 = 0.0f;


   	if ( hueMin_1 < 0.0 )
   	{
   		hueMin_2 = 1.0f + hueMin_1;
   		hueMax_2 = 1.0f + hueMid;
   
      		if ( colorHue >= hueMin_1 && colorHue <= hueMid )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, smootherstep( hueMin_1, hueMid, colorHue ) * ( colorSat * satLimit ));
      		else if ( colorHue >= hueMid && colorHue <= hueMax_1 )
        		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, ( 1.0f - smootherstep( hueMid, hueMax_1, colorHue )) * ( colorSat * satLimit ));
      		else if ( colorHue >= hueMin_2 && colorHue <= hueMax_2 )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, smootherstep( hueMin_2, hueMax_2, colorHue ) * ( colorSat * satLimit ));
      		else
         		fxcolor.xyz = greyVal.xxx;
   	}

   	else if ( hueMax_1 > 1.0 )
   	{
   		hueMin_2 = 0.0f - ( 1.0f - hueMid );
   		hueMax_2 = hueMax_1 - 1.0f;

      		if ( colorHue >= hueMin_1 && colorHue <= hueMid )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, smootherstep( hueMin_1, hueMid, colorHue ) * ( colorSat * satLimit ));
      		else if ( colorHue >= hueMid && colorHue <= hueMax_1 )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, ( 1.0f - smootherstep( hueMid, hueMax_1, colorHue )) * ( colorSat * satLimit ));
      		else if ( colorHue >= hueMin_2 && colorHue <= hueMax_2 )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, ( 1.0f - smootherstep( hueMin_2, hueMax_2, colorHue )) * ( colorSat * satLimit ));
      		else
         		fxcolor.xyz = greyVal.xxx;
   	}	
   
	else
   	{
      		if ( colorHue >= hueMin_1 && colorHue <= hueMid )
        		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, smootherstep( hueMin_1, hueMid, colorHue ) * ( colorSat * satLimit ));
      		else if ( colorHue > hueMid && colorHue <= hueMax_1 )
         		fxcolor.xyz = lerp( greyVal.xxx, fxcolor.xyz, ( 1.0f - smootherstep( hueMid, hueMax_1, colorHue )) * ( colorSat * satLimit ));
      		else
         		fxcolor.xyz = greyVal.xxx;
   	}

   	color.xyz = lerp( color.xyz, fxcolor.xyz, fxcolorMix );

	return color.xyz;

}

float3 SharpPass( float3 colorInput, float2 tex, sampler colorsampler)
{
     	float3 blur_ori = tex2D(colorsampler, tex + float2(0.5 * PixelSize.x,-PixelSize.y * fSharpBias)).rgb*0.25;  	// South South East
    	blur_ori += tex2D(colorsampler, tex + float2(fSharpBias * -PixelSize.x,0.5 * -PixelSize.y)).rgb*0.25; 		// West South West
   	blur_ori += tex2D(colorsampler, tex + float2(fSharpBias * PixelSize.x,0.5 * PixelSize.y)).rgb*0.25; 		// East North East
    	blur_ori += tex2D(colorsampler, tex + float2(0.5 * -PixelSize.x,PixelSize.y * fSharpBias)).rgb*0.25;		// North North West

	float3 sharp = colorInput - blur_ori;
	float sharp_luma = dot(sharp, fSharpStrength);
	
	sharp_luma = clamp(sharp_luma, -fSharpClamp, fSharpClamp);
	
	float3 done = tex2D(colorsampler, tex).rgb + sharp_luma; 

	colorInput = done;

	return colorInput;
}

float3 ExplosionPass( float3 colorInput, float2 tex, sampler colorsampler )
{

  	// -- pseudo random number generator --
  	float2 sine_cosine;
  	sincos(dot(tex, float2(12.9898,78.233)),sine_cosine.x,sine_cosine.y);
  	sine_cosine = sine_cosine * 43758.5453 + tex;
  	float2 noise = frac(sine_cosine);

  	tex = (-fExplosionRadius * PixelSize) + tex; //Slightly faster this way because it can be calculated while we calculate noise.
  
  	colorInput.rgb = tex2D(colorsampler, (2.0 * fExplosionRadius * PixelSize) * noise + tex).rgb;
  
 
  	return colorInput;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Initializing pass which converts LDR image to HDR space														     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float4 PS_ME_Init(VS_OUTPUT_POST IN) : COLOR 
{
	float mask = 1.0f;
#if(USE_HUD_MASKING==1)
	mask = tex2D(SamplerMask, IN.txcoord.xy).x;
#endif
	return tex2D(SamplerLDR, IN.txcoord.xy)*mask;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Ambient Occlusion													     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float4 PS_ME_SSAO(VS_OUTPUT_POST IN) : COLOR
{
	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;

	//global variables
	float depth		= tex2D(SamplerDepth, IN.txcoord.xy).x;
	float fSceneDepthP 	= GetLinearDepth(depth);

#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = fSceneDepthP;
#else
	float blurkey = dot(GetNormalFromDepth(fSceneDepthP, IN.txcoord.xy).xyz,0.333)*0.1;
#endif

	if(fSceneDepthP > min(0.9999,AO_FADE_END)) return float4(0.5,0.5,0.5,blurkey);

	float offsetScale = fSSAOSamplingRange/10000;
	float fSSAODepthClip = 10000000.0;

	float3 vRotation = tex2Dlod(SamplerNoise, float4(IN.txcoord.xy, 0, 0)).rgb - 0.5f;
	
	float3x3 matRotate;

	float hao = 1.0f / (1.0f + vRotation.z);

	matRotate._m00 =  hao * vRotation.y * vRotation.y + vRotation.z;
	matRotate._m01 = -hao * vRotation.y * vRotation.x;
	matRotate._m02 = -vRotation.x;
	matRotate._m10 = -hao * vRotation.y * vRotation.x;
	matRotate._m11 =  hao * vRotation.x * vRotation.x + vRotation.z;
	matRotate._m12 = -vRotation.y;
	matRotate._m20 =  vRotation.x;
	matRotate._m21 =  vRotation.y;
	matRotate._m22 =  vRotation.z;

	float fOffsetScaleStep = 1.0f + 2.4f / iSSAOSamples;
	float fAccessibility = 0;

	int Sample_Scaled = iSSAOSamples;

	#if(SSAO_SmartSampling==1)
	if(fSceneDepthP > 0.5) Sample_Scaled=max(8,round(Sample_Scaled*0.5));
	if(fSceneDepthP > 0.8) Sample_Scaled=max(8,round(Sample_Scaled*0.5));
	#endif
	
	float fAtten = 5000.0/fSSAOSamplingRange/(1.0+fSceneDepthP*10.0);

	[loop]
	for (int i = 0 ; i < (Sample_Scaled / 8) ; i++)
	for (int x = -1 ; x <= 1 ; x += 2)
	for (int y = -1 ; y <= 1 ; y += 2)
	for (int z = -1 ; z <= 1 ; z += 2) {
		//Create offset vector
		float3 vOffset = normalize(float3(x, y, z)) * (offsetScale *= fOffsetScaleStep);
		//Rotate the offset vector
		float3 vRotatedOffset = mul(vOffset, matRotate);

		//Center pixel's coordinates in screen space
		float3 vSamplePos = float3(IN.txcoord.xy, fSceneDepthP);
 
		//Offset sample point
		vSamplePos += float3(vRotatedOffset.xy, vRotatedOffset.z * fSceneDepthP);

		//Read sample point depth
		float fSceneDepthS = GetLinearDepth(tex2Dlod(SamplerDepth, float4(vSamplePos.xy,0,0)).x);

		//Discard if depth equals max
		if (fSceneDepthS >= fSSAODepthClip)
			fAccessibility += 1.0f;
		else {
			//Compute accessibility factor
			float fDepthDist = abs(fSceneDepthP - fSceneDepthS);
			float fRangeIsInvalid = saturate(fDepthDist*fAtten);
			fAccessibility += lerp(fSceneDepthS > vSamplePos.z, 0.5f, fRangeIsInvalid);
		}
	}
 
	//Compute average accessibility
	fAccessibility = fAccessibility / Sample_Scaled;

	return float4(fAccessibility.xxx,blurkey);

}

float4 PS_ME_RayAO(VS_OUTPUT_POST IN) : COLOR	
{

	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;

	float3	avOffsets [78] =
	{
	float3(0.2196607,0.9032637,0.2254677),
	float3(0.05916681,0.2201506,-0.1430302),
	float3(-0.4152246,0.1320857,0.7036734),
	float3(-0.3790807,0.1454145,0.100605),
	float3(0.3149606,-0.1294581,0.7044517),
	float3(-0.1108412,0.2162839,0.1336278),
	float3(0.658012,-0.4395972,-0.2919373),
	float3(0.5377914,0.3112189,0.426864),
	float3(-0.2752537,0.07625949,-0.1273409),
	float3(-0.1915639,-0.4973421,-0.3129629),
	float3(-0.2634767,0.5277923,-0.1107446),
	float3(0.8242752,0.02434147,0.06049098),
	float3(0.06262707,-0.2128643,-0.03671562),
	float3(-0.1795662,-0.3543862,0.07924347),
	float3(0.06039629,0.24629,0.4501176),
	float3(-0.7786345,-0.3814852,-0.2391262),
	float3(0.2792919,0.2487278,-0.05185341),
	float3(0.1841383,0.1696993,-0.8936281),
	float3(-0.3479781,0.4725766,-0.719685),
	float3(-0.1365018,-0.2513416,0.470937),
	float3(0.1280388,-0.563242,0.3419276),
	float3(-0.4800232,-0.1899473,0.2398808),
	float3(0.6389147,0.1191014,-0.5271206),
	float3(0.1932822,-0.3692099,-0.6060588),
	float3(-0.3465451,-0.1654651,-0.6746758),
	float3(0.2448421,-0.1610962,0.13289366),
	float3(0.2448421,0.9032637,0.24254677),
	float3(0.2196607,0.2201506,-0.18430302),
	float3(0.05916681,0.1320857,0.70036734),
	float3(-0.4152246,0.1454145,0.1800605),
	float3(-0.3790807,-0.1294581,0.78044517),
	float3(0.3149606,0.2162839,0.17336278),
	float3(-0.1108412,-0.4395972,-0.269619373),
	float3(0.658012,0.3112189,0.4267864),
	float3(0.5377914,0.07625949,-0.12773409),
	float3(-0.2752537,-0.4973421,-0.31629629),
	float3(-0.1915639,0.5277923,-0.17107446),
	float3(-0.2634767,0.02434147,0.086049098),
	float3(0.8242752,-0.2128643,-0.083671562),
	float3(0.06262707,-0.3543862,0.007924347),
	float3(-0.1795662,0.24629,0.44501176),
	float3(0.06039629,-0.3814852,-0.248391262),
	float3(-0.7786345,0.2487278,-0.065185341),
	float3(0.2792919,0.1696993,-0.84936281),
	float3(0.1841383,0.4725766,-0.7419685),
	float3(-0.3479781,-0.2513416,0.670937),
	float3(-0.1365018,-0.563242,0.36419276),
	float3(0.1280388,-0.1899473,0.23948808),
	float3(-0.4800232,0.1191014,-0.5271206),
	float3(0.6389147,-0.3692099,-0.5060588),
	float3(0.1932822,-0.1654651,-0.62746758),
	float3(-0.3465451,-0.1610962,0.4289366),
	float3(0.2448421,-0.1610962,0.2254677),
	float3(0.2196607,0.9032637,-0.1430302),
	float3(0.05916681,0.2201506,0.7036734),
	float3(-0.4152246,0.1320857,0.100605),
	float3(-0.3790807,0.3454145,0.7044517),
	float3(0.3149606,-0.4294581,0.1336278),
	float3(-0.1108412,0.3162839,-0.2919373),
	float3(0.658012,-0.2395972,0.426864),
	float3(0.5377914,0.33112189,-0.1273409),
	float3(-0.2752537,0.47625949,-0.3129629),
	float3(-0.1915639,-0.3973421,-0.1107446),
	float3(-0.2634767,0.2277923,0.06049098),
	float3(0.8242752,-0.3434147,-0.03671562),
	float3(0.06262707,-0.4128643,0.07924347),
	float3(-0.1795662,-0.3543862,0.4501176),
	float3(0.06039629,0.24629,-0.2391262),
	float3(-0.7786345,-0.3814852,-0.05185341),
	float3(0.2792919,0.4487278,-0.8936281),
	float3(0.1841383,0.3696993,-0.719685),
	float3(-0.3479781,0.2725766,0.470937),
	float3(-0.1365018,-0.5513416,0.3419276),
	float3(0.1280388,-0.163242,0.2398808),
	float3(-0.4800232,-0.3899473,-0.5271206),
	float3(0.6389147,0.3191014,-0.6060588),
	float3(0.1932822,-0.1692099,-0.6746758),
	float3(-0.3465451,-0.2654651,0.1289366)
	};

	float2 vOutSum;
	float3 vRandom, vReflRay, vViewNormal;
	float fCurrDepth, fSampleDepth, fDepthDelta, fAO;
	float depth = tex2D(SamplerDepth, IN.txcoord.xy).x;
	fCurrDepth  = GetLinearDepth(depth);


#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = fCurrDepth;
#else
	float blurkey = dot(GetNormalFromDepth(fCurrDepth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif

	if(fCurrDepth>min(0.9999,AO_FADE_END)) return float4(1.0,1.0,1.0,blurkey);

	vViewNormal = GetNormalFromDepth(fCurrDepth, IN.txcoord.xy);
	vRandom 	= GetRandomVector(IN.txcoord);
	fAO = 0;
	for(int s = 0; s < iRayAOSamples; s++) {
		vReflRay = reflect(avOffsets[s], vRandom);
		
		float fFlip = sign(dot(vViewNormal,vReflRay));
        	vReflRay   *= fFlip;
		
		float sD = fCurrDepth - (vReflRay.z * fRayAOSamplingRange);
		fSampleDepth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(saturate(IN.txcoord.xy + (fRayAOSamplingRange * vReflRay.xy / fCurrDepth)),0,0)).x);
		fDepthDelta = saturate(sD - fSampleDepth);

		fDepthDelta *= 1-smoothstep(0,fRayAOMaxDepth,fDepthDelta);

		if ( fDepthDelta > fRayAOMinDepth && fDepthDelta < fRayAOMaxDepth)
			fAO += pow(1 - fDepthDelta, 2.5);
	}
	vOutSum.x = saturate(1 - (fAO / (float)iRayAOSamples) + fRayAOSamplingRange);

	return float4(vOutSum.xxx,blurkey);
}


float3 GetEyePosition(in float2 uv, in float eye_z) {
	uv = (uv * float2(2.0, -2.0) - float2(1.0, -1.0));
	float3 pos = float3(uv * InvFocalLen * eye_z, eye_z);
	return pos;
}

float2 GetRandom2_10(in float2 uv) {
	float noiseX = (frac(sin(dot(uv, float2(12.9898,78.233) * 2.0)) * 43758.5453));
	float noiseY = sqrt(1 - noiseX * noiseX);
	return float2(noiseX, noiseY);
}

float4 PS_ME_HBAO(VS_OUTPUT_POST IN) : COLOR	
{
	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;

	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x);

#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = depth;
#else
	float blurkey = dot(GetNormalFromDepth(depth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif

	if(depth > min(0.9999,AO_FADE_END)) return float4(1.0,1.0,1.0,blurkey);

	float2 sample_offset[8] =
	{
		float2(1, 0),
		float2(0.7071f, 0.7071f),
		float2(0, 1),
		float2(-0.7071f, 0.7071f),
		float2(-1, 0),
		float2(-0.7071f, -0.7071f),
		float2(0, -1),
		float2(0.7071f, -0.7071f)
	};

	float3 pos = GetEyePosition(IN.txcoord.xy, depth);
	float3 dx = ddx(pos);
	float3 dy = ddy(pos);
	float3 norm = normalize(cross(dx,dy));
 
	float sample_depth=0;
	float3 sample_pos=0;
 
	float ao=0;
	float s=0.0;
 
	float2 rand_vec = GetRandom2_10(IN.txcoord.xy);
	float2 sample_vec_divisor = InvFocalLen*depth/(fHBAOSamplingRange*float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT));
	float2 sample_center = IN.txcoord.xy;
 
	for (int i = 0; i < 8; i++)
	{
		float theta,temp_theta,temp_ao,curr_ao = 0;
		float3 occlusion_vector = 0.0;
 
		float2 sample_vec = reflect(sample_offset[i], rand_vec);
		sample_vec /= sample_vec_divisor;
		float2 sample_coords = (sample_vec*float2(1,(float)BUFFER_WIDTH/(float)BUFFER_HEIGHT))/iHBAOSamples;
 
		for (int k = 1; k <= iHBAOSamples; k++)
		{
			sample_depth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(sample_center + sample_coords*(k-0.5*(i%2)),0,0)).x);
			sample_pos = GetEyePosition(sample_center + sample_coords*(k-0.5*(i%2)), sample_depth);
			occlusion_vector = sample_pos - pos;
			temp_theta = dot( norm, normalize(occlusion_vector) );			
 
			if (temp_theta > theta)
			{
				theta = temp_theta;
				temp_ao = 1-sqrt(1 - theta*theta );
				ao += (1/ (1 + fHBAOAttenuation * pow(length(occlusion_vector)/fHBAOSamplingRange*5000,2)) )*(temp_ao-curr_ao);
				curr_ao = temp_ao;
			}
		}
		s += 1;
	}
 
	ao /= max(0.00001,s);
 	ao = 1.0-ao*fHBAOAmount;
	ao = clamp(ao,fHBAOClamp,1);

	return float4(ao.xxx, blurkey);

}

float tangent(float3 P, float3 S)
{
    	return (P.z - S.z) / length(S.xy - P.xy);
}

float3 uv_to_eye(float2 uv, float eye_z)
{
    	uv = uv * float2(2.0, -2.0) - float2(1.0, -1.0); // uv (0, 1) to (-1, 1)
    	return float3(uv /* invFocalLength */ * eye_z, eye_z); // Position in view space	
}

float3 fetch_eye_pos(float2 uv)
{
	float z = GetLinearDepth(tex2Dlod(SamplerDepth, float4(uv, 0, 0)).x); // Single channel zbuffer texture
    	return uv_to_eye(uv, z);
}

float3 min_diff(float3 P, float3 Pr, float3 Pl)
{
    	float3 V1 = Pr - P;
    	float3 V2 = P - Pl;
    	return (dot(V1,V1) < dot(V2,V2)) ? V1 : V2;
}

float Falloff(float r)
{
	return 1.0f - fRayHBAO_Attenuation * r * r;
}

float2 snap_uv_offset(float2 uv)
{
    	return round(uv * float2(BUFFER_WIDTH, BUFFER_HEIGHT)) * float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT);
}

float2 snap_uv_coord(float2 uv)
{
    	return uv - (frac(uv * float2(BUFFER_WIDTH, BUFFER_HEIGHT)) - 0.5f) * float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT);
}

float tan_to_sin(float x)
{
    	return x / sqrt(1.0f + x*x);
}

float tangent(float3 T)
{
    	return -T.z / length(T.xy);
}

float2 rotate_direction(float2 Dir, float2 CosSin)
{
    	return float2(Dir.x * CosSin.x - Dir.y * CosSin.y, 
                Dir.x * CosSin.y + Dir.y * CosSin.x);
}

float AccumulatedHorizonOcclusionHighQuality(float2 deltaUV, 
                                             float2 uv0, 
                                             float3 P, 
                                             float numSteps, 
                                             float randstep,
                                             float3 dPdu,
                                             float3 dPdv)
{
    	// Jitter starting point within the first sample distance
    	float2 uv = (uv0 + deltaUV) + randstep * deltaUV;
    
    	// Snap first sample uv and initialize horizon tangent
    	float2 snapped_duv = snap_uv_offset(uv - uv0);
	float3 T = snapped_duv.xxx * dPdu + snapped_duv.yyy * dPdv;	
    	float tanH = tangent(T) + fRayHBAO_AngleBiasTan;

    	float ao = 0;
    	float h0 = 0;
    	float3 occluderRadiance = 0;

	[loop]
    	for(float j = 0; j < numSteps; ++j)
	{
        	float2 snapped_uv = snap_uv_coord(uv);
        	float3 S = fetch_eye_pos(snapped_uv);
		// next uv in image space.
		uv += deltaUV;

        	// Ignore any samples outside the radius of influence
        	float d2 = dot(S-P,S-P);
		
		[flatten]
        	if (d2 < fRayHBAO_SampleRadius)
		{ 
            		float tanS = tangent(P, S);

            		[flatten]
            		if (tanS > tanH) // Is this height is bigger than the bigger height of this direction so far then
			{
                		// Compute tangent vector associated with snapped_uv
                	float2 snapped_duv2 = snapped_uv - uv0;
			float3 T2 = snapped_duv2.xxx * dPdu + snapped_duv2.yyy * dPdv;	//2 for faster compilation.
                	float tanT = tangent(T2) + fRayHBAO_AngleBiasTan;

                	// Compute AO between tangent T and sample S
                	float sinS = tan_to_sin(tanS);
                	float sinT = tan_to_sin(tanT);
                	float r = sqrt(d2) / fRayHBAO_SampleRadius;
                	float h = sinS - sinT;
			float falloff = Falloff(r);
                	ao += falloff * (h - h0);
                	h0 = h;

                	// Update the current horizon angle
                	tanH = tanS;
            		}
        	}

    	}
    	return ao;
}

float4 PS_ME_RayHBAO(VS_OUTPUT_POST IN) : COLOR
{
	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;
	
	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x); 

#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = depth;
#else
	float blurkey = dot(GetNormalFromDepth(depth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif

	if(depth > min(0.9999,AO_FADE_END)) return float4(1.0,1.0,1.0,blurkey);

	float3 P = uv_to_eye(IN.txcoord.xy, depth);	

    	float2 step_size = 0.5 * fRayHBAO_SampleRadius / P.z; // Project radius
   
    	float numSteps = min (iRayHBAO_StepCount, min(step_size.x * BUFFER_WIDTH, step_size.y * BUFFER_HEIGHT));	
	step_size = step_size / ( numSteps + 1 );

    
    	float3 Pr, Pl, Pt, Pb;
		
	// Doesn't use normals
	Pr = fetch_eye_pos(IN.txcoord.xy + float2( BUFFER_RCP_WIDTH,  0));
	Pl = fetch_eye_pos(IN.txcoord.xy + float2(-BUFFER_RCP_WIDTH,  0));
	Pt = fetch_eye_pos(IN.txcoord.xy + float2(0, BUFFER_RCP_HEIGHT));
	Pb = fetch_eye_pos(IN.txcoord.xy + float2(0, -BUFFER_RCP_HEIGHT));

    	// Screen-aligned basis for the tangent plane
    	float3 dPdu = min_diff(P, Pr, Pl);
    	float3 dPdv = min_diff(P, Pt, Pb) * (BUFFER_HEIGHT * BUFFER_RCP_WIDTH);

    	// (cos(alpha),sin(alpha),jitter)
    	float3 rand = tex2D(SamplerNoise, IN.txcoord.xy*5).rgb; 
		
	float ao = 0;
    	float alpha = 2.0f * 3.1416 / iRayHBAO_StepDirections;	
	
	// High Quality
	for (int d = 0; d < iRayHBAO_StepDirections; d++) 
	{
		float angle = alpha * d;
		float2 dir = float2(cos(angle), sin(angle));
		float2 deltaUV = rotate_direction(dir, rand.xy) * step_size.xy;
		ao += AccumulatedHorizonOcclusionHighQuality(deltaUV, IN.txcoord.xy, P, numSteps, rand.z, dPdu, dPdv);
	}
		
	float result = saturate(1.0 - ao / iRayHBAO_StepDirections * 2.0);

	return float4(result.xxx,blurkey);
}

float3 GetSAO_CSPosition(float2 S, float z)
{
	//hardcoded FoV. Don't ask me but even single degree differences HEAVILY affect visual result
	//to a point where AO isn't applied or it's way too strong or whatever. Better leave it.

	float nearZ = 0.1; float farZ = 100.0; float vFOV = 68.0;
	float4x4 matProjection = float4x4(
  	1.0f / (aspect * tan(vFOV / 2.0f)),  0.0f,                     0.0f,                   0.0f,
  	0.0f,                                1.0f / tan(vFOV / 2.0f),  0.0f,                   0.0f,
  	0.0f,                                0.0f,                     farZ / (farZ - nearZ),         1.0f,
  	0.0f,                                0.0f,                     (farZ * nearZ) / (nearZ - farZ),  0.0f
	);

	float4 projInfo;
	projInfo.x = -2.0f / ((float)BUFFER_WIDTH * matProjection._11);
	projInfo.y = -2.0f / ((float)BUFFER_HEIGHT * matProjection._22),
	projInfo.z = ((1.0f - matProjection._13) / matProjection._11) + projInfo.x * 0.5f;
	projInfo.w = ((1.0f + matProjection._23) / matProjection._22) + projInfo.y * 0.5f;
	return float3(( (S.xy * float2(BUFFER_WIDTH,BUFFER_HEIGHT)) * projInfo.xy + projInfo.zw) * z, z);
}

float2 GetSAO_TapLocation(int sampleNumber, float spinAngle, out float ssR)
{

	uint ROTATIONS [98] = { 1, 1, 2, 3, 2, 5, 2, 3, 2,
	3, 3, 5, 5, 3, 4, 7, 5, 5, 7,
	9, 8, 5, 5, 7, 7, 7, 8, 5, 8,
	11, 12, 7, 10, 13, 8, 11, 8, 7, 14,
	11, 11, 13, 12, 13, 19, 17, 13, 11, 18,
	19, 11, 11, 14, 17, 21, 15, 16, 17, 18,
	13, 17, 11, 17, 19, 18, 25, 18, 19, 19,
	29, 21, 19, 27, 31, 29, 21, 18, 17, 29,
	31, 31, 23, 18, 25, 26, 25, 23, 19, 34,
	19, 27, 21, 25, 39, 29, 17, 21, 27 };

	uint NUM_SPIRAL_TURNS = ROTATIONS[iSAOSamples-1];

    	// Radius relative to ssR
    	float alpha = float(sampleNumber + 0.5) * (1.0 / iSAOSamples);
    	float angle = alpha * (NUM_SPIRAL_TURNS * 6.28) + spinAngle;

    	ssR = alpha;
    	float sin_v, cos_v;
    	sincos(angle, sin_v, cos_v);
    	return float2(cos_v, sin_v);
}

float GetSAO_CurveDepth(float depth)
{
	return 202.0 / (-99.0 * depth + 101.0);
}

float3 GetSAO_Position(float2 ssPosition)
{
    	float3 Position;
	Position.z = GetSAO_CurveDepth(tex2Dlod(SamplerDepth, float4(ssPosition.xy,0,0)).x);
	Position = GetSAO_CSPosition(ssPosition, Position.z);
    	return Position;
}

float3 GetSAO_OffsetPosition(float2 ssC, float2 unitOffset, float ssR)
{
    	float2 ssP = ssR*unitOffset + ssC;
	float3 P;
	P.z = GetSAO_CurveDepth(tex2Dlod(SamplerDepth, float4(ssP.xy,0,0)).x);
	P = GetSAO_CSPosition(ssP, P.z);
   	return P;
}

float GetSAO_SampleAO(in float2 ssC, in float3 C, in float3 n_C, in float ssDiskRadius, in int tapIndex, in float randomPatternRotationAngle)
{
    	float ssR;
    	float2 unitOffset = GetSAO_TapLocation(tapIndex, randomPatternRotationAngle, ssR);
    	ssR *= ssDiskRadius;
    	float3 Q = GetSAO_OffsetPosition(ssC, unitOffset, ssR);
	float3 v = Q - C;

	float vv = dot(v, v);
    	float vn = dot(v, n_C);

	float f = max(1.0 - vv * (1.0 / fSAORadius), 0.0); 
	return f * max((vn - fSAOBias) * rsqrt( vv), 0.0);	
}

float4 PS_ME_SAO(VS_OUTPUT_POST IN) : COLOR 
{
	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;
	
	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x); 

#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = depth;
#else
	float blurkey = dot(GetNormalFromDepth(depth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif

	if(depth > min(0.9999,AO_FADE_END)) return float4(1.0,1.0,1.0,blurkey);

    	float3 ssPosition = GetSAO_Position(IN.txcoord.xy);
	float rotAngle = frac(sin(IN.txcoord.xy.x + IN.txcoord.xy.y * 543.31) *  493013.0) * 10.0;

	float3 ssNormals = normalize(cross(normalize(ddy(ssPosition)), normalize(ddx(ssPosition))));
	float ssDiskRadius = fSAORadius / max(ssPosition.z,0.1f);

   	float sum = 0.0;

    	[unroll]
    	for (int i = 0; i < iSAOSamples; ++i) 
    	{
         	sum += GetSAO_SampleAO(IN.txcoord.xy, ssPosition, ssNormals, ssDiskRadius, i, rotAngle);
    	}
	
	sum /= pow(fSAORadius,6.0);

	float A = pow(max(0.0, 1.0 - sqrt(sum * (3.0 / iSAOSamples))), fSAOIntensity);

	A = (pow(A, 0.2) + 1.2 * A*A*A*A) / 2.2;
	float ao = lerp(1.0, A, fSAOClamp);

	return float4(ao.xxx,blurkey);
}

float4 PS_ME_AOBlurV(VS_OUTPUT_POST IN) : COLOR 
{
	//It's better to do this here, upscaling must produce artifacts and upscale-> blur is better than blur -> upscale
	//besides: code is easier an I'm very lazy :P
	IN.txcoord.xy *= AO_TEXSCALE;
	float  sum,totalweight=0;
	float4 base = tex2D(SamplerOcclusion1, IN.txcoord.xy), temp=0;
	
	[loop]
	for (int r = -AO_BLUR_STEPS; r <= AO_BLUR_STEPS; ++r) 
	{
		float2 axis = float2(0.0, 1.0);
		temp = tex2D(SamplerOcclusion1, IN.txcoord.xy + axis * PixelSize * r);
		float weight = AO_BLUR_STEPS-abs(r); 
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(temp.w - base.w));
		sum += temp.x * weight;
		totalweight += weight;
	}

	return float4(sum / (totalweight+0.0001),0,0,base.w);
}

float4 PS_ME_AOBlurH(VS_OUTPUT_POST IN) : COLOR 
{
	float  sum,totalweight=0;
	float4 base = tex2D(SamplerOcclusion2, IN.txcoord.xy), temp=0;
	
	[loop]
	for (int r = -AO_BLUR_STEPS; r <= AO_BLUR_STEPS; ++r) 
	{
		float2 axis = float2(1.0, 0.0);
		temp = tex2D(SamplerOcclusion2, IN.txcoord.xy + axis * PixelSize * r);
		float weight = AO_BLUR_STEPS-abs(r); 
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(temp.w - base.w));
		sum += temp.x * weight;
		totalweight += weight;
	}

	return float4(sum / (totalweight+0.0001),0,0,base.w);
}

float4 PS_ME_AOCombine(VS_OUTPUT_POST IN) : COLOR 
{

	float4 color = tex2D(SamplerHDR1, IN.txcoord.xy);
	float ao = tex2D(SamplerOcclusion1, IN.txcoord.xy).x;

#if( AO_METHOD == 1) //SSAO
	ao -= 0.5;
	if(ao < 0) ao *= fSSAODarkeningAmount;
	if(ao > 0) ao *= fSSAOBrighteningAmount;
	ao = 2 * saturate(ao+0.5);	
#endif

#if( AO_METHOD == 2)
	ao = pow(ao, fRayAOPower);
#endif

#if( AO_DEBUG == 1)
 #if(AO_METHOD == 1)	
	ao *= 0.5;
 #endif
	return ao;
#endif

#if(AO_LUMINANCE_CONSIDERATION == 1)
	float origlum = dot(color.xyz, 0.333);
	float aomult = smoothstep(AO_LUMINANCE_LOWER, AO_LUMINANCE_UPPER, origlum);
	ao = lerp(ao, 1.0, aomult);
#endif	

	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x); 
	ao = lerp(ao,1.0,smoothstep(AO_FADE_START,AO_FADE_END,depth));

	color.xyz *= ao;
	return color;
}

float4 PS_ME_SSGI(VS_OUTPUT_POST IN) : COLOR	
{
	IN.txcoord.xy /= AO_TEXSCALE;
	if(IN.txcoord.x > 1.0 || IN.txcoord.y > 1.0) discard;

	float depth = tex2D(SamplerDepth, IN.txcoord.xy).x;
	depth  = GetLinearDepth(depth);

	if(depth > min(0.9999,AO_FADE_END)) return float4(0.0,0.0,0.0,1.0);

	float giClamp = 0.0;

	float2 sample_offset[24] =
	{
		float2(-0.1376476f,  0.2842022f ),float2(-0.626618f ,  0.4594115f ),
		float2(-0.8903138f, -0.05865424f),float2( 0.2871419f,  0.8511679f ),
		float2(-0.1525251f, -0.3870117f ),float2( 0.6978705f, -0.2176773f ),
		float2( 0.7343006f,  0.3774331f ),float2( 0.1408805f, -0.88915f   ),
		float2(-0.6642616f, -0.543601f  ),float2(-0.324815f, -0.093939f   ),
		float2(-0.1208579f , 0.9152063f ),float2(-0.4528152f, -0.9659424f ),
		float2(-0.6059740f,  0.7719080f ),float2(-0.6886246f, -0.5380305f ),
		float2( 0.5380307f, -0.2176773f ),float2( 0.7343006f,  0.9999345f ),
		float2(-0.9976073f, -0.7969264f ),float2(-0.5775355f,  0.2842022f ),
		float2(-0.626618f ,  0.9115176f ),float2(-0.29818942f, -0.0865424f),
		float2( 0.9161239f,  0.8511679f ),float2(-0.1525251f, -0.07103951f ),
		float2( 0.7022788f, -0.823825f ),float2(0.60250657f,  0.64525909f )
	};

	float sample_radius[24] =
	{	
		0.5162497,0.2443335,
		0.1014819,0.1574599,
		0.6538922,0.5637644,
		0.6347278,0.2467654,
		0.5642318,0.0035689,
		0.6384532,0.3956547,
		0.7049623,0.3482861,
		0.7484038,0.2304858,
		0.0043161,0.5423726,
		0.5025704,0.4066662,
		0.2654198,0.8865175,
		0.9505567,0.9936577
	};

	float3 pos = GetEyePosition(IN.txcoord.xy, depth);
	float3 dx = ddx(pos);
	float3 dy = ddy(pos);
	float3 norm = normalize(cross(dx, dy));
	norm.y *= -1;

	float sample_depth;

	float4 gi = float4(0, 0, 0, 0);
	float is = 0, as = 0;

	float rangeZ = 5000;

	float2 rand_vec = GetRandom2_10(IN.txcoord.xy);
	float2 rand_vec2 = GetRandom2_10(-IN.txcoord.xy);
	float2 sample_vec_divisor = InvFocalLen * depth / (fSSGISamplingRange * PixelSize.xy);
	float2 sample_center = IN.txcoord.xy + norm.xy / sample_vec_divisor * float2(1, aspect);
	float ii_sample_center_depth = depth * rangeZ + norm.z * fSSGISamplingRange * 20;
	float ao_sample_center_depth = depth * rangeZ + norm.z * fSSGISamplingRange * 5;

	[fastopt]
	for (int i = 0; i < iSSGISamples; i++) {
		float2 sample_vec = reflect(sample_offset[i], rand_vec) / sample_vec_divisor;
		float2 sample_coords = sample_center + sample_vec *  float2(1, aspect);
		float  sample_depth = rangeZ * GetLinearDepth(tex2Dlod(SamplerDepth,float4(sample_coords.xy,0,0)).x);
 
		float ii_curr_sample_radius = sample_radius[i] * fSSGISamplingRange * 20;
		float ao_curr_sample_radius = sample_radius[i] * fSSGISamplingRange * 5;
 
		gi.a += clamp(0, ao_sample_center_depth + ao_curr_sample_radius - sample_depth, 2 * ao_curr_sample_radius);
		gi.a -= clamp(0, ao_sample_center_depth + ao_curr_sample_radius - sample_depth - fSSGIModelThickness, 2 * ao_curr_sample_radius);
 
		if ((sample_depth < ii_sample_center_depth + ii_curr_sample_radius) &&
		    (sample_depth > ii_sample_center_depth - ii_curr_sample_radius)) {

			float3 sample_pos = GetEyePosition(sample_coords, sample_depth);
			float3 unit_vector = normalize(pos - sample_pos);
 			gi.rgb += tex2Dlod(SamplerLDR, float4(sample_coords,0,0)).rgb;
		}
 
		is += 1.0f;
		as += 2.0f * ao_curr_sample_radius;
	}
 
	gi.rgb /= is * 5.0f;
	gi.a   /= as;
 
	gi.rgb = 0.0 + gi.rgb * fSSGIIlluminationMult;
	gi.a   = 1.0 - gi.a   * fSSGIOcclusionMult;

	gi.rgb = lerp(dot(gi.rgb, 0.333), gi.rgb, fSSGISaturation);

	return gi;
}

float4 PS_ME_GIBlurV(VS_OUTPUT_POST IN) : COLOR 
{
	IN.txcoord.xy *= AO_TEXSCALE;
	float4 sum=0;
	float totalweight=0;
	float4 base = tex2D(SamplerOcclusion1, IN.txcoord.xy), temp = 0;
	float depth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(IN.txcoord.xy,0,0)).x);
#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = depth;
#else
	float blurkey = dot(GetNormalFromDepth(depth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif
	
	[loop]
	for (int r = -AO_BLUR_STEPS; r <= AO_BLUR_STEPS; ++r) 
	{
		float2 axis = float2(0, 1);
		temp = tex2D(SamplerOcclusion1, IN.txcoord.xy + axis * PixelSize * r);
		float tempdepth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(IN.txcoord.xy + axis * PixelSize * r,0,0)).x);
#if( AO_SHARPNESS_DETECT == 1)
		float tempkey = tempdepth;
#else
		float tempkey = dot(GetNormalFromDepth(tempdepth, IN.txcoord.xy + axis * PixelSize * r).xyz,0.333)*0.1;
#endif
		float weight = AO_BLUR_STEPS-abs(r); 
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(tempkey - blurkey));
		sum += temp * weight;
		totalweight += weight;
	}

	return sum / (totalweight+0.0001);
}

float4 PS_ME_GIBlurH(VS_OUTPUT_POST IN) : COLOR 
{
	float4 sum=0;
	float totalweight=0;
	float4 base = tex2D(SamplerOcclusion2, IN.txcoord.xy), temp = 0;

	float depth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(IN.txcoord.xy,0,0)).x);
#if( AO_SHARPNESS_DETECT == 1)
	float blurkey = depth;
#else
	float blurkey = dot(GetNormalFromDepth(depth, IN.txcoord.xy).xyz,0.333)*0.1;
#endif
	
	[loop]
	for (int r = -AO_BLUR_STEPS; r <= AO_BLUR_STEPS; ++r) 
	{
		float2 axis = float2(1, 0);
		temp = tex2D(SamplerOcclusion2, IN.txcoord.xy + axis * PixelSize * r);
		float tempdepth = GetLinearDepth(tex2Dlod(SamplerDepth, float4(IN.txcoord.xy + axis * PixelSize * r,0,0)).x);
#if( AO_SHARPNESS_DETECT == 1)
		float tempkey = tempdepth;
#else
		float tempkey = dot(GetNormalFromDepth(tempdepth, IN.txcoord.xy + axis * PixelSize * r).xyz,0.333)*0.1;
#endif
		float weight = AO_BLUR_STEPS-abs(r); 
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(tempkey - blurkey));
		sum += temp * weight;
		totalweight += weight;
	}

	return sum / (totalweight+0.0001);
}

float4 PS_ME_GICombine(VS_OUTPUT_POST IN) : COLOR 
{

	float4 color = tex2D(SamplerHDR1, IN.txcoord.xy);
	float4 gi = tex2D(SamplerOcclusion1, IN.txcoord.xy);

#if( AO_DEBUG == 1)
	return gi.wwww; //AO
#endif
#if( AO_DEBUG == 2)
	return gi.xyzz; //GI color
#endif	

#if(AO_LUMINANCE_CONSIDERATION == 1)
	float origlum = dot(color.xyz, 0.333);
	float aomult = smoothstep(AO_LUMINANCE_LOWER, AO_LUMINANCE_UPPER, origlum);
	gi.w = lerp(gi.w, 1.0, aomult);
	gi.xyz = lerp(gi.xyz,0.0, aomult);
#endif	

	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x); 
	gi.xyz = lerp(gi.xyz,0.0,smoothstep(AO_FADE_START,AO_FADE_END,depth));
	gi.w = lerp(gi.w,1.0,smoothstep(AO_FADE_START,AO_FADE_END,depth));

	color.xyz = (color.xyz+gi.xyz)*gi.w;
	return color;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Blurring/DOF/TiltShift												     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float GetFocalDepth(float2 focalpoint)
{ 
 	float depthsum = 0;
 	float fcRadius = 0.00;

	for(int r=0;r<6;r++)
	{ 
 		float t = (float)r; 
 		t *= 3.1415*2/6; 
 		float2 coord = float2(cos(t),sin(t)); 
 		coord.y *= ScreenSize.z; 
 		coord *= fcRadius; 
 		float depth = GetLinearDepth(tex2Dlod(SamplerDepth,float4(coord+focalpoint,0,0)).x); 
 		depthsum+=depth; 
 	}

	depthsum = depthsum/6;

#if(DOF_MANUALFOCUS == 1)
	depthsum = DOF_MANUALFOCUSDEPTH;
#endif

	return depthsum; 
}

float4 PS_ME_CoC(VS_OUTPUT_POST IN) : COLOR //schreibt nach HDR2	
{
	float scenedepth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x);
	float scenefocus =  GetFocalDepth(DOF_FOCUSPOINT);
	
	float depthdiff = abs(scenedepth-scenefocus);
	depthdiff = (scenedepth < scenefocus) ? pow(depthdiff, DOF_NEARBLURCURVE) : depthdiff;
	depthdiff = (scenedepth > scenefocus) ? pow(depthdiff, DOF_FARBLURCURVE) : depthdiff;

	float mask = 1.0f;
#if(USE_HUD_MASKING == 1)
	mask = tex2D(SamplerMask, IN.txcoord.xy).x;
#endif
	return saturate(float4(depthdiff,scenedepth,scenefocus,0))*mask;
}

//RING DOF
float4 GetColorDOF(sampler tex, float2 coords,float blur) //processing the sample
{
	float4 colDF = float4(1,1,1,1);
	colDF.x = tex2Dlod(tex,float4(coords + float2(0.0,1.0)*fRingDOFFringe*blur,0,0)).x;
	colDF.y = tex2Dlod(tex,float4(coords + float2(-0.866,-0.5)*fRingDOFFringe*blur,0,0)).y;
	colDF.z = tex2Dlod(tex,float4(coords + float2(0.866,-0.5)*fRingDOFFringe*blur,0,0)).z;
	return colDF;
}

float4 GetDOF(sampler tex, float2 coords, float blur) //processing the sample
{
	float4 colDF = tex2Dlod(tex,float4(coords,0,0));
	float lum = dot(colDF.xyz,LumCoeff);
	float thresh = max((lum-fRingDOFThreshold)*fRingDOFGain, 0.0);
	float3 nullcol = float3(0,0,0);
	colDF.xyz +=max(0,lerp(nullcol.xyz,colDF.xyz,thresh*blur));
	return colDF;
}

float4 PS_ME_RingDOF1(VS_OUTPUT_POST IN) : COLOR 
{

	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;	
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*origCoC.x/iRingDOFRings;

	return GetColorDOF(SamplerHDR2,IN.txcoord.xy, discRadius.x);
}

float4 PS_ME_RingDOF2(VS_OUTPUT_POST IN) : COLOR 
{

	float CoC = tex2D(SamplerCoC, IN.txcoord.xy).x;	
	
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*CoC/iRingDOFRings;
	
	float4 col = tex2D(SamplerHDR1, IN.txcoord.xy);
	
	if(discRadius.x/PixelSize.x > 0.25) //some optimization thingy
	{

	float s = 1.0;
	int ringsamples;

	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;

	[loop]
	for (int g = 1; g <= iRingDOFRings; g += 1)
	{
		ringsamples = g * iRingDOFSamples;
		[loop]
		for (int j = 0 ; j < ringsamples ; j += 1)
		{
			float step = PI*2.0 / ringsamples;
			float2 sampleoffset = discRadius.xy * float2(cos(j*step)*g, sin(j*step)*g);
			float3 sampleCoC = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy + sampleoffset,0,0)).xyz;
			if(origCoC.y>origCoC.z && sampleCoC.x<CoC) sampleoffset = sampleoffset/CoC*sampleCoC.x;

			col.xyz += GetDOF(SamplerHDR1,IN.txcoord.xy + sampleoffset,CoC).xyz*lerp(1.0,g/iRingDOFRings,fRingDOFBias);  
			s += 1.0*lerp(1.0,g/iRingDOFRings,fRingDOFBias);
		}
	}
	col = col/s; //divide by sample count
	}
	
	return col;
}

//MAGIC DOF
float4 PS_ME_MagicDOF1(VS_OUTPUT_POST IN) : COLOR	
{
	float4 res,tapres;	
	float totalweight=0;
	res = tex2D(SamplerHDR2, IN.txcoord.xy);
	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;
	float2 discRadius = origCoC.x*PixelSize.xy*DOF_BLURRADIUS/iMagicDOFBlurQuality;
	
	int passnum = iMagicDOFBlurQuality;
	//Wilham Anggowo please keep your Fingers from this shader, I don't want to see it in ENB!
	res.xyz = 0;
		
	[loop]
	for (int i = -iMagicDOFBlurQuality; i <= iMagicDOFBlurQuality; ++i) 
	{
		float2 tapoffset = float2((float)i,0)*discRadius.xy;
		float3 sampleCoC = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy + tapoffset,0,0)).xyz;
		if(origCoC.y>origCoC.z && sampleCoC.x<origCoC.x) tapoffset = tapoffset/origCoC.x*sampleCoC.x;
		tapres = tex2Dlod(SamplerHDR2, float4(IN.txcoord.xy+tapoffset,0,0));
		res.xyz += tapres.xyz;
		totalweight+=1;
	}

	res.xyz /= totalweight;

	return float4(res.xyz,1);
}

float4 PS_ME_MagicDOF2(VS_OUTPUT_POST IN) : COLOR	
{

	float4 res,tapres1,tapres2;	
	float totalweight=0;
	res = tex2D(SamplerHDR1, IN.txcoord.xy);
	float3 origcolor = res.xyz;
	
	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;

	float2 discRadius = origCoC.x*PixelSize.xy*DOF_BLURRADIUS/iMagicDOFBlurQuality;

	int passnum = iMagicDOFBlurQuality;

	int lodlevel = clamp(round(discRadius.x/6),0,3);

	res.xyz = 0;
		
	[loop]
	for (int i = -iMagicDOFBlurQuality; i <= iMagicDOFBlurQuality; ++i) 
	{
		float2 tapoffset1 = float2((float)i*discRadius.x*0.5,(float)i*discRadius.y*0.5*tan(60*PIOVER180));
		float2 tapoffset2 = float2(-tapoffset1.x,tapoffset1.y);

		float3 sampleCoC1 = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy + tapoffset1,0,0)).xyz;
		float3 sampleCoC2 = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy + tapoffset2,0,0)).xyz;

		if(origCoC.y>origCoC.z && sampleCoC1.x<origCoC.x) tapoffset1 = tapoffset1/origCoC.x*sampleCoC1.x; //never ask why I cam up with this. It works. Better than any masking I found but still not flawless.
		if(origCoC.y>origCoC.z && sampleCoC2.x<origCoC.x) tapoffset2 = tapoffset2/origCoC.x*sampleCoC2.x;

		tapres1 = tex2Dlod(SamplerHDR1, float4(IN.txcoord.xy+tapoffset1,0,lodlevel));
		tapres2 = tex2Dlod(SamplerHDR1, float4(IN.txcoord.xy+tapoffset2,0,lodlevel));

		totalweight += 1;
		res.xyz += pow(min(tapres1.xyz, tapres2.xyz),fMagicDOFColorCurve);
		}

	res.xyz /= totalweight;
	res.xyz = saturate(pow(saturate(res.xyz), 1/fMagicDOFColorCurve));
	
	return res;
}

//GP65CJ042 DOF
float4 PS_ME_GPDOF1(VS_OUTPUT_POST IN) : COLOR
{
	float4 res=tex2D(SamplerHDR2, IN.txcoord.xy);
	float4 origcolor=0;
	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*max(0,origCoC.x-0.1);//optimization to clean focus areas a bit

	float3 distortion=float3(-1.0, 0.0, 1.0);
	distortion*=fGPDOFChromaAmount;

	origcolor=tex2D(SamplerHDR2, IN.txcoord.xy + discRadius.xy*distortion.x);
	origcolor.w=smoothstep(0.0, origCoC.y, origcolor.w);
	res.x=lerp(res.x, origcolor.x, origcolor.w);
	
	origcolor=tex2D(SamplerHDR2, IN.txcoord.xy + discRadius.xy*distortion.z);
	origcolor.w=smoothstep(0.0, origCoC.y, origcolor.w);
	res.z=lerp(res.z, origcolor.z, origcolor.w);

	return res;
}

float4 PS_ME_GPDOF2(VS_OUTPUT_POST IN) : COLOR
{
	float4 res;
	float4 origcolor=tex2D(SamplerHDR1, IN.txcoord.xy);
	float3 origCoC = tex2D(SamplerCoC, IN.txcoord.xy).xyz;
	float2 discRadius = origCoC.x*DOF_BLURRADIUS*PixelSize.xy;

	res.xyz=origcolor.xyz;
	res.w=dot(res.xyz, 0.3333);
	res.w=max((res.w - fGPDOFBrightnessThreshold) * fGPDOFBrightnessMultiplier, 0.0);
	res.xyz*=(1.0 + res.w*origCoC.x);
	res.xyz*=lerp(1.0,0.0,fGPDOFBias);
	res.w=1.0;
	
	int sampleCycle=0;
	int sampleCycleCounter=0;
	int sampleCounterInCycle=0;
	
	#if ( bGPDOFPolygonalBokeh == 1)
		float basedAngle=360.0 / iGPDOFPolygonCount;
		float2 currentVertex;
		float2 nextVertex;
	
		int	dofTaps=iGPDOFQuality * (iGPDOFQuality + 1) * iGPDOFPolygonCount / 2.0;
	#else
		int	dofTaps=iGPDOFQuality * (iGPDOFQuality + 1) * 4;
	#endif

	
	for(int i=0; i < dofTaps; i++)
	{

		//dumb step incoming
		bool dothatstep=0;
		if(sampleCounterInCycle==0) dothatstep=1;
		if(sampleCycle!=0) 
		{
		if(sampleCounterInCycle % sampleCycle == 0) dothatstep=1;
		}
		//until here
		//ask yourself why so complicated? if(sampleCounterInCycle % sampleCycle == 0 ) gives warnings when sampleCycle=0
		//but it can only be 0 when sampleCounterInCycle is also 0 so it essentially is no division through 0 even if
		//the compiler believes it, it's 0/0 actually but without disabling shader optimizations this is the only way to workaround that.
		
		if(dothatstep==1)
		{
			sampleCounterInCycle=0;
			sampleCycleCounter++;
		
			#if ( bGPDOFPolygonalBokeh == 1)
				sampleCycle+=iGPDOFPolygonCount;
				currentVertex.xy=float2(1.0 , 0.0);
				sincos(basedAngle* 0.017453292, nextVertex.y, nextVertex.x);	
			#else	
				sampleCycle+=8;
			#endif
		}

		sampleCounterInCycle++;
		
		#if (bGPDOFPolygonalBokeh==1)
			float sampleAngle=basedAngle / float(sampleCycleCounter) * sampleCounterInCycle;
			float remainAngle=frac(sampleAngle / basedAngle) * basedAngle;
		
			if(remainAngle < 0.000001)
			{
				currentVertex=nextVertex;
				sincos((sampleAngle +  basedAngle) * 0.017453292, nextVertex.y, nextVertex.x);
			}

			float2 sampleOffset=lerp(currentVertex.xy, nextVertex.xy, remainAngle / basedAngle);
		#else
			float sampleAngle=0.78539816 / float(sampleCycleCounter) * sampleCounterInCycle;
			float2 sampleOffset;
			sincos(sampleAngle, sampleOffset.y, sampleOffset.x);
		#endif
		
		sampleOffset*=sampleCycleCounter / float(iGPDOFQuality);
		sampleOffset*=discRadius;

		float3 sampleCoC = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy+sampleOffset.xy,0,0)).xyz;
		if(origCoC.y>origCoC.z && sampleCoC.x<origCoC.x) sampleOffset = sampleOffset/origCoC.x*sampleCoC.x;

		float4 tap=tex2Dlod(SamplerHDR1, float4(IN.txcoord.xy+sampleOffset.xy,0,0));
		tap.w=dot(tap.xyz, 0.3333);
		float brightMultipiler=max((tap.w - fGPDOFBrightnessThreshold) * fGPDOFBrightnessMultiplier, 0.0);

		tap.xyz*=(1.0 + brightMultipiler*origCoC.x);
		//res.w+=1.0 + fGPDOFBias * pow(float(sampleCycleCounter)/float(iGPDOFQuality), fGPDOFBiasCurve);

		float curvemult = lerp(1.0,pow(float(sampleCycleCounter)/float(iGPDOFQuality), fGPDOFBiasCurve),fGPDOFBias);
		res.xyz += tap.xyz*curvemult;
		res.w += curvemult;

	}

	res.xyz /= res.w;

	return res;
}

//MATSO DOF
float4 GetMatsoDOFCA(sampler col, float2 tex, float CoC)
{
	float3 chroma = pow(float3(0.5, 1.0, 1.5), fMatsoDOFChromaPow * CoC);

	float2 tr = ((2.0 * tex - 1.0) * chroma.r) * 0.5 + 0.5;
	float2 tg = ((2.0 * tex - 1.0) * chroma.g) * 0.5 + 0.5;
	float2 tb = ((2.0 * tex - 1.0) * chroma.b) * 0.5 + 0.5;
	
	float3 color = float3(tex2D(col, tr).r, tex2D(col, tg).g, tex2D(col, tb).b) * (1.0 - CoC);
	
	return float4(color, 1.0);
}

float4 GetMatsoDOFBlur(int axis, float2 coord, sampler SamplerHDRX)
{
	float4 res;
	float4 tcol = tex2D(SamplerHDRX, coord.xy);
	
	float3 origCoC = tex2D(SamplerCoC, coord.xy).xyz;
	float2 discRadius = origCoC.x*DOF_BLURRADIUS*PixelSize.xy*0.5/iMatsoDOFBokehQuality;

	int passnumber=1;

	float sf = 0;

	float2 tdirs[4] = { float2(-0.306, 0.739), float2(0.306, 0.739), float2(-0.739, 0.306), float2(-0.739, -0.306) };

#if (bMatsoDOFBokehEnable==1)
	float wValue = (1.0 + pow(length(tcol.rgb) + 0.1, fMatsoDOFBokehCurve)) * (1.0 - fMatsoDOFBokehLight);	// special recipe from papa Matso ;)
#else
	float wValue = 1.0;
#endif

	for (int i = -iMatsoDOFBokehQuality; i < iMatsoDOFBokehQuality; i++)
	{
		float2 taxis =  tdirs[axis];

		taxis.x = cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.x-sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		taxis.y = sin(fMatsoDOFBokehAngle*PIOVER180)*taxis.x+cos(fMatsoDOFBokehAngle*PIOVER180)*taxis.y;
		
		float2 tdir = (float)i * taxis * discRadius;
		float2 tcoord = coord.xy + tdir.xy;
#if(bMatsoDOFChromaEnable == 1)
		float4 ct = GetMatsoDOFCA(SamplerHDRX, tcoord.xy, discRadius.x);
#else
		float4 ct = tex2D(SamplerHDRX, tcoord.xy);
#endif

#if (bMatsoDOFBokehEnable == 0)
		float w = 1.0 + abs(offset[i]);	// weight blur for better effect
#else	
	// my own pseudo-bokeh weighting
		float b = dot(ct.rgb,0.333) + length(ct.rgb) + 0.1;
		float w = pow(b, fMatsoDOFBokehCurve) + abs((float)i);

#endif
		tcol += ct * w;
		wValue += w;
	}

	tcol /= wValue;

	res.xyz = tcol.xyz;

	res.w = 1.0;
	return res;
}

float4 PS_ME_MatsoDOF1(VS_OUTPUT_POST IN) : COLOR
{
	return GetMatsoDOFBlur(2, IN.txcoord.xy, SamplerHDR2);	
}

float4 PS_ME_MatsoDOF2(VS_OUTPUT_POST IN) : COLOR
{
	return GetMatsoDOFBlur(3, IN.txcoord.xy, SamplerHDR1);	
}

float4 PS_ME_MatsoDOF3(VS_OUTPUT_POST IN) : COLOR
{
	return GetMatsoDOFBlur(0, IN.txcoord.xy, SamplerHDR2);	
}

float4 PS_ME_MatsoDOF4(VS_OUTPUT_POST IN) : COLOR
{
	return GetMatsoDOFBlur(1, IN.txcoord.xy, SamplerHDR1);	
}

float4 PS_ME_Blur(VS_OUTPUT_POST IN) : COLOR	
{
	return tex2D(SamplerHDR1, IN.txcoord.xy);
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Bloom 													     	     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float4 PS_ME_BloomPrePass(VS_OUTPUT_POST IN) : COLOR
{
	
	float4 bloom=0.0;
	float2 bloomuv;

	float2 offset[4]=
	{
		float2(1.0, 1.0),
		float2(1.0, 1.0),
		float2(-1.0, 1.0),
		float2(-1.0, -1.0)
	};

	for (int i=0; i<4; i++)
	{
		bloomuv.xy=offset[i]*PixelSize.xy*2;
		bloomuv.xy=IN.txcoord.xy + bloomuv.xy;
		float4 tempbloom=tex2Dlod(SamplerLDR, float4(bloomuv.xy, 0, 0));
		tempbloom.w = max(0,dot(tempbloom.xyz,0.333)-fAnamFlareThreshold);
		tempbloom.xyz = max(0, tempbloom.xyz-fBloomThreshold); 

#if(USE_HUD_MASKING == 1)
		float mask = tex2Dlod(SamplerMask, float4(bloomuv.xy, 0, 0)).x;
		tempbloom *= mask;
#endif

		bloom+=tempbloom;
	}

	bloom *= 0.1;
	return bloom;
}

float4 PS_ME_BloomPass1(VS_OUTPUT_POST IN) : COLOR
{

	float4 bloom=0.0;
	float2 bloomuv;

	float2 offset[8]=
	{
		float2(1.0, 1.0),
		float2(0.0, -1.0),
		float2(-1.0, 1.0),
		float2(-1.0, -1.0),
		float2(0.0, 1.0),
		float2(0.0, -1.0),
		float2(1.0, 0.0),
		float2(-1.0, 0.0)
	};

	for (int i=0; i<8; i++)
	{
		bloomuv.xy=offset[i]*PixelSize.xy*4;
		bloomuv.xy=IN.txcoord.xy + bloomuv.xy;
		float4 tempbloom=tex2Dlod(SamplerBloom1, float4(bloomuv.xy, 0, 0));
		bloom+=tempbloom;
	}

	bloom *= 0.0625;
	return bloom;
}

float4 PS_ME_BloomPass2(VS_OUTPUT_POST IN) : COLOR
{

	float4 bloom=0.0;
	float2 bloomuv;

	float2 offset[8]=
	{
		float2(0.707, 0.707),
		float2(0.707, -0.707),
		float2(-0.707, 0.707),
		float2(-0.707, -0.707),
		float2(0.0, 1.0),
		float2(0.0, -1.0),
		float2(1.0, 0.0),
		float2(-1.0, 0.0)
	};

	for (int i=0; i<8; i++)
	{
		bloomuv.xy=offset[i]*PixelSize.xy*8;
		bloomuv.xy=IN.txcoord.xy + bloomuv.xy;
		float4 tempbloom=tex2Dlod(SamplerBloom2, float4(bloomuv.xy, 0, 0));
		bloom+=tempbloom;
	}

	bloom *= 0.5; //to brighten up the sample, it will lose brightness in H/V gaussian blur 
	return bloom;
}


float4 PS_ME_BloomPass3(VS_OUTPUT_POST IN) : COLOR
{
	float4 bloom;
	bloom = GaussBlur22(IN.txcoord.xy, SamplerBloom3, 16, 0, 0);
	bloom.a *= fAnamFlareAmount;
	bloom.xyz *= fBloomAmount;
	return bloom;
}

float4 PS_ME_BloomPass4(VS_OUTPUT_POST IN) : COLOR
{
	float4 bloom;
	bloom.xyz = GaussBlur22(IN.txcoord.xy, SamplerBloom4, 16, 0, 1).xyz*2.5;	
	bloom.w   = GaussBlur22(IN.txcoord.xy, SamplerBloom4, 32*fAnamFlareWideness, 0, 0).w*2.5; //to have anamflare texture (bloom.w) avoid vertical blur
	return bloom;
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Lensflares 													     	     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float3 GetDnB (sampler2D tex, float2 coords)
{
	float3 Color = max(0,dot(tex2Dlod(tex,float4(coords.xy,0,4)).rgb,0.333) - ChapFlareTreshold)*ChapFlareIntensity;
	#if(CHAPMANDEPTHCHECK == 1)
	if(tex2Dlod(SamplerDepth,float4(coords.xy,0,3)).x<0.99999) Color = 0;
	#endif
	return Color;
}

float2 GetFlippedTC(float2 texcoords) 
{
	return -texcoords + 1.0;
}

float3 GetDistortedTex(
	sampler2D tex,
	float2 sample_center, // where we'd normally sample
	float2 sample_vector,
	float3 distortion // per-channel distortion coeffs
) {

	float2 final_vector = sample_center + sample_vector * min(min(distortion.r, distortion.g),distortion.b); 

	if(final_vector.x > 1.0 
	|| final_vector.y > 1.0 
	|| final_vector.x < -1.0 
	|| final_vector.y < -1.0)
	return 0;

	else return float3(
		GetDnB(tex,sample_center + sample_vector * distortion.r).r,
		GetDnB(tex,sample_center + sample_vector * distortion.g).g,
		GetDnB(tex,sample_center + sample_vector * distortion.b).b
	);
}

float3 GetBrightPass(float2 tex)
{
	float3 c = tex2D(SamplerHDR1, tex).rgb;
    	float3 bC = max(c - float3(fFlareLuminance, fFlareLuminance, fFlareLuminance), 0.0);
    	float bright = dot(bC, 1.0);
    	bright = smoothstep(0.0f, 0.5, bright);
	float3 result = lerp(0.0, c, bright);
#if (bFlareDepthCheckEnable == 1)
	float checkdepth = tex2D(SamplerDepth, tex).x;
	if(checkdepth < 0.99999) result = 0;
#endif
	return result;

}

float3 GetAnamorphicSample(int axis, float2 tex, float blur)
{
	tex = 2.0 * tex - 1.0;
	tex.x /= -blur;
	tex = 0.5 * tex + 0.5;
	return GetBrightPass(tex);
}

float4 PS_ME_LensPrepass(VS_OUTPUT_POST IN) : COLOR
{
	float4 lens=0;

#if (USE_LENZFLARE == 1)

	float3 lfoffset[19]={
		float3(0.9, 0.01, 4),
		float3(0.7, 0.25, 25),
		float3(0.3, 0.25, 15),
		float3(1, 1.0, 5),
		float3(-0.15, 20, 1),
		float3(-0.3, 20, 1),
		float3(6, 6, 6),
		float3(7, 7, 7),
		float3(8, 8, 8),
		float3(9, 9, 9),
		float3(0.24, 1, 10),
		float3(0.32, 1, 10),
		float3(0.4, 1, 10),
		float3(0.5, -0.5, 2),
		float3(2, 2, -5),
		float3(-5, 0.2, 0.2),
		float3(20, 0.5, 0),
		float3(0.4, 1, 10),
		float3(0.00001, 10, 20)
	};

	float3 lffactors[19]={
		float3(1.5, 1.5, 0),
		float3(0, 1.5, 0),
		float3(0, 0, 1.5),
		float3(0.2, 0.25, 0),
		float3(0.15, 0, 0),
		float3(0, 0, 0.15),
		float3(1.4, 0, 0),
		float3(1, 1, 0),
		float3(0, 1, 0),
		float3(0, 0, 1.4),
		float3(1, 0.3, 0),
		float3(1, 1, 0),
		float3(0, 2, 4),
		float3(0.2, 0.1, 0),
		float3(0, 0, 1),
		float3(1, 1, 0),
		float3(1, 1, 0),
		float3(0, 0, 0.2),
 	       	float3(0.012,0.313,0.588)
	};

	float3 lenstemp = 0;

	float2 lfcoord = float2(0,0);
	float2 distfact=(IN.txcoord.xy-0.5);
	distfact.x *= ScreenSize.z;

	for (int i=0; i<19; i++)
	{
		lfcoord.xy=lfoffset[i].x*distfact;
		lfcoord.xy*=pow(2.0*length(float2(distfact.x,distfact.y)), lfoffset[i].y*3.5);
		lfcoord.xy*=lfoffset[i].z;
		lfcoord.xy=0.5-lfcoord.xy;
		float2 tempfact = (lfcoord.xy-0.5)*2;
		float templensmult = clamp(1.0-dot(tempfact,tempfact),0,1);
		float3 lenstemp1 = dot(tex2Dlod(SamplerHDR1, float4(lfcoord.xy,0,1)).xyz,0.333);

		#if (LENZDEPTHCHECK == 1)
		float templensdepth = tex2D(SamplerDepth, lfcoord.xy).x;
		if(templensdepth < 0.99999) lenstemp1 = 0;
		#endif	
	
		lenstemp1 = max(0,lenstemp1.xyz - fLenzThreshold);
		lenstemp1 *= lffactors[i].xyz*templensmult;

		lenstemp += lenstemp1;
	}

	lens.xyz += lenstemp.xyz*fLenzIntensity;
#endif

#if(USE_CHAPMAN_LENS == 1)
	float2 sample_vector = (float2(0.5,0.5) - IN.txcoord.xy) * ChapFlareDispersal;
	float2 halo_vector = normalize(sample_vector) * ChapFlareSize;

	float3 chaplens = GetDistortedTex(SamplerHDR1, IN.txcoord.xy + halo_vector,halo_vector,ChapFlareCA*2.5f).rgb;

	for (int i = 0; i < ChapFlareCount; ++i) 
	{
		float2 foffset = sample_vector * float(i);
		chaplens += GetDistortedTex(SamplerHDR1, IN.txcoord.xy + foffset,foffset,ChapFlareCA).rgb;

	}
	chaplens *= 1/float(ChapFlareCount);
	lens.xyz += chaplens;
#endif

#if( USE_GODRAYS == 1)
	float2 ScreenLightPos = float2(0.5, 0.5);
	float2 texCoord = IN.txcoord.xy;
	float2 deltaTexCoord = (texCoord.xy - ScreenLightPos.xy);
	deltaTexCoord *= 1.0 / (float)iGodraySamples * fGodrayDensity;


	float illuminationDecay = 1.0;

	for(int g = 0; g < iGodraySamples; g++) {
	
		texCoord -= deltaTexCoord;;
		float4 sample2 = tex2D(SamplerHDR1, texCoord.xy);
		float sampledepth = tex2D(SamplerDepth, texCoord.xy).x;
		sample2.w = saturate(dot(sample2.xyz, 0.3333) - fGodrayThreshold);
		sample2.r *= 1.0;
		sample2.g *= 0.95;
		sample2.b *= 0.85;
		sample2 *= illuminationDecay * fGodrayWeight;
	#if (bGodrayDepthCheck == 1)
		if(sampledepth>0.99999) lens.xyz += sample2.xyz*sample2.w;
	#else
		lens.xyz += sample2;
	#endif
		illuminationDecay *= fGodrayDecay;
	}
#endif

#if(USE_ANAMFLARE == 1)
	float3 anamFlare=0;
	float gaussweight[5] = {0.2270270270, 0.1945945946, 0.1216216216, 0.0540540541, 0.0162162162};
	for(int z=-4; z < 5; z++)
	{
		anamFlare+=GetAnamorphicSample(0, IN.txcoord.xy + float2(0, z * PixelSize.y * 2), fFlareBlur) * fFlareTint* gaussweight[abs(z)];
	}
	lens.xyz += anamFlare * fFlareIntensity;
#endif

	return lens;
}

float4 PS_ME_LensPass1(VS_OUTPUT_POST IN) : COLOR
{
	return GaussBlur22(IN.txcoord.xy, SamplerLens1, 2, 0, 1);	
}

float4 PS_ME_LensPass2(VS_OUTPUT_POST IN) : COLOR
{
	return GaussBlur22(IN.txcoord.xy, SamplerLens2, 2, 0, 0);	
}

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Lighting combine													     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float4 PS_ME_LightingCombine(VS_OUTPUT_POST IN) : COLOR 
{
 
	float4 color = tex2D(SamplerHDR2, IN.txcoord.xy);

#if (USE_BLOOM == 1)
	float3 colorbloom=0;

	colorbloom.xyz += tex2D(SamplerBloom3, IN.txcoord.xy).xyz*1.0;
	colorbloom.xyz += tex2D(SamplerBloom5, IN.txcoord.xy).xyz*9.0;
	colorbloom.xyz *= 0.1;

	colorbloom.xyz = saturate(colorbloom.xyz);
	float colorbloomgray = dot(colorbloom.xyz, 0.333);
	colorbloom.xyz = lerp(colorbloomgray, colorbloom.xyz, fBloomSaturation);
	colorbloom.xyz *= fBloomTint;
	float colorgray = dot(color.xyz, 0.333);

	if(iBloomMixmode == 1) color.xyz = color.xyz + colorbloom.xyz;
	if(iBloomMixmode == 2) color.xyz = 1-(1-color.xyz)*(1-colorbloom.xyz);
	if(iBloomMixmode == 3) color.xyz = max(0.0f,max(color.xyz,lerp(color.xyz,(1.0f - (1.0f - saturate(colorbloom.xyz)) *(1.0f - saturate(colorbloom.xyz * 1.0))),1.0)));
	if(iBloomMixmode == 4) color.xyz = max(color.xyz, colorbloom.xyz);
#endif

#if(USE_GAUSSIAN_ANAMFLARE == 1)
	float3 anamflare = tex2D(SamplerBloom5, IN.txcoord.xy).w*2*fAnamFlareColor;
	anamflare.xyz = max(anamflare.xyz,0);
	color.xyz += pow(anamflare.xyz,1/fAnamFlareCurve);
#endif

#if(USE_LENSDIRT == 1)
	float lensdirtmult = dot(tex2D(SamplerBloom5, IN.txcoord.xy).xyz,0.333);
	float3 dirttex = tex2D(SamplerDirt, IN.txcoord.xy).xyz;

	float3 lensdirt = dirttex.xyz*lensdirtmult*fLensdirtIntensity*fLensdirtTint;
	lensdirt = lerp(dot(lensdirt.xyz,0.333), lensdirt.xyz, fLensdirtSaturation);

	if(iLensdirtMixmode == 1) color.xyz = color.xyz + lensdirt.xyz;
	if(iLensdirtMixmode == 2) color.xyz = 1-(1-color.xyz)*(1-lensdirt.xyz);
	if(iLensdirtMixmode == 3) color.xyz = max(0.0f,max(color.xyz,lerp(color.xyz,(1.0f - (1.0f - saturate(lensdirt.xyz)) *(1.0f - saturate(lensdirt.xyz * 1.0))),1.0)));
	if(iLensdirtMixmode == 4) color.xyz = max(color.xyz, lensdirt.xyz);
#endif

	float3 LensflareSample = tex2D(SamplerLens1, IN.txcoord.xy).xyz;
	float3 LensflareMask   = tex2D(SamplerSprite, IN.txcoord.xy+float2(0.5,0.5)*PixelSize.xy).xyz;
	LensflareMask   += tex2D(SamplerSprite, IN.txcoord.xy+float2(-0.5,0.5)*PixelSize.xy).xyz;
	LensflareMask   += tex2D(SamplerSprite, IN.txcoord.xy+float2(0.5,-0.5)*PixelSize.xy).xyz;
	LensflareMask   += tex2D(SamplerSprite, IN.txcoord.xy+float2(-0.5,-0.5)*PixelSize.xy).xyz;

	color.xyz += LensflareMask*0.25*LensflareSample;

	return color;

}

float4 PS_ME_Colors(VS_OUTPUT_POST IN) : COLOR 
{
	float4 color = tex2D(SamplerHDR1, IN.txcoord.xy);

#if(USE_LED_SHADER == 1 )
	float LEDradius = fLEDCircleSize * 0.375;
	float2 quadPos = floor(IN.txcoord.xy * float2(BUFFER_WIDTH,BUFFER_HEIGHT) / fLEDCircleSize) * fLEDCircleSize;
	float2 quad = quadPos/float2(BUFFER_WIDTH,BUFFER_HEIGHT);
	float2 quadCenter = (quadPos + fLEDCircleSize/2.0);
	float quaddist = length(quadCenter - IN.txcoord.xy * float2(BUFFER_WIDTH,BUFFER_HEIGHT));
	float4 LEDtexel = tex2D(SamplerHDR1, quad);
	float LEDtexellum = saturate(dot(LEDtexel.xyz,0.333)-0.75)*fLEDCircleSize*0.33;
	color.xyz = lerp(fLEDBackgroundColor, LEDtexel.xyz, smoothstep(1.5*(1.0+LEDtexellum),-1.5*(1.0+LEDtexellum), quaddist-LEDradius));
#endif	

#if(USE_LUT == 1)
	float3 LUTcolor = 0.0;	
 #if(iLookupTableMode == 1)  
	LUTcolor.x = tex2D(SamplerLUT, float2(saturate(color.x),0)).x;
	LUTcolor.y = tex2D(SamplerLUT, float2(saturate(color.y),0)).y;
	LUTcolor.z = tex2D(SamplerLUT, float2(saturate(color.z),0)).z;
 #elif (iLookupTableMode == 2)
	float2 GridSize = float2(0.00390625, 0.0625);
	float3 coord3D = saturate(color.xyz);
	coord3D.z  *= 15;
	float shift = floor(coord3D.z);
	coord3D.xy =  coord3D.xy * 15 * GridSize + 0.5 * GridSize;
	coord3D.x += shift * 0.0625;
	LUTcolor.xyz = lerp( tex2D(SamplerLUT3D, coord3D.xy).xyz, tex2D(SamplerLUT3D, coord3D.xy + float2(GridSize.y, 0)).xyz, coord3D.z - shift);
 #endif
	color.xyz = lerp(color.xyz, LUTcolor.xyz, fLookupTableMix);
#endif

#if (USE_CARTOON == 1)
	color.xyz = CartoonPass(color.xyz, IN.txcoord.xy, PixelSize.xy, SamplerHDR1);
#endif

#if (USE_LEVELS== 1)
	color.xyz = LevelsPass(color.xyz);
#endif

#if (USE_TECHNICOLOR == 1)
	color.xyz = TechniPass_prod80(color.xyz);
#endif

#if (USE_SWFX_TECHNICOLOR == 1)
	color.xyz = TechnicolorPass(color.xyz);
#endif

#if (USE_DPX == 1)
	color.xyz = DPXPass(color.xyz);
#endif

#if (USE_MONOCHROME == 1)
	color.xyz = dot(color.xyz, 0.333);
#endif

#if (USE_LIFTGAMMAGAIN == 1)
	color.xyz = LiftGammaGainPass(color.xyz);
#endif
	
#if (USE_TONEMAP == 1)
	color.xyz = TonemapPass(color.xyz);
#endif
	
#if (USE_VIBRANCE == 1)
	color.xyz = VibrancePass(color.xyz);
#endif
	
#if (USE_CURVES == 1)
	color.xyz = CurvesPass(color.xyz);
#endif

#if (USE_SEPIA == 1)
	color.xyz = SepiaPass(color.xyz);
#endif

#if (USE_SKYRIMTONEMAP == 1)
	color.xyz = SkyrimTonemapPass(color.xyz);
#endif

#if (USE_COLORMOOD == 1)
	color.xyz = MoodPass(color.xyz);
#endif
 
#if (USE_CROSSPROCESS == 1)
	color.xyz = CrossPass(color.xyz);
#endif
	
#if (USE_FILMICPASS == 1)
	color.xyz = FilmPass(color.xyz);
#endif

#if (USE_REINHARD == 1)
	color.xyz = ReinhardToneMapping(color.xyz);
#endif

#if (USE_REINHARDLINEAR == 1)
	color.xyz = ReinhardLinearToneMapping(color.xyz);
#endif

#if (USE_HPD == 1)
	color.xyz = HaarmPeterDuikerFilmicToneMapping(color.xyz);
#endif
	
#if (USE_FILMICCURVE == 1)
	color.xyz = CustomToneMapping(color.xyz);
#endif

#if(USE_WATCHDOG_TONEMAP == 1)
	color.xyz = ColorFilmicToneMapping(color.xyz);
#endif

#if (USE_COLORMOD == 1)
	color.xyz = ColormodPass(color.xyz);
#endif

#if (USE_SPHERICALTONEMAP == 1)
	color.xyz = SphericalPass(color.xyz);
#endif

#if (USE_SINCITY == 1)
	color.xyz = SincityPass(color.xyz);
#endif

#if (USE_COLORHUEFX == 1)
	color.xyz = colorhuefx_prod80(color.xyz);
#endif

	return color;

}

float4 PS_ME_PostFX(VS_OUTPUT_POST IN) : COLOR 
{

#if (USE_SMAA == 1)
 #define SamplerCurrent SamplerHDR1
#else
 #define SamplerCurrent SamplerHDR2
#endif

	float4 color = tex2D(SamplerCurrent, IN.txcoord.xy);

#if(USE_FISHEYE_CA == 1)
	float4 coord=0.0;
	coord.xy=IN.txcoord.xy;
	coord.w=0.0;  
	float3 eta = float3(1.0+fFisheyeColorshift*0.9,1.0+fFisheyeColorshift*0.6,1.0+fFisheyeColorshift*0.3);
	float2 center;
	center.x = coord.x-0.5;
	center.y = coord.y-0.5;
	float LensZoom = 1.0/fFisheyeZoom;

	float r2 = (IN.txcoord.x-0.5) * (IN.txcoord.x-0.5) + (IN.txcoord.y-0.5) * (IN.txcoord.y-0.5);     
	float f = 0;

	if( fFisheyeDistortionCubic == 0.0){
		f = 1 + r2 * fFisheyeDistortion;
	}else{
                f = 1 + r2 * (fFisheyeDistortion + fFisheyeDistortionCubic * sqrt(r2));
	};

	float x = f*LensZoom*(coord.x-0.5)+0.5;
	float y = f*LensZoom*(coord.y-0.5)+0.5;
	float2 rCoords = (f*eta.r)*LensZoom*(center.xy*0.5)+0.5;
	float2 gCoords = (f*eta.g)*LensZoom*(center.xy*0.5)+0.5;
	float2 bCoords = (f*eta.b)*LensZoom*(center.xy*0.5)+0.5;
	
	color.x = tex2D(SamplerCurrent,rCoords).r;
	color.y = tex2D(SamplerCurrent,gCoords).g;
	color.z = tex2D(SamplerCurrent,bCoords).b;	
#endif

#undef SamplerCurrent

	return color;
}

float4 PS_ME_Overlay(VS_OUTPUT_POST IN) : COLOR 
{

#if (USE_SMAA == 1)
 #define SamplerCurrent SamplerHDR2
#else
 #define SamplerCurrent SamplerHDR1
#endif

	float4 color = tex2D(SamplerCurrent, IN.txcoord.xy);

#if( USE_HEATHAZE == 1)
	float3 heatnormal = tex2Dlod(SamplerHeat, float4(IN.txcoord.xy*fHeatHazeTextureScale+float2(0.0,Timer.x*0.0001*fHeatHazeSpeed),0,0)).rgb - 0.5;
    float2 heatoffset = normalize(heatnormal.xy) * pow(length(heatnormal.xy), 0.5);
	float3 heathazecolor = 0;
	if (IN.txcoord.x > 0.5) {
		heathazecolor.y = tex2D(SamplerCurrent, IN.txcoord.xy + heatoffset.xy * 0.001 * fHeatHazeOffset).y;
		heathazecolor.x = tex2D(SamplerCurrent, IN.txcoord.xy + heatoffset.xy * 0.001 * fHeatHazeOffset * (1.0 + fHeatHazeChromaAmount)).x;
		heathazecolor.z = tex2D(SamplerCurrent, IN.txcoord.xy + heatoffset.xy * 0.001 * fHeatHazeOffset * (1.0 - fHeatHazeChromaAmount)).z;
	}
	else {
		heathazecolor.y = tex2D(SamplerCurrent, IN.txcoord.xy).y;
		heathazecolor.x = tex2D(SamplerCurrent, IN.txcoord.xy).x;
		heathazecolor.z = tex2D(SamplerCurrent, IN.txcoord.xy).z;
	}
	color.xyz = heathazecolor;
 #if(bHeatHazeDebug == 1)
	color.xyz = heatnormal.xyz+0.5;
 #endif
#endif


/*   xyz

Must be moved elsewhere in order to be used correctly.
Reminder to me: Enable CoC for FOG, too.


#define USE_FOG 1

#define fFogStart 	 0.1
#define	fFogEnd 	 0.7
#define	fFogAmount   	 1.0
#define fFogColor	float3(0.67,0.67,0.67)

#if( USE_FOG == 1)

	float lineardepth = tex2Dlod(SamplerCoC, float4(IN.txcoord.xy,0,0)).y;
	float fogdepth=0;
	fogdepth += tex2Dlod(SamplerCoC, float4(IN.txcoord.xy,0,2)).y;
	fogdepth += tex2Dlod(SamplerCoC, float4(IN.txcoord.xy,0,3)).y;
	fogdepth += tex2Dlod(SamplerCoC, float4(IN.txcoord.xy,0,4)).y;
	fogdepth /= 3;

	fogdepth = lerp(lineardepth, fogdepth, saturate(lineardepth*3));

	float fogamount = saturate(smoothstep(fFogStart, fFogEnd, fogdepth)*fFogAmount);

	float3 fogblur=0;
	fogblur += tex2Dlod(SamplerCurrent, float4(IN.txcoord.xy,0,2)).xyz;
	fogblur += tex2Dlod(SamplerCurrent, float4(IN.txcoord.xy,0,3)).xyz;
	fogblur += tex2Dlod(SamplerCurrent, float4(IN.txcoord.xy,0,4)).xyz;
	fogblur /= 3;

	float3 fogcolor = fFogColor;

	fogblur = (fogblur - fogcolor) * 0.75 + fogcolor;
	color.xyz = lerp(color.xyz, fogblur, saturate(fogamount*2));
	color.xyz = lerp(color.xyz, fogcolor, fogamount);
#endif
*/

#if(USE_SHARPENING == 1)
	color.xyz = SharpPass(color.xyz, IN.txcoord.xy, SamplerCurrent);
#endif

#if (USE_EXPLOSION == 1)
	color.xyz = ExplosionPass(color.xyz, IN.txcoord.xy, SamplerCurrent);
#endif

#if(USE_GRAIN == 1)
	float3 noisesample = tex2D(SamplerNoise, IN.txcoord.xy).xyz;
	float noisegray = dot(noisesample, 0.333);
	noisesample.xyz = lerp(noisegray.xxx, noisesample.xyz, fGrainSaturation);
	float colorgray = dot(color.xyz, 0.333);
	float fGrainAmount = fGrainIntensityMid;

	if(colorgray > 0.5) fGrainAmount = lerp(fGrainIntensityMid, fGrainIntensityBright, saturate(colorgray-0.5)*2);
	if(colorgray < 0.5) fGrainAmount = lerp(fGrainIntensityDark, fGrainIntensityMid, colorgray*2);

	noisesample.xyz = (noisesample.xyz-0.5)*fGrainAmount;
	color.xyz = max(0, color.xyz + noisesample.xyz);
#endif

#if (USE_COLORVIGNETTE==1)
        float2	uv=(IN.txcoord-0.5)*fVignetteRadius;
	float	vignetteold=saturate(dot(uv.xy, uv.xy));
	vignetteold=pow(vignetteold, fVignetteCurve);
	float3	EVignetteColor=fVignetteColor;
	color.xyz=lerp(color.xyz, EVignetteColor, vignetteold*fVignetteAmount);
#endif

#if (USE_HD6_VIGNETTE==1)
	float rovigpwr = fHD6VignetteRoundness; //for a circular vignette
	float2 sqvigpwr = float2( fHD6VignetteTop, fHD6VignetteBottom ); // for the top and bottom of the screen
	float vsatstrength = fHD6VignetteColorDistort; // color distortion
	float vignettepow = fHD6VignetteContrast; // increases the contrast and sharpness
	float vstrengthatnight = fHD6VignetteBorder;
 
 	float2 inTex = IN.txcoord;
 	float vhnd = 0.5;
 	float4 voriginal = color;
 	float4 vcolor = voriginal;
 	vcolor.xyz=1;
 	inTex -= 0.5; // center
 	inTex.y += 0.01; // offset from the center
 	float vignette = saturate(1.0 - dot( inTex, inTex ));
 	vcolor *= pow( vignette, vignettepow );
 
 	float4 rvigtex = vcolor;
 	rvigtex.xyz = pow( vcolor.xyz, 1 );
 	rvigtex.xyz = lerp(float3(0.5, 0.5, 0.5), rvigtex.xyz, 2.25); // contrast
 	rvigtex.xyz = lerp(float3(1,1,1),rvigtex.xyz,rovigpwr); // strength of the circular vinetty
 
	//darken the top and bottom
 	float4 vigtex = vcolor;
 	vcolor.xyz = float3(1,1,1);
 #if (fHD6VignetteMode==1)
 	float3 topv = min((inTex.x+0.5)*2,1.5) * 2; // top
 	float3 botv = min(((0-inTex.x)+0.5)*2,1.5) * 2; // botton
	topv= lerp(float3(1,1,1), topv, sqvigpwr.x);
 	botv= lerp(float3(1,1,1), botv, sqvigpwr.y);
	vigtex.xyz = (topv)*(botv);
 #endif
 #if (fHD6VignetteMode==2)
        float3 topv = min((inTex.y+0.5)*2,1.5) * 2; // top
 	float3 botv = min(((0-inTex.y)+0.5)*2,1.5) * 2; // botton
	topv= lerp(float3(1,1,1), topv, sqvigpwr.x);
 	botv= lerp(float3(1,1,1), botv, sqvigpwr.y);
	vigtex.xyz = (topv)*(botv);
 #endif
 #if (fHD6VignetteMode==3)
	float3 rightv = min((inTex.x+0.5)*2,1.5) * 2;
 	float3 leftv = min(((0-inTex.x)+0.5)*2,1.5) * 2; 
        float3 topv = min((inTex.y+0.5)*2,1.5) * 2; 
 	float3 botv = min(((0-inTex.y)+0.5)*2,1.5) * 2; 
 	rightv= lerp(float3(1,1,1), rightv, sqvigpwr.y);
 	leftv= lerp(float3(1,1,1), leftv, sqvigpwr.x);
        topv= lerp(float3(1,1,1), topv, sqvigpwr.x);
 	botv= lerp(float3(1,1,1), botv, sqvigpwr.y);
 	vigtex.xyz = (topv)*(botv)*(rightv)*(leftv);
 #endif
 	// mix the two types of vignettes
 	vigtex.xyz*=rvigtex.xyz;
	vigtex.xyz = lerp(vigtex.xyz,float3(1,1,1),(vhnd-vstrengthatnight*vhnd)); //for a dark screen
 	vigtex.xyz = min(vigtex.xyz,1);
 	vigtex.xyz = max(vigtex.xyz,0);
 	float3 vtintensity = dot(voriginal.xyz, float3(0.2125, 0.7154, 0.0721));
 	color.xyz = lerp(vtintensity, voriginal.xyz, ((((1-(vigtex.xyz*2))+2)-1)*vsatstrength)+1 );
  	color.xyz *= (vigtex.xyz);
#endif

#if (USE_BORDER==1)
	color.xyz = BorderPass(color, IN.txcoord.xy).xyz;
#endif

#if (USE_MOVIEBARS == 1)
	color.xyz = IN.txcoord.y > 0.12 && IN.txcoord.y < 0.88 ? color.xyz : 0.0;
#endif

#if(USE_DEPTHBUFFER_OUTPUT == 1)
	color.xyz = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord.xy).x);
#endif

#if(USE_SPLITSCREEN == 1)
	if(IN.txcoord.x > 0.5) color.xyz = tex2D(SamplerLDR, IN.txcoord.xy).xyz;
#endif

#if(USE_HUD_MASKING == 1)
	float HUDMaskSample = tex2D(SamplerMask, IN.txcoord.xy).x;
	float3 origcolor = tex2D(SamplerLDR, IN.txcoord.xy).xyz;
	color.xyz = lerp(origcolor.xyz, color.xyz, saturate(HUDMaskSample));
#endif

#if(USE_W_TEST == 1)
	float diff = 0.0001;
	float k = 0.03;
	float depth = GetLinearDepth(tex2D(SamplerDepth, IN.txcoord).x).x;
	float depthL = GetLinearDepth(tex2D(SamplerDepth, float2(IN.txcoord.x - diff, IN.txcoord.y)).x).x;
	float depthR = GetLinearDepth(tex2D(SamplerDepth, float2(IN.txcoord.x + diff, IN.txcoord.y)).x).x;
	float depthU = GetLinearDepth(tex2D(SamplerDepth, float2(IN.txcoord.x, IN.txcoord.y - diff)).x).x;
	float depthD = GetLinearDepth(tex2D(SamplerDepth, float2(IN.txcoord.x, IN.txcoord.y + diff)).x).x;
	float ddx = (depthR - depthL)/(2*diff);
	float ddy = (depthU - depthD)/(2*diff);
	float mag = sqrt(ddx*ddx + ddy*ddy);
	float ddxn = (mag == 0) ? 1 : ddx / mag;
	float ddyn = (mag == 0) ? 0 : ddy / mag;
	float fac = (depth == 0) ? 0 : -0.3/mag*(1 - depth);
	float newx = IN.txcoord.x - ddxn * fac;
	float newy = IN.txcoord.y + ddyn * fac;
	//color.xyz = 1;
	//color /= (1 + mag);
	color.xyz = lerp(color.xyz, tex2D(SamplerLDR, float2(newx, newy)).xyz, (1-pow(1 + k, -mag)));
	//if (tex2D(SamplerLDR, float2(IN.txcoord.x / 2, IN.txcoord.y / 2)).x > 0.8)
	//	color.xyz = tex2D(SamplerLDR, float2((IN.txcoord.x + 1) / 2, IN.txcoord.y / 2)).xyz;
	//else
	//	color.xyz = tex2D(SamplerLDR, float2(IN.txcoord.x / 2, IN.txcoord.y / 2)).xyz;
#endif

	return color;
}

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

technique MasterEffect < bool enabled = 1; toggle = MASTEREFFECT_TOGGLEKEY; >
{
	pass ME_Init
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_Init;
		RenderTarget = texHDR1;
	}
								//so both HDR1 and HDR2 textures are filled with input color
	pass ME_Init						//later, numerous DOF shaders have different passnumber but later passes depend
	{							//on fixed HDR1 HDR2 HDR1 HDR2... sequence so a 2 pass DOF outputs HDR1 in pass 1 and 	
		VertexShader = VS_MasterEffect;			//HDR2 in second pass, a 3 pass DOF outputs HDR2, HDR1, HDR2 so last pass outputs always HDR2
		PixelShader = PS_ME_Init;
		RenderTarget = texHDR2;
	}

#if(USE_AMBIENTOCCLUSION == 1)
  #if(AO_METHOD==1)
	pass ME_SSAO
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_SSAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==2)
	pass ME_RayAO
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_RayAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==3)
	pass ME_HBAO
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_HBAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==5)
	pass ME_HBAO
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_RayHBAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==6)
	pass ME_HBAO
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_SAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD != 4)
	pass ME_AOBlurV
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_AOBlurV;
		RenderTarget = texOcclusion2;
	}
	
	pass ME_AOBlurH
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_AOBlurH;
		RenderTarget = texOcclusion1;
	}

	pass ME_AOCombine
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_AOCombine;
		RenderTarget = texHDR2; 
	}	
  #endif
  #if(AO_METHOD == 4)
	pass ME_SSGI
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_SSGI;
		RenderTarget = texOcclusion1;
	}

	pass ME_GIBlurV
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_GIBlurV;
		RenderTarget = texOcclusion2;
	}

	pass ME_GIBlurH
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_GIBlurH;
		RenderTarget = texOcclusion1;
	}

	pass ME_GICombine
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_GICombine;
		RenderTarget = texHDR2; 
	}
  #endif
#endif

#if (USE_BLOOM == 1 || USE_GAUSSIAN_ANAMFLARE == 1 || USE_LENSDIRT == 1)
	pass BloomPrePass
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_BloomPrePass;
		RenderTarget = texBloom1;
	}
	
	pass BloomPass1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_BloomPass1;
		RenderTarget = texBloom2;
	}

	pass BloomPass2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_BloomPass2;
		RenderTarget = texBloom3;
	}

	pass BloomPass3
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_BloomPass3;
		RenderTarget = texBloom4;
	}

	pass BloomPass4
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_BloomPass4;
		RenderTarget = texBloom5;
	}
#endif

#if (USE_LENZFLARE == 1 || USE_CHAPMAN_LENS == 1 || USE_GODRAYS == 1 || USE_ANAMFLARE == 1)
	pass ME_LensPrepass
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_LensPrepass;
		RenderTarget = texLens1;
	}
	
	pass ME_LensPass1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_LensPass1;
		RenderTarget = texLens2;
	}

	pass ME_LensPass2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_LensPass2;
		RenderTarget = texLens1;
	}
#endif
#if(USE_DEPTHOFFIELD == 1)
	pass ME_CoC
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_CoC;
		RenderTarget = texCoC;
	}
#endif
#if(USE_DEPTHOFFIELD==1)
   #if(DOF_METHOD==1)	
	pass ME_RingDOF1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_RingDOF1;
		RenderTarget = texHDR1;
	}
	pass ME_RingDOF2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_RingDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==2)	
	pass ME_MagicDOF1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MagicDOF1;
		RenderTarget = texHDR1;
	}
	pass ME_MagicDOF2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MagicDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==3)	
	pass ME_GPDOF1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_GPDOF1;
		RenderTarget = texHDR1;
	}
	pass ME_GPDOF2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_GPDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==4)	
	pass ME_MatsoDOF1
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MatsoDOF1;
		RenderTarget = texHDR1;
	}
	pass ME_MatsoDOF2
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MatsoDOF2;
		RenderTarget = texHDR2;
	}
	pass ME_MatsoDOF3
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MatsoDOF3;
		RenderTarget = texHDR1;
	}
	pass ME_MatsoDOF4
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_MatsoDOF4;
		RenderTarget = texHDR2;
	}
   #endif
#endif
	pass ME_LightingCombine
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_LightingCombine;
		RenderTarget = texHDR1;
	}

	pass ME_Colors
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_Colors;
		RenderTarget = texHDR2;
	}

#if (USE_SMAA == 1)
	pass SMAA_EdgeDetection //First SMAA Pass
	{
		VertexShader = SMAAEdgeDetectionVSWrap;
 #if (iSMAAEdgeDetectionMode == 1)
		PixelShader = SMAALumaEdgeDetectionPSWrap;
 #elif (iSMAAEdgeDetectionMode == 3)
		PixelShader = SMAADepthEdgeDetectionPSWrap;
 #else
		PixelShader = SMAAColorEdgeDetectionPSWrap; //Probably the best in most cases so I default to this.
 #endif
		StencilEnable = true;
		StencilPass = REPLACE;
		StencilRef = 1;
		RenderTarget = texEdges;
	}
	
	pass SMAA_BlendWeightCalculation //Second SMAA Pass
	{
		VertexShader = SMAABlendingWeightCalculationVSWrap;
		PixelShader = SMAABlendingWeightCalculationPSWrap;
     		StencilEnable = true;
		StencilPass = KEEP;
		StencilFunc = EQUAL;
		StencilRef = 1;
		RenderTarget = texBlend;
	}
	
	pass SMAA_NeighborhoodBlending //Third SMAA Pass
	{
		VertexShader = SMAANeighborhoodBlendingVSWrap;
		PixelShader  = SMAANeighborhoodBlendingPSWrap;
		
 #if (iSMAADebugOutput == 5)
		// Use the stencil so we can show it.
		StencilEnable = true;
		StencilPass = KEEP;
		StencilFunc = EQUAL;
		StencilRef = 1;
 #else
		// Process all the pixels.
		StencilEnable = false;
 #endif
		RenderTarget = texHDR1;	
	}
#endif
	pass ME_PostFX
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_PostFX;
#if (USE_SMAA == 1)					//exactly what I wanted to prevent with new system :(
		RenderTarget = texHDR2;			//but I see now, first AA and then chromatic abberation gives smoother result...
#else							//this of course eliminates the option to use 2 AA's together because with each count 
		RenderTarget = texHDR1;			//this here would get more and more complicated...damn
#endif							//Noone will ever read this so I'm basically talking to a text file. Hello text file.
	}						

	pass ME_Overlay
	{
		VertexShader = VS_MasterEffect;
		PixelShader = PS_ME_Overlay;
	}
}
