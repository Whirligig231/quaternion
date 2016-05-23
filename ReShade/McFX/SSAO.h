NAMESPACE_ENTER(MFX)

#include MFX_SETTINGS_DEF

#if USE_AMBIENTOCCLUSION

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
//Credits :: PetkaGtA (Raymarch AO idea), Ethatron (SSAO ported from Crysis), Ethatron and tomerk (HBAO and SSGI)
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

#if( HDR_MODE == 0)
 #define RENDERMODE RGBA8
#elif( HDR_MODE == 1)
 #define RENDERMODE RGBA16F
#else
 #define RENDERMODE RGBA32F
#endif

//textures
texture   texOcclusion1 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16F;};
texture   texOcclusion2 { Width = BUFFER_WIDTH; Height = BUFFER_HEIGHT;  Format = RGBA16F;};

texture   texNoise  < string source = "ReShade/McFX/Textures/mcnoise.png";  > {Width = BUFFER_WIDTH;Height = BUFFER_HEIGHT;Format = RGBA8;};

//samplers
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

sampler2D SamplerNoise
{
	Texture = texNoise;
	MinFilter = LINEAR;
	MagFilter = LINEAR;
	MipFilter = NONE;
	AddressU = Clamp;
	AddressV = Clamp;
};

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Functions														     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
float3 GetNormalFromDepth(float fDepth, float2 vTexcoord) {
  
  	const float2 offset1 = float2(0.0,0.001);
  	const float2 offset2 = float2(0.001,0.0);
  
  	float depth1 = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(vTexcoord + offset1,0,0)).x);
  	float depth2 = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(vTexcoord + offset2,0,0)).x);
  
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

//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// Ambient Occlusion													     //
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
void PS_AO_SSAO(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0)
{
	//global variables
	float depth		= tex2D(RFX_depthColor, texcoord.xy).x;
	float fSceneDepthP 	= GetLinearDepth(depth);

	if(depth > 0.9999) Occlusion1R = float4(0.5,0.5,0.5,fSceneDepthP);
	else {
		float offsetScale = fSSAOSamplingRange/10000;
		float fSSAODepthClip = 10000000.0;

		float3 vRotation = tex2Dlod(SamplerNoise, float4(texcoord.xy, 0, 0)).rgb - 0.5f;
	
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
			float3 vSamplePos = float3(texcoord.xy, fSceneDepthP);
 
			//Offset sample point
			vSamplePos += float3(vRotatedOffset.xy, vRotatedOffset.z * fSceneDepthP);

			//Read sample point depth
			float fSceneDepthS = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(vSamplePos.xy,0,0)).x);

			//Discard if depth equals max
			if (fSceneDepthS >= fSSAODepthClip)
			fAccessibility += 1.0f;
			else {
				//Compute accessibility factor
				float fDepthDist = fSceneDepthP - fSceneDepthS;
				float fRangeIsInvalid = saturate(fDepthDist);
				fAccessibility += lerp(fSceneDepthS > vSamplePos.z, 0.5f, fRangeIsInvalid);
			}
		}
 
		//Compute average accessibility
		fAccessibility = fAccessibility / Sample_Scaled;
	
		Occlusion1R = float4(fAccessibility.xxx,fSceneDepthP);
	}
}

void PS_AO_RayAO(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0)	
{

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
	float depth = tex2D(RFX_depthColor, texcoord.xy).x;
	fCurrDepth  = GetLinearDepth(depth);

	if(fCurrDepth>0.9999) Occlusion1R = float4(1.0,1.0,1.0,fCurrDepth);
	else {
		vViewNormal = GetNormalFromDepth(fCurrDepth, texcoord.xy);
		vRandom 	= GetRandomVector(texcoord);
		fAO = 0;
		for(int s = 0; s < iRayAOSamples; s++) {
			vReflRay = reflect(avOffsets[s], vRandom);
		
			float fFlip = sign(dot(vViewNormal,vReflRay));
        		vReflRay   *= fFlip;
		
			float sD = fCurrDepth - (vReflRay.z * fRayAOSamplingRange);
			fSampleDepth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(saturate(texcoord.xy + (fRayAOSamplingRange * vReflRay.xy / fCurrDepth)),0,0)).x);
			fDepthDelta = saturate(sD - fSampleDepth);

			fDepthDelta *= 1-smoothstep(0,fRayAOMaxDepth,fDepthDelta);

			if ( fDepthDelta > fRayAOMinDepth && fDepthDelta < fRayAOMaxDepth)
				fAO += pow(1 - fDepthDelta, 2.5);
		}
		vOutSum.x = saturate(1 - (fAO / (float)iRayAOSamples) + fRayAOSamplingRange);
		Occlusion1R = float4(vOutSum.xxx,fCurrDepth);
	}
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

void PS_AO_HBAO(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0)		
{
	float depth = GetLinearDepth(tex2D(RFX_depthColor, texcoord.xy).x);

	if(depth > 0.9999) Occlusion1R = float4(1.0,1.0,1.0,depth);
	else {
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

		float3 pos = GetEyePosition(texcoord.xy, depth);
		float3 dx = ddx(pos);
		float3 dy = ddy(pos);
		float3 norm = normalize(cross(dx,dy));
 
		float sample_depth=0;
		float3 sample_pos=0;
 
		float ao=0;
		float s=0.0;
 
		float2 rand_vec = GetRandom2_10(texcoord.xy);
		float2 sample_vec_divisor = InvFocalLen*depth/(fHBAOSamplingRange*float2(BUFFER_RCP_WIDTH, BUFFER_RCP_HEIGHT));
		float2 sample_center = texcoord.xy;
 		
		for (int i = 0; i < 8; i++)
		{
			float theta,temp_theta,temp_ao,curr_ao = 0;
			float3 occlusion_vector = 0.0;
 
			float2 sample_vec = reflect(sample_offset[i], rand_vec);
			sample_vec /= sample_vec_divisor;
			float2 sample_coords = (sample_vec*float2(1,(float)BUFFER_WIDTH/(float)BUFFER_HEIGHT))/iHBAOSamples;
 			
			for (int k = 1; k <= iHBAOSamples; k++)
			{
				sample_depth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(sample_center + sample_coords*(k-0.5*(i%2f)),0,0)).x);
				sample_pos = GetEyePosition(sample_center + sample_coords*(k-0.5*(i%2f)), sample_depth);
				occlusion_vector = sample_pos - pos;
				temp_theta = dot( norm, normalize(occlusion_vector) );			
 				[branch]
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

		Occlusion1R = float4(ao.xxx, depth);
	}
}

void PS_AO_AOBlurV(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion2R : SV_Target0)
{

	float gaussweight[7] = { 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 };
	float  sum,totalweight=0;
	float4 base = tex2D(SamplerOcclusion1, texcoord.xy), temp=0;
	
	[loop]
	for (int r = -6; r <= 6; ++r) 
	{
		float2 axis = float2(0, 1);
		temp = tex2D(SamplerOcclusion1, texcoord.xy + axis * PixelSize * r);
		float weight = 0.3 + gaussweight[(int)abs(r)];
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(temp.w - base.w));
		sum += temp.x * weight;
		totalweight += weight;
	}

	Occlusion2R = float4(sum / (totalweight+0.0001),0,0,base.w);
}

void PS_AO_AOBlurH(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0)
{

	float gaussweight[7] = { 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 };
	float  sum,totalweight=0;
	float4 base = tex2D(SamplerOcclusion2, texcoord.xy), temp=0;
	
	[loop]
	for (int r = -6; r <= 6; ++r) 
	{
		float2 axis = float2(1, 0);
		temp = tex2D(SamplerOcclusion2, texcoord.xy + axis * PixelSize * r);
		float weight = 0.3 + gaussweight[(int)abs(r)];
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(temp.w - base.w));
		sum += temp.x * weight;
		totalweight += weight;
	}

	Occlusion1R = float4(sum / (totalweight+0.0001),0,0,base.w);
}

void PS_AO_AOCombine(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0) 
{

	float4 color = tex2D(SamplerHDR1, texcoord.xy);
	float ao = tex2D(SamplerOcclusion1, texcoord.xy).x;

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
	hdr2R = ao;
#else

 #if(AO_LUMINANCE_CONSIDERATION == 1)
	float origlum = dot(color.xyz, 0.333);
	float aomult = smoothstep(AO_LUMINANCE_LOWER, AO_LUMINANCE_UPPER, origlum);
	ao = lerp(ao, 1.0, aomult);
 #endif	

	color.xyz *= ao;
	hdr2R = color;
#endif
}

void PS_AO_SSGI(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0)	
{

	float depth = tex2D(RFX_depthColor, texcoord.xy).x;
	depth  = GetLinearDepth(depth);

	if(depth > 0.9999) Occlusion1R = float4(0.0,0.0,0.0,1.0);
	else {
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

		float3 pos = GetEyePosition(texcoord.xy, depth);
		float3 dx = ddx(pos);
		float3 dy = ddy(pos);
		float3 norm = normalize(cross(dx, dy));
		norm.y *= -1;

		float sample_depth;

		float4 gi = float4(0, 0, 0, 0);
		float is = 0, as = 0;

		float rangeZ = 5000;

		float2 rand_vec = GetRandom2_10(texcoord.xy);
		float2 rand_vec2 = GetRandom2_10(-texcoord.xy);
		float2 sample_vec_divisor = InvFocalLen * depth / (fSSGISamplingRange * PixelSize.xy);
		float2 sample_center = texcoord.xy + norm.xy / sample_vec_divisor * float2(1, aspect);
		float ii_sample_center_depth = depth * rangeZ + norm.z * fSSGISamplingRange * 20;
		float ao_sample_center_depth = depth * rangeZ + norm.z * fSSGISamplingRange * 5;

		[fastopt]
		for (int i = 0; i < iSSGISamples; i++) {
			float2 sample_vec = reflect(sample_offset[i], rand_vec) / sample_vec_divisor;
			float2 sample_coords = sample_center + sample_vec *  float2(1, aspect);
			float  sample_depth = rangeZ * GetLinearDepth(tex2Dlod(RFX_depthColor,float4(sample_coords.xy,0,0)).x);
 
			float ii_curr_sample_radius = sample_radius[i] * fSSGISamplingRange * 20;
			float ao_curr_sample_radius = sample_radius[i] * fSSGISamplingRange * 5;
 
			gi.a += clamp(0, ao_sample_center_depth + ao_curr_sample_radius - sample_depth, 2 * ao_curr_sample_radius);
			gi.a -= clamp(0, ao_sample_center_depth + ao_curr_sample_radius - sample_depth - fSSGIModelThickness, 2 * ao_curr_sample_radius);
 
			if ((sample_depth < ii_sample_center_depth + ii_curr_sample_radius) &&
		    	(sample_depth > ii_sample_center_depth - ii_curr_sample_radius)) {
				float3 sample_pos = GetEyePosition(sample_coords, sample_depth);
				float3 unit_vector = normalize(pos - sample_pos);
 				gi.rgb += tex2Dlod(RFX_originalColor, float4(sample_coords,0,0)).rgb;
			}
 
			is += 1.0f;
			as += 2.0f * ao_curr_sample_radius;
		}
 
		gi.rgb /= is * 5.0f;
		gi.a   /= as;
 
		gi.rgb = 0.0 + gi.rgb * fSSGIIlluminationMult;
		gi.a   = 1.0 - gi.a   * fSSGIOcclusionMult;

		gi.rgb = lerp(dot(gi.rgb, 0.333), gi.rgb, fSSGISaturation);

		Occlusion1R = gi;
	}
}

void PS_AO_GIBlurV(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion2R : SV_Target0) 
{
	float gaussweight[7] = { 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 };
	float4 sum=0;
	float totalweight=0;
	float4 base = tex2D(SamplerOcclusion1, texcoord.xy);
	float4 temp=0;

	float centerdepth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(texcoord.xy,0,0)).x);
	
	[loop]
	for (int r = -6; r <= 6; ++r) 
	{
		float2 axis = float2(0, 1);
		temp = tex2D(SamplerOcclusion1, texcoord.xy + axis * PixelSize * r);
		float tempdepth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(texcoord.xy + axis * PixelSize * r,0,0)).x);
		float weight = 0.3 + gaussweight[(int)abs(r)];
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(tempdepth - centerdepth));
		sum += temp * weight;
		totalweight += weight;
	}

	Occlusion2R = sum / (totalweight+0.0001);
}

void PS_AO_GIBlurH(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 Occlusion1R : SV_Target0) 
{

	float gaussweight[7] = { 0.111220, 0.107798, 0.098151, 0.083953, 0.067458, 0.050920, 0.036108 };
	float4 sum=0;
	float totalweight=0;
	float4 base = tex2D(SamplerOcclusion2, texcoord.xy);
	float4 temp=0;

	float centerdepth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(texcoord.xy,0,0)).x);
	
	[loop]
	for (int r = -6; r <= 6; ++r) 
	{
		float2 axis = float2(1, 0);
		temp = tex2D(SamplerOcclusion2, texcoord.xy + axis * PixelSize * r);
		float tempdepth = GetLinearDepth(tex2Dlod(RFX_depthColor, float4(texcoord.xy + axis * PixelSize * r,0,0)).x);
		float weight = 0.3 + gaussweight[(int)abs(r)];
		weight *= max(0.0, 1.0 - (1000.0 * AO_SHARPNESS) * abs(tempdepth - centerdepth));
		sum += temp * weight;
		totalweight += weight;
	}

	Occlusion1R = sum / (totalweight+0.0001);
}

void PS_AO_GICombine(float4 vpos : SV_Position, float2 texcoord : TEXCOORD, out float4 hdr2R : SV_Target0)
{

	float4 color = tex2D(SamplerHDR1, texcoord.xy);
	float4 gi = tex2D(SamplerOcclusion1, texcoord.xy);

	//gi.xyz *= dot(color.xyz,0.333);
	color.xyz = (color.xyz+gi.xyz)*gi.w;
	hdr2R = color;

#if( AO_DEBUG == 1)
	hdr2R = gi.wwww;
#endif
#if( AO_DEBUG == 2)
	hdr2R = gi.xyzz;
#endif	
}

technique AO_Tech <bool enabled = RFX_Start_Enabled; int toggle = AO_ToggleKey; >
{
  #if(AO_METHOD==1)
	pass AO_SSAO
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_SSAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==2)
	pass AO_RayAO
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_RayAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD==3)
	pass AO_HBAO
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_HBAO;
		RenderTarget = texOcclusion1;
	}
  #endif
  #if(AO_METHOD != 4)
	pass AO_AOBlurV
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_AOBlurV;
		RenderTarget = texOcclusion2;
	}
	
	pass AO_AOBlurH
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_AOBlurH;
		RenderTarget = texOcclusion1;
	}

	pass AO_AOCombine
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_AOCombine;
		RenderTarget = texHDR2; 
	}	
  #endif
  #if(AO_METHOD == 4)
	pass AO_SSGI
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_SSGI;
		RenderTarget = texOcclusion1;
	}

	pass AO_GIBlurV
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_GIBlurV;
		RenderTarget = texOcclusion2;
	}

	pass AO_GIBlurH
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_GIBlurH;
		RenderTarget = texOcclusion1;
	}

	pass AO_GICombine
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_AO_GICombine;
		RenderTarget = texHDR2; 
	}
  #endif

	pass Overlay_SSAO
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_Overlay;
	}
}

#endif

#include MFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()