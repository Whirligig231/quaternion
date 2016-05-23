//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

function TriggerData::onAdd(%this, %obj)
{
	%halfExtent = VectorScale(%obj.scale, 0.5);
	%obj.btShape = btCreateShapeBox(%halfExtent);
	%pos = %obj.getWorldBoxCenter();
	%rot = getWords(%obj.rotation, 0, 2) SPC (getWord(%obj.rotation, 3)*3.1415926535898/180);
	%obj.btBody = btCreateBody(%obj.btShape, 0, %pos SPC %rot);
	btSetSolid(%obj.btBody, false);
	$btGlobalBodyIndex[%obj.btBody] = %obj;
}

function TriggerData::onFrame(%this, %obj)
{
	%iscol = %obj.isColliding || %obj.collisionCache[0] || %obj.collisionCache[1];
	%prevcol = %obj.collisionCache[0] || %obj.collisionCache[1] || %obj.collisionCache[2];
	
	if (%iscol && !%prevcol)
		%obj.getDatablock().onEnterTrigger(%obj, p());
	else if (!%iscol && %prevcol)
		%obj.getDatablock().onLeaveTrigger(%obj, p());
	
	%obj.collisionCache[2] = %obj.collisionCache[1];
	%obj.collisionCache[1] = %obj.collisionCache[0];
	%obj.collisionCache[0] = %obj.isColliding;
	%obj.isColliding = false;
}

function TriggerData::onCollision(%this, %obj, %col)
{
	%obj.isColliding = true;
}

//-----------------------------------------------------------------------------

datablock TriggerData(InBoundsTrigger)
{
   tickPeriodMS = 100;
};

function InBoundsTrigger::onLeaveTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   %obj.getDatablock().onOutOfBounds(%obj);
}


//-----------------------------------------------------------------------------

datablock TriggerData(OutOfBoundsTrigger)
{
   tickPeriodMS = 100;
};

function OutOfBoundsTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Entering an out of bounds area
   %obj.getDatablock().onOutOfBounds(%obj);
}

//-----------------------------------------------------------------------------

datablock TriggerData(HelpTrigger)
{
   tickPeriodMS = 100;
};

function HelpTrigger::onEnterTrigger(%this,%trigger,%obj)
{
   // Leaving an in bounds area.
   addHelpLine(%trigger.text, true);
}

