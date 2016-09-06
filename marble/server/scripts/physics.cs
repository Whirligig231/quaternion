$Core::physicsDisabled = 1;

btResetWorld();

function p() {
	return LocalClientConnection.player;
}

function NetObject::getClient(%this) {
	if (%this.syncId $= "" || %this.syncId < 0) {
		%this.syncId = %this.getSyncId();
		if (%this.getClassName() $= "Marble")
			%this.setModeInt(Normal);
		%this.setTransformA(%this.getTransformA());
	}
	%cli = getClientSyncObject(%this.syncId);
	if (%cli < 0) {
		warn("Unable to find client object on" SPC %this);
		%cli = %this;
		%this.forceNetUpdate();
	}
	return %cli;
}

function NetObject::getServer(%this) {
	if (%this.syncId $= "") {
		%this.syncId = %this.getSyncId();
		if (%this.getClassName() $= "Marble")
			%this.setModeInt(Normal);
			%this.setTransformA(%this.getTransformA());
		}
	%serv = getServerSyncObject(%this.syncId);
	if (%serv < 0)
		%serv = %this;
	return %serv;
}

function GameBaseData::onAdd(%this, %obj) {

    %obj.syncId = "";
	%obj.forceNetUpdate();
	
	%obj.initialTransform = %obj.getTransformA();
	%obj.initialWorldBox = %obj.getWorldBox();
	
	if (%this.btShapeType !$= "") {
		if (%this.btShape $= "") {
			switch$ (%this.btShapeType) {
			case "box":
				%this.btShape = btCreateShapeBox(%this.btHalfExtent);
			case "sphere":
				%this.btShape = btCreateShapeSphere(%this.btRadius);
			case "raw":
				%this.btShape = btCreateShapeRAW(%this.btShapeFile);
			default:
				error("No Bullet shape type defined (or recognized) for datablock" SPC %this.getName() SPC ".");
			}
		}
		
		%obj.btBody = btCreateBody(%this.btShape, %this.btMass, %obj.getTransformA());
		$btGlobalBodyIndex[%obj.btBody] = %obj;
		btSetFriction(%obj.btBody, %this.btFriction);
		btSetRestitution(%obj.btBody, %this.btRestitution);
		btSetLinearDamping(%obj.btBody, %this.btLinearDamping);
		btSetAngularDamping(%obj.btBody, %this.btAngularDamping);
		
		if (isObject(EditorGui)) {
			%obj.locked = !EditorGui.isAwake();
			btSetActive(%obj.btBody, !EditorGui.isAwake());
		}
		else {
			%obj.locked = 1;
			btSetActive(%obj.btBody, 1);
		}
		
		if (%this.btCallback)
			btEnableCallback(%obj.btBody);
        if (%this.btPlayer)
			btEnablePlayer(%obj.btBody);
	}
	else
		%obj.btBody = "";
}

function GameBaseData::onFrame(%this, %obj, %timeDelta) {
    //%obj.TD += %timeDelta;
    //if (%obj.TD < 0) return;
    //%obj.TD = 0;
	if (%obj.btBody !$= "" && %obj.locked) {
		%objc = %obj.getClient();
		//%obj.setTransformA(MatInterpolate(%objc.getTransformA(), btGetTransform(%obj.btBody), 0.9));
		//%objc.setTransformA(MatInterpolate(%objc.getTransformA(), btGetTransform(%obj.btBody), 0.9));
        
        %obj.setTransformA(btGetTransform(%obj.btBody));
        %objc.setTransformA(btGetTransform(%obj.btBody));
	}
}

function MarbleData::onFrame(%this, %obj, %timeDelta) {

	Parent::onFrame(%this, %obj, %timeDelta);
	%objc = %obj.getClient();
	%right = MatrixRight(%objc.getCameraTransform());
	%forward = MatrixForward(%objc.getCameraTransform());
	%right = VectorNormalize(VectorRej(%right, getGravityDir()));
	%forward = VectorNormalize(VectorRej(%forward, getGravityDir()));
	if (LocalClientConnection.getControlObject().getID() == %obj.getID()) {
		btApplyTorque(%obj.btBody, VectorScale(%right, -($mvForwardAction - $mvBackwardAction) * %this.btControlAngular));
		btApplyTorque(%obj.btBody, VectorScale(%forward, ($mvRightAction - $mvLeftAction) * %this.btControlAngular));
		btApplyCentralForce(%obj.btBody, VectorScale(%forward, ($mvForwardAction - $mvBackwardAction) * %this.btControlLinear));
		btApplyCentralForce(%obj.btBody, VectorScale(%right, ($mvRightAction - $mvLeftAction) * %this.btControlLinear));
	}
	
	// warn("compression level " @ btGetCompression(%obj.btBody));
	
	if (btGetCompression(%obj.btBody) > 100)
		explodeMarble();
	
	%nor = btGetCollisionNormal(%obj.btBody);
	%vel = btGetVelocity(%obj.btBody);
	if (VectorLen(%nor) > 0.001 && VectorLen(%obj.prevNor) <= 0.001) {
		%len = VectorLen(VectorProj(VectorSub(%vel, %obj.prevVel), %nor));
		// error(%len);
		if (%len > 3) {
			%sound = fmodPlay(%this.bounce1);
			fmodSetPosition(%sound, MatrixPos(btGetTransform(%obj.btBody)));
			fmodSetVolume(%sound, (%len - 3)/10);
			fmodSetFrequency(%sound, 2+mClamp(%len/20, 0, 1));
		}
		
		%obj.rollSound = fmodPlay(%this.rollHardSound);
		fmodSetVolume(%obj.rollSound, 0);
		%obj.slipSound = fmodPlay(%this.slipSound);
		fmodSetVolume(%obj.slipSound, 0);
	}
	else if (VectorLen(%nor) <= 0.001 && VectorLen(%obj.prevNor) > 0.001) {
		fmodStop(%obj.rollSound);
		fmodStop(%obj.slipSound);
	}
	%obj.prevNor = %nor;
	%obj.prevVel = %vel;
	
	if (VectorLen(%nor) > 0.001) {
		// Compute the deviation
		%expectedVel = VectorScale(VectorCross(btGetAngularVelocity(%obj.btBody), %nor), 0.2);
		%deviation = VectorSub(%expectedVel, btGetVelocity(%obj.btBody));
		%projDeviation = VectorRej(%deviation, %nor);
		%projVelocity = VectorRej(btGetVelocity(%obj.btBody), %nor);
		%slipSpeed = VectorLen(%projDeviation);
		%rollSpeed = VectorLen(%projVelocity);

		fmodSetFrequency(%obj.rollSound, %rollSpeed / 10);
		fmodSetVolume(%obj.rollSound, mPow(%rollSpeed / 10, 2));
		fmodSetPosition(%obj.rollSound, MatrixPos(btGetTransform(%obj.btBody)));
		
		fmodSetFrequency(%obj.slipSound, 1.5);
		fmodSetVolume(%obj.slipSound, mPow(%slipSpeed / 20, 2));
		fmodSetPosition(%obj.slipSound, MatrixPos(btGetTransform(%obj.btBody)));
	}

	if (($mvTriggerCount2 % 2) && !%obj.jumpDisabled && LocalClientConnection.getControlObject().getID() == %obj.getID()) {
		%obj.jumpDisabled = 1;
		schedule(100, 0, "enableJump", %obj);
		
		if (VectorLen(%nor) > 0.001) {
		 	btApplyCentralImpulse(%obj.btBody, VectorScale(%nor, %this.btControlJump - VectorDot(%vel, %nor)));
		 	%sound = fmodPlay(%this.jumpSound);
		 	fmodSetPosition(%sound, MatrixPos(btGetTransform(%obj.btBody)));
		}
	}
}

function enableJump(%obj) {
	%obj.jumpDisabled = 0;
}

function GameBase::setPositionB(%this, %pos, %angle) {
	%this.setPosition(%pos, %angle);
	if (%this.btBody !$= "") {
		btSetTransform(%this.btBody, %pos SPC "1 0 0 0");
		btSetVelocity(%this.btBody, "0 0 0");
		btSetAngularVelocity(%this.btBody, "0 0 0");
	}
}

function GameBase::setTransformB(%this, %mat) {
	if (%this.btBody !$= "") {
		btSetTransform(%this.btBody, %mat);
	}
	else
		%this.setTransformA(%mat);
}

package TransformPackage
{

function GameBase::setTransformA(%this, %mat) {
	Parent::setTransform(%this, %mat);
}

function GameBase::setTransform(%this, %mat) {
	%this.setTransformB(%mat);
}

function GameBase::getTransformA(%this) {
	return Parent::getTransform(%this);
}

function GameBase::getTransform(%this) {
	if (%this.isClientObject())
		return %this.getTransformA();
	return %this.getClient().getTransformA();
}

};

activatePackage(TransformPackage);

function GameBase::setVelocity(%this, %vel) {
	if (%this.btBody !$= "") {
		btSetVelocity(%this.btBody, %vel);
	}
}

function ShapeBase::setVelocity(%this, %vel) {
	if (%this.btBody !$= "") {
		btSetVelocity(%this.btBody, %vel);
	}
}

function Marble::setVelocity(%this, %vel) {
	if (%this.btBody !$= "") {
		btSetVelocity(%this.btBody, %vel);
	}
}

function GameBase::getVelocity(%this) {
	if (%this.btBody !$= "") {
		return btGetVelocity(%this.btBody);
	}
}

function GameBase::setAngularVelocity(%this, %vel) {
	if (%this.btBody !$= "") {
		btSetAngularVelocity(%this.btBody, %vel);
	}
}

function GameBase::applyForce(%this, %force) {
	if (%this.btBody !$= "") {
		btApplyCentralForce(%this.btBody, %force);
	}
}

function GameBase::applyTorque(%this, %torque) {
	if (%this.btBody !$= "") {
		btApplyTorque(%this.btBody, %torque);
	}
}

function GameBase::applyImpulse(%this, %force) {
	if (%this.btBody !$= "") {
		btApplyCentralImpulse(%this.btBody, %force);
	}
}

function Marble::applyImpulse(%this, %force) {
	if (%this.btBody !$= "") {
		btApplyCentralImpulse(%this.btBody, %force);
	}
}

function GameBase::applyTorqueImpulse(%this, %torque) {
	if (%this.btBody !$= "") {
		btApplyTorqueImpulse(%this.btBody, %torque);
	}
}

function GameBase::setFactor(%this, %fac) {
	if (%this.btBody !$= "") {
		btSetFactor(%this.btBody, %fac);
	}
}

function GameBase::setActive(%this, %active) {
	if (%this.locked == %active)
		return;
	%this.getServer().locked = %active;
	%this.getClient().locked = %active;
	if (%this.btBody !$= "") {
		if (%active) {
			if (%this.getClassName() !$= "Marble") {
				btSetTransform(%this.btBody, %this.getTransformA());
				if (%this.getDataBlock().btMass > 0) {
					btSetVelocity(%this.btBody, "0 0 0");
					btSetAngularVelocity(%this.btBody, "0 0 0");
				}
			}
			%this.getServer().initialTransform = %this.getTransformA();
			%this.getServer().initialWorldBox = %this.getWorldBox();
		}
		else {
			if (%this.getClassName() !$= "Marble") {
				%this.getServer().setTransformA(%this.getServer().initialTransform);
				%this.getClient().setTransformA(%this.getServer().initialTransform);
				btSetTransform(%this.btBody, %this.getServer().initialTransform);
			}
		}
		btSetActive(%this.btBody, %active);
	}
}

function GameBase::setInitialPosition(%this) {
	%this.getServer().initialTransform = %this.getTransformA();
	%this.getServer().initialWorldBox = %this.getWorldBox();
}

function GameBaseData::onCollision(%this, %obj, %col) {
	// Nothing except removing warnings
}

package SetModePackage
{

function Marble::setMode(%this, %mode) {
	Parent::setMode(%this, %mode);
	switch$ (%mode) {
	case "Start":
		%this.setFactor("0 0 0");
        %this.setVelocity("0 0 0");
	case "Victory":
		%this.setFactor("0 0 0");
		%this.setVelocity("0 0 0");
	default:
		%this.setFactor("1 1 1");
	}
}

function Marble::setModeInt(%this, %mode) {
	Parent::setMode(%this, %mode);
}

};

activatePackage(SetModePackage);