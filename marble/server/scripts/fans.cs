datablock StaticShapeData(Fan)
{
   className = "Fan";
   category = "Fans";
   shapeFile = "~/data/Shared/fans/fan.dts";
   scopeAlways = true;
   
    btShapeType = "raw";
    btShapeFile = "~/data/Shared/fans/fan.raw";
    btMass = 0;
    btFriction = 1.1;
    btRestitution = 0.7;

   powerOn = true;         // Default state
};

function Fan::onAdd(%this,%obj)
{
   Parent::onAdd(%this, %obj);
   if (%this.powerOn)
   {
      //%obj.playAudio(0, DuctFanSfx);
      %obj.playThread(0,"spin");
   }
   %obj.setPoweredState(%this.powerOn);
}

function Fan::onFrame(%this, %obj) {
    %pos = getWords(%obj.getTransform(), 0, 2);
    %nor = MatrixMulVector(%obj.getTransform(), "0 0 1");
    %pos = VectorAdd(%pos, VectorScale(%nor, 0.35));
    %play = getWords(p().getTransform(), 0, 2);
    %diff = VectorSub(%play, %pos);
    %nordist = VectorDot(%diff, %nor);
    %rej = VectorSub(%diff, VectorScale(%nor, %nordist));
    %planedist = VectorLen(%rej);

    if (%planedist < 1 && %nordist < 0.2)
        explodeMarble();
}

function Fan::onTrigger(%this,%obj,%mesg)
{
   if (%mesg)
   {
      //%obj.playAudio(0, DuctFanSfx);
      %obj.playThread(0,"spin");
   }
   else
   {
      %obj.stopThread(0);
      //%obj.stopAudio(0);
   }
   %obj.setPoweredState(%mesg);
}