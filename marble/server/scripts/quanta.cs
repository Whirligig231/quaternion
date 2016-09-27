$quantumIndex = "";
$quantumTimer = 0;

datablock AudioProfile(QuantumOffSfx)
{
   filename    = "~/data/Shared/sound/quantum_off.wav";
   description = Audio2D;
   preload = true;
};

datablock AudioProfile(QuantumOnSfx)
{
   filename    = "~/data/Shared/sound/quantum_on.wav";
   description = Audio2D;
   preload = true;
};

function giveQuantum(%quantum) {
	if ($quantumStop[$quantumIndex] !$= "")
		call($quantumStop[$quantumIndex], %timeDelta);
		
	if ($quantumSound[$quantumIndex] !$= "")
		fmodStop($quantumSoundHandle);
	
	p().setSkinName($quantumSkin[%quantum]);
	$quantumIndex = %quantum;
	$quantumTimer = $quantumTime[%quantum];
	
	if ($quantumStart[$quantumIndex] !$= "")
		call($quantumStart[$quantumIndex], %timeDelta);
		
	if ($quantumSound[$quantumIndex] !$= "")
		$quantumSoundHandle = fmodPlay($quantumSound[$quantumIndex]);
		
	if (%quantum $= "") {
		QuantumBar.setValue(0);
		fmodPlay(QuantumOffSfx);
	}
	else {
		QuantumBar.setValue(1);
		fmodPlay(QuantumOnSfx);
	}
}

function updateQuantum(%timeDelta) {
	if ($quantumIndex $= "")
		return;
	
	if ($quantumUpdate[$quantumIndex] !$= "")
		call($quantumUpdate[$quantumIndex], %timeDelta);
		
	if ($quantumSound[$quantumIndex] !$= "")
		fmodSetPosition($quantumSoundHandle, MatrixPos(p().getTransform()));
		
	$quantumTimer -= %timeDelta;

	QuantumBar.setValue($quantumTimer / $quantumTime[$quantumIndex]);
	
	if ($quantumTimer <= 0)
		giveQuantum("");
}

$globalFrameEvent[$globalFrameEvents] = "updateQuantum";
$globalFrameEvents++;

$quantumSkin[""] = "base";
$quantumTime[""] = 0;

// -----------------------------------------------------------------------------------------------------------------

datablock ParticleData(BlastParticle)
{
   textureName          = "~/data/Shared/particles/line";
   dragCoefficient      = 2;
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

   colors[0]     = "1 0.8 0 1";
   colors[1]     = "1 0.8 0 1";
   colors[2]     = "1 0.8 0 1";

   sizes[0]      = 0.2;
   sizes[1]      = 0.05;
   sizes[2]      = 0.05;

   times[0]      = 0.0;
   times[1]      = 0.8;
   times[2]      = 1.0;
};

datablock ParticleEmitterData(BlastEmitter)
{
   ejectionPeriodMS = 1;
   periodVarianceMS = 0;
   ejectionVelocity = 6;
   velocityVariance = 0;
   ejectionOffset   = 0;
   thetaMin         = 60;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 100;
   particles = "BlastParticle";
};

datablock ParticleEmitterData(BlastLeakEmitter)
{
   ejectionPeriodMS = 30;
   periodVarianceMS = 10;
   ejectionVelocity = 4;
   velocityVariance = 4;
   ejectionOffset   = 0;
   thetaMin         = 0;
   thetaMax         = 180;
   phiReferenceVel  = 0;
   phiVariance      = 360;
   //overrideAdvances = false;
   orientParticles  = true;
   lifetimeMS       = 15000;
   particles = "BlastParticle";
};

datablock ParticleEmitterNodeData(BlastNode)
{
   timeMultiple = 1;
};

function BlastNode::onFrame(%this, %obj, %timeDelta) {
	%obj.getClient().setTransform(MatrixPos(p().getTransform()) SPC "1 0 0 0");
	if (%obj.cleanupSchedule $= "")
		%obj.cleanupSchedule = %obj.schedule(%obj.emitter.lifetimeMS + 1000, "delete");
}

function makeBlastParticle() {
    %obj = new ParticleEmitterNode(){
        datablock = BlastNode;
        emitter = BlastEmitter;
        position = getWords(p().getTransform(), 0, 2);
    };
    addToMissionCleanup(%obj);
}

function makeBlastLeakParticle() {
    $blastLeaker = new ParticleEmitterNode(){
        datablock = BlastNode;
        emitter = BlastLeakEmitter;
        position = getWords(p().getTransform(), 0, 2);
    };
    addToMissionCleanup($blastLeaker);
}

$quantumSkin["blast"] = "blast";
$quantumTime["blast"] = 15000;

function startBlast() {
	$canBlast = 1;
	$blastTimer = 0;

	makeBlastParticle();
	makeBlastLeakParticle();
}

datablock AudioProfile(BlastSfx) {
	filename    = "~/data/Shared/sound/blast.wav";
	description = AudioDefault3d;
	preload = true;
};

function updateBlast(%timeDelta) {
	$blastTimer -= %timeDelta;

	if (!$canBlast && $blastTimer <= 0 && VectorLen(btGetCollisionNormal(p().btBody)) > 0.001) {
		$canBlast = 1;
	}

	if ($mouseFiring && $canBlast) {
		$canBlast = 0;
		$blastTimer = 100;
		
		%vel = btGetVelocity(p().btBody);
		
		%vel = VectorRej(%vel, "0 0 1");
		%vel = VectorScale(%vel, 1.2);
		%vel = VectorAdd(%vel, "0 0 7.5");
		btSetVelocity(p().btBody, %vel);
		
		fmodSetPosition(fmodPlay(BlastSfx), MatrixPos(p().getTransform()));
		
		makeBlastParticle();
		
		event("onBlast", MatrixPos(p().getTransform()));
	}
}

function onBlast(%pos) {
	// Prevent warning
}

function stopBlast() {
	makeBlastParticle();
	
	if (isObject($blastLeaker))
		$blastLeaker.delete();
}

$quantumStart["blast"] = "startBlast";
$quantumUpdate["blast"] = "updateBlast";
$quantumStop["blast"] = "stopBlast";

datablock AudioProfile(BlastHumSfx) {
	filename    = "~/data/Shared/sound/blast_hum.wav";
	description = AudioDefaultLooping3d;
	preload = true;
};

$quantumSound["blast"] = BlastHumSfx;