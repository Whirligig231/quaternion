// Torque Input Map File
MoveMap.delete();
new ActionMap(MoveMap);
MoveMap.bindCmd(keyboard, "escape", "", "escapeFromGame();");
MoveMap.bind(keyboard, "s", moveleft);
MoveMap.bind(keyboard, "space", mouseFire);
MoveMap.bind(keyboard, "alt c", toggleCamera);
MoveMap.bind(keyboard, "f8", dropCameraAtPlayer);
MoveMap.bind(keyboard, "f7", dropPlayerAtCamera);
MoveMap.bind(keyboard, "4", moveforward);
MoveMap.bind(keyboard, "f", moveright);
MoveMap.bind(keyboard, "d", movebackward);
MoveMap.bind(keyboard, "e", jump);
MoveMap.bind(keyboard, "3", panUp);
MoveMap.bind(keyboard, "5", panDown);
MoveMap.bind(keyboard, "r", turnRight);
MoveMap.bind(keyboard, "w", turnLeft);
MoveMap.bind(keyboard, "v", freelook);
MoveMap.bind(mouse0, "xaxis", yaw);
MoveMap.bind(mouse0, "yaxis", pitch);
