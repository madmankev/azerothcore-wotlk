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

    // Called by the caravan script when a faction's supply caravan
    // reaches Cenarion Hold. When both factions have arrived (or the
    // first one if the other was already complete) the phase advances
    // from TRANSIT to WAR.
    void NotifyCaravanArrived(TeamId team);

    bool CaravanArrived(TeamId team) const { return _caravanArrived[team]; }

private:
    uint8 _phase = PHASE_GATHERING;
    bool  _caravanArrived[2] = { false, false };
};

#define sAqWarEffortMgr AqWarEffortMgr::instance()

#endif // AQ_WAR_EFFORT_H_
