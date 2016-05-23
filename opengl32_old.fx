  /*----------------------------.
  | ::   ReShade Framework   :: |
  '----------------------------*/

// Keycode aliases
#include "ReShade\Common\KeyCodes.h"
// Global configuration
#include "ReShade\Common_settings.cfg"

// Shared resources and useful helpers
#include "ReShade\Common\Util.h"

#include EFFECT(CustomFX, Util)
#include EFFECT(GemFX, Util)
#include EFFECT(McFX, Util)
#include EFFECT(SweetFX, Util)

  /*----------------------------.
  | ::    Effect Ordering    :: |
  '----------------------------*/

#include EFFECT(McFX, SSAO)
#include EFFECT(McFX, DOF)
#include EFFECT(CustomFX, TiltShift)
#include EFFECT(GemFX, MotionBlur)
#include EFFECT(GemFX, AdvMotionBlur)
#include EFFECT(GemFX, MotionFocus)
#include EFFECT(GemFX, Bloom)
#include EFFECT(SweetFX, Shared)
#include EFFECT(GemFX, AmbientLight)
#include EFFECT(McFX, HeatHaze)
// #include EFFECT(GemFX, AdvAmbientLight) //WIP
#include EFFECT(CustomFX, TuningPalette)
#include EFFECT(SweetFX, Ascii)
#include EFFECT(SweetFX, Cartoon)
#include EFFECT(SweetFX, LumaSharpen)
#include EFFECT(SweetFX, SMAAWrap)
#include EFFECT(SweetFX, Explosion)
#include EFFECT(SweetFX, FXAAWrap)
#include EFFECT(SweetFX, Bloom)
#include EFFECT(SweetFX, HDR)
#include EFFECT(SweetFX, CA)
#include EFFECT(SweetFX, AdvancedCRT)
#include EFFECT(SweetFX, PixelartCRT)
#include EFFECT(SweetFX, LensDistortion)
#include EFFECT(CustomFX, ColorCorrection)
#include EFFECT(CustomFX, Cel)
#include EFFECT(CustomFX, Gaussian)
#include EFFECT(CustomFX, Custom)
#include EFFECT(McFX, FishEyeCA)
#include EFFECT(CustomFX, Gr8mmFilm)
#include EFFECT(Common, UIMask)
#include EFFECT(Common, Border)
#include EFFECT(Common, SplitScreen)
#include EFFECT(Common, DisplayDepth)
#include EFFECT(SweetFX, Transition)
#include EFFECT(Common, ToggleMessage)
