datablock StaticShapeData(SkyShape) {
	shapeFile = "~/data/Shared/skies/sky.dts";
};

function SkyShape::onAdd(%this, %obj) {
    %scale = Sky.visibleDistance / 2;
    %obj.setScale(%scale SPC %scale SPC %scale);
    %obj.playThread(0, "ambient");
}

function SkyShape::onFrame(%this, %obj, %timeDelta) {
	if (!isObject(LocalClientConnection.getControlObject()))
		return;
    %mat = MatrixPos(LocalClientConnection.getControlObject().getTransform()) SPC MatrixRot(%obj.getTransform());
    // %rot = 0.0005*%timeDelta;
    // %mat = MatrixMultiply("0 0 0 0 0 1" SPC %rot, %mat);
    %obj.getClient().setTransform(%mat);
}