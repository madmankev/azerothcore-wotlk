/*
 * AQ War Effort shared state for the world scripts in this directory.
 * npc_aq_war_effort.cpp owns the singleton; aq_war_effort_war.cpp consumes
 * it to decide when to run the 10-hour war spawns.
 */
#ifndef AQ_WAR_EFFORT_H_
#define AQ_WAR_EFFORT_H_

#include <cstdint>

enum AqWarEffortPhase : uint8
{
    PHASE_GATHERING = 0,
    PHASE_TRANSIT   = 1,
    PHASE_WAR       = 2,
    PHASE_COMPLETE  = 3,
    PHASE_OPEN      = 4,   // Scarab Gong has been rung
};

class AqWarEffortMgr
{
public:
    static AqWarEffortMgr* instance();

    uint8 GetPhase() const { return _phase; }
    void  SetPhase(uint8 phase, bool announce = true);

    // Implemented in npc_aq_war_effort.cpp.
    void Load();
    void Save();

private:
    uint8 _phase = PHASE_GATHERING;
};

#define sAqWarEffortMgr AqWarEffortMgr::instance()

#endif // AQ_WAR_EFFORT_H_
