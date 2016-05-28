// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <TorqueLib/TGE.h>
#include <TorqueLib/QuickOverride.h>
#include <PluginLoader/PluginInterface.h>


// TODO: reference additional headers your program requires here

const char* getObjectField(const TGE::SimObject* obj, const char* name);
bool toBool(const char* str);