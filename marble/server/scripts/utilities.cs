function addToMissionCleanup(%obj) {
    if (!isObject(MissionCleanup)) {
        ServerGroup.add(new SimGroup(MissionCleanup()));
    }
    
    MissionCleanup.add(%obj);
}