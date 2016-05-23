function startTimeline(%timeline) {
    $globalTimeline[$globalTimelines] = %timeline;
    $globalTime[$globalTimelines] = getRealTime();
    $globalTimelines++;
}