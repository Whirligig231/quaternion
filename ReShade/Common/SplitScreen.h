#if USE_RFX_SplitScreen

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

//Split Screen Shader
NAMESPACE_ENTER(RFX)

texture sMaskTex < source = "ReShade/Common/Textures/RFX_sMask.png"; > { Width = 1920; Height = 1080; MipLevels = 1; Format = RGBA8; };
sampler sMaskColor { Texture = sMaskTex; };

uniform float2 sSlider < source = "pingpong"; min = 0; max = 1; step = float2(RFX_SSsliderSpeed, RFX_SSsliderSpeed); >;

float4 PS_Split(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{

//REDUNDANCY NEEDS TO BE DECREASED :-)

	#if RFX_SSaxis
		#if RFX_SScomparable && RFX_SSslider
			if(texcoord.x < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(0.5f+texcoord.x-sSlider.x/2.0f,texcoord.y));
			else if(texcoord.x > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x-sSlider.x/2.0f,texcoord.y));		
			else return float4(0.95,0.95,0.9,1);
		#elif RFX_SScomparable
			#if RFX_SScomparableStretch
				if(texcoord.y < 0.25f || texcoord.y > 0.75f) return float4(0,0,0,0);
				if(texcoord.x < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(texcoord.x*2.0f,texcoord.y*2.0f-0.5f));
				else if(texcoord.x > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x*2.0f-1.0f,texcoord.y*2.0f-0.5f));
				else return float4(0.95,0.95,0.9,1);	
			#else
				if(texcoord.x < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(texcoord.x+0.25f,texcoord.y));
				else if(texcoord.x > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x-0.25f,texcoord.y));
				else return float4(0.95,0.95,0.9,1);
			#endif
		#elif RFX_SSslider
			if(texcoord.x < sSlider.x-0.005f) return tex2D(RFX_originalColor, texcoord);
			else if(texcoord.x > sSlider.x+0.005f) return tex2D(RFX_backbufferColor, texcoord);
			else return float4(0.95,0.95,0.9,1);
		#endif		
	#else
		#if RFX_SScomparable && RFX_SSslider
			if(texcoord.y < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(texcoord.x,0.5f+texcoord.y-sSlider.x/2.0f));
			else if(texcoord.y > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x,texcoord.y-sSlider.x/2.0f));
			else return float4(0.95,0.95,0.9,1);		
		#elif RFX_SScomparable
			#if RFX_SScomparableStretch
				if(texcoord.x < 0.25f || texcoord.x > 0.75f) return float4(0,0,0,0);
				if(texcoord.y < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(texcoord.x*2.0f-0.5f,texcoord.y*2.0f));
				else if(texcoord.y > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x*2.0f-0.5f,texcoord.y*2.0f-1-0f));
				else return float4(0.95,0.95,0.9,1);
			#else
				if(texcoord.y < 0.5f-RFX_SSborderWidth) return tex2D(RFX_originalColor, float2(texcoord.x,texcoord.y+0.25f));
				else if(texcoord.y > 0.5f+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, float2(texcoord.x,texcoord.y-0.25f));
				else return float4(0.95,0.95,0.9,1);
			#endif
		#elif RFX_SSslider
			if(texcoord.y < sSlider.x-RFX_SSborderWidth) return tex2D(RFX_originalColor, texcoord);
			else if(texcoord.y > sSlider.x+RFX_SSborderWidth) return tex2D(RFX_backbufferColor, texcoord);
			else return float4(0.95,0.95,0.9,1);
		#endif		
	#endif

	return lerp(tex2D(RFX_originalColor, texcoord), tex2D(RFX_backbufferColor, texcoord), tex2D(sMaskColor, texcoord).r); 
}

technique SplitScreen_Tech <bool enabled = RFX_Start_Enabled; int toggle = RFX_SS_ToggleKey; >
{
	pass 
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_Split;
	}
}

NAMESPACE_LEAVE()

#endif