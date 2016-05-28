// QuaternionPlugins.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "audio.h"
#include "physics.h"
#include <string>
#include <algorithm>
#include <fstream>
#include <sstream>

PLUGINCALLBACK void preEngineInit(PluginInterface *plugin)
{
	TGE::Con::setBoolVariable("$Core::DisablePhysics", false);
}

void onClientProcess(U32 timeDelta) {
	updateFmod(timeDelta);
	updateBullet(timeDelta);
}

PLUGINCALLBACK void postEngineInit(PluginInterface *plugin)
{
	initFmod();
	initBullet();

	plugin->onClientProcess(onClientProcess);
}

PLUGINCALLBACK void engineShutdown(PluginInterface *plugin)
{
}

TorqueOverrideMember(bool, Marble::onAdd, (TGE::Marble* thisPtr), originalOnAdd)
{
	bool returnValue = originalOnAdd(thisPtr);
	if (returnValue)
		thisPtr->scriptOnAdd();
	return returnValue;
}

ConsoleFunction(simulateKey, void, 2, 2, "simulateKey( keycode )") {
	// This structure will be used to create the keyboard
	// input event.
	INPUT ip;

	// Set up a generic keyboard event.
	ip.type = INPUT_KEYBOARD;
	ip.ki.wScan = 0; // hardware scan code for key
	ip.ki.time = 0;
	ip.ki.dwExtraInfo = 0;

	// Press the "A" key
	ip.ki.wVk = atoi(argv[1]); // virtual-key code for the "a" key
	ip.ki.dwFlags = 0; // 0 for key press
	SendInput(1, &ip, sizeof(INPUT));

	// Release the "A" key
	ip.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
	SendInput(1, &ip, sizeof(INPUT));
}

std::size_t strReplace(std::string &text, std::string from, std::string to, std::size_t pos = 0) {
	std::size_t start_pos = text.find(from, pos);
	if (start_pos == std::string::npos)
		return std::string::npos;
	text.replace(start_pos, from.length(), to);
	return start_pos;
}

void strReplaceAll(std::string &text, std::string from, std::string to) {
	std::size_t pos = 0;
	while (pos != std::string::npos) {
		pos = strReplace(text, from, to, pos);
		if (pos != std::string::npos)
			pos += to.length();
	}
}

void escape(std::string &text) {
	strReplaceAll(text, "\\", "\\\\");
	strReplaceAll(text, "\n", "\\n");
	strReplaceAll(text, "\t", "\\t");
	strReplaceAll(text, "\"", "\\\"");
}

ConsoleFunction(escape, const char*, 2, 2, "escape( str )") {
	std::ostringstream scriptss;
	scriptss << argv[1];
	std::string script(scriptss.str());
	escape(script);
	char *ret = TGE::Con::getReturnBuffer(script.size() + 1);
	strcpy_s(ret, script.size() + 1, script.c_str());
	return ret;
}