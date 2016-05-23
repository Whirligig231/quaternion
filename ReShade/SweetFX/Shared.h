  /*--------------------.
  | :: Shared passes :: |
  '--------------------*/

NAMESPACE_ENTER(SFX)

#include SFX_SETTINGS_DEF

float4 SharedPass(float2 tex, float4 FinalColor)
{
    // Palette
#if (USE_NOSTALGIA == 1)
    FinalColor = Nostalgia(FinalColor);
#endif

	// Levels
#if (USE_LEVELS == 1)
	FinalColor = LevelsPass(FinalColor);
#endif

	// Technicolor
#if (USE_TECHNICOLOR == 1)
	FinalColor = TechnicolorPass(FinalColor);
#endif

  	// Technicolor2
#if (USE_TECHNICOLOR2 == 1)
    	FinalColor = Technicolor2(FinalColor);
#endif

	// DPX
#if (USE_DPX == 1)
	FinalColor = DPXPass(FinalColor);
#endif

	// Monochrome
#if (USE_MONOCHROME == 1)
	FinalColor = MonochromePass(FinalColor);
#endif

	// ColorMatrix
#if (USE_COLORMATRIX == 1)
	FinalColor = ColorMatrixPass(FinalColor);
#endif

	// Lift Gamma Gain
#if (USE_LIFTGAMMAGAIN == 1)
	FinalColor = LiftGammaGainPass(FinalColor);
#endif

	// Tonemap
#if (USE_TONEMAP == 1)
	FinalColor = TonemapPass(FinalColor);
#endif

	// Vibrance
#if (USE_VIBRANCE == 1)
	FinalColor = VibrancePass(FinalColor);
#endif

	// Curves
#if (USE_CURVES == 1)
	FinalColor = CurvesPass(FinalColor);
#endif

	// Sepia
#if (USE_SEPIA == 1)
	FinalColor = SepiaPass(FinalColor);
#endif

	//FilmicPass
#if (USE_FILMICPASS == 1)
	FinalColor = FilmPass(FinalColor);
#endif

	//ReinhardLinear
#if (USE_REINHARDLINEAR == 1)
	FinalColor = ReinhardLinearToneMapping(FinalColor);
#endif

	// Vignette
#if (USE_VIGNETTE == 1)
	FinalColor = VignettePass(FinalColor,tex);
#endif

	// FilmGrain
#if (USE_FILMGRAIN == 1)
	FinalColor = FilmGrainPass(FinalColor,tex);
#endif

	// Dither (should go near the end as it only dithers what went before it)
#if (USE_DITHER == 1)
	FinalColor = DitherPass(FinalColor,tex);
#endif

	// Border
#if (USE_BORDER == 1)
	FinalColor = BorderPass(FinalColor,tex);
#endif

	// Splitscreen
#if (USE_SPLITSCREEN == 1)
	FinalColor = SplitscreenPass(FinalColor,tex);
#endif

	return FinalColor;
}

#if (SFX_SHARED == 1)
	#if (SFX_PIGGY == 0)
		float4 SharedWrap(in float4 position : SV_Position, in float2 texcoord : TEXCOORD0) : SV_Target
		{
			float3 color = tex2D(RFX_backbufferColor, texcoord).rgb;

			return SharedPass(texcoord, color.rgbb);
		}

		technique SharedShader_Tech <bool enabled = RFX_Start_Enabled; int toggle = SharedShader_ToggleKey; >
		{
			pass // the effects that don't require a seperate pass are all done in this one.
			{
				VertexShader = RFX_VS_PostProcess;
				PixelShader = SharedWrap;
			}
		}
	#else
		#undef SFX_PIGGY
		#define SFX_PIGGY 1
	#endif
#endif

#include SFX_SETTINGS_UNDEF

NAMESPACE_LEAVE()