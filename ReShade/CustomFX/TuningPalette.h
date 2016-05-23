NAMESPACE_ENTER(CFX)

#include CFX_SETTINGS_DEF

#if USE_TUNINGPALETTE

/**
 * Copyright (C) 2015 Lucifer Hawk (mediehawk@gmail.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software with restriction, including without limitation the rights to
 * use and/or sell copies of the Software, and to permit persons to whom the Software 
 * is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and the permission notices (this and below) shall 
 * be included in all copies or substantial portions of the Software.
 *
 * Permission needs to be specifically granted by the author of the software to any
 * person obtaining a copy of this software and associated documentation files 
 * (the "Software"), to deal in the Software without restriction, including without 
 * limitation the rights to copy, modify, merge, publish, distribute, and/or 
 * sublicense the Software, and subject to the following conditions:
 *
 * The above copyright notice and the permission notices (this and above) shall 
 * be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

texture mapTex	< string source = "ReShade/CustomFX/Textures/" TuningColorMapTexture; > {Width = 256; Height = 256; Format = RGBA8;};
sampler	mapColor 	{ Texture = mapTex; };

texture paletteTex	< string source = "ReShade/CustomFX/Textures/" TuningColorPaletteTexture; > {Width = TuningTileAmountX*16; Height = TuningTileAmountY*16; Format = RGBA8;};
sampler	paletteColor 	{ Texture = paletteTex; };

texture ColorLUTDstTex	< string source = "ReShade/CustomFX/Textures/" TuningColorLUTDstTexture; > {Width = 256; Height = 16; Format = RGBA8;};
sampler	ColorLUTDstColor 	{ Texture = ColorLUTDstTex; };

float4 PS_TuningPalette(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	float4 original = tex2D(RFX_backbufferColor, texcoord.xy);

#if TuningColorLUT
	float4 ColorLUTDst = float4((original.rg*15f+0.5f)*TuningColorLUTNorm,original.b*15,original.w);
	ColorLUTDst.x += trunc(ColorLUTDst.z)*TuningColorLUTNorm.y;
	ColorLUTDst = lerp(tex2D(ColorLUTDstColor, ColorLUTDst.xy),tex2D(ColorLUTDstColor, float2(ColorLUTDst.x+TuningColorLUTNorm.y,ColorLUTDst.y)),frac(ColorLUTDst.z));
	original = lerp(original,ColorLUTDst,TuningColorLUTIntensity);
#endif

#if TuningColorMap
#if TuningPaletteDependency
	float mapX = sqrt(0.641*original.r*original.r+0.291*original.g*original.g+0.068*original.b*original.b);
#else
	float mapX = (original.r+original.g+original.b)/3.0f;
#endif
	float mapY = 0.5f;

//DetectLow
#if AL_HQAdapt	
	float4 detectLow = tex2D(detectLowHQColor, float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT));
#else
	float4 detectLow = tex2D(detectLowColor, float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT));
#endif
	float low = sqrt(0.641*detectLow.r*detectLow.r+0.291*detectLow.g*detectLow.g+0.068*detectLow.b*detectLow.b);
	low *= min(1.0f,1.641*detectLow.r/(1.719*detectLow.g+1.932*detectLow.b));
//.DetectLow

	mapY = low*8f;

	float4 palette = tex2D(mapColor, float2(mapX,mapY));
	original.rgb = lerp(original.rgb,palette.rgb,TuningPalettePower*palette.a);
#endif

#if TuningColorPalette
	float2 paletteCoord = float2(8f/(TuningTileAmountX*16f),8f/(TuningTileAmountY*16f)); //shorten 8/16
	float3 paletteColors = float3(0,0,0);
	float diff = 3f;
	#if TuningTileAmountX > 10
		[loop]
	#else
		[unroll]
	#endif
	for (int x = 0; x < TuningTileAmountX; x++)
		#if TuningTileAmountY > 10 
			[loop]
		#else
			[unroll]
		#endif
		for (int y = 0; y < TuningTileAmountY; y++) {
			float3 paletteColorsNew = tex2Dlod(paletteColor, float4(paletteCoord+16f*float2(x/(TuningTileAmountX*16f),y/(TuningTileAmountY*16f)),0.0,0.0)).rgb;
#if TuningPaletteDependency
			float diffNew = abs(sqrt(0.641*paletteColorsNew.r*paletteColorsNew.r+0.291*paletteColorsNew.g*paletteColorsNew.g+0.068*paletteColorsNew.b*paletteColorsNew.b)-sqrt(0.641*original.r*original.r+0.291*original.g*original.g+0.068*original.b*original.b));
#else
			float diffNew = abs(paletteColorsNew.r-original.r)*TuningColorPalettePower.r+abs(paletteColorsNew.g-original.g)*TuningColorPalettePower.g+abs(paletteColorsNew.b-original.b)*TuningColorPalettePower.b;
#endif
			if (diffNew == 0) { original.rgb = paletteColorsNew; return original; }
			
			[flatten]
			if (diff > diffNew) {
				paletteColors = paletteColorsNew;
				diff = diffNew;
			}
		}
	original.rgb = lerp(original.rgb,paletteColors.rgb,max(0,TuningPalettePower-diff*TuningColorPaletteSmoothMix));
#endif
	return original;

}

technique TuningPalette_Tech <bool enabled = RFX_Start_Enabled; int toggle = TuningPalette_ToggleKey; >
{
	pass TuningPalettePass
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_TuningPalette;
	}
}

#endif

#include CFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()
