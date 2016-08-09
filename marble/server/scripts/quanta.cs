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
	
	p().setSkinName($quantumSkin[%quantum]);
	$quantumIndex = %quantum;
	$quantumTimer = $quantumTime[%quantum];
	
	if ($quantumStart[$quantumIndex] !$= "")
		call($quantumStart[$quantumIndex], %timeDelta);
		
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
		
	$quantumTimer -= %timeDelta;

	QuantumBar.setValue($quantumTimer / $quantumTime[$quantumIndex]);
	
	if ($quantumTimer <= 0)
		giveQuantum("");
}

$globalFrameEvent[$globalFrameEvents] = "updateQuantum";
$globalFrameEvents++;

$quantumSkin[""] = "base";
$quantumTime[""] = 0;

$quantumSkin["blast"] = "blast";
$quantumTime["blast"] = 3000;