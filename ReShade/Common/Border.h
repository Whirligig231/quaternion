#if USE_RFX_Border

//Border Shader
NAMESPACE_ENTER(RFX)

texture bMaskTex < source = "ReShade/Common/Textures/RFX_bMask.png"; > { Width = 1920; Height = 1080; MipLevels = 1; Format = RGBA8; };
sampler bMaskColor { Texture = bMaskTex; };

float4 PS_Border(float4 vpos : SV_Position, float2 texcoord : TEXCOORD) : SV_Target
{
	return lerp(0.0.xxxx, tex2D(RFX_backbufferColor, texcoord), tex2D(bMaskColor, texcoord).r); 
}

technique Border_Tech <bool enabled = RFX_Start_Enabled; int toggle = RFX_Border_ToggleKey; >
{
	pass 
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_Border;
	}
}

NAMESPACE_LEAVE()

#endif