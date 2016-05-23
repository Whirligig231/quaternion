//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Console onEditorRender functions:
//------------------------------------------------------------------------------
// Functions:
//   - renderSphere([pos], [radius], <sphereLevel>);
//   - renderCircle([pos], [normal], [radius], <segments>);
//   - renderTriangle([pnt], [pnt], [pnt]);
//   - renderLine([start], [end], <thickness>);
//
// Variables:
//   - consoleFrameColor - line prims are rendered with this
//   - consoleFillColor
//   - consoleSphereLevel - level of polyhedron subdivision
//   - consoleCircleSegments
//   - consoleLineWidth
//------------------------------------------------------------------------------

function WorldEditor::renderBox(%this, %box, %thickness) {
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 2), getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 5), %thickness);
	%this.renderLine(getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 2), getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 5), %thickness);
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 2), getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 5), %thickness);
	%this.renderLine(getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 2), getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 5), %thickness);

	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 2), getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 2), %thickness);
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 2), getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 2), %thickness);
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 5), getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 5), %thickness);
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 5), getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 5), %thickness);

	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 2), getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 2), %thickness);
	%this.renderLine(getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 2), getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 2), %thickness);
	%this.renderLine(getWord(%box, 0) SPC getWord(%box, 1) SPC getWord(%box, 5), getWord(%box, 0) SPC getWord(%box, 4) SPC getWord(%box, 5), %thickness);
	%this.renderLine(getWord(%box, 3) SPC getWord(%box, 1) SPC getWord(%box, 5), getWord(%box, 3) SPC getWord(%box, 4) SPC getWord(%box, 5), %thickness);
}

function WorldEditor::renderArrow(%this, %start, %end) {
	%this.renderLine(%start,%end);
	%mat = MatrixPoint(%end SPC "1 0 0 0",%start);
	%up = MatrixMulPoint(%mat,"0 0.1 -0.2");
	%left = MatrixMulPoint(%mat,"-0.1 0 -0.2");
	%right = MatrixMulPoint(%mat,"0.1 0 -0.2");
	%down = MatrixMulPoint(%mat,"0 -0.1 -0.2");
	%this.renderLine(%end,%up);
	%this.renderLine(%end,%down);
	%this.renderLine(%end,%left);
	%this.renderLine(%end,%right);
}

function SpawnSphere::onEditorRender(%this, %editor, %selected, %expanded)
{
   if(%selected $= "true")
   {
      %editor.consoleFrameColor = "255 0 0";
      %editor.consoleFillColor = "0 0 0 0";
      %editor.renderSphere(%this.getWorldBoxCenter(), %this.radius, 1);
   }
}

function AudioEmitter::onEditorRender(%this, %editor, %selected, %expanded)
{
   if(%selected $= "true" && %this.is3D && !%this.useProfileDescription)
   {
      %editor.consoleFillColor = "0 0 0 0";

      %editor.consoleFrameColor = "255 0 0";
      %editor.renderSphere(%this.getTransform(), %this.minDistance, 1);

      %editor.consoleFrameColor = "0 0 255";
      %editor.renderSphere(%this.getTransform(), %this.maxDistance, 1);
   }
}

function GameBase::onEditorRender(%this, %editor, %selected, %expanded)
{
   if(%selected $= "true")
   {
	  %editor.consoleFrameColor = "0 170 0";
      %editor.consoleFillColor = "0 0 0 0";
      %editor.renderBox(%this.initialWorldBox, 1);
   }
}

//function Item::onEditorRender(%this, %editor, %selected, %expanded)
//{
//   if(%this.getDataBlock().getName() $= "MineDeployed")
//   {
//      %editor.consoleFillColor = "0 0 0 0";
//      %editor.consoleFrameColor = "255 0 0";
//      %editor.renderSphere(%this.getWorldBoxCenter(), 6, 1);
//   }
//}