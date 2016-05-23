NAMESPACE_ENTER(MFX)

#include MFX_SETTINGS_DEF

#if USE_DEPTHOFFIELD

//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//LICENSE AGREEMENT AND DISTRIBUTION RULES:
//1 Copyrights of the Master Effect exclusively belongs to author - Gilcher Pascal aka Marty McFly.
//2 Master Effect (the SOFTWARE) is DonateWare application, which means you may or may not pay for this software to the author as donation.
//3 If included in ENB presets, credit the author (Gilcher Pascal aka Marty McFly).
//4 Software provided "AS IS", without warranty of any kind, use it on your own risk. 
//5 You may use and distribute software in commercial or non-commercial uses. For commercial use it is required to warn about using this software (in credits, on the box or other places). Commercial distribution of software as part of the games without author permission prohibited.
//6 Author can change license agreement for new versions of the software.
//7 All the rights, not described in this license agreement belongs to author.
//8 Using the Master Effect means that user accept the terms of use, described by this license agreement.
 //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//For more information about license agreement contact me:
//https://www.facebook.com/MartyMcModding
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Copyright (c) 2009-2015 Gilcher Pascal aka Marty McFly
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//Credits :: Matso (Matso DOF), PetkaGtA, gp65cj042
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

texture   texCoC	{ Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16F;};

sampler2D SamplerCoC
{
	Texture = texCoC;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = LINEAR;
	AddressU = Clamp;
	AddressV = Clamp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Blurring/DOF/TiltShift												     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

float GetFocalDepth(float2 focalpoint)
{ 
 	float depthsum = 0;
 	float fcRadius = 0.00;

	[unroll]
	for(int r=0;r<6;r++)
	{ 
 		float t = (float)r; 
 		t *= 3.1415*2/6; 
 		float2 coord = float2(cos(t),sin(t)); 
 		coord.y *= ScreenSize.z; 
 		coord *= fcRadius; 
 		float depth = GetLinearDepth(tex2Dlod(RFX_depthColor,float4(coord+focalpoint,0,0)).x); 
 		depthsum+=depth; 
 	}

	depthsum = depthsum/6;

#if(DOF_MANUALFOCUS == 1)
	depthsum = DOF_MANUALFOCUSDEPTH;
#endif

	return depthsum; 
}

#if(USE_DEPTHOFFIELD==1)
void PS_DOF_CoC(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0) //schreibt nach HDR2	
{
	float scenedepth = tex2D(RFX_depthTexColor,texcoord).r;//GetLinearDepth(tex2D(RFX_depthColor, texcoord.xy).x);
	float scenefocus =  GetFocalDepth(DOF_FOCUSPOINT);
	
	float depthdiff = abs(scenedepth-scenefocus);
#if RFX_LogDepth
	depthdiff = (scenedepth < scenefocus) ? pow(depthdiff, DOF_FARBLURCURVE) : depthdiff;
	depthdiff = (scenedepth > scenefocus) ? pow(depthdiff, DOF_NEARBLURCURVE)*(1.0f+pow(abs(0.5f-texcoord.x)*depthdiff+0.1f,2)*DOF_VIGNETTE) : depthdiff;
#else
	depthdiff = (scenedepth < scenefocus) ? pow(depthdiff, DOF_NEARBLURCURVE)*(1.0f+pow(abs(0.5f-texcoord.x)*depthdiff+0.1f,2)*DOF_VIGNETTE) : depthdiff;
	depthdiff = (scenedepth > scenefocus) ? pow(depthdiff, DOF_FARBLURCURVE) : depthdiff;
#endif	
	float mask = 1.0f;
#if(USE_HUD_MASKING == 1)
	mask = tex2D(SamplerMask, texcoord.xy).x;
#endif
	hdr2R = saturate(float4(depthdiff,scenedepth,scenefocus,0))*mask;
}
#endif

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
	float lum = dot(colDF.xyz,MFX_LumCoeff);
	float thresh = max((lum-fRingDOFThreshold)*fRingDOFGain, 0.0);
	float3 nullcol = float3(0,0,0);
	colDF.xyz +=max(0,lerp(nullcol.xyz,colDF.xyz,thresh*blur));
	return colDF;
}

void PS_DOF_RingDOF1(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr1R : SV_Target0)
{

	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;	
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*origCoC.x/iRingDOFRings;

	hdr1R = GetColorDOF(SamplerHDR2,texcoord.xy, discRadius.x);
}

void PS_DOF_RingDOF2(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0) 
{

	float CoC = tex2D(SamplerCoC, texcoord.xy).x;	
	
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*CoC/iRingDOFRings;
	
	float4 col = tex2D(SamplerHDR1, texcoord.xy);
	
	if(discRadius.x/PixelSize.x > 0.25) //some optimization thingy
	{

	float s = 1.0;
	int ringsamples;

	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;

	[loop]
	for (int g = 1; g <= iRingDOFRings; g += 1)
	{
		ringsamples = g * iRingDOFSamples;
		[loop]
		for (int j = 0 ; j < ringsamples ; j += 1)
		{
			float step = PI*2.0 / ringsamples;
			float2 sampleoffset = discRadius.xy * float2(cos(j*step)*g, sin(j*step)*g);
			float3 sampleCoC = tex2Dlod(SamplerCoC, float4(texcoord.xy + sampleoffset,0,0)).xyz;
			if(origCoC.y>origCoC.z && sampleCoC.x<CoC) sampleoffset = sampleoffset/CoC*sampleCoC.x;

			col.xyz += GetDOF(SamplerHDR1,texcoord.xy + sampleoffset,CoC).xyz*lerp(1.0,g/iRingDOFRings,fRingDOFBias);  
			s += 1.0*lerp(1.0,g/iRingDOFRings,fRingDOFBias);
		}
	}
	col = col/s; //divide by sample count
	}
	
	hdr2R = col;
}

//MAGIC DOF
void PS_DOF_MagicDOF1(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr1R : SV_Target0)	
{
	float4 res,tapres;	
	float totalweight=0;
	res = tex2D(SamplerHDR2, texcoord.xy);
	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;
	float2 discRadius = origCoC.x*PixelSize.xy*DOF_BLURRADIUS/iMagicDOFBlurQuality;
	
	int passnum = iMagicDOFBlurQuality;
	//Wilham Anggowo please keep your Fingers from this shader, I don't want to see it in ENB!
	res.xyz = 0;
		
	[loop]
	for (int i = -iMagicDOFBlurQuality; i <= iMagicDOFBlurQuality; ++i) 
	{
		float2 tapoffset = float2((float)i,0)*discRadius.xy;
		float3 sampleCoC = tex2Dlod(SamplerCoC, float4(texcoord.xy + tapoffset,0,0)).xyz;
		if(origCoC.y>origCoC.z && sampleCoC.x<origCoC.x) tapoffset = tapoffset/origCoC.x*sampleCoC.x;
		tapres = tex2Dlod(SamplerHDR2, float4(texcoord.xy+tapoffset,0,0));
		res.xyz += tapres.xyz;
		totalweight+=1;
	}

	res.xyz /= totalweight;

	hdr1R = float4(res.xyz,1);
}

void PS_DOF_MagicDOF2(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0)	
{

	float4 res,tapres1,tapres2;	
	float totalweight=0;
	res = tex2D(SamplerHDR1, texcoord.xy);
	float3 origcolor = res.xyz;
	
	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;

	float2 discRadius = origCoC.x*PixelSize.xy*DOF_BLURRADIUS/iMagicDOFBlurQuality;

	int passnum = iMagicDOFBlurQuality;

	int lodlevel = clamp(round(discRadius.x/6),0,3);

	res.xyz = 0;
		
	[loop]
	for (int i = -iMagicDOFBlurQuality; i <= iMagicDOFBlurQuality; ++i) 
	{
		float2 tapoffset1 = float2((float)i*discRadius.x*0.5,(float)i*discRadius.y*0.5*tan(60*PIOVER180));
		float2 tapoffset2 = float2(-tapoffset1.x,tapoffset1.y);

		float3 sampleCoC1 = tex2Dlod(SamplerCoC, float4(texcoord.xy + tapoffset1,0,0)).xyz;
		float3 sampleCoC2 = tex2Dlod(SamplerCoC, float4(texcoord.xy + tapoffset2,0,0)).xyz;

		if(origCoC.y>origCoC.z && sampleCoC1.x<origCoC.x) tapoffset1 = tapoffset1/origCoC.x*sampleCoC1.x; //never ask why I cam up with this. It works. Better than any masking I found but still not flawless.
		if(origCoC.y>origCoC.z && sampleCoC2.x<origCoC.x) tapoffset2 = tapoffset2/origCoC.x*sampleCoC2.x;

		tapres1 = tex2Dlod(SamplerHDR1, float4(texcoord.xy+tapoffset1,0,lodlevel));
		tapres2 = tex2Dlod(SamplerHDR1, float4(texcoord.xy+tapoffset2,0,lodlevel));

		totalweight += 1;
		res.xyz += pow(min(tapres1.xyz, tapres2.xyz),fMagicDOFColorCurve);
		}

	res.xyz /= totalweight;
	res.xyz = saturate(pow(saturate(res.xyz), 1/fMagicDOFColorCurve));
	
	hdr2R = res;
}

//GP65CJ042 DOF
void PS_DOF_GPDOF1(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr1R : SV_Target0)
{
	float4 res=tex2D(SamplerHDR2, texcoord.xy);
	float4 origcolor=0;
	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;
	float2 discRadius = DOF_BLURRADIUS*PixelSize.xy*max(0,origCoC.x-0.1);//optimization to clean focus areas a bit

	float3 distortion=float3(-1.0, 0.0, 1.0);
	distortion*=fGPDOFChromaAmount;

	origcolor=tex2D(SamplerHDR2, texcoord.xy + discRadius.xy*distortion.x);
	origcolor.w=smoothstep(0.0, origCoC.y, origcolor.w);
	res.x=lerp(res.x, origcolor.x, origcolor.w);
	
	origcolor=tex2D(SamplerHDR2, texcoord.xy + discRadius.xy*distortion.z);
	origcolor.w=smoothstep(0.0, origCoC.y, origcolor.w);
	res.z=lerp(res.z, origcolor.z, origcolor.w);

	hdr1R = res;
}

void PS_DOF_GPDOF2(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0)
{
	float4 res;
	float4 origcolor=tex2D(SamplerHDR1, texcoord.xy);
	float3 origCoC = tex2D(SamplerCoC, texcoord.xy).xyz;
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

		float3 sampleCoC = tex2Dlod(SamplerCoC, float4(texcoord.xy+sampleOffset.xy,0,0)).xyz;
		if(origCoC.y>origCoC.z && sampleCoC.x<origCoC.x) sampleOffset = sampleOffset/origCoC.x*sampleCoC.x;

		float4 tap=tex2Dlod(SamplerHDR1, float4(texcoord.xy+sampleOffset.xy,0,0));
		tap.w=dot(tap.xyz, 0.3333);
		float brightMultipiler=max((tap.w - fGPDOFBrightnessThreshold) * fGPDOFBrightnessMultiplier, 0.0);

		tap.xyz*=(1.0 + brightMultipiler*origCoC.x);
		//res.w+=1.0 + fGPDOFBias * pow(float(sampleCycleCounter)/float(iGPDOFQuality), fGPDOFBiasCurve);

		float curvemult = lerp(1.0,pow(float(sampleCycleCounter)/float(iGPDOFQuality), fGPDOFBiasCurve),fGPDOFBias);
		res.xyz += tap.xyz*curvemult;
		res.w += curvemult;

	}

	res.xyz /= res.w;

	hdr2R = res;
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
	[unroll]
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

void PS_DOF_MatsoDOF1(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr1R : SV_Target0)
{
	hdr1R = GetMatsoDOFBlur(2, texcoord.xy, SamplerHDR2);	
}

void PS_DOF_MatsoDOF2(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0)
{
	hdr2R = GetMatsoDOFBlur(3, texcoord.xy, SamplerHDR1);	
}

void PS_DOF_MatsoDOF3(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr1R : SV_Target0)
{
	hdr1R = GetMatsoDOFBlur(0, texcoord.xy, SamplerHDR2);	
}

void PS_DOF_MatsoDOF4(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0)
{
	hdr2R = GetMatsoDOFBlur(1, texcoord.xy, SamplerHDR1);	
}

technique DoF_Tech <bool enabled = RFX_Start_Enabled; int toggle = DoF_ToggleKey; >
{
	pass DOF_CoC
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_CoC;
		RenderTarget = texCoC;
	}

   #if(DOF_METHOD==1)	
	pass DOF_RingDOF1
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_RingDOF1;
		RenderTarget = texHDR1;
	}
	pass DOF_RingDOF2
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_RingDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==2)	
	pass DOF_MagicDOF1
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MagicDOF1;
		RenderTarget = texHDR1;
	}
	pass DOF_MagicDOF2
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MagicDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==3)	
	pass DOF_GPDOF1
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_GPDOF1;
		RenderTarget = texHDR1;
	}
	pass DOF_GPDOF2
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_GPDOF2;
		RenderTarget = texHDR2;
	}
   #endif
   #if(DOF_METHOD==4)	
	pass DOF_MatsoDOF1
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MatsoDOF1;
		RenderTarget = texHDR1;
	}
	pass DOF_MatsoDOF2
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MatsoDOF2;
		RenderTarget = texHDR2;
	}
	pass DOF_MatsoDOF3
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MatsoDOF3;
		RenderTarget = texHDR1;
	}
	pass DOF_MatsoDOF4
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_DOF_MatsoDOF4;
		RenderTarget = texHDR2;
	}
   #endif

	pass Overlay_DOF
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_Overlay;
	}
}

#endif

#include MFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()