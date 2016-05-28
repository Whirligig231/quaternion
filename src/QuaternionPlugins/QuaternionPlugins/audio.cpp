#include "stdafx.h"
#include "MathExtension.h"
#include <fmod.hpp>
#include <fmod_errors.h>
#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <iterator>

/**
* Get the camera transformation for an object.
* @arg obj The object whose camera transformation to get.
* @return The object's camera transform.
*/
ConsoleMethod(ShapeBase, getCameraTransform, const char *, 2, 2, "obj.getCameraTransform()")
{
	TGE::ShapeBase *obj = static_cast<TGE::ShapeBase*>(TGE::Sim::findObject(argv[1]));
	if (!obj) {
		TGE::Con::errorf("getCameraTransform() :: Obj is null!");
		return "0 0 0 1 0 0 0";
	}

	//Actual getting of transform here
	F32 pos;
	MatrixF mat;
	obj->getCameraTransform(&pos, &mat);

	return StringMath::print(mat);
}

/**
* Sound information structure, used to store loaded sounds.
* @field sound The FMOD sound instance created when we loaded this sound.
* @field channelGroup The FMOD channel group (these correspond to MB's channel groups as well).
* @field volume The volume of this sound, 0-1.
* @field frequency The frequency ratio of this sound to a normal sound.
*/
struct SoundInfo {
	FMOD::Sound* sound;
	FMOD::ChannelGroup* channelGroup;
	double volume;
	double frequency;
};

FMOD::System *fmodSystem = NULL; // The FMOD system instance.
std::vector<FMOD::Channel*> channelsPlaying; // A vector of all of the channels (sound instances) loaded.
std::map<int, SoundInfo> soundsLoaded; // A map from AudioProfile ID's to their SoundInfo structures.
std::map<U32, FMOD::ChannelGroup*> channelGroups; // A map from Torque channel group ID's to the FMOD groups.
std::map<std::string, FMOD_REVERB_PROPERTIES> reverbPresets; // A map of presets for the reverb effects.
std::vector<FMOD::Reverb3D*> reverbs; // A vector of all local reverbs currently defined.

/**
* Convert a string to boolean with "true" accepting.
* @arg str The string to convert to bool.
* @return A boolean representing the string converted.
*/
bool toBool(const char* str) {
	std::string strpp = std::string(str);
	std::transform(strpp.begin(), strpp.end(), strpp.begin(), ::tolower);
	return atoi(str) || (strpp == "true");
}

/**
* Convert a three-word string to an FMOD vector.
* @arg str A vector string, like "0 0 0" or similar.
* @return An FMOD_VECTOR of the string.
*/
FMOD_VECTOR toVector(const char* str) {
	std::string strpp = std::string(str);
	std::vector<std::string> words{std::istream_iterator<std::string>{std::istringstream(strpp)}, std::istream_iterator<std::string>{}};
	if (words.size() < 3)
		return { 0.0f, 0.0f, 0.0f };
	return{ (float)atof(words[0].c_str()), (float)atof(words[2].c_str()), (float)atof(words[1].c_str())};
}

/**
* Get a field from a SimObject.
* @arg obj The SimObject to get a field from.
* @arg name The name of the field to get.
* @return A string of the field's value.
*/
const char* getObjectField(const TGE::SimObject* obj, const char* name) {
	const char *fieldName = TGE::StringTable->insert(name, false);
	return (const_cast<TGE::SimObject*>(obj))->getDataField(fieldName, NULL);
}

/**
* Get a channel group of a given index, creating it if it doesn't exist.
* @arg index The index of the channel group (e.g. 1 = SFX, 2 = music in MB).
* @return The associated FMOD::ChannelGroup.
*/
FMOD::ChannelGroup* getGroup(U32 index) {
	if (channelGroups.find(index) != channelGroups.end())
		return channelGroups[index];
	else {
		FMOD::ChannelGroup* cg;
		fmodSystem->createChannelGroup(NULL, &cg);
		channelGroups[index] = cg;
		return cg;
	}
}

/**
* Get sound info for an AudioProfile, creating it if it doesn't exist.
* @arg sndObj The audio profile object.
* @return The SoundInfo object associated with that AudioProfile.
*/
SoundInfo getSoundInfo(TGE::SimObject* sndObj) {
	FMOD::Sound *sound1 = NULL; // The sound object in FMOD.
	FMOD_RESULT result; // The result (for error checking).

	if (soundsLoaded.find(sndObj->getId()) != soundsLoaded.end()) {
		return soundsLoaded[sndObj->getId()];
	}
	else {
		// Create the sound from the AudioDescription
		TGE::SimObject *descObj = TGE::Sim::findObject(getObjectField(sndObj, "description"));
		bool is3D = false, streaming = false;
		if (descObj) {
			// Figure out whether it's 3D
			const char* is3DStr = getObjectField(descObj, "is3D");
			is3D = toBool(is3DStr);

			const char* streamingStr = getObjectField(descObj, "isStreaming");
			streaming = toBool(streamingStr);
		}
		const char* fname = getObjectField(sndObj, "filename");
		// Actually create the sound
		if (streaming)
			result = fmodSystem->createStream(fname, is3D ? FMOD_3D : FMOD_DEFAULT, 0, &sound1);
		else
			result = fmodSystem->createSound(fname, is3D ? FMOD_3D : FMOD_DEFAULT, 0, &sound1);
		if (result != FMOD_OK)
		{
			TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
			SoundInfo returnInfo;
			returnInfo.sound = NULL;
			return returnInfo;
		}
		SoundInfo info;
		info.sound = sound1;
		bool looping = false;
		FMOD::ChannelGroup* group = NULL;
		if (descObj) {
			// Set whether it loops, as well as the channel index and default volume
			const char* loopStr = getObjectField(descObj, "isLooping");
			looping = toBool(loopStr);
			U32 channelInd = atoi(getObjectField(descObj, "type"));
			info.channelGroup = getGroup(channelInd);
			info.volume = atof(getObjectField(descObj, "volume"));
		}
		info.frequency = 1.0;
		soundsLoaded[sndObj->getId()] = info;
		sound1->setMode(looping ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		return info;
	}
}

/**
* Get sound info from the name of the object.
* @arg sname The name of the object.
* @return The SoundInfo for that AudioProfile.
*/
SoundInfo getSoundInfo(const char* sname) {
	TGE::SimObject *sndObj = TGE::Sim::findObject(sname);
	if (!sndObj) {
		TGE::Con::errorf("Unknown sound object!");
		SoundInfo returnInfo;
		returnInfo.sound = NULL;
		return returnInfo;
	}

	return getSoundInfo(sndObj);
}

/**
* Update the FMOD library on a time loop.
* @arg timeDelta The amount of time passed (not used).
*/
void updateFmod(U32 timeDelta) {
	FMOD_RESULT result;
	result = fmodSystem->update();
	if (result != FMOD_OK)
	{
		TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		return;
	}
}

/**
* Initialize the FMOD client.
*/
void initFmod() {
	FMOD_RESULT result;
	result = FMOD::System_Create(&fmodSystem);      // Create the main system object.
	if (result != FMOD_OK)
	{
		TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		return;
	}

	result = fmodSystem->init(512, FMOD_INIT_NORMAL, 0);    // Initialize FMOD.
	if (result != FMOD_OK)
	{
		TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		return;
	}

	fmodSystem->set3DSettings(1.f, 40.f, 1.f);

	reverbPresets = std::map<std::string, FMOD_REVERB_PROPERTIES>();
	reverbPresets[""] = FMOD_PRESET_OFF;
	reverbPresets["none"] = FMOD_PRESET_OFF;
	reverbPresets["off"] = FMOD_PRESET_OFF;
	reverbPresets["alley"] = FMOD_PRESET_ALLEY;
	reverbPresets["arena"] = FMOD_PRESET_ARENA;
	reverbPresets["auditorium"] = FMOD_PRESET_AUDITORIUM;
	reverbPresets["bathroom"] = FMOD_PRESET_BATHROOM;
	reverbPresets["carpettedhallway"] = FMOD_PRESET_CARPETTEDHALLWAY;
	reverbPresets["cave"] = FMOD_PRESET_CAVE;
	reverbPresets["city"] = FMOD_PRESET_CITY;
	reverbPresets["concerthall"] = FMOD_PRESET_CONCERTHALL;
	reverbPresets["forest"] = FMOD_PRESET_FOREST;
	reverbPresets["generic"] = FMOD_PRESET_GENERIC;
	reverbPresets["hallway"] = FMOD_PRESET_HALLWAY;
	reverbPresets["hangar"] = FMOD_PRESET_HANGAR;
	reverbPresets["livingroom"] = FMOD_PRESET_LIVINGROOM;
	reverbPresets["mountains"] = FMOD_PRESET_MOUNTAINS;
	reverbPresets["paddedcell"] = FMOD_PRESET_PADDEDCELL;
	reverbPresets["parkinglot"] = FMOD_PRESET_PARKINGLOT;
	reverbPresets["plain"] = FMOD_PRESET_PLAIN;
	reverbPresets["quarry"] = FMOD_PRESET_QUARRY;
	reverbPresets["room"] = FMOD_PRESET_ROOM;
	reverbPresets["sewerpipe"] = FMOD_PRESET_SEWERPIPE;
	reverbPresets["stonecorridor"] = FMOD_PRESET_STONECORRIDOR;
	reverbPresets["stoneroom"] = FMOD_PRESET_STONEROOM;
	reverbPresets["underwater"] = FMOD_PRESET_UNDERWATER;
}

/**
* Play a sound effect with FMOD.
* @arg sfx The AudioProfile to play.
* @return A handle to use to set the sound's properties while it plays.
*/
int fmodPlay(TGE::AudioProfile *sfx) {
	SoundInfo si = getSoundInfo(sfx);
	FMOD::Sound *sound1 = si.sound;
	if (sound1 == NULL)
		return -1;
	FMOD_RESULT result;
	FMOD::Channel *channel;

	result = fmodSystem->playSound(sound1, 0, true, &channel);
	if (result != FMOD_OK)
	{
		TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		return -1;
	}
	channel->setVolume((float)si.volume);
	channel->setChannelGroup(si.channelGroup);
	channel->setPaused(false);

	channelsPlaying.push_back(channel);
	return channelsPlaying.size() - 1;
}

/**
* Play a sound effect with FMOD. (This version is exposed to script.)
* @arg sfx The AudioProfile to play.
* @return A handle to use to set the sound's properties while it plays.
*/
ConsoleFunction(fmodPlay, int, 2, 2, "fmodPlay( sound )") {
	SoundInfo si = getSoundInfo(argv[1]);
	FMOD::Sound *sound1 = si.sound;
	if (sound1 == NULL)
		return -1;
	FMOD_RESULT result;
	FMOD::Channel *channel;

	result = fmodSystem->playSound(sound1, 0, true, &channel);
	if (result != FMOD_OK)
	{
		TGE::Con::errorf("FMOD error! (%d) %s", result, FMOD_ErrorString(result));
		return -1;
	}
	channel->setVolume((float)si.volume);
	channel->setChannelGroup(si.channelGroup);
	channel->setPaused(false);
	
	channelsPlaying.push_back(channel);
	return channelsPlaying.size() - 1;
}

/**
* Stop a sound effect that is currently playing.
* @arg handle The integer handle for the sound.
*/
ConsoleFunction(fmodStop, void, 2, 2, "fmodStop( handle )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= channelsPlaying.size()) {
		TGE::Con::errorf("Invalid sound handle!");
		return;
	}
	FMOD::Channel *channel;
	channel = channelsPlaying[ind];
	channel->stop();
}

/**
* Stop all sound effects currently playing.
*/
ConsoleFunction(fmodStopAll, void, 1, 1, "fmodStopAll()") {
	for (U32 ind = 0; ind < channelsPlaying.size(); ind++)
		channelsPlaying[ind]->stop();
}

/**
* Check whether a given sound handle is still playing.
* @arg handle The integer handle for the sound.
* @return Whether the sound is playing.
*/
ConsoleFunction(fmodIsPlaying, bool, 2, 2, "fmodIsPlaying( handle )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= channelsPlaying.size()) {
		TGE::Con::errorf("Invalid sound handle!");
		return false;
	}
	FMOD::Channel *channel;
	channel = channelsPlaying[ind];
	bool ret;
	channel->isPlaying(&ret);
	return ret;
}

/**
* Set the volume of a channel group.
* @arg channel The index of the channel group.
* @arg volume The volume, 0-1.
*/
ConsoleFunction(fmodSetChannelVolume, void, 3, 3, "fmodSetChannelVolume( channel, volume )") {
	FMOD::ChannelGroup* group = getGroup(atoi(argv[1]));
	group->setVolume(pow((float)atof(argv[2]),2));
}

/**
* Set the frequency of a channel group.
* @arg channel The index of the channel group.
* @arg frequency The frequency ratio to use.
*/
ConsoleFunction(fmodSetChannelFrequency, void, 3, 3, "fmodSetChannelFrequency( channel, frequency )") {
	FMOD::ChannelGroup* group = getGroup(atoi(argv[1]));
	group->setPitch((float)atof(argv[2]));
}

/**
* Set the volume of a currently playing sound.
* @arg handle The integer handle for the sound.
* @arg volume The volume, 0-1.
*/
ConsoleFunction(fmodSetVolume, void, 3, 3, "fmodSetVolume( handle, volume )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= channelsPlaying.size()) {
		TGE::Con::errorf("Invalid sound handle!");
		return;
	}
	FMOD::Channel *channel;
	channel = channelsPlaying[ind];
	channel->setVolume(pow((float)atof(argv[2]),2));
}

/**
* Set the frequency of a currently playing sound.
* @arg handle The integer handle for the sound.
* @arg frequency The frequency ratio to use.
*/
ConsoleFunction(fmodSetFrequency, void, 3, 3, "fmodSetFrequency( handle, frequency )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= channelsPlaying.size()) {
		TGE::Con::errorf("Invalid sound handle!");
		return;
	}
	FMOD::Channel *channel;
	channel = channelsPlaying[ind];
	channel->setPitch((float)atof(argv[2]));
}

/**
* Set the position in space of a currently playing sound.
* @arg handle The integer handle for the sound.
* @arg position The position to set the sound at.
* @arg velocity Optional velocity, to use for Doppler effects.
*/
ConsoleFunction(fmodSetPosition, void, 3, 4, "fmodSetPosition( handle, position, [velocity] )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= channelsPlaying.size()) {
		TGE::Con::errorf("Invalid sound handle!");
		return;
	}
	FMOD::Channel *channel;
	channel = channelsPlaying[ind];
	FMOD_VECTOR pos = toVector(argv[2]);
	FMOD_VECTOR vel = toVector(argv[3]);
	channel->set3DAttributes(&pos, &vel);
}

/**
* Set the transformation of the camera, for FMOD sound efffects.
* @arg transform The transformation matrix of the camera.
* @arg velocity Optional velocity, to use for Doppler effects.
*/
ConsoleFunction(fmodSetCameraTransform, void, 2, 3, "fmodSetCameraTransform( transform, [velocity] )") {
	MatrixF mat = StringMath::scan<MatrixF>(argv[1]);
	FMOD_VECTOR pos = { mat.getPosition().x, mat.getPosition().z, mat.getPosition().y };
	FMOD_VECTOR forward = { mat.getForwardVector().x, mat.getForwardVector().z, mat.getForwardVector().y };
	FMOD_VECTOR up = { mat.getUpVector().x, mat.getUpVector().z, mat.getUpVector().y };
	FMOD_VECTOR vel = toVector(argv[2]);
	fmodSystem->set3DListenerAttributes(0, &pos, &vel, &forward, &up);
}

/**
* Create a new local reverb. This currently doesn't seem to work.
* @arg type The reverb type.
* @return A handle for the reverb.
*/
ConsoleFunction(fmodCreateReverb, int, 2, 2, "fmodCreateReverb( type )") {
	std::string strpp = std::string(argv[1]);
	std::transform(strpp.begin(), strpp.end(), strpp.begin(), ::tolower);
	if (reverbPresets.find(strpp) == reverbPresets.end()) {
		TGE::Con::errorf("Invalid reverb type!");
		return -1;
	}
	FMOD_REVERB_PROPERTIES props = reverbPresets[strpp];
	FMOD::Reverb3D *reverb;
	fmodSystem->createReverb3D(&reverb);
	reverb->setProperties(&props);
	reverbs.push_back(reverb);
	return reverbs.size() - 1;
}

/**
* Set the type of a reverb.
* @arg reverb The reverb handle.
* @arg type The reverb type, 0-23.
*/
ConsoleFunction(fmodSetReverbType, void, 3, 3, "fmodSetReverbType( reverb, type )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= reverbs.size()) {
		TGE::Con::errorf("Invalid reverb!");
		return;
	}
	std::string strpp = std::string(argv[2]);
	std::transform(strpp.begin(), strpp.end(), strpp.begin(), ::tolower);
	if (reverbPresets.find(strpp) == reverbPresets.end()) {
		TGE::Con::errorf("Invalid reverb type!");
		return;
	}
	FMOD_REVERB_PROPERTIES props = reverbPresets[strpp];
	FMOD::Reverb3D *reverb = reverbs[ind];
	reverb->setProperties(&props);
}

/**
* Set the position of a reverb.
* @arg reverb The reverb handle.
* @arg position The vector for the reverb's position.
* @arg maxDist The maximum distance of the reverb.
* @arg minDist The minimum distance of the reverb.
*/
ConsoleFunction(fmodSetReverbPosition, void, 3, 5, "fmodSetReverbPosition( reverb, position, [maxDist], [minDist] )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= reverbs.size()) {
		TGE::Con::errorf("Invalid reverb!");
		return;
	}
	FMOD::Reverb3D *reverb = reverbs[ind];
	FMOD_VECTOR pos = toVector(argv[2]);
	reverb->set3DAttributes(&pos, (float)atof(argv[3]), (float)atof(argv[4]));
}

/**
* Set whether a reverb is active.
* @arg reverb The reverb handle.
* @arg active Whether the reverb is active.
*/
ConsoleFunction(fmodSetReverbActive, void, 3, 3, "fmodSetReverbActive( reverb, active )") {
	U32 ind = atoi(argv[1]);
	if (ind < 0 || ind >= reverbs.size()) {
		TGE::Con::errorf("Invalid reverb!");
		return;
	}
	FMOD::Reverb3D *reverb = reverbs[ind];
	reverb->setActive(toBool(argv[2]));
}

/**
* Set the ambient reverb to use everywhere.
* @arg type The type of reverb.
*/
ConsoleFunction(fmodSetAmbientReverb, void, 2, 2, "fmodSetAmbientReverb( type )") {
	std::string strpp = std::string(argv[1]);
	std::transform(strpp.begin(), strpp.end(), strpp.begin(), ::tolower);
	if (reverbPresets.find(strpp) == reverbPresets.end()) {
		TGE::Con::errorf("Invalid reverb type!");
		return;
	}
	FMOD_REVERB_PROPERTIES props = reverbPresets[strpp];
	props.WetLevel *= 0.2;
	fmodSystem->setReverbProperties(0, &props);
}

/**
* List valid reverb types.
*/
ConsoleFunction(listReverbTypes, void, 1, 1, "listReverbTypes()") {
	for (std::map<std::string, FMOD_REVERB_PROPERTIES>::iterator it = reverbPresets.begin(); it != reverbPresets.end(); it++) {
		TGE::Con::printf("%s", (*it).first.c_str());
	}
}

/**
* Override alxPlay to do nothing.
*/
TorqueOverride(int, alxPlay, (int source), originalAlxPlay) {
	return source;
}

/**
* Override alxCreateSource to do fmodPlay.
*/
TorqueOverride(int, alxCreateSource, (TGE::AudioProfile* sfx, const MatrixF* tf), originalAlxCreateSource) {
	return fmodPlay(sfx);
}