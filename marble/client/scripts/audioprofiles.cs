//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

// Channel assignments (channel 0 is unused in-game).

$GuiAudioType     = 1;
$SimAudioType     = 1;
$MessageAudioType = 1;
$EffectAudioType = 1;
$MusicAudioType = 2;

new AudioDescription(AudioGui)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $GuiAudioType;
};

new AudioDescription(AudioMessage)
{
   volume   = 1.0;
   isLooping= false;
   is3D     = false;
   type     = $MessageAudioType;
};

new AudioDescription(ClientAudioLooping2D)
{
   volume = 1.0;
   isLooping = true;
   is3D = false;
   type = $EffectAudioType;
};

new AudioProfile(TimeTravelLoopSfx)
{
   filename    = "~/data/Shared/sound/TimeTravelActive.wav";
   description = ClientAudioLooping2d;
   preload = true;
};

new AudioProfile(AudioButtonOver)
{
   filename = "~/data/Shared/sound/buttonOver.wav";
   description = "AudioGui";
	preload = true;
};

new AudioProfile(AudioButtonDown)
{
   filename = "~/data/Shared/sound/ButtonPress.wav";
   description = "AudioGui";
	preload = true;
};

new AudioProfile(TestingSfx)
{
   filename    = "~/data/Shared/sound/testing.wav";
   description = AudioGui;
   preload = true;
};

new AudioDescription(AudioMusic)
{
   volume   = 1.0;
   isLooping = true;
   isStreaming = true;
   is3D     = false;
   type     = $MusicAudioType;
};

function playMusic(%musicFileBase)
{
   fmodStop($currentMusicHandle);
   if(isObject(MusicProfile))
      MusicProfile.delete();

   %file = %musicFileBase;
   new AudioProfile(MusicProfile) {
      fileName = %file;
      description = "AudioMusic";
      preload = false;
   };
   $currentMusicBase = %musicFileBase;
   $currentMusicHandle = fmodPlay(MusicProfile);  //add this line
}

function playShellMusic()
{
   playMusic("marble/data/Shared/sound/Shell.ogg");
}

function playGameMusic()
{
   if(isFile(FilePath($Server::MissionFile) @ "/music.ogg"))
      playMusic(FilePath($Server::MissionFile) @ "/music.ogg");
   else
      playMusic("marble/data/Shared/sound/Shell.ogg");
}

function pauseMusic()
{
   fmodStop($currentMusicHandle);
}

function resumeMusic()
{
   playMusic($currentMusicBase);
}

// Update the listener

function updateListener(%timeDelta) {
	if (!isObject(ServerConnection))
		return;
	%obj = ServerConnection.getControlObject();
	if (!isObject(%obj))
		return;
	%mat = %obj.getCameraTransform();
	%pos = getWords(%mat,0,2);
	if (%obj.cameraPositionPrev $= "")
		%obj.cameraVelocity = "0 0 0";
	else
	%obj.cameraVelocity = VectorScale(VectorSub(%pos, %obj.cameraPositionPrev), 1000/%timeDelta);
	%obj.cameraPositionPrev = %pos;
	
	fmodSetCameraTransform(%mat, %obj.cameraVelocity);
}

// Set alx volume to 0 so fmod takes over
for (%i=0;%i<10;%i++)
	alxSetChannelVolume(%i,0);

package AudioOverrides {

function GameConnection::play2d(%this, %sfx) {
	fmodPlay(%sfx);
}

};

activatePackage(AudioOverrides);