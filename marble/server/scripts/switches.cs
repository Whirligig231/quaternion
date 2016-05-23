datablock AudioProfile(ButtonPressSfx)
{
   filename    = "~/data/Shared/switches/press.wav";
   description = AudioDefault3d;
   preload = true;
};

datablock StaticShapeData(PressSwitch) {
	category = "Switches";
	shapeFile = "~/data/Shared/switches/button.dts";
	
	btShapeType = "box";
	btHalfExtent = "0.5 0.5 0.05";
	btMass = 0;
	btFriction = 1.1;
	btRestitution = 0.7;
};

function PressSwitch::onAdd(%this, %obj) {
    Parent::onAdd(%this, %obj);
    %obj.activated = 0;
}

function PressSwitch::onCollision(%this, %obj, %col) {
	if (!%obj.activated && !call(%obj.callback, %obj)) {
		%obj.activated = 1;
		%sound = fmodPlay(ButtonPressSfx);
		fmodSetPosition(%sound, MatrixPos(%obj.getTransform()));
		%obj.setSkinName("on");
	}
}

datablock StaticShapeData(BallSwitch) {
	category = "Switches";
    className = "BallSwitchClass";
	shapeFile = "~/data/Shared/switches/ballswitch.dts";
	
	btShapeType = "raw";
	btShapeFile = "~/data/Shared/switches/ballswitch.raw";
	btMass = 0;
	btFriction = 1.1;
	btRestitution = 0.7;
};

function BallSwitchClass::onAdd(%this, %obj) {
    Parent::onAdd(%this, %obj);
    %obj.activated = 0;
}

function BallSwitchClass::onFrame(%this, %obj)
{
	%iscol = %obj.isColliding || %obj.collisionCache[0] || %obj.collisionCache[1];
	%prevcol = %obj.collisionCache[0] || %obj.collisionCache[1] || %obj.collisionCache[2];
	
	if (%iscol && !%prevcol) {
        %obj.setSkinName("on");
        %sound = fmodPlay(ButtonPressSfx);
		fmodSetPosition(%sound, MatrixPos(%obj.getTransform()));
        call(%obj.onCallback, %obj);
    }
	else if (!%iscol && %prevcol) {
        %obj.setSkinName("base");
        %sound = fmodPlay(ButtonPressSfx);
		fmodSetPosition(%sound, MatrixPos(%obj.getTransform()));
        fmodSetFrequency(%sound, 0.71);
        call(%obj.offCallback, %obj);
    }
	
	%obj.collisionCache[2] = %obj.collisionCache[1];
	%obj.collisionCache[1] = %obj.collisionCache[0];
	%obj.collisionCache[0] = %obj.isColliding;
	%obj.isColliding = false;
}

function BallSwitchClass::onCollision(%this, %obj, %col) {
	%thisPos = getWords(%obj.getTransform(), 0, 2);
    %otherPos = getWords(%col.getTransform(), 0, 2);
    %diff = getWords(VectorSub(%thisPos, %otherPos), 0, 1) SPC 0;
    %rad = VectorLen(%diff);
    if (%rad < 0.26)
        %obj.isColliding = true;
}

datablock StaticShapeData(BallSwitchSmall : BallSwitch) {
	shapeFile = "~/data/Shared/switches/ballswitch2.dts";
	btShapeFile = "~/data/Shared/switches/ballswitch2.raw";
};