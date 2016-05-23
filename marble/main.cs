//-----------------------------------------------------------------------------
// Torque Game Engine
// 
// Copyright (c) 2001 GarageGames.Com
// Portions Copyright (c) 2001 by Sierra Online, Inc.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Load up defaults console values.

// Defaults console values
exec("./client/defaults.cs");
exec("./server/defaults.cs");

// Preferences (overide defaults)
exec("./client/prefs.cs");
exec("./client/save.cs");
exec("./server/prefs.cs");
exec("./referrer.cs");


//-----------------------------------------------------------------------------
// Package overrides to initialize the mod.
package marble {

function displayHelp() {
   Parent::displayHelp();
   error(
      "Marble Mod options:\n"@
      "  -dedicated             Start as dedicated server\n"@
      "  -connect <address>     For non-dedicated: Connect to a game at <address>\n" @
      "  -mission <filename>    For dedicated or non-dedicated: Load the mission\n" @
      "  -test <.dif filename>  Test an interior map file\n"
   );
}

function parseArgs()
{
   // Call the parent
   Parent::parseArgs();

   // Arguments, which override everything else.
   for (%i = 1; %i < $Game::argc ; %i++)
   {
      %arg = $Game::argv[%i];
      %nextArg = $Game::argv[%i+1];
      %hasNextArg = $Game::argc - %i > 1;
   
      switch$ (%arg)
      {
         //--------------------
         case "-referrer":
            $argUsed[%i]++;
            if (%hasNextArg) {
               $referrerId = %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -referrer <referrerid>");
         case "-mission":
            $argUsed[%i]++;
            if (%hasNextArg) {
               $missionArg = %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -mission <filename>");

         case "-test":
            $argUsed[%i]++;
            if(%hasNextArg) {
               $testCheats = true;
               $interiorArg = %nextArg;
               $argUsed[%i+1]++;
               %i++;
            }
            else
               error("Error: Missing Command Line argument. Usage: -test <interior filename>");
         //--------------------
         case "-cheats":
            $testCheats = true;
            $argUsed[%i]++;
      }
   }
}

function onStart()
{
   Parent::onStart();
   echo("\n--------- Initializing MOD: FPS ---------");

   // Load the scripts that start it all...
   exec("./client/init.cs");
   exec("./server/init.cs");
   exec("./data/init.cs");

   // Server gets loaded for all sessions, since clients
   // can host in-game servers.
   initServer();

   // Start up in either client, or dedicated server mode
   if ($Server::Dedicated)
      initDedicated();
   else
      initClient();
}

function onExit()
{
   echo("Exporting client prefs");
   export("$pref::*", "./client/prefs.cs", False);
   
   echo("Exporting save data");
   export("$save::*", "./client/save.cs", False);

   echo("Exporting server prefs");
   export("$Pref::Server::*", "./server/prefs.cs", False);
   BanList::Export("./server/banlist.cs");

   Parent::onExit();
}

}; // Client package
activatePackage(marble);

function listResolutions()
{
   %deviceList = getDisplayDeviceList();
   for(%deviceIndex = 0; (%device = getField(%deviceList, %deviceIndex)) !$= ""; %deviceIndex++)
   {
      %resList = getResolutionList(%device);
      for(%resIndex = 0; (%res = getField(%resList, %resIndex)) !$= ""; %resIndex++)
         echo(%device @ " - " @ getWord(%res, 0) @ " x " @ getWord(%res, 1) @ " (" @ getWord(%res, 2) @ " bpp)");
   }
}

function event(%name,%arg) {
   //error("BEGINNING EVENT -" SPC %name);
   //%time1 = getRealTime();
   call(%name,%arg);
   groupEvent(%name,%arg,MissionGroup);
   groupEvent(%name,%arg,MissionCleanup);
   groupEvent(%name,%arg,MissionPersistent);
   //%time2 = getRealTime();
   //warn("Event" SPC %name SPC "completed in" SPC (%time2 - %time1) SPC "ms");
}

function groupEvent(%name,%arg,%group) {
   if (!isObject(%group)) return;
   for (%i=0;%i<%group.getCount();%i++) {
      %object = %group.getObject(%i);
      %class = %object.getClassName();
      if (%class $= "SimGroup") groupEvent(%name,%arg,%object);
	  %object.schedule(0,%name,%arg);
      if (%object.getType() & $TypeMasks::GameBaseObjectType) {
		 %object.getDatablock().schedule(0,%name,%object,%arg);
      }
   }
}

$anonymousCounter = 0;

function anonymous(%code) {
	%fname = "_anonymous" @ $anonymousCounter;
	$anonymousCounter++;
	eval("function" SPC %fname @ "(%this) {" @ %code @ "}");
	return %fname;
}

function addTo(%group, %obj) {
	if (!isObject(%group)) {
		error("MAKING" SPC %group);
		ServerGroup.add(%g = new SimGroup());
		%g.setName(%group);
	}
	
	%group.add(%obj);
}