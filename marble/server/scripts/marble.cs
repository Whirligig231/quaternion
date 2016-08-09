//-----------------------------------------------------------------------------
// Torque Game Engine
//
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

datablock ParticleData(BounceParticle)
{
   textureName          = "~/data/Shared/particles/star";
   dragCoeffiecient     = 1.0;
   gravityCoefficient   = 0;
   windCoefficient      = 0;
   inheritedVelFactor   = 0;
   constantAcceleration = -2;
   lifetimeMS           = 500;
   lifetimeVarianceMS   = 100;
   useInvAlpha =  true;
   spinSpeed     = 90;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;

   colors[0]     = "0.9 0.0 0.0 1.0";
   colors[1]     = "0.9 0.9 0.0 1.0";
   colors[2]     = "0.9 0.9 0.0 0.0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.25;
   sizes[2]      = 0.25;

   times[0]      = 0;
   times[1]      = 0.75;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleBounceEmitter)
{
   ejectionPeriodMS = 80;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 250;
   particles = "BounceParticle";
};

//-----------------------------------------------------------------------------

datablock ParticleData(TrailParticle)
{
   textureName          = "~/data/Shared/particles/smoke";
   dragCoeffiecient     = 1.0;
   gravityCoefficient   = 0;
   windCoefficient      = 0;
   inheritedVelFactor   = 1;
   constantAcceleration = 0;
   lifetimeMS           = 100;
   lifetimeVarianceMS   = 10;
   useInvAlpha =  true;
   spinSpeed     = 0;

   colors[0]     = "1 1 0 0.0";
   colors[1]     = "1 1 0 1";
   colors[2]     = "1 1 1 0.0";

   sizes[0]      = 0.7;
   sizes[1]      = 0.4;
   sizes[2]      = 0.1;

   times[0]      = 0;
   times[1]      = 0.15;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleTrailEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 0.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 10000;
   particles = "TrailParticle";
};

//-----------------------------------------------------------------------------

datablock ParticleData(SuperJumpParticle)
{
   textureName          = "~/data/Shared/particles/twirl";
   dragCoefficient      = 0.25;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0.1;
   constantAcceleration = 0;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 150;
   spinSpeed     = 90;
   spinRandomMin = -90.0;
   spinRandomMax =  90.0;

   colors[0]     = "0 0.5 1 0";
   colors[1]     = "0 0.6 1 1.0";
   colors[2]     = "0 0.6 1 0.0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.25;
   sizes[2]      = 0.5;

   times[0]      = 0;
   times[1]      = 0.75;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleSuperJumpEmitter)
{
   ejectionPeriodMS = 10;
   periodVarianceMS = 0;
   ejectionVelocity = 1.0;
   velocityVariance = 0.25;
   thetaMin         = 150.0;
   thetaMax         = 170.0;
   lifetimeMS       = 5000;
   particles = "SuperJumpParticle";
};

//-----------------------------------------------------------------------------

datablock ParticleData(SuperSpeedParticle)
{
   textureName          = "~/data/Shared/particles/spark";
   dragCoefficient      = 0.25;
   gravityCoefficient   = 0;
   inheritedVelFactor   = 0.25;
   constantAcceleration = 0;
   lifetimeMS           = 1500;
   lifetimeVarianceMS   = 150;

   colors[0]     = "0.8 0.8 0 0";
   colors[1]     = "0.8 0.8 0 1.0";
   colors[2]     = "0.8 0.8 0 0.0";

   sizes[0]      = 0.25;
   sizes[1]      = 0.25;
   sizes[2]      = 1.0;

   times[0]      = 0;
   times[1]      = 0.25;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(MarbleSuperSpeedEmitter)
{
   ejectionPeriodMS = 5;
   periodVarianceMS = 0;
   ejectionVelocity = 1.0;
   velocityVariance = 0.25;
   thetaMin         = 130.0;
   thetaMax         = 170.0;
   lifetimeMS       = 5000;
   particles = "SuperSpeedParticle";
};

//-----------------------------------------------------------------------------

datablock ParticleEmitterData(MarbleSuperBounceEmitter)
{
   ejectionPeriodMS = 20;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 250;
   particles = "MarbleStar";
};

//-----------------------------------------------------------------------------

datablock ParticleEmitterData(MarbleShockAbsorberEmitter)
{
   ejectionPeriodMS = 20;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 250;
   particles = "MarbleStar";
};

//-----------------------------------------------------------------------------

datablock ParticleEmitterData(MarbleHelicopterEmitter)
{
   ejectionPeriodMS = 20;
   periodVarianceMS = 0;
   ejectionVelocity = 3.0;
   velocityVariance = 0.25;
   thetaMin         = 80.0;
   thetaMax         = 90.0;
   lifetimeMS       = 5000;
   particles = "MarbleStar";
};

//-----------------------------------------------------------------------------
// ActivePowerUp
// 0 - no active powerup
// 1 - Super Jump
// 2 - Super Speed
// 3 - Super Bounce
// 4 - Indestructible

datablock AudioProfile(Bounce1Sfx)
{
   filename    = "~/data/Shared/sound/bouncehard1.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(Bounce2Sfx)
{
   filename    = "~/data/Shared/sound/bouncehard2.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(Bounce3Sfx)
{
   filename    = "~/data/Shared/sound/bouncehard3.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(Bounce4Sfx)
{
   filename    = "~/data/Shared/sound/bouncehard4.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(JumpSfx)
{
   filename    = "~/data/Shared/sound/Jump.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock AudioProfile(RollingHardSfx)
{
   filename    = "~/data/Shared/sound/Rolling_Hard.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock AudioProfile(SlippingSfx)
{
   filename    = "~/data/Shared/sound/Sliding.wav";
   description = AudioClosestLooping3d;
   preload = true;
};

datablock MarbleData(DefaultMarble)
{
   shapeFile = "~/data/Shared/balls/ball-superball.dts";
   emap = true;
   renderFirstPerson = true;
// maxRollVelocity = 55; NONE OF THIS MATTERS ANYMORE
// angularAcceleration = 120;
// maxRollVelocity = 15;
// angularAcceleration = 75;
// brakingAcceleration = 30;
// gravity = 20;
// staticFriction = 1.1;
// kineticFriction = 0.7;
// bounceKineticFriction = 0.2;
// maxDotSlide = 0.5;
// bounceRestitution = 0.5;
// jumpImpulse = 7.5;
// maxForceRadius = 50;
   
   btShapeType = "sphere";
   btRadius = 0.2;
   btMass = 1;
   btFriction = 1.1;
   btRestitution = 0.7;
   btAngularDamping = 0.9;
   
   btControlAngular = 1.5;
   btControlLinear = 5;
   btControlJump = 7.5;
   
   btCallback = true;
   btPlayer = true;

   bounce1 = Bounce1Sfx;
   bounce2 = Bounce2Sfx;
   bounce3 = Bounce3Sfx;
   bounce4 = Bounce4Sfx;

   rollHardSound = RollingHardSfx;
   slipSound = SlippingSfx;
   jumpSound = JumpSfx;
   
   // Emitters
   minTrailSpeed = 10;            // Trail threshold
   trailEmitter = MarbleTrailEmitter;
   
   minBounceSpeed = 3;           // Bounce threshold
   bounceEmitter = MarbleBounceEmitter;
   
   powerUpEmitter[1] = MarbleSuperJumpEmitter; 		// Super Jump
   powerUpEmitter[2] = MarbleSuperSpeedEmitter; 	// Super Speed
// powerUpEmitter[3] = MarbleSuperBounceEmitter; 	// Super Bounce
// powerUpEmitter[4] = MarbleShockAbsorberEmitter; 	// Shock Absorber
// powerUpEmitter[5] = MarbleHelicopterEmitter; 	// Helicopter
   
   // Power up timouts. Timeout on the speed and jump only affect
   // the particle trail
   powerUpTime[1] = 1000;	// Super Jump
   powerUpTime[2] = 1000; 	// Super Speed
   powerUpTime[3] = 5000; 	// Super Bounce
   powerUpTime[4] = 5000; 	// Shock Absorber
   powerUpTime[5] = 5000; 	// Helicopter

   // Allowable Inventory Items
   maxInv[SuperJumpItem] = 20;
   maxInv[SuperSpeedItem] = 20;
   maxInv[SuperBounceItem] = 20;
   maxInv[IndestructibleItem] = 20;
   maxInv[TimeTravelItem] = 20;
//   maxInv[GoodiesItem] = 10;
};


//-----------------------------------------------------------------------------

function DefaultMarble::onAdd(%this, %obj)
{
   Parent::onAdd(%this, %obj);
   echo("New Marble: " @ %obj);
}

function DefaultMarble::onTrigger(%this, %obj, %triggerNum, %val)
{
}


//-----------------------------------------------------------------------------

function DefaultMarble::onCollision(%this,%obj,%col)
{
   // Try and pickup all items
   if (%col.getClassName() $= "Item")
   {
      %data = %col.getDatablock();
      %obj.pickup(%col,1);
   }
}


//-----------------------------------------------------------------------------
// The following event callbacks are punted over to the connection
// for processing

function DefaultMarble::onEnterPad(%this,%object)
{
   %object.client.onEnterPad();
}

function DefaultMarble::onLeavePad(%this,%object)
{
   %object.client.onLeavePad();
}

function DefaultMarble::onStartPenalty(%this,%object)
{
   %object.client.onStartPenalty();
}

function DefaultMarble::onOutOfBounds(%this,%object)
{
   %object.client.onOutOfBounds();
}

function DefaultMarble::setCheckpoint(%this,%object,%check)
{
   %object.client.setCheckpoint(%check);
}


//-----------------------------------------------------------------------------
// Marble object
//-----------------------------------------------------------------------------

function Marble::setPowerUp(%this,%item,%reset)
{
   PlayGui.setPowerUp(%item.shapeFile);
   %this.powerUpData = %item;
   %this.setPowerUpId(%item.powerUpId,%reset);
}

function Marble::getPowerUp(%this)
{
   return %this.powerUpData;
}

function Marble::onPowerUpUsed(%obj)
{
   PlayGui.setPowerUp("");
   %obj.playAudio(0, %obj.powerUpData.activeAudio);
   %obj.powerUpData = "";
}


//-----------------------------------------------------------------------------

function marbleVel()
{
   return $MarbleVelocity;
}

function metricsMarble()
{
   Canvas.pushDialog(FrameOverlayGui, 1000);
   TextOverlayControl.setValue("$MarbleVelocity");
}

//-----------------------------------------------------------------------------

datablock StaticShapeData(ExplodingBall)
{
   className = "Effect";
   category = "Effects";
   shapeFile = "~/data/Shared/balls/ball-explode.dts";
   scopeAlways = true;
};

function ExplodingBall::onAdd(%this, %obj) {
    Parent::onAdd(%this, %obj);
	%obj.setSkinName(p().getSkinName());
    %obj.startFade(1000, 2500, true);
    %obj.schedule(3500, delete);
}

//-----------------------------------------------------------------------------

datablock AudioProfile(BallDieSfx)
{
   filename    = "~/data/Shared/sound/explode.wav";
   description = Audio2D;
   preload = true;
};

datablock ParticleData(BallExplodeParticle)
{
   textureName          = "~/data/Shared/particles/spark";
   dragCoefficient      = 4;
   gravityCoefficient   = 1;
   inheritedVelFactor   = 0;
   windCoefficient      = 0;
   constantAcceleration = 0;
   lifetimeMS           = 1000;
   lifetimeVarianceMS   = 0;
   spinSpeed     = 0;
   spinRandomMin = 0;
   spinRandomMax =  0;
   useInvAlpha   = true;

   colors[0]     = "1 1 1 1";
   colors[1]     = "1 0.8 0 1.0";
   colors[2]     = "1 0.8 0 0.0";

   sizes[0]      = 0.1;
   sizes[1]      = 0.025;
   sizes[2]      = 0.025;

   times[0]      = 0.0;
   times[1]      = 0.8;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BallExplodeEmitter)
{
   ejectionPeriodMS = 0.5;
   periodVarianceMS = 0;
   ejectionVelocity = 10;
   velocityVariance = 0;
   ejectionOffset   = 0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   orientParticles  = false;
   lifetimeMS       = 150;
   particles = "BallExplodeParticle";
};

datablock ParticleEmitterNodeData(BallNode)
{
   timeMultiple = 1;
};

function explodeMarble() {
    if(isEventPending(LocalClientConnection.respawnSchedule))
        return;
    p().startFade(0, 0, true);
    p().setMode(Start);
    p().setOOB(true);
    LocalClientConnection.respawnSchedule = LocalClientConnection.schedule(2500, respawnPlayer);
    addToMissionCleanup(%exp = new StaticShape() {
        position = getWords(p().getTransform(), 0, 2);
        rotation = "1 0 0 0";
        scale = "1 1 1";
        datablock = ExplodingBall;
    });
    %exp.playThread(0, "explode");
    
    serverPlay2d(BallDieSfx);
    
    %obj = new ParticleEmitterNode(){
        datablock = BallNode;
        emitter = BallExplodeEmitter;
        position = getWords(p().getTransform(), 0, 2);
    };
    addToMissionCleanup(%obj);
}