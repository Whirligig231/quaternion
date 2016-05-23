#if USE_RFX_ShowToggleMessage
NAMESPACE_ENTER(RFX)

void PS_ToggleMessage(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0, out float4 color : SV_Target)
{
	color = tex2D(RFX_backbufferColor,texcoord);
	//Message Decoy
}

technique Framework <bool enabled = RFX_Start_Enabled; int toggle = RFX_ToggleKey;>
{
	pass 
	{
		VertexShader = RFX_VS_PostProcess;
		PixelShader = PS_ToggleMessage;
	}
}

NAMESPACE_LEAVE()
#endif