function DtsRaw(%dName, %fName, %lName) {
    if (isObject(%dName))
        return;
        
    echo("Importing" SPC %lName @ ":" SPC %dName SPC "(DTS) ...");
        
    eval("datablock StaticShapeData(" @ %dName @ ") {category = \"Level - " @ %lName @ "\"; shapeFile = \"marble/data/" @ %lName @ "/" @ %fname @ ".dts\"; btShapeType = \"raw\"; btShapeFile = \"marble/data/" @ %lName @ "/" @ %fname @ ".raw\"; btMass = 0; btFriction = 1.1; btRestitution = 0.7; };");
}

function makeDif(%this) {
	if (isObject(%this.interior))
		return;
	
	addTo("MissionCleanup", %this.interior = new InteriorInstance() {
		position = "0 0 0";
		rotation = "1 0 0 0";
		scale = "1 1 1";
		interiorFile = %this.getDataBlock().interiorFile;
		showTerrainInside = "0";
	});
	
	%this.interior.syncId = "";
	%this.interior.forceNetUpdate();
	
	%this.interior.getClient().setTransform(%this.getClient().getTransform());
}

function updateDif(%this) {
	if (!isObject(%this.interior))
		return;
	
	%this.interior.getClient().setTransform(%this.getClient().getTransform());
}

function DifRaw(%dName, %fName, %lName) {
    if (isObject(%dName))
        return;
        
    echo("Importing" SPC %lName @ ":" SPC %dName SPC "(DIF) ...");
        
    eval("datablock StaticShapeData(" @ %dName @ ") {category = \"Level - " @ %lName @ "\"; shapeFile = \"marble/data/Shared/empty.dts\"; interiorFile = \"marble/data/" @ %lName @ "/" @ %fname @ ".dif\"; btShapeType = \"raw\"; btShapeFile = \"marble/data/" @ %lName @ "/" @ %fname @ ".raw\"; btMass = 0; btFriction = 1.1; btRestitution = 0.7; };");
	
	eval("function " @ %dName @ "::onAdd(%this, %obj) { Parent::onAdd(%this, %obj); makeDif(%obj); }");
	
	eval("function " @ %dName @ "::onFrame(%this, %obj, %timeDelta) { Parent::onFrame(%this, %obj, %timeDelta); updateDif(%obj); }");
}