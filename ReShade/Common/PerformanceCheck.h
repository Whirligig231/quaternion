	#if USE_RFX_DisplayDepth
		#define RFX_DepthBufferCalcS 1
	#endif

	#if USE_RFX_SplitScreen || USE_RFX_UIMask
		#define RFX_InitialStorageS 1
	#endif

	#define CFX_SETTINGS_DEF "ReShade/CustomFX_settings.cfg"
	#define CFX_SETTINGS_UNDEF "ReShade/CustomFX_settings.undef" 
	#include CFX_SETTINGS_DEF
	
	#if USE_TUNINGPALETTE
		#undef RFX_InitialStorageS
		#define RFX_InitialStorageS 1
	#endif

	#include CFX_SETTINGS_UNDEF
	#undef CFX_SETTINGS_UNDEF
	#undef CFX_SETTINGS_DEF

	#define MFX_SETTINGS_DEF "ReShade/McFX_settings.cfg"
	#define MFX_SETTINGS_UNDEF "ReShade/McFX_settings.undef" 
	#include MFX_SETTINGS_DEF

	#if USE_DEPTHOFFIELD || USE_AMBIENTOCCLUSION
		#undef RFX_DepthBufferCalcS
		#define RFX_DepthBufferCalcS 1
	#endif
	
	#if USE_HEATHAZE
		#define RFX_DepthBufferCalcSHeat 1
	#endif

	#include MFX_SETTINGS_UNDEF
	#undef MFX_SETTINGS_UNDEF
	#undef MFX_SETTINGS_DEF

	#define GFX_SETTINGS_DEF "ReShade/GemFX_settings.cfg"
	#define GFX_SETTINGS_UNDEF "ReShade/GemFX_settings.undef" 
	#include GFX_SETTINGS_DEF

	#if USE_ADV_MOTION_BLUR || (AL_HeatHazeControle && Depth_HeatHazeControle && USE_AMBIENT_LIGHT && RFX_DepthBufferCalcSHeat)
		#undef RFX_DepthBufferCalcS
		#define RFX_DepthBufferCalcS 1
	#endif

	#if USE_AMBIENT_LIGHT || USE_BLOOM || USE_LENSDIRT || USE_GAUSSIAN_ANAMFLARE || USE_LENZFLARE || USE_CHAPMAN_LENS || USE_GODRAYS || USE_ANAMFLARE
		#undef RFX_InitialStorageS
		#define RFX_InitialStorageS 1
	#endif

	#include GFX_SETTINGS_UNDEF
	#undef GFX_SETTINGS_UNDEF
	#undef GFX_SETTINGS_DEF

	#define SFX_SETTINGS_DEF "ReShade/SweetFX_settings.cfg"
	#define SFX_SETTINGS_UNDEF "ReShade/SweetFX_settings.undef" 
	#include SFX_SETTINGS_DEF

	#if USE_SPLITSCREEN
		#define RFX_InitialStorageS 1
		#undef RFX_InitialStorageS
	#endif

	#include SFX_SETTINGS_UNDEF
	#undef SFX_SETTINGS_UNDEF
	#undef SFX_SETTINGS_DEF