//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

datablock AudioProfile(GotCell1Sfx)
{
   filename    = "~/data/Shared/ions/cell1.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(GotCell2Sfx)
{
   filename    = "~/data/Shared/ions/cell2.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(GotCell3Sfx)
{
   filename    = "~/data/Shared/ions/cell3.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(GotCell4Sfx)
{
   filename    = "~/data/Shared/ions/cell4.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(GotPackSfx)
{
   filename    = "~/data/Shared/ions/ionpack.wav";
   description = AudioDefault3d;
   preload = true;
};

// Because this will get *really* hackish otherwise

$ionCellSounds[1] = GotCell1Sfx;
$ionCellSounds[2] = GotCell2Sfx;
$ionCellSounds[3] = GotCell3Sfx;
$ionCellSounds[4] = GotCell4Sfx;
$ionCellSounds[0] = GotPackSfx;

datablock ItemData(IonPackItem)
{
   // Mission editor category
   category = "Ions";
   className = "Ion";

   pickupAudio = IonPickupSfx;
   pickupName = "an Ion Pack!";

   // Basic Item properties
   shapeFile = "~/data/Shared/ions/ionpack.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;
   noPickupMessage = true;
   noRespawn = true;

   // Dynamic properties defined by the scripts
   maxInventory = 1;
};

function IonPackItem::onAdd(%this, %obj) {
    Parent::onAdd(%this, %obj);
    %level = MissionInfo.level;
    %index = %obj.index;
    if ($save::hasIonPack[%level, %index])
        %obj.setCloaked(true);
}

function IonPackItem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this, %obj, %user, %amount);
   startIonEffects(%obj);
   
   %sound = fmodPlay(GotPackSfx);
   fmodSetPosition(%sound, MatrixPos(%obj.getTransform()));
   
   %level = MissionInfo.level;
   %index = %obj.index;
   if (!$save::hasIonPack[%level, %index]) {
      $save::hasIonPack[%level, %index] = 1;
      $save::ionPacks++;
      messageClient(%user.client, 'MsgItemPickup', '\c0You found a new Ion Pack!');
   }
   else {
      messageClient(%user.client, 'MsgItemPickup', '\c0You already found this Ion Pack!');
   }
}

datablock ParticleData(IonParticle)
{
   textureName          = "~/data/Shared/particles/spark";
   dragCoefficient      = 2;
   gravityCoefficient   = 2;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 100;
   spinSpeed     = 0;
   spinRandomMin = 0;
   spinRandomMax =  0;
   useInvAlpha   = true;

   colors[0]     = "1 1 1 1";
   colors[1]     = "1 1 1 1.0";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 0.03;
   sizes[1]      = 0.03;
   sizes[2]      = 0.03;

   times[0]      = 0.0;
   times[1]      = 0.8;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(IonParticleEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 10;
   velocityVariance = 0.2;
   ejectionOffset   = 0;
   thetaMin         = 0;
   thetaMax         = 45;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 700;
   particles = "IonParticle";
};

datablock ParticleEmitterData(IonParticleEmitter2)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 10;
   velocityVariance = 0.2;
   ejectionOffset   = 0;
   thetaMin         = 0;
   thetaMax         = 45;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   //orientParticles  = true;
   lifetimeMS       = 200;
   particles = "IonParticle";
};

datablock ParticleEmitterNodeData(IonNode)
{
   timeMultiple = 1;
};

function startIonEffects(%obj)
{
   // Create a ParticleNode to run the emitter
   %position = %obj.getPosition();

   %obj = new ParticleEmitterNode(){
      datablock = IonNode;
      emitter = (%obj.getDataBlock().getId() == IonCellItem.getId()) ? IonParticleEmitter2 : IonParticleEmitter;
      position = %position;
   };
   addToMissionCleanup(%obj);
}

//-----------------------------------------------------------------------------

datablock ItemData(IonCellItem)
{
   // Mission editor category
   category = "Ions";
   className = "Ion";

   pickupAudio = IonPickupSfx;
   pickupName = "an Ion Cell!";

   // Basic Item properties
   shapeFile = "~/data/Shared/ions/ioncell.dts";
   mass = 1;
   friction = 1;
   elasticity = 0.3;
   emap = false;
   noPickupMessage = true;
   noRespawn = true;

   // Dynamic properties defined by the scripts
   maxInventory = 1;
};

function IonCellItem::onPickup(%this,%obj,%user,%amount)
{
   Parent::onPickup(%this, %obj, %user, %amount);
   startIonEffects(%obj);
   
   %level = MissionInfo.level;
   %index = %obj.index;
   if (!$save::hasIonCell[%level, %index]) {
      $save::hasIonCell[%level, %index] = 1;
      $save::ionPacks += 0.2;
      messageClient(%user.client, 'MsgItemPickup', '\c0You found a new Ion Cell!');
      %sfx = $ionCellSounds[$save::ionPacks * 5 % 5];
   }
   else {
      %sfx = GotCell1Sfx;
      messageClient(%user.client, 'MsgItemPickup', '\c0You already found this Ion Cell!');
   }
   %sound = fmodPlay(%sfx);
   fmodSetPosition(%sound, MatrixPos(%obj.getTransform()));
}

function IonCellItem::onAdd(%this, %obj) {
    Parent::onAdd(%this, %obj);
    %level = MissionInfo.level;
    %index = %obj.index;
    if ($save::hasIonCell[%level, %index])
        %obj.hide(true);
}