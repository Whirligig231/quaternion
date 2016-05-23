//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
//-----------------------------------------------------------------------------

datablock AudioProfile(teleOnSfx)
{
   filename    = "~/data/Shared/sound/teleport_on.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(teleOffSfx)
{
   filename    = "~/data/Shared/sound/teleport_off.wav";
   description = Audio2D;
   preload = true;
};

datablock ParticleData(TeleParticle)
{
   textureName          = "~/data/Shared/particles/bolt";
   dragCoefficient      = 0;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 250;
   lifetimeVarianceMS   = 0;
   spinSpeed     = 0;
   spinRandomMin = 0;
   spinRandomMax =  0;
   useInvAlpha   = true;

   colors[0]     = "0.2 0 1 1";
   colors[1]     = "1 0.8 0 1.0";
   colors[2]     = "1 0.8 0 0.0";

   sizes[0]      = 0.05;
   sizes[1]      = 0.2;
   sizes[2]      = 0.2;

   times[0]      = 0.0;
   times[1]      = 0.8;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(TeleParticleEmitter)
{
   ejectionPeriodMS = 2;
   periodVarianceMS = 0;
   ejectionVelocity = 6;
   velocityVariance = 0;
   ejectionOffset   = 0;
   thetaMin         = 145;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 3000;
   particles = "TeleParticle";
};

datablock ParticleData(TeleExplodeParticle)
{
   textureName          = "~/data/Shared/particles/bolt";
   dragCoefficient      = 2;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 0;
   spinSpeed     = 0;
   spinRandomMin = 0;
   spinRandomMax =  0;
   useInvAlpha   = true;

   colors[0]     = "1 0.8 0 1";
   colors[1]     = "0.2 0 1 1.0";
   colors[2]     = "0.2 0 1 0.0";

   sizes[0]      = 0.2;
   sizes[1]      = 0.05;
   sizes[2]      = 0.05;

   times[0]      = 0.0;
   times[1]      = 0.8;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(TeleExplodeEmitter)
{
   ejectionPeriodMS = 1;
   periodVarianceMS = 0;
   ejectionVelocity = 6;
   velocityVariance = 0;
   ejectionOffset   = 0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 100;
   particles = "TeleExplodeParticle";
};

datablock ParticleEmitterNodeData(TeleNode)
{
   timeMultiple = 1;
};

//-----------------------------------------------------------------------------

datablock StaticShapeData(StartPad)
{
   className = "Pad";
   category = "Pads";
   shapeFile = "~/data/Shared/pads/teleport2.dts";
   scopeAlways = true;
   emap = false;
   
   btShapeType = "raw";
   btShapeFile = "~/data/Shared/pads/teleport2.raw";
   	btMass = 0;
	btFriction = 1.1;
	btRestitution = 0.7;
};

function StartPad::onAdd(%this, %obj)
{
   Parent::onAdd(%this, %obj);
   $Game::StartPad = %obj;
   %obj.setName("StartPoint");
   %obj.playThread(0,"ambient");
}


//-----------------------------------------------------------------------------

datablock StaticShapeData(EndPad)
{
   className = "Pad";
   category = "Pads";
   shapeFile = "~/data/Shared/pads/teleport.dts";
   scopeAlways = true;
   emap = false;
   
   btShapeType = "raw";
   btShapeFile = "~/data/Shared/pads/teleport.raw";
   	btMass = 0;
	btFriction = 1.1;
	btRestitution = 0.7;
};

function EndPad::onAdd(%this, %obj)
{
   Parent::onAdd(%this, %obj);
   $Game::EndPad = %obj;
   %obj.setName("EndPoint");
   %obj.playThread(0,"ambient");
   %obj.teleporting = false;
}

function startTele(%pad) {
    // Create a ParticleNode to run the emitter
    %position = VectorAdd(%pad.getPosition(), "0 0 1.55");

    %pad.pnode = new ParticleEmitterNode(){
        datablock = TeleNode;
        emitter = TeleParticleEmitter;
        position = %position;
    };
    addToMissionCleanup(%pad.pnode);
    
    p().startFade(3000,0,true);
    %pad.teleporting = true;
    %pad.teleSchedule = schedule(3000, 0, "finishTele", %pad);
    
    %pad.playHandle = fmodPlay(teleOnSfx);
}

function finishTele(%pad) {
    %obj = new ParticleEmitterNode(){
        datablock = TeleNode;
        emitter = TeleExplodeEmitter;
        position = getWords(p().getTransform(), 0, 2);
    };
    addToMissionCleanup(%obj);
    serverPlay2d(spawnSfx);
    p().setMode(Start);
    PlayGui.schedule(1000, "fade", "0 0 0", 500, true);
    schedule(2000, 0, "LevelTeleport", %pad.level, %pad.index);
}

function stopTele(%pad) {
    %pad.pnode.delete();
    p().startFade(0,0,false);
    %pad.teleporting = false;
    cancel(%pad.teleSchedule);
    fmodStop(%pad.playHandle);
    serverPlay2d(teleOffSfx);
}

function EndPad::onFrame(%this, %obj, %timeDelta) {
    Parent::onFrame(%this, %obj, %timeDelta);
    
    %pos1 = %obj.getPosition();
    %pos2 = getWords(p().getTransform(), 0, 2);
    
    %diff = VectorSub(%pos2, %pos1);
    %diff = VectorSub(%diff, "0 0 0.4");
    %dlen = VectorLen(%diff);
    
    if (%dlen < 0.15 && !%obj.teleporting)
        startTele(%obj);
    else if (%dlen > 0.4 && %obj.teleporting)
        stopTele(%obj);
}

//-----------------------------------------------------------------------------

datablock StaticShapeData(EndPadFoot)
{
   className = "Pad";
   category = "Pads";
   shapeFile = "~/data/Shared/pads/teleport_foot.dts";
   scopeAlways = true;
   emap = false;
   
   btShapeType = "raw";
   btShapeFile = "~/data/Shared/pads/teleport_foot.raw";
   	btMass = 0;
	btFriction = 1.1;
	btRestitution = 0.7;
};