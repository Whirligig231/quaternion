#if USE_RFX_DisplayDepth

NAMESPACE_ENTER(RFX) 

void DisplayDepth(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0, out float3 color : SV_Target)
{
	color.rgb = tex2D(RFX_depthTexColor,texcoord).rgb;
}

technique Depth_Tech < bool enabled = false; int toggle = RFX_DepthToggleKey;>
{
	pass
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = DisplayDepth;
	}
}


NAMESPACE_LEAVE()

#endif