//-----------------------------------------------------------------------------
// Copyright (c) 2015 The Platinum Team
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <cstdlib>
#include <PluginLoader/PluginInterface.h>
#include <TorqueLib/QuickOverride.h>
#include <TorqueLib/TGE.h>
#include "MathExtension.h"
#include <map>

using namespace std;
using namespace TGE;

/* HIGH LEVEL OVERVIEW
*
* I felt like this is worthy of some giant explaination of what the fuck is
* going on here, because it's a custom system that we wrote.
*
* SO, WHAT IS THESE SYNC IDS?
*
* The sync system is made up of unique id's generated on the server and mapped
* to the current object for it's reference lookup. This unique id is then
* shared between the server and client objects through the UDP transmission
* channel. When we want to do something on an object, such as client
* interpolation, we now know what object is what on the client.
*
* HOW IS THIS DIFFERENT THAN THE OLD SYNC METHOD IN MULTIPLAYER ON PLAYERS?
*
* Under the old method, we subtracted each marble's scale by 0.0001 to provide
* and used that as an unique identifier for each marble, as that was sent
* across the UDP channel by default, being a statically mapped field.
*
* WHAT IS THIS CURRENTLY BEING USED FOR?
*
* Currently this is being used for moving objects. Besides just sending the
* unique identifier for the ghost lookup, we also send stuff such as the
* moving object's position between point A and point B on a path, as well
* as the path's current ghost node. This data is send and synced across the
* UDP channel. We use the ghost lookups for synchronizing the script-driven
* interpolation system.
*
* DOESN'T KNOWING WHAT EACH OBJECT IS ON THE CLIENT DEFEAT THE WHOLE PURPOSE
* OF HAVING A GHOTSING SYSTEM IN THE FIRST PLACE?
*
* Yes, but if you're going to bitch about that, then you better bitch about
* the marble physics being client sided, the marble sending its position to
* the server, and among other things that we had to do to get multiplayer
* to work good and fairly stable in this old engine.
*/

//-----------------------------------------------------------------------------

/// The global ID value that is used for generating the sync IDs.
S32 gSyncId = 0;

/// The map that holds all of the client ghost objects that have sync IDs
map<S32, S32> gClientGhostActiveList;

/// The map that holds all of the server ghost objects that have sync IDs
map<S32, S32> gServerGhostActiveList;

//-----------------------------------------------------------------------------

/* Gets the shared syncable ID between client and server objects that are
* ghosted across the network.
* @arg id - The TorqueScript object ID (must be GameBase, InteriorInstance,
*           or TSStatic) in which we are requesting the sync id.
* @return the unique id for the object, or an empty string if the torque
*         object doesn't have a unique id.
*/
S32 getSyncId(S32 id, map<S32, S32> &ghostList) {
	for (map<S32, S32>::iterator iter = ghostList.begin(); iter != ghostList.end(); ++iter) {
		if (iter->second == id)
			return iter->first;
	}
	return -1;
}

/* Flushes old ghosts that do not exist anymore on the client out of the map
* Note: This is only for the client as server ghosts do not get flushed
*        as much as the client ghosts.
*/
void flushOldSyncIds() {
	map<S32, S32>::iterator iter = gClientGhostActiveList.begin();

	// http://stackoverflow.com/questions/4600567/how-can-i-delete-elements-of-a-stdmap-with-an-iterator/4600592#4600592
	while (iter != gClientGhostActiveList.end()) {
		// Remove reference from the map if the object no longer exists
		if (Sim::findObject(StringMath::print<S32>(iter->second)) == NULL)
			gClientGhostActiveList.erase(iter++);
		else
			++iter;
	}
}

//-----------------------------------------------------------------------------
// This section handles the transmission UDP data between the client and the
// server. Methods that send data to clients are marked as ::packUpdate and
// methods that receive data from the server are marked as ::unpackUpdate
//
// Whenever packupdate is called, it will attempt to generate a unique
// id that can be shared between the client and the server.
//
// This id will then be cached within a global lookup table
// and be used as reference to keep a syncpoint.
//
// Note: We have to use seperate methods for GameBase, InteriorInstance, 
//       and TSStatic because we don't need to sync ever SceneObject.
//-----------------------------------------------------------------------------

TorqueOverrideMember(U32, GameBase::packUpdate, (GameBase *thisObj, TGE::NetConnection *conn, U32 mask, BitStream *stream), originalGameBasePackUpdate) {
	if (getSyncId(thisObj->getId(), gServerGhostActiveList) == -1) {
		// Save it to the ghost active list
		gServerGhostActiveList[gSyncId] = thisObj->getId();
		gSyncId++;
	}
	S32 var = getSyncId(thisObj->getId(), gServerGhostActiveList);
	stream->writeInt(var, 32);

	// moving objects should also send the current frame snapshot to the client.
	S8 isMoving = (S8)atoi(thisObj->getDataField(StringTable->insert("moving", false), NULL));
	stream->writeInt(isMoving, 8);
	if (isMoving) {
		// Send the current path node ghost ID as well as the current position
		// snapshot of where the object is moving on the path (in terms of 
		// milliseconds).
		S32 timeDelta = atoi(thisObj->getDataField(StringTable->insert("pathPosition", false), NULL));
		S32 syncPathId = atoi(thisObj->getDataField(StringTable->insert("pathSyncId", false), NULL));
		stream->writeInt(timeDelta, 32);
		stream->writeInt(syncPathId, 32);
	}

	return originalGameBasePackUpdate(thisObj, conn, mask, stream);
}

TorqueOverrideMember(void, GameBase::unpackUpdate, (GameBase *thisObj, TGE::NetConnection *conn, BitStream *stream), originalGameBaseUnPackUpdate) {
	S32 ghostId = stream->readInt(32);

	// perform cleanup of old ghosties and then add the new one to the map
	flushOldSyncIds();
	gClientGhostActiveList[ghostId] = thisObj->getId();

	// check if we are a moving object
	if (stream->readInt(8)) {
		// Extrapolate the position time delta based upon the ping.
		// This results in about 95% accuracy of the simulation of the
		// netobject on the client from whenever it received the update
		// from the server end.
		S32 timeDelta = stream->readInt(32) + atoi(Con::evaluatef("if (isObject(ServerConnection)) return ServerConnection.getPing() / 2; return 0;"));
		S32 syncPathId = stream->readInt(32);

		// Update the current ID for the node, as well as the time delta
		thisObj->setDataField(StringTable->insert("pathPosition", false), NULL, StringMath::print<S32>(timeDelta));
		thisObj->setDataField(StringTable->insert("pathSyncId", false), NULL, StringMath::print<S32>(syncPathId));

		// script callback, received callback of moving object
		Con::evaluatef("onUnpackUpdateMovingObject(%d);", thisObj->getId());
	}

	originalGameBaseUnPackUpdate(thisObj, conn, stream);
}

TorqueOverrideMember(U32, InteriorInstance::packUpdate, (InteriorInstance *thisObj, TGE::NetConnection *conn, U32 mask, BitStream *stream), originalInteriorPackUpdate) {
	if (getSyncId(thisObj->getId(), gServerGhostActiveList) == -1) {
		// Save it to the ghost active list
		gServerGhostActiveList[gSyncId] = thisObj->getId();
		gSyncId++;
	}
	S32 var = getSyncId(thisObj->getId(), gServerGhostActiveList);
	stream->writeInt(var, 32);

	return originalInteriorPackUpdate(thisObj, conn, mask, stream);
}

TorqueOverrideMember(void, InteriorInstance::unpackUpdate, (TGE::InteriorInstance *thisObj, TGE::NetConnection *conn, BitStream *stream), originalInteriorUnPackUpdate) {
	S32 ghostId = stream->readInt(32);

	// perform cleanup of old ghosties and then add the new one to the map
	flushOldSyncIds();
	gClientGhostActiveList[ghostId] = thisObj->getId();

	originalInteriorUnPackUpdate(thisObj, conn, stream);
}

TorqueOverrideMember(U32, TSStatic::packUpdate, (TSStatic *thisObj, TGE::NetConnection *conn, U32 mask, BitStream *stream), originalTSStaticPackUpdate) {
	if (getSyncId(thisObj->getId(), gServerGhostActiveList) == -1) {
		// Save it to the ghost active list
		gServerGhostActiveList[gSyncId] = thisObj->getId();
		gSyncId++;
	}
	S32 var = getSyncId(thisObj->getId(), gServerGhostActiveList);
	stream->writeInt(var, 32);
	return originalTSStaticPackUpdate(thisObj, conn, mask, stream);
}

TorqueOverrideMember(void, TSStatic::unpackUpdate, (TSStatic *thisObj, TGE::NetConnection *conn, BitStream *stream), originalTSStaticUnPackUpdate) {
	S32 ghostId = stream->readInt(32);

	// perform cleanup of old ghosties and then add the new one to the map
	flushOldSyncIds();
	gClientGhostActiveList[ghostId] = thisObj->getId();

	originalTSStaticUnPackUpdate(thisObj, conn, stream);
}

//-----------------------------------------------------------------------------
// Script API
//-----------------------------------------------------------------------------

/* Gets the shared syncable ID between client and server objects that are
* ghosted across the network.
* @return the unique id for the object, or -1 if the object is not found
*         or does not contain a sync id.
*/
ConsoleMethod(SceneObject, getSyncId, S32, 2, 2, "%sceneObject.getSyncId();") {
	return getSyncId(object->getId(), (object->isServerObject() ? gServerGhostActiveList : gClientGhostActiveList));
}

/* Gets the object from the sync ID on the client.
* @arg ghostId - The ID that is used for the ghost lookup table to find
*                the actual ghost object exposed to script.
* @return The ghosted object ID (torque id) or -1 if the object doesn't
*         exist from the referenced ghost ID.
*/
ConsoleFunction(getClientSyncObject, S32, 2, 2, "getClientSyncObject(%ghostId);") {
	S32 ghostId = atoi(argv[1]);

	for (map<S32, S32>::iterator iter = gClientGhostActiveList.begin(); iter != gClientGhostActiveList.end(); ++iter) {
		if (iter->first == ghostId) {
			SimObject *obj = Sim::findObject(StringMath::print(iter->second));
			if (obj == NULL)
				return -1;
			return obj->getId();
		}
	}

	return -1;
}

/* Gets the object from the sync ID on the server.
* @arg ghostId - The ID that is used for the ghost lookup table to find
*                the actual ghost object exposed to script.
* @return The ghosted object ID (torque id) or -1 if the object doesn't
*         exist from the referenced ghost ID.
*/
ConsoleFunction(getServerSyncObject, S32, 2, 2, "getServerSyncObject(%ghostId);") {
	S32 ghostId = atoi(argv[1]);

	for (map<S32, S32>::iterator iter = gServerGhostActiveList.begin(); iter != gServerGhostActiveList.end(); ++iter) {
		if (iter->first == ghostId) {
			SimObject *obj = Sim::findObject(StringMath::print(iter->second));
			if (obj == NULL)
				return -1;
			return obj->getId();
		}
	}

	return -1;
}

/* Checks if the object is currently simulated on the server.
* @return true if the object is simulated on the server, false if the object
*         is simulated on the client.
*/
ConsoleMethod(NetObject, isServerObject, bool, 2, 2, "obj.isServerObject()") {
	return object->isServerObject();
}

/* Checks if the object is currently simulated on the client.
* @return true if the object is simulated on the client, false if the object
*         is simulated on the server.
*/
ConsoleMethod(NetObject, isClientObject, bool, 2, 2, "obj.isClientObject()") {
	return object->isClientObject();
}

ConsoleMethod(NetObject, forceNetUpdate, void, 2, 2, "obj.forceNetUpdate()") {
	object->setMaskBits(1);
}